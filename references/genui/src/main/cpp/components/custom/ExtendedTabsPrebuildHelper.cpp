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

#include "components/custom/ExtendedTabsPrebuildHelper.h"

#include <map>
#include <memory>
#include <optional>
#include <set>

#include "components/Component.h"
#include "composition/ChildListParser.h"
#include "composition/TemplateAdapterNode.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"

#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

std::string GetShortComponentType(const std::string& componentType)
{
    size_t separatorIndex = componentType.find_last_of('.');
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= componentType.size()) {
        return componentType;
    }
    return componentType.substr(separatorIndex + 1);
}

bool IsExtendedTabsDescriptorType(const SurfaceSlot& surfaceSlot, const std::string& componentType)
{
    return componentType == "Tabs" && surfaceSlot.IsExtendedProtocolSurface();
}

std::map<std::string, JsonValue> BuildTemplateLocalVariables(
    const ChildListDescriptor& childList, const JsonValue& itemValue, int32_t itemIndex)
{
    std::map<std::string, JsonValue> localVariables;
    if (itemValue.IsValid() && !childList.resolvedItemVarName.empty()) {
        localVariables[childList.resolvedItemVarName] = itemValue;
    }
    if (!childList.resolvedIndexVarName.empty()) {
        std::unique_ptr<JsonAdapter> indexAdapter = JsonAdapter::CreateNumber(static_cast<double>(itemIndex));
        if (indexAdapter != nullptr) {
            JsonValue indexValue = indexAdapter->GetRoot();
            if (indexValue.IsValid()) {
                localVariables[childList.resolvedIndexVarName] = indexValue;
            }
        }
    }
    return localVariables;
}

void PrebuildStaticChildren(SurfaceSlot& surfaceSlot, const ChildListDescriptor& childList,
    const std::map<std::string, JsonValue>& descriptorStore)
{
    std::set<std::string> prebuiltChildIds;
    for (const std::string& childId : childList.staticChildIds) {
        if (childId.empty() || prebuiltChildIds.count(childId) > 0) {
            continue;
        }
        prebuiltChildIds.insert(childId);
        bool hasProcessedNode = false;
        bool sawRootDescriptor = false;
        surfaceSlot.BuildRootFromComponents(childId, descriptorStore, hasProcessedNode, sawRootDescriptor, false);
    }
}

void PrebuildTemplateChildren(SurfaceSlot& surfaceSlot, const JsonValue& nodeValue,
    const ChildListDescriptor& childList, const std::map<std::string, JsonValue>& descriptorStore)
{
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    if (dataModel == nullptr) {
        return;
    }
    std::optional<JsonValue> arrayValueOpt = dataModel->GetNode(childList.templatePath);
    if (!arrayValueOpt.has_value()) {
        DynamicResolveContext context = { .renderId = surfaceSlot.GetRenderId(),
            .surfaceId = surfaceSlot.GetSurfaceId(),
            .componentId = nodeValue.GetString("id", ""),
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        DynamicValueResolver::ReportMissingPath(context, childList.templatePath);
        return;
    }
    if (!arrayValueOpt->IsArray()) {
        return;
    }

    JsonValue arrayValue = arrayValueOpt.value();
    for (int32_t itemIndex = 0; itemIndex < arrayValue.GetArraySize(); ++itemIndex) {
        std::map<std::string, JsonValue> generatedDescriptors;
        std::string templateRootId = childList.templateComponentId;
        TemplateAdapterNode::TemplateInstanceBuildContext buildContext = {
            .templateComponentId = childList.templateComponentId,
            .arrayPath = childList.templatePath,
            .itemIndex = itemIndex,
            .allDescriptors = &descriptorStore,
            .generatedDescriptors = &generatedDescriptors,
        };
        std::string generatedInstanceId =
            TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(templateRootId, buildContext);
        if (generatedInstanceId.empty()) {
            continue;
        }

        std::map<std::string, JsonValue> localVariables =
            BuildTemplateLocalVariables(childList, arrayValue.GetArrayItem(itemIndex), itemIndex);
        Component::RegisterPendingLocalVariablesForComponents(generatedDescriptors, localVariables);
        bool hasProcessedNode = false;
        bool sawRootDescriptor = false;
        surfaceSlot.BuildRootFromComponents(
            generatedInstanceId, generatedDescriptors, hasProcessedNode, sawRootDescriptor, false);
        Component::ClearPendingLocalVariablesForComponents(generatedDescriptors);
    }
}

} // namespace

bool IsExtendedTabsChildComponentType(const std::string& componentType)
{
    return GetShortComponentType(componentType) == "TabContent";
}

ChildListDescriptor ParseExtendedTabsChildList(const JsonValue& descriptor)
{
    return ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

std::list<std::string> ResolveExtendedTabsChildIds(const JsonValue& descriptor,
    const std::function<std::list<std::string>(const std::string&, const std::string&)>& resolveTemplateChildIds)
{
    ChildListDescriptor childList = ParseExtendedTabsChildList(descriptor);
    if (childList.type == ChildListType::STATIC_IDS && !childList.staticChildIds.empty()) {
        return childList.staticChildIds;
    }
    if (childList.type == ChildListType::TEMPLATE_PATH && resolveTemplateChildIds) {
        return resolveTemplateChildIds(childList.templateComponentId, childList.templatePath);
    }
    return {};
}

void MergeExtendedTabsChildIds(const std::vector<std::string>& tabChildIds, JsonValue& childrenArray)
{
    if (!childrenArray.IsArray() || childrenArray.GetArraySize() > 0 || tabChildIds.empty()) {
        return;
    }

    for (const auto& childId : tabChildIds) {
        if (childId.empty()) {
            continue;
        }
        std::unique_ptr<JsonAdapter> childIdAdapter = JsonAdapter::CreateString(childId);
        if (childIdAdapter == nullptr) {
            continue;
        }
        childrenArray.Append(childIdAdapter->GetRoot());
    }
}

void PrebuildExtendedTabsChildren(SurfaceSlot& surfaceSlot, const JsonValue& nodeValue)
{
    if (!nodeValue.IsObject() || !IsExtendedTabsDescriptorType(surfaceSlot, nodeValue.GetString("component", ""))) {
        return;
    }

    ChildListDescriptor childList = ParseExtendedTabsChildList(nodeValue);
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();
    if (childList.type == ChildListType::STATIC_IDS) {
        PrebuildStaticChildren(surfaceSlot, childList, descriptorStore);
        return;
    }

    if (childList.type == ChildListType::TEMPLATE_PATH) {
        PrebuildTemplateChildren(surfaceSlot, nodeValue, childList, descriptorStore);
    }
}

} // namespace NativeModule
