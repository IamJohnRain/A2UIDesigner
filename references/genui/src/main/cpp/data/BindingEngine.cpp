/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Created on 2026/3/25.
//
// Node APIs are not fully supported. To solve the compilation error of the
// interface cannot be found, please include "napi/native_api.h".

#include "BindingEngine.h"

#include <algorithm>

#include "utils/LogA2UI.h"

#include "PathValidator.h"

#ifdef ENABLE_EXPRESSION_ENGINE
#include "../RenderManager.h"
#include "../RenderSlot.h"
#include "../SurfaceManager.h"
#include "../expression/EvaluationContext.h"
#include "../expression/ExpressionEngine.h"
#include "../expression/ThemeContextUtils.h"
#endif

namespace NativeModule {

namespace {

bool HasPendingComponent(
    const std::vector<std::shared_ptr<Component>>& pendingComponents, const std::string& componentId)
{
    for (const auto& pendingComponent : pendingComponents) {
        if (pendingComponent != nullptr && pendingComponent->GetComponentId() == componentId) {
            return true;
        }
    }
    return false;
}

using ComponentIdIndex = std::unordered_map<std::string, std::unordered_set<std::string>>;

void InsertComponentIntoIndex(ComponentIdIndex& index, const std::string& key, const std::string& componentId)
{
    index[key].insert(componentId);
}

void EraseComponentFromIndex(ComponentIdIndex& index, const std::string& key, const std::string& componentId)
{
    auto it = index.find(key);
    if (it == index.end()) {
        return;
    }
    it->second.erase(componentId);
    if (it->second.empty()) {
        index.erase(it);
    }
}

} // namespace

// 日志标签常量
// ========== 初始化和工厂方法 ==========

std::shared_ptr<BindingEngine> BindingEngine::Create()
{
    // Create using shared_ptr and private constructor
    auto engine = std::shared_ptr<BindingEngine>(new BindingEngine());
    engine->Initialize();
    return engine;
}

void BindingEngine::Initialize()
{
    // Now we can safely use shared_from_this() because the object is already
    // owned by shared_ptr
    defaultDataModel_->SetEngine(shared_from_this());
    // 初始化默认数据模型到集合中
    dataModels_["default"] = defaultDataModel_;
    LOG_A2UI(LOG_INFO, "BindingEngine initialized");
}

// ========== 数据模型管理 ==========

std::shared_ptr<DataModel> BindingEngine::GetOrCreateDataModel(const std::string& surfaceId)
{
    auto it = dataModels_.find(surfaceId);
    if (it != dataModels_.end()) {
        return it->second;
    }

    // 创建新的数据模型
    auto dataModel = std::make_shared<DataModel>(surfaceId);
    dataModel->SetEngine(shared_from_this());
    dataModels_[surfaceId] = dataModel;

    LOG_A2UI(LOG_DEBUG, "Created new DataModel for surfaceId: %{public}s", surfaceId.c_str());

    return dataModel;
}

bool BindingEngine::UpdateDataModelByPath(const std::string& surfaceId, const std::string& path, const JsonValue& value)
{
    LOG_A2UI(LOG_INFO, "UpdateDataModelByPath: surfaceId=%{public}s, path=%{public}s", surfaceId.c_str(), path.c_str());

    if (!IsValidDataPath(path)) {
        LOG_A2UI(LOG_WARN, "UpdateDataModelByPath: invalid path=%{public}s", path.c_str());
        return false;
    }

    int32_t depth = DataModel::MeasureJsonDepth(value);
    if (depth > DataModel::MAX_DATA_MODEL_DEPTH) {
        LOG_A2UI(LOG_ERROR,
            "UpdateDataModelByPath: data model nesting depth %{public}d exceeds maximum allowed %{public}d", depth,
            DataModel::MAX_DATA_MODEL_DEPTH);
    }

    auto dataModel = GetOrCreateDataModel(surfaceId);
    bool success = dataModel->UpdateByPath(path, value);
    dataModelReady_ = dataModelReady_ || success;
    return success;
}

bool BindingEngine::DeleteDataModelByPath(const std::string& surfaceId, const std::string& path)
{
    LOG_A2UI(LOG_INFO, "DeleteDataModelByPath: surfaceId=%{public}s, path=%{public}s", surfaceId.c_str(), path.c_str());

    if (!IsValidDataPath(path)) {
        LOG_A2UI(LOG_WARN, "DeleteDataModelByPath: invalid path=%{public}s", path.c_str());
        return false;
    }

    auto it = dataModels_.find(surfaceId);
    if (it == dataModels_.end()) {
        LOG_A2UI(LOG_WARN, "DataModel not found for surfaceId: %{public}s", surfaceId.c_str());
        return true;
    }

    return it->second->DeleteByPath(path);
}

bool BindingEngine::ReplaceDataModel(const std::string& surfaceId, const JsonValue& value)
{
    LOG_A2UI(LOG_INFO, "ReplaceDataModel: surfaceId=%{public}s", surfaceId.c_str());

    int32_t depth = DataModel::MeasureJsonDepth(value);
    if (depth > DataModel::MAX_DATA_MODEL_DEPTH) {
        LOG_A2UI(LOG_ERROR, "ReplaceDataModel: data model nesting depth %{public}d exceeds maximum allowed %{public}d",
            depth, DataModel::MAX_DATA_MODEL_DEPTH);
    }

    auto dataModel = GetOrCreateDataModel(surfaceId);
    bool success = dataModel->ReplaceAll(value);
    dataModelReady_ = dataModelReady_ || success;
    return success;
}

void BindingEngine::ProcessUpdate(const DataModelUpdate& updateRequest)
{
    LOG_A2UI(LOG_INFO, "ProcessUpdate: surfaceId=%{public}s", updateRequest.surfaceId.c_str());

    auto dataModel = GetOrCreateDataModel(updateRequest.surfaceId);
    dataModel->ProcessUpdate(updateRequest);
    dataModelReady_ = true;
}

void BindingEngine::UpdateDataModel(const std::map<std::string, std::string>& data)
{
    LOG_A2UI(LOG_INFO, "Update Data Model with %{public}zu entries", data.size());
    defaultDataModel_->Update(data);
    dataModelReady_ = true;

    // 处理待绑定的组件
    ProcessPendingComponents();

    // 通知所有已绑定组件数据已更新
    NotifyAllBindings();
}

// ========== 组件管理 ==========

void BindingEngine::RegisterComponent(std::shared_ptr<Component> comp)
{
    if (comp == nullptr || comp->GetDataBindings().empty()) {
        return;
    }
    LOG_A2UI(LOG_DEBUG, "Register Component: %{public}s, type: %{public}s, surfaceId: %{public}s",
        comp->GetComponentId().c_str(), comp->GetType().c_str(), comp->GetSurfaceId().c_str());

    // 获取组件对应的 DataModel（根据 surfaceId）
    std::string surfaceId = comp->GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default"; // 如果没有设置 surfaceId，使用默认
    }

    auto dataModel = GetOrCreateDataModel(surfaceId);
    components_[comp->GetComponentId()] = comp;
    RegisterBindingPaths(comp, surfaceId, comp->GetDataBindings());
    componentBindings_[comp->GetComponentId()] = comp->GetDataBindings();

    if (dataModelReady_) {
        if (!comp->ConsumeDescriptorDynamicBindingsResolved()) {
            BindComponentImmediate(comp, surfaceId, false);
        }
    } else {
        pendingComponents_.push_back(comp);
    }
}

void BindingEngine::SyncComponentBindings(std::shared_ptr<Component> comp)
{
    if (comp == nullptr) {
        return;
    }

    std::string surfaceId = comp->GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }

    components_[comp->GetComponentId()] = comp;

    std::vector<DataBinding> previousBindings;
    auto previousIt = componentBindings_.find(comp->GetComponentId());
    if (previousIt != componentBindings_.end()) {
        previousBindings = previousIt->second;
    }

    UnregisterBindingPaths(comp, surfaceId, previousBindings);

    const std::vector<DataBinding> currentBindings = comp->GetDataBindings();
    RegisterBindingPaths(comp, surfaceId, currentBindings);
    componentBindings_[comp->GetComponentId()] = currentBindings;

    if (dataModelReady_) {
        if (!comp->ConsumeDescriptorDynamicBindingsResolved()) {
            BindComponentImmediate(comp, surfaceId, false);
        }
    } else if (!currentBindings.empty() && !HasPendingComponent(pendingComponents_, comp->GetComponentId())) {
        pendingComponents_.push_back(comp);
    }
}

void BindingEngine::BindComponentImmediate(
    std::shared_ptr<Component> comp, const std::string& surfaceId, bool refreshExpressionBindings)
{
    LOG_A2UI(LOG_INFO, "Binding: %{public}s immediately, surfaceId: %{public}s", comp->GetComponentId().c_str(),
        surfaceId.c_str());

    auto dataModel = GetOrCreateDataModel(surfaceId);
    std::unordered_set<std::string> refreshedExpressionProperties;
    for (const auto& binding : comp->GetDataBindings()) {
        if (binding.type_ == BindingType::EXPRESSION) {
            if (!refreshExpressionBindings || binding.dataPath_.empty()) {
                continue;
            }
            if (!refreshedExpressionProperties.insert(binding.propertyName_).second) {
                continue;
            }
            comp->OnDataUpdate(binding.propertyName_, JsonValue());
            continue;
        }
        auto nodeOpt = dataModel->GetNode(binding.dataPath_);
        if (nodeOpt.has_value()) {
            comp->OnDataUpdate(binding.propertyName_, nodeOpt.value());
        } else {
            LOG_A2UI(LOG_INFO, "[WARN] Path not found: %{public}s ", binding.dataPath_.c_str());
            comp->OnDataUpdate(binding.propertyName_, JsonValue());
        }
    }
}

// 辅助函数：处理待绑定的组件
void BindingEngine::ProcessPendingComponents()
{
    if (pendingComponents_.empty()) {
        return;
    }

    LOG_A2UI(LOG_INFO, "Processing %{public}zu pending components...", pendingComponents_.size());

    for (auto& comp : pendingComponents_) {
        std::string surfaceId = comp->GetSurfaceId();
        if (surfaceId.empty()) {
            surfaceId = "default";
        }
        BindComponentImmediate(comp, surfaceId);
    }
    pendingComponents_.clear();
}

std::string BindingEngine::BuildBindingKey(const DataBinding& binding) const
{
    return binding.propertyName_ + "\n" + binding.dataPath_;
}

void BindingEngine::RegisterBindingPaths(
    const std::shared_ptr<Component>& comp, const std::string& surfaceId, const std::vector<DataBinding>& bindings)
{
    if (comp == nullptr || bindings.empty()) {
        return;
    }

    auto dataModel = GetOrCreateDataModel(surfaceId);
    for (const auto& binding : bindings) {
        if (binding.type_ == BindingType::EXPRESSION) {
            RegisterExpressionBindingPaths(comp, dataModel, binding);
        } else {
            RegisterNonExpressionBindingPaths(comp, dataModel, binding);
        }
    }
}

void BindingEngine::RegisterExpressionBindingPaths(
    const std::shared_ptr<Component>& comp, const std::shared_ptr<DataModel>& dataModel, const DataBinding& binding)
{
    const std::string componentId = comp->GetComponentId();
    for (const auto& varName : binding.globalVarDeps_) {
        if (varName != "__dataModel") {
            LOG_A2UI(LOG_INFO, "Expression binding (global): %{public}s -> %{public}s", binding.propertyName_.c_str(),
                varName.c_str());
            InsertComponentIntoIndex(globalVarBindingIndex_, varName, componentId);
            continue;
        }
        if (binding.dataPath_.empty()) {
            continue;
        }
        LOG_A2UI(LOG_INFO, "Expression binding (dataModel): %{public}s -> %{public}s", binding.propertyName_.c_str(),
            binding.dataPath_.c_str());
        InsertComponentIntoIndex(bindingIndex_, binding.dataPath_, componentId);
        dataModel->RegisterInterest(binding.dataPath_, comp);
    }
}

void BindingEngine::RegisterNonExpressionBindingPaths(
    const std::shared_ptr<Component>& comp, const std::shared_ptr<DataModel>& dataModel, const DataBinding& binding)
{
    if (!binding.dataPath_.empty()) {
        LOG_A2UI(LOG_INFO, "Binding: %{public}s -> binding.dataPath_: %{public}s", binding.propertyName_.c_str(),
            binding.dataPath_.c_str());
        InsertComponentIntoIndex(bindingIndex_, binding.dataPath_, comp->GetComponentId());
        dataModel->RegisterInterest(binding.dataPath_, comp);
    }
    if (binding.type_ != BindingType::FUNCTION_CALL) {
        return;
    }
    const std::string componentId = comp->GetComponentId();
    for (const auto& varName : binding.globalVarDeps_) {
        if (varName.empty() || varName == "__dataModel") {
            continue;
        }
        LOG_A2UI(LOG_INFO, "FunctionCall binding (global): %{public}s -> %{public}s", binding.propertyName_.c_str(),
            varName.c_str());
        InsertComponentIntoIndex(globalVarBindingIndex_, varName, componentId);
    }
}

void BindingEngine::UnregisterBindingPaths(
    const std::shared_ptr<Component>& comp, const std::string& surfaceId, const std::vector<DataBinding>& bindings)
{
    if (comp == nullptr || bindings.empty()) {
        return;
    }

    const std::string componentId = comp->GetComponentId();
    auto dataModel = GetOrCreateDataModel(surfaceId);
    for (const auto& binding : bindings) {
        if (binding.type_ == BindingType::EXPRESSION) {
            UnregisterExpressionBindingPaths(dataModel, binding, componentId);
        } else {
            UnregisterNonExpressionBindingPaths(dataModel, binding, componentId);
        }
    }
}

void BindingEngine::UnregisterExpressionBindingPaths(
    const std::shared_ptr<DataModel>& dataModel, const DataBinding& binding, const std::string& componentId)
{
    for (const auto& varName : binding.globalVarDeps_) {
        if (varName != "__dataModel") {
            EraseComponentFromIndex(globalVarBindingIndex_, varName, componentId);
            continue;
        }
        if (binding.dataPath_.empty()) {
            continue;
        }
        EraseComponentFromIndex(bindingIndex_, binding.dataPath_, componentId);
        dataModel->UnregisterInterest(binding.dataPath_, componentId);
    }
}

void BindingEngine::UnregisterNonExpressionBindingPaths(
    const std::shared_ptr<DataModel>& dataModel, const DataBinding& binding, const std::string& componentId)
{
    if (!binding.dataPath_.empty()) {
        EraseComponentFromIndex(bindingIndex_, binding.dataPath_, componentId);
        dataModel->UnregisterInterest(binding.dataPath_, componentId);
    }
    if (binding.type_ != BindingType::FUNCTION_CALL) {
        return;
    }
    for (const auto& varName : binding.globalVarDeps_) {
        if (varName.empty() || varName == "__dataModel") {
            continue;
        }
        EraseComponentFromIndex(globalVarBindingIndex_, varName, componentId);
    }
}

// ========== 通知和查询 ==========

void BindingEngine::NotifyAllBindings() {}

void BindingEngine::NotifyGlobalVariableChanged(const std::string& varName)
{
#ifdef ENABLE_EXPRESSION_ENGINE
    auto it = globalVarBindingIndex_.find(varName);
    if (it == globalVarBindingIndex_.end()) {
        return;
    }

    std::vector<std::string> componentIds(it->second.begin(), it->second.end());
    for (const auto& compId : componentIds) {
        auto compIt = components_.find(compId);
        if (compIt == components_.end()) {
            continue;
        }

        auto& comp = compIt->second;
        std::unordered_set<std::string> refreshedProperties;
        for (const auto& binding : comp->GetDataBindings()) {
            if (binding.type_ != BindingType::EXPRESSION && binding.type_ != BindingType::FUNCTION_CALL) {
                continue;
            }

            bool depends = std::find(binding.globalVarDeps_.begin(), binding.globalVarDeps_.end(), varName) !=
                           binding.globalVarDeps_.end();
            if (!depends) {
                continue;
            }
            if (!refreshedProperties.insert(binding.propertyName_).second) {
                continue;
            }
            comp->OnDataUpdate(binding.propertyName_, JsonValue());
        }
    }
#endif
}

std::shared_ptr<Component> BindingEngine::GetComponent(const std::string& id)
{
    auto it = components_.find(id);
    return (it != components_.end()) ? it->second : nullptr;
}

void BindingEngine::RenderAll() const
{
    LOG_A2UI(LOG_INFO, "RenderAll");
    // This function is currently commented out in the original code
    // Reserved for future use
}

void BindingEngine::PrintBindingStats() const
{
    LOG_A2UI(LOG_INFO, "========== Binding Statistics ==========");
    LOG_A2UI(LOG_INFO, "Total components: %{public}zu", components_.size());
    LOG_A2UI(LOG_INFO, "Total binding paths: %{public}zu", bindingIndex_.size());
    LOG_A2UI(LOG_INFO, "Total data models: %{public}zu", dataModels_.size());
}

} // namespace NativeModule
