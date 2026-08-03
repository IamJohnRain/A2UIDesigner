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

#include "GridAdapterNode.h"

#include "components/extended/ExtendedGridComponent.h"
#include "utils/LogA2UI.h"

#include "A2UIArkUITypeConverter.h"

namespace NativeModule {

namespace {

void ApplyGridItemDefaultLayoutPolicy(ArkUI_NodeHandle gridItemNode, bool wrapContent)
{
    if (gridItemNode == nullptr) {
        return;
    }
    if (!wrapContent) {
        ArkUINodeApiAdapter::ResetNodeHeightLayoutPolicy(gridItemNode);
        return;
    }
    ArkUINodeApiAdapter::SetNodeHeightLayoutPolicy(
        gridItemNode, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT));
}

} // namespace

void GridAdapterNode::SetGridItemHeightWrapContent(bool wrapContent)
{
    gridItemHeightWrapContent_ = wrapContent;
}

void GridAdapterNode::UpdateNestedItemCounts(
    const std::shared_ptr<Component>& component, const std::string& parentPath, std::shared_ptr<DataModel> dataModel)
{
    if (!component || !dataModel) {
        return;
    }

    auto gridComponent = std::dynamic_pointer_cast<ExtendedGridComponent>(component);
    if (gridComponent && gridComponent->IsLazyMode()) {
        auto adapter = gridComponent->GetAdapterNode();
        if (adapter) {
            std::string adapterPath = adapter->GetDataPath();
            if (!adapterPath.empty()) {
                std::string path = adapterPath;
                if (path[0] != '/') {
                    path = parentPath + "/" + path;
                }

                auto arrayOpt = dataModel->GetNode(path);
                if (arrayOpt.has_value()) {
                    JsonValue arrayValue = arrayOpt.value();
                    if (arrayValue.IsArray()) {
                        int itemCount = arrayValue.GetArraySize();
                        adapter->UpdateItemCount(itemCount);
                        LOG_A2UI(LOG_INFO,
                            "GridAdapterNode::UpdateNestedItemCounts: updated itemCount=%{public}d, path=%{public}s",
                            itemCount, path.c_str());
                    }
                }
            }
        }
    }

    const std::list<std::shared_ptr<Component>>& children = component->GetChildren();
    for (const auto& child : children) {
        UpdateNestedItemCounts(child, parentPath, dataModel);
    }
}

void GridAdapterNode::OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath)
{
    UpdateNestedItemCounts(component, parentPath, GetDataModel());
}

void GridAdapterNode::SetupNestedAdapter(const std::shared_ptr<Component>& component, const std::string& componentType,
    const std::string& templateComponentId, const std::string& templatePath,
    const std::map<std::string, JsonValue>& descriptors)
{
    if (componentType != "Grid") {
        return;
    }

    auto gridComponent = std::dynamic_pointer_cast<ExtendedGridComponent>(component);
    if (!gridComponent) {
        return;
    }

    auto templateIt = descriptors.find(templateComponentId);
    if (templateIt == descriptors.end()) {
        LOG_A2UI(LOG_ERROR, "GridAdapterNode::SetupNestedAdapter: template not found, templateComponentId=%{public}s",
            templateComponentId.c_str());
        return;
    }

    auto adapterNode = std::make_shared<GridAdapterNode>();
    adapterNode->Initialize(templateComponentId, templatePath, 0);
    adapterNode->SetDataModel(GetDataModel());
    adapterNode->SetTemplateDescriptor(templateIt->second);
    adapterNode->SetAllDescriptors(descriptors);
    adapterNode->SetSurfaceInfo(GetSurfaceId(), GetRenderId());
    adapterNode->SetSurfaceContext(component->GetSurfaceContext());

    gridComponent->SetLazyMode(true);
    gridComponent->SetAdapterNode(adapterNode);

    LOG_A2UI(LOG_INFO, "GridAdapterNode::SetupNestedAdapter: set up adapter for nested Grid, templatePath=%{public}s",
        templatePath.c_str());
}

TemplateAdapterNode::ItemWrapperInfo GridAdapterNode::BuildItemWrapper(
    const std::shared_ptr<Component>& component) const
{
    ItemWrapperInfo wrapperInfo;
    if (component == nullptr || component->GetNativeView() == nullptr) {
        return wrapperInfo;
    }

    ArkUI_NodeHandle gridItemNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::GRID_ITEM);
    if (gridItemNode == nullptr) {
        return wrapperInfo;
    }

    ApplyGridItemDefaultLayoutPolicy(gridItemNode, gridItemHeightWrapContent_);
    ArkUINodeApiAdapter::AddChild(gridItemNode, component->GetNativeView());
    wrapperInfo.rootNode = gridItemNode;
    wrapperInfo.contentParentNode = gridItemNode;
    return wrapperInfo;
}

} // namespace NativeModule
