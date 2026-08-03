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

#ifndef A2UIRENDER_BINDINGENGINE_H
#define A2UIRENDER_BINDINGENGINE_H

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "components/Component.h"
#include "data/DataModel.h"

namespace NativeModule {

class BindingEngine : public std::enable_shared_from_this<BindingEngine> {
public:
    // Factory method to create BindingEngine instances
    static std::shared_ptr<BindingEngine> Create();

    // ========== 组件管理 ==========

    void RegisterComponent(std::shared_ptr<Component> comp);
    void SyncComponentBindings(std::shared_ptr<Component> comp);

    // ========== 数据模型管理 ==========

    void UpdateDataModel(const std::map<std::string, std::string>& data);

    // 新的更新接口：支持路径、删除和替换
    void UpdateDataModelByPath(const std::string& surfaceId, const std::string& path, const JsonValue& value);
    void DeleteDataModelByPath(const std::string& surfaceId, const std::string& path);
    void ReplaceDataModel(const std::string& surfaceId, const JsonValue& value);

    // 处理 DataModelUpdate 请求
    void ProcessUpdate(const DataModelUpdate& updateRequest);

    // 获取或创建指定 surfaceId 的数据模型
    std::shared_ptr<DataModel> GetOrCreateDataModel(const std::string& surfaceId);

    // ========== 绑定执行 ==========

    void BindComponentImmediate(
        std::shared_ptr<Component> comp, const std::string& surfaceId, bool refreshExpressionBindings = true);

    void NotifyAllBindings();

    void NotifyGlobalVariableChanged(const std::string& varName);

    // ========== 查询接口 ==========

    std::shared_ptr<Component> GetComponent(const std::string& id);

    void RenderAll() const;

    void PrintBindingStats() const;

    // 获取默认数据模型（向后兼容）
    std::shared_ptr<DataModel> GetDataModel() const
    {
        return defaultDataModel_;
    }

private:
    // Private constructor
    BindingEngine() : defaultDataModel_(std::make_shared<DataModel>("default")) {}

    void Initialize();
    void ProcessPendingComponents();
    std::string BuildBindingKey(const DataBinding& binding) const;
    void RegisterBindingPaths(
        const std::shared_ptr<Component>& comp, const std::string& surfaceId, const std::vector<DataBinding>& bindings);
    void UnregisterBindingPaths(
        const std::shared_ptr<Component>& comp, const std::string& surfaceId, const std::vector<DataBinding>& bindings);

    // 组件注册表
    std::unordered_map<std::string, std::shared_ptr<Component>> components_;

    // 数据模型集合：支持多个 surfaceId
    std::unordered_map<std::string, std::shared_ptr<DataModel>> dataModels_;

    // 默认数据模型（向后兼容）
    std::shared_ptr<DataModel> defaultDataModel_;

    // 绑定关系索引：path -> set<component_id>
    std::unordered_map<std::string, std::unordered_set<std::string>> bindingIndex_;
    std::unordered_map<std::string, std::vector<DataBinding>> componentBindings_;

    // 全局变量绑定索引：varName -> set<component_id>
    std::unordered_map<std::string, std::unordered_set<std::string>> globalVarBindingIndex_;

    // 待处理队列（处理时序问题）
    std::vector<std::shared_ptr<Component>> pendingComponents_;
    bool dataModelReady_ = false;
};

} // namespace NativeModule

#endif // A2UIRENDER_BINDINGENGINE_H
