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

#include "TemplateAdapterNode.h"

#include <hilog/log.h>

#include <memory>
#include <vector>

#include "adapter/ArkUINodeApiAdapter.h"
#include "adapter/ArkUIOHApiAdapter.h"
#include "components/NativeComponentFactory.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "../RenderManager.h"
#include "../SurfaceSlot.h"

namespace NativeModule {

namespace {

std::map<std::string, JsonValue> BuildTemplateLocalVariables(std::shared_ptr<DataModel> dataModel,
    const std::string& dataPath, int32_t itemIndex, const std::string& indexVarName, const std::string& itemVarName)
{
    std::map<std::string, JsonValue> localVariables;
    if (!indexVarName.empty()) {
        std::unique_ptr<JsonAdapter> indexAdapter = JsonAdapter::CreateNumber(static_cast<double>(itemIndex));
        if (indexAdapter != nullptr) {
            localVariables[indexVarName] = indexAdapter->GetRoot();
        }
    }

    if (dataModel == nullptr || dataPath.empty() || itemVarName.empty()) {
        return localVariables;
    }
    std::string itemPath = dataPath + "/" + std::to_string(itemIndex);
    auto itemOpt = dataModel->GetNode(itemPath);
    if (itemOpt.has_value()) {
        localVariables[itemVarName] = itemOpt.value();
    }
    return localVariables;
}

std::map<std::string, JsonValue> MergeLocalVariables(
    const std::map<std::string, JsonValue>& outerVariables, const std::map<std::string, JsonValue>& innerVariables)
{
    std::map<std::string, JsonValue> mergedVariables = outerVariables;
    for (const auto& [name, value] : innerVariables) {
        if (!name.empty() && value.IsValid()) {
            mergedVariables[name] = value;
        }
    }
    return mergedVariables;
}

} // namespace

void TemplateAdapterNode::RewriteDataPaths(JsonValue& json, const std::string& arrayPath, int32_t itemIndex)
{
    if (!json.IsValid()) {
        return;
    }

    if (json.IsObject()) {
        if (json.Has("path")) {
            JsonValue pathValue = json.GetItem("path");
            if (pathValue.IsString()) {
                std::string originalPath = pathValue.GetStringValue();
                if (!originalPath.empty()) {
                    std::string newPath;
                    if (originalPath[0] == '/') {
                        newPath = arrayPath + "/" + std::to_string(itemIndex) + originalPath;
                    } else {
                        newPath = arrayPath + "/" + std::to_string(itemIndex) + "/" + originalPath;
                    }
                    json.ReplaceString("path", newPath);
                }
            }
        }

        for (JsonValue child = json.GetChild(); child.IsValid();) {
            JsonValue next = child.GetNext();
            if (child.IsString()) {
                std::string str = child.GetStringValue();
                if (str.find("${") != std::string::npos) {
                    std::string key = child.GetKey();
                    std::string rewritten = RewriteTemplateStringPaths(str, arrayPath, itemIndex);
                    if (rewritten != str) {
                        json.ReplaceString(key.c_str(), rewritten);
                    }
                }
            } else {
                RewriteDataPaths(child, arrayPath, itemIndex);
            }
            child = next;
        }
    } else if (json.IsArray()) {
        int arraySize = json.GetArraySize();
        for (int i = 0; i < arraySize; ++i) {
            JsonValue item = json.GetArrayItem(i);
            TemplateAdapterNode::RewriteDataPaths(item, arrayPath, itemIndex);
        }
    }
}

std::string TemplateAdapterNode::RewriteTemplateStringPaths(
    const std::string& str, const std::string& arrayPath, int32_t itemIndex)
{
    std::string indexStr = std::to_string(itemIndex);
    std::string result;
    size_t i = 0;

    while (i < str.size()) {
        if (i + 2 < str.size() && str[i] == '\\' && str[i + 1] == '$' && str[i + 2] == '{') {
            result += "${";
            i += 3;
            continue;
        }

        if (i + 1 < str.size() && str[i] == '$' && str[i + 1] == '{') {
            int depth = 0;
            int closePos = -1;
            for (int j = static_cast<int>(i) + 1; j < static_cast<int>(str.size()); ++j) {
                if (str[j] == '{') {
                    ++depth;
                } else if (str[j] == '}') {
                    --depth;
                    if (depth == 0) {
                        closePos = j;
                        break;
                    }
                }
            }

            if (closePos < 0) {
                result += str[i];
                ++i;
                continue;
            }

            std::string expr = str.substr(i + 2, static_cast<size_t>(closePos) - i - 2);

            size_t parenPos = expr.find('(');
            if (parenPos != std::string::npos && !expr.empty() && expr.back() == ')') {
                result += "${" + RewriteTemplateStringPaths(expr, arrayPath, itemIndex) + "}";
            } else {
                std::string newPath;
                if (!expr.empty() && expr[0] == '/') {
                    newPath = arrayPath + "/" + indexStr + expr;
                } else {
                    newPath = arrayPath + "/" + indexStr + "/" + expr;
                }
                result += "${" + newPath + "}";
            }

            i = static_cast<size_t>(closePos) + 1;
        } else {
            result += str[i];
            ++i;
        }
    }

    return result;
}

void TemplateAdapterNode::CollectReferencedDescriptorIdsRecursive(const std::string& id,
    const std::map<std::string, JsonValue>& allDescriptors, std::set<std::string>& result,
    std::set<std::string>& visited)
{
    if (id.empty() || visited.count(id) > 0) {
        return;
    }
    visited.insert(id);
    result.insert(id);

    auto it = allDescriptors.find(id);
    if (it == allDescriptors.end()) {
        return;
    }

    JsonValue descriptor = it->second;
    if (!descriptor.IsObject()) {
        return;
    }

    if (descriptor.Has("children")) {
        JsonValue childrenValue = descriptor.GetItem("children");
        if (childrenValue.IsArray()) {
            int childCount = childrenValue.GetArraySize();
            for (int i = 0; i < childCount; ++i) {
                JsonValue childValue = childrenValue.GetArrayItem(i);
                if (childValue.IsString()) {
                    std::string childId = childValue.GetStringValue("");
                    if (!childId.empty()) {
                        CollectReferencedDescriptorIdsRecursive(childId, allDescriptors, result, visited);
                    }
                }
            }
        } else if (childrenValue.IsObject() && childrenValue.Has("componentId")) {
            std::string componentId = childrenValue.GetString("componentId", "");
            if (!componentId.empty()) {
                CollectReferencedDescriptorIdsRecursive(componentId, allDescriptors, result, visited);
            }
        }
    }

    if (descriptor.Has("child")) {
        JsonValue childValue = descriptor.GetItem("child");
        if (childValue.IsString()) {
            std::string childId = childValue.GetStringValue("");
            if (!childId.empty()) {
                CollectReferencedDescriptorIdsRecursive(childId, allDescriptors, result, visited);
            }
        }
    }
}

std::set<std::string> TemplateAdapterNode::CollectReferencedDescriptorIds(
    const std::string& rootId, const std::map<std::string, JsonValue>& allDescriptors)
{
    std::set<std::string> result;
    std::set<std::string> visited;
    CollectReferencedDescriptorIdsRecursive(rootId, allDescriptors, result, visited);
    return result;
}

std::unique_ptr<JsonAdapter> TemplateAdapterNode::BuildTemplateInstanceDescriptorById(std::string& id,
    const std::string& templateComponentId, const std::string& arrayPath, int32_t itemIndex,
    const std::map<std::string, JsonValue>* allDescriptors, std::map<std::string, JsonValue>* generatedDescriptors)
{
    if (allDescriptors == nullptr || generatedDescriptors == nullptr) {
        return nullptr;
    }
    auto childIt = allDescriptors->find(id);
    if (childIt != allDescriptors->end()) {
        std::string generatedInstanceId = arrayPath + templateComponentId + ":" + std::to_string(itemIndex) + ":" + id;
        JsonValue templateDescriptor = childIt->second;
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(templateDescriptor);
        if (adapter != nullptr) {
            JsonValue root = adapter->GetRoot();
            if (root.Has("id")) {
                root.ReplaceString("id", generatedInstanceId);
            } else {
                root.PutString("id", generatedInstanceId);
            }
            TemplateAdapterNode::RewriteDataPaths(root, arrayPath, itemIndex);
            (*generatedDescriptors)[generatedInstanceId] = root;
        }
        return adapter;
    }
    LOG_A2UI(LOG_WARN, "BuildTemplateInstanceDescriptor: child descriptor not found for '%{public}s'", id.c_str());
    return nullptr;
}

std::string TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(std::string& id,
    const std::string& templateComponentId, const std::string& arrayPath, int32_t itemIndex,
    const std::map<std::string, JsonValue>* allDescriptors, std::map<std::string, JsonValue>* generatedDescriptors)
{
    if (allDescriptors == nullptr || generatedDescriptors == nullptr) {
        return "";
    }

    std::unique_ptr<JsonAdapter> genDescriptor = BuildTemplateInstanceDescriptorById(
        id, templateComponentId, arrayPath, itemIndex, allDescriptors, generatedDescriptors);
    if (genDescriptor == nullptr) {
        return "";
    }

    JsonValue itemValue = genDescriptor->GetRoot();
    std::string generatedInstanceId = itemValue.GetString("id", "");
    if (generatedInstanceId.empty()) {
        return "";
    }
    (*generatedDescriptors)[generatedInstanceId] = itemValue;

    if (itemValue.Has("children")) {
        std::unique_ptr<JsonAdapter> childrenAdapter = JsonAdapter::Clone(itemValue.GetItem("children"));
        JsonValue childrenValue = childrenAdapter->GetRoot();
        if (!childrenValue.IsArray()) {
            return generatedInstanceId;
        }
        int childCount = childrenValue.GetArraySize();
        JsonValue newChildren = itemValue.ReplaceArray("children");
        for (int i = 0; i < childCount; ++i) {
            JsonValue childValue = childrenValue.GetArrayItem(i);
            if (!childValue.IsString()) {
                continue;
            }
            std::string childId = childValue.GetStringValue("");
            if (childId.empty()) {
                continue;
            }
            std::string genChildId = BuildTemplateInstanceTreeDescriptors(
                childId, templateComponentId, arrayPath, itemIndex, allDescriptors, generatedDescriptors);
            if (!genChildId.empty()) {
                newChildren.Append(JsonAdapter::CreateString(genChildId)->GetRoot());
            }
        }
    }

    if (itemValue.Has("child")) {
        JsonValue childValue = itemValue.GetItem("child");
        if (!childValue.IsString()) {
            return generatedInstanceId;
        }
        std::string childId = childValue.GetStringValue("");
        if (childId.empty()) {
            return generatedInstanceId;
        }
        std::string genChildId = BuildTemplateInstanceTreeDescriptors(
            childId, templateComponentId, arrayPath, itemIndex, allDescriptors, generatedDescriptors);
        if (!genChildId.empty()) {
            itemValue.ReplaceString("child", genChildId);
        }
    }
    return generatedInstanceId;
}

TemplateAdapterNode::TemplateAdapterNode() : handle_(ArkUIOHApiAdapter::NodeAdapterCreate()) {}

TemplateAdapterNode::~TemplateAdapterNode()
{
    LOG_A2UI(LOG_INFO, "TemplateAdapterNode::Destructor: Cleaning up");

    items_.clear();
    itemContentParents_.clear();

    if (handle_) {
        ArkUIOHApiAdapter::NodeAdapterUnregisterEventReceiver(handle_);
        ArkUIOHApiAdapter::NodeAdapterDispose(handle_);
        handle_ = nullptr;
    }
}

void TemplateAdapterNode::Initialize(const std::string& templateId, const std::string& dataPath, int itemCount,
    const std::string& indexVarName, const std::string& itemVarName)
{
    templateId_ = templateId;
    dataPath_ = dataPath;
    indexVarName_ = indexVarName.empty() ? "index" : indexVarName;
    itemVarName_ = itemVarName.empty() ? "item" : itemVarName;
    itemCount_ = itemCount;

    LOG_A2UI(LOG_DEBUG, "TemplateAdapterNode::Initialize: templateId=%{public}s, dataPath=%{public}s, count=%{public}d",
        templateId_.c_str(), dataPath_.c_str(), itemCount_);

    ArkUIOHApiAdapter::NodeAdapterSetTotalNodeCount(handle_, itemCount_);
    ArkUIOHApiAdapter::NodeAdapterRegisterEventReceiver(handle_, this, OnStaticAdapterEvent);
}

void TemplateAdapterNode::SetDataModel(std::shared_ptr<DataModel> dataModel)
{
    dataModel_ = dataModel;
}

void TemplateAdapterNode::SetTemplateDescriptor(const JsonValue& templateDescriptor)
{
    templateDescriptor_ = templateDescriptor;
}

void TemplateAdapterNode::SetAllDescriptors(const std::map<std::string, JsonValue>& descriptors)
{
    allDescriptors_ = descriptors;
    LOG_A2UI(
        LOG_DEBUG, "TemplateAdapterNode::SetAllDescriptors: set %{public}zu component descriptors", descriptors.size());
}

void TemplateAdapterNode::SetInheritedLocalVariables(const std::map<std::string, JsonValue>& localVariables)
{
    inheritedLocalVariables_ = localVariables;
}

void TemplateAdapterNode::SetSurfaceInfo(const std::string& surfaceId, int32_t renderId)
{
    surfaceId_ = surfaceId;
    renderId_ = renderId;
}

void TemplateAdapterNode::SetSurfaceContext(const SurfaceContext& surfaceContext)
{
    surfaceContext_ = surfaceContext;
}

void TemplateAdapterNode::UpdateItemCount(int itemCount)
{
    itemCount_ = itemCount;
    ArkUIOHApiAdapter::NodeAdapterSetTotalNodeCount(handle_, itemCount_);
    LOG_A2UI(LOG_DEBUG, "TemplateAdapterNode::UpdateItemCount: count=%{public}d", itemCount_);
}

TemplateAdapterNode::ItemWrapperInfo TemplateAdapterNode::BuildItemWrapper(
    const std::shared_ptr<Component>& component) const
{
    ItemWrapperInfo wrapperInfo;
    if (component == nullptr || component->GetNativeView() == nullptr) {
        return wrapperInfo;
    }

    ArkUI_NodeHandle wrapperNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::STACK);
    if (wrapperNode == nullptr) {
        return wrapperInfo;
    }

    ArkUINodeApiAdapter::AddChild(wrapperNode, component->GetNativeView());
    wrapperInfo.rootNode = wrapperNode;
    wrapperInfo.contentParentNode = wrapperNode;
    return wrapperInfo;
}

void TemplateAdapterNode::OnStaticAdapterEvent(A2UINodeAdapterEvent* event)
{
    auto* adapter = reinterpret_cast<TemplateAdapterNode*>(ArkUIOHApiAdapter::NodeAdapterEventGetUserData(event));
    if (adapter) {
        adapter->OnAdapterEvent(event);
    }
}

void TemplateAdapterNode::OnAdapterEvent(A2UINodeAdapterEvent* event)
{
    auto type = ArkUIOHApiAdapter::NodeAdapterEventGetType(event);

    switch (type) {
        case A2UINodeAdapterEventType::WILL_ATTACH_TO_NODE:
        case A2UINodeAdapterEventType::WILL_DETACH_FROM_NODE:
            break;
        case A2UINodeAdapterEventType::ON_GET_NODE_ID:
            OnNewItemIdCreated(event);
            break;
        case A2UINodeAdapterEventType::ON_ADD_NODE_TO_ADAPTER:
            OnNewItemAttached(event);
            break;
        case A2UINodeAdapterEventType::ON_REMOVE_NODE_FROM_ADAPTER:
            OnItemDetached(event);
            break;
        default:
            break;
    }
}

void TemplateAdapterNode::OnNewItemIdCreated(A2UINodeAdapterEvent* event)
{
    auto index = ArkUIOHApiAdapter::NodeAdapterEventGetItemIndex(event);

    std::hash<std::string> hasher;
    std::string itemId = templateId_ + ":" + std::to_string(index) + ":v" + std::to_string(templateVersion_);
    // ArkUI adapter node ids are int32-based. Keep the hashed id stable, non-negative, and away from 0.
    int32_t id = static_cast<int32_t>(hasher(itemId) & 0x7fffffffU);
    if (id == 0) {
        id = static_cast<int32_t>(index + 1);
    }

    int32_t result = ArkUIOHApiAdapter::NodeAdapterEventSetNodeId(event, id);

    LOG_A2UI(LOG_DEBUG, "TemplateAdapterNode::OnNewItemIdCreated: index=%{public}d, id=%{public}d, result=%{public}d",
        index, id, result);
}

void TemplateAdapterNode::OnNewItemAttached(A2UINodeAdapterEvent* event)
{
    auto index = ArkUIOHApiAdapter::NodeAdapterEventGetItemIndex(event);

    std::map<std::string, JsonValue> generatedDescriptors;
    std::string generatedInstanceId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(
        templateId_, templateId_, dataPath_, index, &allDescriptors_, &generatedDescriptors);
    if (generatedInstanceId.empty()) {
        LOG_A2UI(LOG_WARN,
            "OnNewItemAttached: failed to build template instance descriptor, templateId=%{public}s, "
            "itemIndex=%{public}d",
            templateId_.c_str(), index);
        return;
    }

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(renderId_, surfaceId_);
    if (surfaceSlot == nullptr) {
        return;
    }

    std::map<std::string, JsonValue> currentLocalVariables =
        BuildTemplateLocalVariables(dataModel_, dataPath_, index, indexVarName_, itemVarName_);
    std::map<std::string, JsonValue> localVariables =
        MergeLocalVariables(inheritedLocalVariables_, currentLocalVariables);
    Component::RegisterPendingLocalVariablesForComponents(generatedDescriptors, localVariables);
    auto component = surfaceSlot->BuildRootFromComponents(
        generatedInstanceId, generatedDescriptors, hasProcessedNode, sawRootDescriptor, false);
    Component::ClearPendingLocalVariablesForComponents(generatedDescriptors);
    if (component == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "TemplateAdapterNode::OnNewItemAttached: Failed to build component tree for index=%{public}d", index);
        return;
    }

    std::string parentPath = dataPath_ + "/" + std::to_string(index);
    OnNestedAdapterUpdate(component, parentPath);
    surfaceSlot->RestoreRuntimeStateTree(component);

    auto nativeView = component->GetNativeView();
    if (nativeView != nullptr) {
        for (auto prevIt = items_.begin(); prevIt != items_.end(); ++prevIt) {
            if (prevIt->second == component) {
                ArkUI_NodeHandle contentParent = prevIt->first;
                auto contentParentIt = itemContentParents_.find(prevIt->first);
                if (contentParentIt != itemContentParents_.end() && contentParentIt->second != nullptr) {
                    contentParent = contentParentIt->second;
                }
                ArkUINodeApiAdapter::RemoveChild(contentParent, nativeView);
                movedWrappers_.insert(prevIt->first);
                LOG_A2UI(LOG_DEBUG,
                    "OnNewItemAttached: pre-emptive detach nativeView from old wrapper=%{public}p for index=%{public}d",
                    prevIt->first, index);
                break;
            }
        }
    }

    ItemWrapperInfo wrapperInfo = BuildItemWrapper(component);
    if (wrapperInfo.rootNode == nullptr || wrapperInfo.contentParentNode == nullptr) {
        LOG_A2UI(LOG_ERROR, "TemplateAdapterNode::OnNewItemAttached: Failed to build wrapper node for index=%{public}d",
            index);
        return;
    }
    items_.emplace(wrapperInfo.rootNode, component);
    itemContentParents_[wrapperInfo.rootNode] = wrapperInfo.contentParentNode;

    int32_t result = ArkUIOHApiAdapter::NodeAdapterEventSetItem(event, wrapperInfo.rootNode);
    LOG_A2UI(LOG_DEBUG,
        "TemplateAdapterNode::OnNewItemAttached: Built component tree for index=%{public}d, wrapperNode=%{public}p, "
        "setItemResult=%{public}d",
        index, wrapperInfo.rootNode, result);
}

void TemplateAdapterNode::OnItemDetached(A2UINodeAdapterEvent* event)
{
    auto wrapperHandle = ArkUIOHApiAdapter::NodeAdapterEventGetRemovedNode(event);

    auto it = items_.find(wrapperHandle);
    if (it != items_.end()) {
        auto& component = it->second;
        SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(renderId_, surfaceId_);
        if (surfaceSlot != nullptr) {
            surfaceSlot->CaptureRuntimeStateTree(component);
        }
        bool alreadyMoved = (movedWrappers_.erase(wrapperHandle) > 0);
        ArkUI_NodeHandle contentParent = wrapperHandle;
        auto contentParentIt = itemContentParents_.find(wrapperHandle);
        if (contentParentIt != itemContentParents_.end() && contentParentIt->second != nullptr) {
            contentParent = contentParentIt->second;
        }
        if (!alreadyMoved && component != nullptr && component->GetNativeView() != nullptr) {
            ArkUINodeApiAdapter::RemoveChild(contentParent, component->GetNativeView());
        }
        ArkUINodeApiAdapter::DisposeNode(wrapperHandle);
        items_.erase(it);
        itemContentParents_.erase(wrapperHandle);
        LOG_A2UI(LOG_DEBUG,
            "TemplateAdapterNode::OnItemDetached: Released wrapperNode=%{public}p, alreadyMoved=%{public}d",
            wrapperHandle, alreadyMoved ? 1 : 0);
    }
}

void TemplateAdapterNode::ReloadAllItems()
{
    ArkUIOHApiAdapter::NodeAdapterReloadAllItems(handle_);
}

void TemplateAdapterNode::IncrementTemplateVersion()
{
    ++templateVersion_;
    LOG_A2UI(LOG_DEBUG, "TemplateAdapterNode::IncrementTemplateVersion: version=%{public}u", templateVersion_);
}
} // namespace NativeModule
