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

#include "ListAdapterNode.h"

#include "components/A2UI/list/ListComponent.h"
#include "data/DynamicValueResolver.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

void UpdateAdapterItemCounts(const std::shared_ptr<TemplateAdapterNode>& adapter, const std::string& parentPath,
    const std::shared_ptr<DataModel>& dataModel)
{
    if (adapter == nullptr) {
        return;
    }
    std::string adapterPath = adapter->GetDataPath();
    if (adapterPath.empty()) {
        return;
    }
    std::string path = adapterPath;
    if (path[0] != '/') {
        path = parentPath + "/" + path;
    }
    auto arrayOpt = dataModel->GetNode(path);
    if (!arrayOpt.has_value()) {
        DynamicResolveContext context = { .renderId = adapter->GetRenderId(),
            .surfaceId = adapter->GetSurfaceId(),
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        DynamicValueResolver::ReportMissingPath(context, path);
        adapter->UpdateItemCount(0);
        return;
    }
    if (!arrayOpt.value().IsArray()) {
        return;
    }
    int itemCount = arrayOpt.value().GetArraySize();
    adapter->UpdateItemCount(itemCount);
    LOG_A2UI(LOG_INFO, "UpdateNestedItemCounts: updated adapter itemCount=%{public}d, path=%{public}s", itemCount,
        path.c_str());
}

} // namespace

/**
 * @brief 当列表项创建后，递归更新嵌套 List 适配器的 itemCount。
 *
 * 遍历组件树，查找所有处于懒加载模式的 ListComponent，
 * 解析其相对/绝对数据路径，从数据模型读取数组长度并更新。
 *
 * @param component  当前遍历的组件
 * @param parentPath 父级数据路径（如 "/users/2"）
 * @param dataModel  数据模型引用
 */
void ListAdapterNode::UpdateNestedItemCounts(
    const std::shared_ptr<Component>& component, const std::string& parentPath, std::shared_ptr<DataModel> dataModel)
{
    if (!component || !dataModel) {
        return;
    }

    auto listComp = std::dynamic_pointer_cast<ListComponent>(component);
    if (listComp && listComp->IsLazyMode()) {
        UpdateAdapterItemCounts(listComp->GetAdapterNode(), parentPath, dataModel);
    }

    const std::list<std::shared_ptr<Component>>& children = component->GetChildren();
    for (const auto& child : children) {
        UpdateNestedItemCounts(child, parentPath, dataModel);
    }
}

/**
 * @brief 列表项创建后，递归更新嵌套 List 适配器的 itemCount。
 */
void ListAdapterNode::OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath)
{
    UpdateNestedItemCounts(component, parentPath, GetDataModel());
}

/**
 * @brief 当 BuildComponentTree 检测到嵌套的模板 List 时，递归创建 ListAdapterNode。
 *
 * 这是 List 组件特有的逻辑：List 内部的子 List 也使用懒加载适配器时，
 * 需要递归创建 ListAdapterNode 并配置到嵌套的 ListComponent 上。
 */
void ListAdapterNode::SetupNestedAdapter(const std::shared_ptr<Component>& component, const std::string& componentType,
    const std::string& templateComponentId, const std::string& templatePath,
    const std::map<std::string, JsonValue>& descriptors)
{
    if (componentType != "List") {
        return;
    }

    auto listComp = std::dynamic_pointer_cast<ListComponent>(component);
    if (!listComp) {
        return;
    }

    auto templateIt = descriptors.find(templateComponentId);
    if (templateIt == descriptors.end()) {
        LOG_A2UI(LOG_ERROR, "ListAdapterNode::SetupNestedAdapter: template not found, templateComponentId=%{public}s",
            templateComponentId.c_str());
        return;
    }

    auto adapterNode = std::make_shared<ListAdapterNode>();

    adapterNode->Initialize(templateComponentId, templatePath, 0);
    adapterNode->SetDataModel(GetDataModel());
    adapterNode->SetTemplateDescriptor(templateIt->second);
    adapterNode->SetAllDescriptors(descriptors);
    adapterNode->SetSurfaceInfo(GetSurfaceId(), GetRenderId());
    adapterNode->SetSurfaceContext(component->GetSurfaceContext());

    listComp->SetLazyMode(true);
    listComp->SetAdapterNode(adapterNode);

    LOG_A2UI(LOG_INFO, "ListAdapterNode::SetupNestedAdapter: set up adapter for nested List, templatePath=%{public}s",
        templatePath.c_str());
}

TemplateAdapterNode::ItemWrapperInfo ListAdapterNode::BuildItemWrapper(
    const std::shared_ptr<Component>& component) const
{
    ItemWrapperInfo wrapperInfo;
    if (component == nullptr || component->GetNativeView() == nullptr) {
        return wrapperInfo;
    }

    ArkUI_NodeHandle listItemNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST_ITEM);
    if (listItemNode == nullptr) {
        return wrapperInfo;
    }

    ArkUINodeApiAdapter::AddChild(listItemNode, component->GetNativeView());
    wrapperInfo.rootNode = listItemNode;
    wrapperInfo.contentParentNode = listItemNode;
    return wrapperInfo;
}

} // namespace NativeModule
