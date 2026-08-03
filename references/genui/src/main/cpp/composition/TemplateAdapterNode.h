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

#ifndef A2UI_TEMPLATE_ADAPTER_NODE_H
#define A2UI_TEMPLATE_ADAPTER_NODE_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "adapter/A2UIArkUITypes.h"
#include "components/Component.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

#include "SurfaceContext.h"

namespace NativeModule {

/**
 * @brief 基于模板和数据的懒加载适配器基类。
 *
 * 封装 ArkUI 原生 NodeAdapter 机制，提供模板实例化、数据路径重写、
 * 组件树构建、事件回调处理等通用能力。子类只需实现特定组件的适配器
 * 配置逻辑（如嵌套适配器的发现和更新）。
 *
 * 生命周期：
 * 1. 构造 → 创建原生 NodeAdapter 句柄。
 * 2. Initialize → 配置模板 ID、数据路径、项数，注册事件回调。
 * 3. ArkUI 框架通过事件回调驱动项的创建/回收。
 * 4. 析构 → 注销回调、释放句柄。
 */
class TemplateAdapterNode {
public:
    struct ItemWrapperInfo {
        ArkUI_NodeHandle rootNode = nullptr;
        ArkUI_NodeHandle contentParentNode = nullptr;
    };

    virtual ~TemplateAdapterNode();
    static void RewriteDataPaths(JsonValue& json, const std::string& arrayPath, int32_t itemIndex);
    static std::string BuildTemplateInstanceTreeDescriptors(std::string& id, const std::string& templateComponentId,
        const std::string& arrayPath, int32_t itemIndex, const std::map<std::string, JsonValue>* allDescriptors,
        std::map<std::string, JsonValue>* generatedDescriptors);

    static std::set<std::string> CollectReferencedDescriptorIds(
        const std::string& rootId, const std::map<std::string, JsonValue>& allDescriptors);

    void Initialize(const std::string& templateId, const std::string& dataPath, int itemCount,
        const std::string& indexVarName = "index", const std::string& itemVarName = "item");

    A2UINodeAdapterHandle GetHandle() const
    {
        return handle_;
    }

    void SetDataModel(std::shared_ptr<DataModel> dataModel);
    void SetTemplateDescriptor(const JsonValue& templateDescriptor);
    void SetAllDescriptors(const std::map<std::string, JsonValue>& descriptors);
    void SetInheritedLocalVariables(const std::map<std::string, JsonValue>& localVariables);
    void SetSurfaceInfo(const std::string& surfaceId, int32_t renderId);
    void SetSurfaceContext(const SurfaceContext& surfaceContext);

    void UpdateItemCount(int itemCount);
    const std::string& GetDataPath() const
    {
        return dataPath_;
    }
    const std::string& GetTemplateId() const
    {
        return templateId_;
    }
    const std::string& GetSurfaceId() const
    {
        return surfaceId_;
    }
    int32_t GetRenderId() const
    {
        return renderId_;
    }
    std::shared_ptr<DataModel> GetDataModel() const
    {
        return dataModel_;
    }
    const std::map<std::string, JsonValue>& GetAllDescriptors() const
    {
        return allDescriptors_;
    }
    void ReloadAllItems();
    void IncrementTemplateVersion();

protected:
    TemplateAdapterNode();

    /**
     * @brief 当一个列表项被创建并挂载后，通知子类更新嵌套适配器。
     *
     * 子类重写此方法以处理特定组件的嵌套适配器逻辑（如 List 中的
     * 嵌套 List 需要更新 itemCount，WaterFlow 可能有不同的嵌套策略）。
     *
     * @param component  已创建的项组件
     * @param parentPath 当前列表项的数据路径（如 "/users/0"）
     */
    virtual void OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath) = 0;

    /**
     * @brief 当在 BuildComponentTree 中检测到嵌套的模板子组件时，
     * 通知子类为该组件配置适配器。
     *
     * @param component          检测到的组件
     * @param componentType      组件类型
     * @param templateComponentId 模板组件 ID
     * @param templatePath       数据路径
     * @param descriptors        全局描述符映射
     */
    virtual void SetupNestedAdapter(const std::shared_ptr<Component>& component, const std::string& componentType,
        const std::string& templateComponentId, const std::string& templatePath,
        const std::map<std::string, JsonValue>& descriptors) = 0;

    virtual ItemWrapperInfo BuildItemWrapper(const std::shared_ptr<Component>& component) const;

    std::string templateId_;
    std::string dataPath_;
    std::string indexVarName_ = "index";
    std::string itemVarName_ = "item";
    int itemCount_ = 0;

    std::shared_ptr<DataModel> dataModel_;
    JsonValue templateDescriptor_;
    std::map<std::string, JsonValue> allDescriptors_;
    std::map<std::string, JsonValue> inheritedLocalVariables_;
    std::string surfaceId_;
    int32_t renderId_ = 0;
    SurfaceContext surfaceContext_;

    A2UINodeAdapterHandle handle_;

    uint32_t templateVersion_ = 0;
    std::unordered_map<ArkUI_NodeHandle, std::shared_ptr<Component>> items_;
    std::unordered_map<ArkUI_NodeHandle, ArkUI_NodeHandle> itemContentParents_;
    std::unordered_set<ArkUI_NodeHandle> movedWrappers_;

private:
    static void OnStaticAdapterEvent(A2UINodeAdapterEvent* event);
    void OnAdapterEvent(A2UINodeAdapterEvent* event);

    void OnNewItemIdCreated(A2UINodeAdapterEvent* event);
    void OnNewItemAttached(A2UINodeAdapterEvent* event);
    void OnItemDetached(A2UINodeAdapterEvent* event);

    static void CleanUpTargetComponentTreeFromAllComponents(
        const std::shared_ptr<Component>& component, std::map<std::string, std::shared_ptr<Component>>& allComponents);
    static std::string RewriteTemplateStringPaths(
        const std::string& str, const std::string& arrayPath, int32_t itemIndex);
    static std::unique_ptr<JsonAdapter> BuildTemplateInstanceDescriptorById(std::string& id,
        const std::string& templateComponentId, const std::string& arrayPath, int32_t itemIndex,
        const std::map<std::string, JsonValue>* allDescriptors, std::map<std::string, JsonValue>* generatedDescriptors);
    static void CollectReferencedDescriptorIdsRecursive(const std::string& id,
        const std::map<std::string, JsonValue>& allDescriptors, std::set<std::string>& result,
        std::set<std::string>& visited);
};

} // namespace NativeModule

#endif // A2UI_TEMPLATE_ADAPTER_NODE_H
