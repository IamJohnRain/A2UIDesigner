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

#include "DataModel.h"

#include <regex>
#include <sstream>

#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

// ========== 辅助函数 ==========

// 辅助函数：创建空的 JSON 对象
static std::shared_ptr<JsonValue> CreateEmptyJsonObject()
{
    auto adapter = JsonAdapter::Parse("{}");
    if (adapter && adapter->GetRoot().IsValid()) {
        return std::make_shared<JsonValue>(adapter->GetRoot());
    }
    return nullptr;
}

namespace {

std::string DecodeJsonPointerToken(const std::string& token)
{
    std::string result;
    result.reserve(token.size());
    size_t i = 0;
    while (i < token.size()) {
        if (token[i] == '~' && i + 1 < token.size()) {
            if (token[i + 1] == '1') {
                result.push_back('/');
                i += 2;
                continue;
            }
            if (token[i + 1] == '0') {
                result.push_back('~');
                i += 2;
                continue;
            }
        }
        result.push_back(token[i]);
        ++i;
    }
    return result;
}

int32_t MeasureJsonDepthInternal(const JsonValue& value, int32_t currentDepth)
{
    if (!value.IsValid()) {
        return currentDepth;
    }

    if (value.IsObject()) {
        int32_t maxChildDepth = currentDepth;
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            JsonValue childValue = value.GetItem(child.GetKey().c_str());
            int32_t childDepth = MeasureJsonDepthInternal(childValue, currentDepth + 1);
            if (childDepth > maxChildDepth) {
                maxChildDepth = childDepth;
            }
        }
        return maxChildDepth;
    }

    if (value.IsArray()) {
        int32_t maxChildDepth = currentDepth;
        int32_t size = value.GetArraySize();
        for (int32_t i = 0; i < size; ++i) {
            int32_t childDepth = MeasureJsonDepthInternal(value.GetArrayItem(i), currentDepth + 1);
            if (childDepth > maxChildDepth) {
                maxChildDepth = childDepth;
            }
        }
        return maxChildDepth;
    }

    return currentDepth;
}

} // namespace

int32_t DataModel::MeasureJsonDepth(const JsonValue& value)
{
    return MeasureJsonDepthInternal(value, 0);
}

// ========== 路径解析相关 ==========

std::vector<std::string> DataModel::ParsePath(const std::string& path, bool decodePointer) const
{
    std::vector<std::string> parts;
    if (path.empty() || path == "/") {
        return parts;
    }

    // 移除开头的 /
    std::string workPath = path;
    if (workPath[0] == '/') {
        workPath = workPath.substr(1);
    }

    // 分割路径
    std::stringstream ss(workPath);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            parts.push_back(decodePointer ? DecodeJsonPointerToken(part) : part);
        }
    }

    return parts;
}

std::pair<JsonValue*, std::string> DataModel::ResolvePathParent(
    JsonValue* node, const std::vector<std::string>& pathParts)
{
    if (pathParts.empty() || !node || !node->IsValid()) {
        return { nullptr, "" };
    }

    // 如果只有一个部分，返回当前节点和该键
    if (pathParts.size() == 1) {
        return { node, pathParts[0] };
    }

    // 遍历路径到倒数第二层
    JsonValue* current = node;
    for (size_t i = 0; i < pathParts.size() - 1; ++i) {
        const std::string& key = pathParts[i];

        if (!current->IsObject()) {
            return { nullptr, "" };
        }

        // 检查是否存在该键
        if (!current->Has(key.c_str())) {
            // 如果不存在，创建一个空对象
            current->PutObject(key.c_str());
        }

        // 获取子节点（注意：这里需要返回一个可修改的引用）
        // 由于 JsonValue 是值类型，我们需要特殊处理
        // 这里我们简化处理，直接返回父节点指针
        auto child = current->GetObject(key.c_str());
        if (!child.IsValid()) {
            return { nullptr, "" };
        }

        // 注意：这个实现有局限性，因为 JsonValue 是值语义
        // 实际上我们需要修改现有的节点结构
        // 为了简化，我们直接返回最后一个键
    }

    // 返回最后一级的键
    // 注意：这个实现需要修改以支持真正的路径解析
    return { current, pathParts.back() };
}

bool DataModel::IsPathAffected(const std::string& updatedPath, const std::string& registeredPath) const
{
    // 如果更新路径是根路径，影响所有注册路径
    if (updatedPath.empty() || updatedPath == "/") {
        return true;
    }

    // 如果注册路径是更新路径的前缀，则受影响
    // 例如：更新 "/user/name" 影响 "/user"
    if (registeredPath.empty() || registeredPath == "/") {
        return true;
    }

    // 检查更新路径是否以注册路径开头
    if (updatedPath.find(registeredPath) == 0) {
        // 如果长度相同或更新路径在注册路径后有一个 /
        size_t regLen = registeredPath.length();
        if (updatedPath.length() == regLen) {
            return true; // 完全相同
        }
        if (updatedPath.length() > regLen && updatedPath[regLen] == '/') {
            return true; // 更新路径是注册路径的子路径
        }
    }

    // 检查注册路径是否以更新路径开头
    // 例如：更新 "/user" 影响 "/user/name"
    if (registeredPath.find(updatedPath) == 0) {
        size_t updLen = updatedPath.length();
        if (registeredPath.length() == updLen) {
            return true; // 完全相同
        }
        if (registeredPath.length() > updLen && registeredPath[updLen] == '/') {
            return true; // 注册路径是更新路径的子路径
        }
    }

    return false;
}

// ========== 数据更新相关 ==========

void DataModel::Update(const std::map<std::string, std::string>& data)
{
    // 如果根节点不存在，创建一个
    if (!root_ || !root_->IsValid()) {
        root_ = CreateEmptyJsonObject();
        if (!root_ || !root_->IsValid()) {
            LOG_A2UI(LOG_ERROR, "Failed to create root JSON object");
            return;
        }
    }

    // 构建新的根节点
    // 将 map<string, string> 转换为 JSON 对象
    for (const auto& [path, jsonStr] : data) {
        LOG_A2UI(LOG_INFO, "Updating path: %{public}s", path.c_str());

        // 解析 JSON 字符串
        auto adapter = JsonAdapter::Parse(jsonStr);
        if (adapter != nullptr && adapter->GetRoot().IsValid()) {
            // TODO: Store the parsed JSON data in the data model
            // For now, just log the update
            LOG_A2UI(LOG_INFO, "Successfully parsed JSON for path: %{public}s", path.c_str());
        }
    }

    // 通知所有已注册路径更新
    for (const auto& [path, _] : pathToComponents_) {
        NotifyPathUpdate(path);
    }
}

namespace {

bool CloneJsonValue(const JsonValue& source, JsonValue& target)
{
    if (!source.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(source);
    if (adapter == nullptr) {
        return false;
    }
    target = adapter->GetRoot();
    return target.IsValid();
}

bool CreateObjectValue(JsonValue& value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return false;
    }
    value = adapter->GetRoot();
    return value.IsObject();
}

bool BuildUpdatedNode(const JsonValue& current, const std::vector<std::string>& pathParts, size_t index,
    const JsonValue& leafValue, JsonValue& updated)
{
    if (index >= pathParts.size()) {
        return CloneJsonValue(leafValue, updated);
    }

    JsonValue objectNode;
    if (current.IsValid() && current.IsObject()) {
        if (!CloneJsonValue(current, objectNode)) {
            return false;
        }
    } else if (!CreateObjectValue(objectNode)) {
        return false;
    }

    const std::string& key = pathParts[index];
    JsonValue child = objectNode.GetItem(key.c_str());
    JsonValue updatedChild;
    if (!BuildUpdatedNode(child, pathParts, index + 1, leafValue, updatedChild)) {
        return false;
    }
    if (!objectNode.Set(key.c_str(), updatedChild)) {
        return false;
    }

    updated = objectNode;
    return true;
}

bool BuildNodeWithoutPath(const JsonValue& current, const std::vector<std::string>& pathParts, size_t index,
    JsonValue& updated, bool& removed)
{
    if (!current.IsValid() || !current.IsObject() || index >= pathParts.size()) {
        removed = false;
        return CloneJsonValue(current, updated);
    }

    JsonValue objectNode;
    if (!CloneJsonValue(current, objectNode)) {
        removed = false;
        return false;
    }

    const std::string& key = pathParts[index];
    if (index == pathParts.size() - 1) {
        removed = objectNode.Remove(key.c_str());
        updated = objectNode;
        return true;
    }

    JsonValue child = objectNode.GetItem(key.c_str());
    if (!child.IsValid()) {
        removed = false;
        updated = objectNode;
        return true;
    }

    JsonValue updatedChild;
    if (!BuildNodeWithoutPath(child, pathParts, index + 1, updatedChild, removed)) {
        return false;
    }
    if (removed && !objectNode.Set(key.c_str(), updatedChild)) {
        return false;
    }

    updated = objectNode;
    return true;
}

} // namespace

bool DataModel::UpdateByPath(const std::string& path, const JsonValue& value)
{
    LOG_A2UI(LOG_INFO, "UpdateByPath: path=%{public}s", path.c_str());

    // 确保根节点存在
    if (!root_ || !root_->IsValid()) {
        root_ = CreateEmptyJsonObject();
        if (!root_ || !root_->IsValid()) {
            LOG_A2UI(LOG_ERROR, "Failed to create root JSON object");
            return false;
        }
    }

    // 解析路径
    auto pathParts = ParsePath(path);
    if (pathParts.empty()) {
        // 如果路径为空或只是 "/"，替换整个根节点
        return ReplaceAll(value);
    }

    JsonValue updatedRoot;
    if (!BuildUpdatedNode(*root_, pathParts, 0, value, updatedRoot)) {
        LOG_A2UI(LOG_ERROR, "UpdateByPath: build updated root failed");
        return false;
    }

    root_ = std::make_shared<JsonValue>(updatedRoot);
    LOG_A2UI(LOG_INFO, "Successfully updated path: %{public}s", path.c_str());

    // 通知受影响的组件
    for (const auto& [regPath, _] : pathToComponents_) {
        if (IsPathAffected(path, regPath)) {
            NotifyPathUpdate(regPath);
        }
    }
    return true;
}

bool DataModel::DeleteByPath(const std::string& path)
{
    LOG_A2UI(LOG_INFO, "DeleteByPath: path=%{public}s", path.c_str());

    if (!root_ || !root_->IsValid()) {
        LOG_A2UI(LOG_WARN, "Cannot delete: root is null or invalid");
        return false;
    }

    // 解析路径
    auto pathParts = ParsePath(path);
    if (pathParts.empty()) {
        // 删除整个根节点
        root_ = nullptr;
        LOG_A2UI(LOG_INFO, "Deleted entire root node");
    } else {
        JsonValue updatedRoot;
        bool removed = false;
        if (!BuildNodeWithoutPath(*root_, pathParts, 0, updatedRoot, removed)) {
            LOG_A2UI(LOG_ERROR, "DeleteByPath: build updated root failed");
            return false;
        }
        if (removed) {
            root_ = std::make_shared<JsonValue>(updatedRoot);
            LOG_A2UI(LOG_INFO, "Successfully deleted path: %{public}s", path.c_str());
        } else {
            LOG_A2UI(LOG_WARN, "DeleteByPath: key not found, path=%{public}s", path.c_str());
            return false;
        }
    }

    // 通知受影响的组件
    for (const auto& [regPath, _] : pathToComponents_) {
        if (IsPathAffected(path, regPath)) {
            NotifyPathUpdate(regPath);
        }
    }
    return true;
}

bool DataModel::ReplaceAll(const JsonValue& value)
{
    LOG_A2UI(LOG_INFO, "ReplaceAll: pathToComponents size: %{public}zu", pathToComponents_.size());

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(value);
    if (adapter == nullptr || !adapter->GetRoot().IsValid()) {
        LOG_A2UI(LOG_ERROR, "ReplaceAll: failed to clone root value");
        return false;
    }

    root_ = std::make_shared<JsonValue>(adapter->GetRoot());

    // 通知所有已注册路径更新
    for (const auto& [path, _] : pathToComponents_) {
        NotifyPathUpdate(path);
    }
    return true;
}

void DataModel::ProcessUpdate(const DataModelUpdate& updateRequest)
{
    LOG_A2UI(LOG_INFO, "ProcessUpdate: surfaceId=%{public}s", updateRequest.surfaceId.c_str());

    // 检查 surfaceId 是否匹配
    if (updateRequest.surfaceId != surfaceId_) {
        LOG_A2UI(LOG_WARN, "Surface ID mismatch: expected=%{public}s, got=%{public}s", surfaceId_.c_str(),
            updateRequest.surfaceId.c_str());
        return;
    }

    // 判断更新类型
    if (!updateRequest.path.empty() && updateRequest.value.has_value()) {
        // 有 path 和 value：更新特定字段
        UpdateByPath(updateRequest.path, updateRequest.value.value());
    } else if (!updateRequest.path.empty() && !updateRequest.value.has_value()) {
        // 只有 path：删除字段
        DeleteByPath(updateRequest.path);
    } else if (updateRequest.path.empty() && updateRequest.value.has_value()) {
        // 只有 value：替换整个数据模型
        ReplaceAll(updateRequest.value.value());
    } else {
        LOG_A2UI(LOG_WARN, "Invalid update request: both path and value are empty");
    }
}

// ========== 数据获取相关 ==========

std::optional<JsonValue> DataModel::GetNode(const std::string& path, bool decodePointer) const
{
    if (!root_ || !root_->IsValid()) {
        return std::nullopt;
    }

    // 如果路径为空或只是 "/"，返回根节点
    if (path.empty() || path == "/") {
        return *root_;
    }

    // 解析路径
    auto pathParts = ParsePath(path, decodePointer);
    if (pathParts.empty()) {
        return *root_;
    }

    // 遍历路径
    JsonValue current = *root_;
    for (const auto& part : pathParts) {
        // 新增：支持数组索引访问
        if (current.IsArray()) {
            try {
                int index = std::stoi(part);
                if (index < 0 || index >= current.GetArraySize()) {
                    LOG_A2UI(LOG_WARN, "GetNode: array index out of range, index=%{public}d, size=%{public}d", index,
                        current.GetArraySize());
                    return std::nullopt;
                }
                current = current.GetArrayItem(index);
            } catch (...) {
                LOG_A2UI(LOG_WARN, "GetNode: invalid array index '%{public}s'", part.c_str());
                return std::nullopt; // 不是有效的数字索引
            }
        } else if (current.IsObject()) {
            if (!current.Has(part.c_str())) {
                LOG_A2UI(LOG_WARN, "GetNode: object property not found, property='%{public}s'", part.c_str());
                return std::nullopt;
            }
            current = current.GetObject(part.c_str());
        } else {
            LOG_A2UI(LOG_WARN, "GetNode: current node is neither object nor array", part.c_str());
            return std::nullopt; // 既不是对象也不是数组
        }

        if (!current.IsValid()) {
            LOG_A2UI(LOG_WARN, "GetNode: invalid node encountered at part='%{public}s'", part.c_str());
            return std::nullopt;
        }
    }

    return current;
}

// ========== 组件注册与通知相关 ==========

void DataModel::RegisterInterest(const std::string& path, std::shared_ptr<Component> component)
{
    pathToComponents_[path].push_back(component);
    LOG_A2UI(LOG_INFO, "Registered interest for path: %{public}s", path.c_str());
}

void DataModel::UnregisterInterest(const std::string& path, const std::string& componentId)
{
    auto it = pathToComponents_.find(path);
    if (it == pathToComponents_.end()) {
        return;
    }

    std::vector<std::weak_ptr<Component>> validRefs;
    for (auto& weakComp : it->second) {
        auto comp = weakComp.lock();
        if (comp == nullptr) {
            continue;
        }
        if (comp->GetComponentId() == componentId) {
            continue;
        }
        validRefs.push_back(weakComp);
    }

    if (validRefs.empty()) {
        pathToComponents_.erase(it);
        return;
    }
    it->second = std::move(validRefs);
}

JsonValue DataModel::BuildNotificationValue(const std::string& path) const
{
    std::optional<JsonValue> valueOpt = GetNode(path);
    if (!valueOpt.has_value()) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::Clone(valueOpt.value());
    if (valueAdapter == nullptr) {
        return JsonValue();
    }
    return valueAdapter->GetRoot();
}

std::vector<std::shared_ptr<Component>> DataModel::CollectLiveSubscribers(
    const std::vector<std::weak_ptr<Component>>& subscribers) const
{
    std::vector<std::shared_ptr<Component>> liveSubscribers;
    liveSubscribers.reserve(subscribers.size());
    for (auto& weakComp : subscribers) {
        auto comp = weakComp.lock();
        if (comp != nullptr) {
            liveSubscribers.push_back(comp);
        }
    }
    return liveSubscribers;
}

void DataModel::NotifySubscribersForPath(const std::string& path, const JsonValue& valueToUpdate,
    const std::vector<std::shared_ptr<Component>>& liveSubscribers) const
{
    for (const auto& comp : liveSubscribers) {
        if (comp == nullptr) {
            continue;
        }
        for (const auto& binding : comp->GetDataBindings()) {
            if (binding.dataPath_ == path) {
                LOG_A2UI(LOG_INFO, "Notifying component %{public}s: property=%{public}s",
                    comp->GetComponentId().c_str(), binding.propertyName_.c_str());
                comp->OnDataUpdate(binding.propertyName_, valueToUpdate);
            }
        }
    }
}

void DataModel::CleanupExpiredSubscribers(const std::string& path)
{
    auto latestIt = pathToComponents_.find(path);
    if (latestIt == pathToComponents_.end()) {
        return;
    }
    std::vector<std::weak_ptr<Component>> cleanedRefs;
    cleanedRefs.reserve(latestIt->second.size());
    for (auto& weakComp : latestIt->second) {
        if (weakComp.lock() != nullptr) {
            cleanedRefs.push_back(weakComp);
        }
    }
    if (cleanedRefs.empty()) {
        pathToComponents_.erase(latestIt);
    } else {
        latestIt->second = std::move(cleanedRefs);
    }
}

void DataModel::NotifyPathUpdate(const std::string& path)
{
    LOG_A2UI(LOG_INFO, "NotifyPathUpdate: %{public}s", path.c_str());
    JsonValue valueToUpdate = BuildNotificationValue(path);
    auto it = pathToComponents_.find(path);
    if (it == pathToComponents_.end()) {
        return;
    }

    std::vector<std::shared_ptr<Component>> liveSubscribers = CollectLiveSubscribers(it->second);
    NotifySubscribersForPath(path, valueToUpdate, liveSubscribers);
    CleanupExpiredSubscribers(path);
}

std::vector<std::string> DataModel::GetRegisteredPaths() const
{
    std::vector<std::string> paths;
    for (const auto& [path, _] : pathToComponents_) {
        paths.push_back(path);
    }
    return paths;
}

} // namespace NativeModule
