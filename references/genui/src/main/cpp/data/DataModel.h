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

#ifndef A2UIRENDER_DATAMODEL_H
#define A2UIRENDER_DATAMODEL_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "components/Component.h"

namespace NativeModule {

class BindingEngine;

// 更新数据模型的结构
struct DataModelUpdate {
    std::string surfaceId;          // 目标 surface ID
    std::string path;               // 可选：JSON 路径 (如 "/user/name")
    std::optional<JsonValue> value; // 可选：新值（json）
};

class DataModel : public std::enable_shared_from_this<DataModel> {
public:
    static constexpr int32_t MAX_DATA_MODEL_DEPTH = 20;

    DataModel() : surfaceId_("default") {}
    explicit DataModel(const std::string& surfaceId) : surfaceId_(surfaceId) {}

    void SetEngine(const std::shared_ptr<BindingEngine>& eng)
    {
        engine_ = eng;
    }

    // 设置/更新数据
    void Update(const std::map<std::string, std::string>& data);

    // 新的更新方法：支持路径、删除和替换
    bool UpdateByPath(const std::string& path, const JsonValue& value);
    bool DeleteByPath(const std::string& path);
    bool ReplaceAll(const JsonValue& value);

    // 处理 DataModelUpdate 请求
    void ProcessUpdate(const DataModelUpdate& updateRequest);

    // 获取 JsonValue 节点
    std::optional<JsonValue> GetNode(const std::string& path, bool decodePointer = false) const;

    // 注册组件对某路径的兴趣
    void RegisterInterest(const std::string& path, std::shared_ptr<Component> component);
    void UnregisterInterest(const std::string& path, const std::string& componentId);

    // 通知所有关注某路径的组件
    void NotifyPathUpdate(const std::string& path);

    std::shared_ptr<JsonValue> GetRoot() const
    {
        return root_;
    }
    void SetRoot(std::shared_ptr<JsonValue> root)
    {
        root_ = root;
    }

    const std::string& GetSurfaceId() const
    {
        return surfaceId_;
    }

    // 获取所有已注册的路径
    std::vector<std::string> GetRegisteredPaths() const;

    // 测量 JsonValue 的嵌套深度
    static int32_t MeasureJsonDepth(const JsonValue& value);

private:
    // 路径解析：将 "/user/name" 分割为 ["user", "name"]
    std::vector<std::string> ParsePath(const std::string& path, bool decodePointer = false) const;

    // 解析路径并获取父节点和最后一段键
    std::pair<JsonValue*, std::string> ResolvePathParent(JsonValue* node, const std::vector<std::string>& pathParts);

    // 检查路径是否被某个注册路径匹配
    bool IsPathAffected(const std::string& updatedPath, const std::string& registeredPath) const;
    JsonValue BuildNotificationValue(const std::string& path) const;
    std::vector<std::shared_ptr<Component>> CollectLiveSubscribers(
        const std::vector<std::weak_ptr<Component>>& subscribers) const;
    void NotifySubscribersForPath(const std::string& path, const JsonValue& valueToUpdate,
        const std::vector<std::shared_ptr<Component>>& liveSubscribers) const;
    void CleanupExpiredSubscribers(const std::string& path);

    std::string surfaceId_;
    std::shared_ptr<JsonValue> root_;
    std::unordered_map<std::string, std::vector<std::weak_ptr<Component>>> pathToComponents_;
    std::weak_ptr<BindingEngine> engine_;
};

} // namespace NativeModule

#endif // A2UIRENDER_DATAMODEL_H
