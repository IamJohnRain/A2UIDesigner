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

#include "SurfaceSlot.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <vector>

#include "catalog/CatalogConstants.h"
#include "components/A2UI/A2UIComponent.h"
#include "components/CustomComponentFactory.h"
#include "components/NativeComponentFactory.h"
#include "components/custom/ExtendedTabsPrebuildHelper.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "components/extended/ExtendedCheckboxGroupComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/RenderContext.h"
#include "components/extended/if/IfComponent.h"
#include "composition/ChildListParser.h"
#include "composition/TemplateAdapterNode.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "ArkUIOHApiAdapter.h"
#include "NativeEntry.h"
#include "SurfaceSlotSchemaValidation.h"

namespace NativeModule {

namespace {

constexpr int32_t MIN_API_VERSION_EXTENDED_ROW_NATIVE = 23;

std::string ToLowerCopy(const std::string& value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return normalized;
}

bool IsBasicCatalogId(const std::string& catalogId)
{
    return catalogId == ToLowerCopy(A2UI_BASIC_CATALOG_ID);
}

bool IsExtendedCatalogId(const std::string& catalogId)
{
    return catalogId == ToLowerCopy(A2UI_EXTENDED_CATALOG_ID);
}

bool ShouldRefreshLazyAdapter(const std::string& changedPath, const std::string& adapterPath)
{
    if (changedPath.empty() || adapterPath.empty() || adapterPath[0] != '/') {
        return false;
    }
    if (changedPath == adapterPath) {
        return true;
    }
    if (changedPath == "/") {
        return true;
    }
    if (adapterPath.size() > changedPath.size() && adapterPath.compare(0, changedPath.size(), changedPath) == 0 &&
        adapterPath[changedPath.size()] == '/') {
        return true;
    }
    return false;
}

const char* SurfaceProtocolModeToString(SurfaceProtocolMode mode)
{
    switch (mode) {
        case SurfaceProtocolMode::A2UI_STANDARD:
            return "basic";
        case SurfaceProtocolMode::EXTENDED_PROTOCOL:
            return "extended";
        default:
            return "unknown";
    }
}

std::string GetShortComponentType(const std::string& componentType)
{
    size_t separatorIndex = componentType.find_last_of('.');
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= componentType.size()) {
        return componentType;
    }
    return componentType.substr(separatorIndex + 1);
}

bool IsExtendedRowType(const std::string& componentType)
{
    return GetShortComponentType(componentType) == "Row";
}

bool IsCheckboxType(const std::string& componentType)
{
    return GetShortComponentType(componentType) == "Checkbox";
}

bool IsCheckboxGroupType(const std::string& componentType)
{
    return GetShortComponentType(componentType) == "CheckboxGroup";
}

bool ShouldUseCustomExtendedRow(int32_t apiVersion, const std::string& componentType)
{
    return apiVersion > 0 && apiVersion < MIN_API_VERSION_EXTENDED_ROW_NATIVE && IsExtendedRowType(componentType);
}

std::string NormalizeExtendedProtocolComponentType(
    const std::string& componentType, const std::shared_ptr<Catalog>& catalog)
{
    if (componentType.empty()) {
        return componentType;
    }
    static constexpr const char* EXTENDED_PREFIX = "Extended.";

    if (catalog == nullptr) {
        return componentType;
    }
    if (catalog->GetCatalogItemByName(componentType) != nullptr) {
        return componentType;
    }
    if (componentType.rfind(EXTENDED_PREFIX, 0) == 0) {
        std::string shortType = GetShortComponentType(componentType);
        if (catalog->GetCatalogItemByName(shortType) != nullptr) {
            return shortType;
        }
        return componentType;
    }
    if (componentType.find('.') != std::string::npos) {
        return componentType;
    }
    std::string extendedType = std::string(EXTENDED_PREFIX) + componentType;
    if (catalog->GetCatalogItemByName(extendedType) != nullptr) {
        return extendedType;
    }
    return componentType;
}

bool HasUsableNativeRootView(const std::shared_ptr<Component>& node)
{
    return node != nullptr && ArkUINodeApiAdapter::IsAvailable() && node->GetNativeView() != nullptr;
}

void AttachRootComponentToContent(A2UINodeContentHandle contentHandle, const std::shared_ptr<Component>& rootComponent)
{
    if (contentHandle == nullptr || !HasUsableNativeRootView(rootComponent)) {
        return;
    }
    ArkUIOHApiAdapter::NodeContentAddNode(contentHandle, rootComponent->GetNativeView());
}

void DetachRootComponentFromContent(
    A2UINodeContentHandle contentHandle, const std::shared_ptr<Component>& rootComponent)
{
    if (contentHandle == nullptr || !HasUsableNativeRootView(rootComponent)) {
        return;
    }
    ArkUIOHApiAdapter::NodeContentRemoveNode(contentHandle, rootComponent->GetNativeView());
}

void CollectTemplateRootIdsFromDescriptor(const JsonValue& nodeValue, std::set<std::string>& templateRootIds)
{
    if (!nodeValue.IsObject() || !nodeValue.Has("children")) {
        return;
    }
    JsonValue childrenValue = nodeValue.GetItem("children");
    if (!childrenValue.IsObject()) {
        return;
    }
    std::string templateComponentId = childrenValue.GetString("componentId", "");
    if (!templateComponentId.empty()) {
        templateRootIds.insert(templateComponentId);
    }
}

void CollectStaticReachableDescriptorIds(const std::string& nodeId,
    const std::map<std::string, JsonValue>& descriptorsById, std::set<std::string>& reachableIds);

void CollectReachableIdsFromStringArray(const JsonValue& arrayValue,
    const std::map<std::string, JsonValue>& descriptorsById, std::set<std::string>& reachableIds)
{
    if (!arrayValue.IsArray()) {
        return;
    }
    int childCount = arrayValue.GetArraySize();
    for (int index = 0; index < childCount; ++index) {
        JsonValue childValue = arrayValue.GetArrayItem(index);
        if (childValue.IsString()) {
            CollectStaticReachableDescriptorIds(childValue.GetStringValue(""), descriptorsById, reachableIds);
        }
    }
}

void CollectReachableIdsFromTabsArray(const JsonValue& tabsValue,
    const std::map<std::string, JsonValue>& descriptorsById, std::set<std::string>& reachableIds)
{
    if (!tabsValue.IsArray()) {
        return;
    }
    int tabCount = tabsValue.GetArraySize();
    for (int index = 0; index < tabCount; ++index) {
        JsonValue tabValue = tabsValue.GetArrayItem(index);
        if (!tabValue.IsObject()) {
            continue;
        }
        std::string childId = tabValue.GetString("child", "");
        if (!childId.empty()) {
            CollectStaticReachableDescriptorIds(childId, descriptorsById, reachableIds);
        }
    }
}

void CollectStaticReachableDescriptorIds(const std::string& nodeId,
    const std::map<std::string, JsonValue>& descriptorsById, std::set<std::string>& reachableIds)
{
    if (nodeId.empty() || reachableIds.count(nodeId) > 0) {
        return;
    }
    reachableIds.insert(nodeId);
    auto descriptorIt = descriptorsById.find(nodeId);
    if (descriptorIt == descriptorsById.end() || !descriptorIt->second.IsObject()) {
        return;
    }

    JsonValue descriptor = descriptorIt->second;
    if (descriptor.Has("children")) {
        CollectReachableIdsFromStringArray(descriptor.GetItem("children"), descriptorsById, reachableIds);
    }

    if (descriptor.Has("child")) {
        JsonValue childValue = descriptor.GetItem("child");
        if (childValue.IsString()) {
            CollectStaticReachableDescriptorIds(childValue.GetStringValue(""), descriptorsById, reachableIds);
        }
    }

    if (descriptor.Has("childrenIf")) {
        CollectReachableIdsFromStringArray(descriptor.GetItem("childrenIf"), descriptorsById, reachableIds);
    }

    if (descriptor.Has("childrenElse")) {
        CollectReachableIdsFromStringArray(descriptor.GetItem("childrenElse"), descriptorsById, reachableIds);
    }

    if (descriptor.Has("tabs")) {
        CollectReachableIdsFromTabsArray(descriptor.GetItem("tabs"), descriptorsById, reachableIds);
    }
}

bool ShouldBuildDetachedCheckboxForReachableGroup(const std::string& nodeId, const JsonValue& nodeValue,
    const std::map<std::string, JsonValue>& descriptorsById, const std::set<std::string>& reachableIds)
{
    if (nodeId.empty() || !nodeValue.IsObject() || !IsCheckboxType(nodeValue.GetString("component", ""))) {
        return false;
    }

    std::string checkboxGroup = nodeValue.GetString("group", "");
    if (checkboxGroup.empty()) {
        return false;
    }

    for (const auto& reachableId : reachableIds) {
        auto descriptorIt = descriptorsById.find(reachableId);
        if (descriptorIt == descriptorsById.end() || !descriptorIt->second.IsObject()) {
            continue;
        }
        const JsonValue& descriptor = descriptorIt->second;
        if (!IsCheckboxGroupType(descriptor.GetString("component", ""))) {
            continue;
        }
        if (descriptor.GetString("group", "") == checkboxGroup) {
            return true;
        }
    }
    return false;
}

void ApplyRootDefaultSizeToContentArea(const std::shared_ptr<Component>& node, bool forceRootFill)
{
    if (!HasUsableNativeRootView(node)) {
        return;
    }

    if ((node->GetType() != "Column" && node->GetType() != "Row")) {
        return;
    }
    if (forceRootFill) {
        constexpr float FULL_PERCENT = 1.0f;
        ArkUINodeApiAdapter::SetNodeWidthPercent(node->GetNativeView(), FULL_PERCENT);
        ArkUINodeApiAdapter::SetNodeHeightPercent(node->GetNativeView(), FULL_PERCENT);
        return;
    }
}

int32_t MeasureComponentTreeDepth(const std::shared_ptr<Component>& root)
{
    if (root == nullptr) {
        return 0;
    }
    int32_t maxDepth = 0;
    std::function<void(const std::shared_ptr<Component>&, int32_t)> traverse;
    traverse = [&](const std::shared_ptr<Component>& node, int32_t depth) {
        if (depth > maxDepth) {
            maxDepth = depth;
        }
        for (const auto& child : node->GetChildren()) {
            traverse(child, depth + 1);
        }
    };
    traverse(root, 1);
    return maxDepth;
}

int32_t ResolveBuildDepth(const std::string& nodeId, const std::map<std::string, std::string>& parentsRelations)
{
    if (nodeId.empty()) {
        return 0;
    }
    std::string currentId = nodeId;
    int32_t depth = 0;
    size_t guard = 0;
    const size_t maxStep = parentsRelations.size();
    while (guard <= maxStep) {
        auto relationIt = parentsRelations.find(currentId);
        if (relationIt == parentsRelations.end()) {
            break;
        }
        const std::string& parentId = relationIt->second;
        if (parentId.empty()) {
            break;
        }
        ++depth;
        currentId = parentId;
        ++guard;
    }
    if (guard > maxStep) {
        LOG_A2UI(LOG_WARN, "SurfaceSlot::ResolveBuildDepth - parent relation cycle detected, nodeId=%{public}s",
            nodeId.c_str());
    }
    return depth;
}

} // namespace

bool SurfaceSlot::BuildNodeDepthComparator::operator()(
    const std::shared_ptr<Component>& lhs, const std::shared_ptr<Component>& rhs) const
{
    if (lhs == rhs) {
        return false;
    }
    if (lhs == nullptr) {
        return rhs != nullptr;
    }
    if (rhs == nullptr) {
        return false;
    }
    if (lhs->GetBuildDepth() != rhs->GetBuildDepth()) {
        return lhs->GetBuildDepth() < rhs->GetBuildDepth();
    }
    const std::string& lhsId = lhs->GetComponentId();
    const std::string& rhsId = rhs->GetComponentId();
    if (lhsId != rhsId) {
        return lhsId < rhsId;
    }
    return lhs.get() < rhs.get();
}

SurfaceSlot::SurfaceSlot()
{
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot: Constructor - Creating BindingEngine");
    ArkUINodeApiAdapter::GetNativeDialogAPI();
    bindingEngine_ = BindingEngine::Create();
    modalCoordinator_ = std::make_unique<ModalCoordinator>();
}

SurfaceSlot::~SurfaceSlot()
{
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot: Destructor");
}

void SurfaceSlot::SetContentHandle(A2UINodeContentHandle handle)
{
    if (contentHandle_ == handle) {
        LOG_A2UI(LOG_DEBUG,
            "SurfaceSlot::SetContentHandle - renderId=%{public}d, surfaceId=%{public}s ignored, handle "
            "unchanged=%{public}p",
            renderId_, surfaceId_.c_str(), handle);
        return;
    }

    LOG_A2UI(LOG_DEBUG,
        "SurfaceSlot::SetContentHandle - renderId=%{public}d, surfaceId=%{public}s, oldHandle=%{public}p, "
        "newHandle=%{public}p, hasRootComponent=%{public}s",
        renderId_, surfaceId_.c_str(), contentHandle_, handle, rootComponent_ != nullptr ? "true" : "false");

    DetachRootComponentFromContent(contentHandle_, rootComponent_);

    contentHandle_ = handle;

    AttachRootComponentToContent(contentHandle_, rootComponent_);
}

A2UINodeContentHandle SurfaceSlot::GetContentHandle() const
{
    return contentHandle_;
}

void SurfaceSlot::SetRootComponent(const std::shared_ptr<Component>& rootComponent)
{
    if (rootComponent_ == rootComponent) {
        return;
    }
    LOG_A2UI(LOG_DEBUG,
        "SurfaceSlot::SetRootComponent - renderId=%{public}d, surfaceId=%{public}s, oldRoot=%{public}p, "
        "newRoot=%{public}p, contentHandle=%{public}p",
        renderId_, surfaceId_.c_str(), rootComponent_.get(), rootComponent.get(), contentHandle_);

    DetachRootComponentFromContent(contentHandle_, rootComponent_);

    rootComponent_ = rootComponent;

    AttachRootComponentToContent(contentHandle_, rootComponent_);
}

std::shared_ptr<Component> SurfaceSlot::GetRootComponent() const
{
    return rootComponent_;
}

void SurfaceSlot::DismissActiveModal()
{
    if (modalCoordinator_ == nullptr) {
        return;
    }
    modalCoordinator_->DismissActiveModal();
}

bool SurfaceSlot::UpdateComponents(const JsonValue& messageRoot)
{
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot::UpdateComponents(json) - renderId=%{public}d, surfaceId=%{public}s", renderId_,
        surfaceId_.c_str());
    if (!messageRoot.IsValid() || !messageRoot.IsObject() || !messageRoot.Has("components")) {
        LOG_A2UI(LOG_ERROR, "UpdateComponents: root is invalid or missing components");
        return false;
    }

    return UpdateComponentsArray(messageRoot.GetItem("components"));
}

bool SurfaceSlot::UpdateComponentsArray(const JsonValue& componentsValue)
{
    if (!componentsValue.IsArray()) {
        LOG_A2UI(LOG_ERROR, "UpdateComponents: components is not an array");
        return false;
    }

    int32_t componentCount = componentsValue.GetArraySize();
    LOG_A2UI(LOG_DEBUG,
        "SurfaceSlot::UpdateComponents(array) - renderId=%{public}d, surfaceId=%{public}s, componentCount=%{public}d, "
        "existingComponentCount=%{public}zu, surfaceProtocolMode=%{public}s",
        renderId_, surfaceId_.c_str(), componentCount, allComponents_.size(),
        SurfaceProtocolModeToString(surfaceProtocolMode_));

    if (componentCount > 1000) {
        LOG_A2UI(
            LOG_ERROR, "UpdateComponents: component count %{public}d exceeds maximum allowed 1000", componentCount);
    }

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    auto rootComponent = BuildRootFromComponents(componentsValue, hasProcessedNode, sawRootDescriptor);
    ProcessPendingTemplateContainers();
    if (rootComponent == nullptr) {
        if (hasProcessedNode && !sawRootDescriptor) {
            LOG_A2UI(LOG_WARN, "UpdateComponents: cached components without root, waiting for later root update");
            return true;
        }
        LOG_A2UI(LOG_ERROR, "UpdateComponents: build root failed");
        return false;
    }

    static constexpr int32_t MAX_COMPONENT_NESTING_DEPTH = 50;
    int32_t maxDepth = MeasureComponentTreeDepth(rootComponent);
    if (maxDepth > MAX_COMPONENT_NESTING_DEPTH) {
        LOG_A2UI(LOG_ERROR, "UpdateComponents: component nesting depth %{public}d exceeds maximum allowed %{public}d",
            maxDepth, MAX_COMPONENT_NESTING_DEPTH);
    }

    LOG_A2UI(LOG_DEBUG,
        "SurfaceSlot::UpdateComponents(array) - renderId=%{public}d, surfaceId=%{public}s success, "
        "rootComponent=%{public}p, "
        "trackedComponentCount=%{public}zu",
        renderId_, surfaceId_.c_str(), rootComponent.get(), allComponents_.size());
    return true;
}

bool SurfaceSlot::UpdateDataModel(const JsonValue& messageRoot)
{
    LOG_A2UI(LOG_DEBUG, "UpdateDataModel: process json message body");

    if (!messageRoot.IsValid()) {
        LOG_A2UI(LOG_ERROR, "UpdateDataModel: messageBody is invalid");
        return false;
    }

    if (!messageRoot.IsObject()) {
        LOG_A2UI(LOG_ERROR, "UpdateDataModel: root is not an object");
        return false;
    }

    // Get path (optional - if empty, will replace entire data model)
    std::string path = messageRoot.GetString("path", "");

    // Get value (optional - if missing, will delete the path)
    JsonValue value;
    bool hasValue = messageRoot.Has("value");

    if (hasValue) {
        JsonValue valueJson = messageRoot.GetItem("value");
        if (!valueJson.IsValid()) {
            LOG_A2UI(LOG_ERROR, "UpdateDataModel: value is invalid");
            return false;
        }

        int32_t depth = DataModel::MeasureJsonDepth(valueJson);
        if (depth > DataModel::MAX_DATA_MODEL_DEPTH) {
            LOG_A2UI(LOG_ERROR,
                "UpdateDataModel: data model nesting depth %{public}d exceeds maximum allowed %{public}d", depth,
                DataModel::MAX_DATA_MODEL_DEPTH);
        }

        value = valueJson;
    }

    // Determine the operation type and delegate to appropriate BindingEngine method
    if (bindingEngine_ == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateDataModel: BindingEngine is nullptr");
        return false;
    }

    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default"; // Use default surfaceId if not set
        LOG_A2UI(LOG_INFO, "UpdateDataModel: surfaceId not set, using 'default'");
    }

    if (!path.empty() && hasValue) {
        // Case 1: Update specific field - both path and value are present
        LOG_A2UI(LOG_DEBUG, "UpdateDataModel: UPDATE field - path=%{public}s", path.c_str());
        bindingEngine_->UpdateDataModelByPath(surfaceId, path, value);
        RefreshLazyAdapters(path);
        if (modalCoordinator_ != nullptr) {
            modalCoordinator_->RefreshModalBindings();
        }
        return true;
    } else if (!path.empty() && !hasValue) {
        // Case 2: Delete field - only path is present, no value
        LOG_A2UI(LOG_DEBUG, "UpdateDataModel: DELETE field - path=%{public}s", path.c_str());
        bindingEngine_->DeleteDataModelByPath(surfaceId, path);
        RefreshLazyAdapters(path);
        if (modalCoordinator_ != nullptr) {
            modalCoordinator_->RefreshModalBindings();
        }
        return true;
    } else if (path.empty() && hasValue) {
        // Case 3: Replace entire data model - only value is present, no path
        LOG_A2UI(LOG_DEBUG, "UpdateDataModel: REPLACE entire model");
        ClearRuntimeStateStore();
        bindingEngine_->ReplaceDataModel(surfaceId, value);
        RefreshLazyAdapters("", true);
        if (modalCoordinator_ != nullptr) {
            modalCoordinator_->RefreshModalBindings();
        }
        return true;
    } else {
        // Both path and value are missing - invalid request
        LOG_A2UI(LOG_ERROR, "UpdateDataModel: Invalid request - both path and value are missing");
        return false;
    }
}

std::shared_ptr<BindingEngine> SurfaceSlot::GetBindingEngine() const
{
    return bindingEngine_;
}

void SurfaceSlot::OnTemplateExpansionDeferred(const std::string& containerId)
{
    pendingTemplateContainers_.insert(containerId);
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot::OnTemplateExpansionDeferred: containerId=%{public}s, pendingCount=%{public}zu",
        containerId.c_str(), pendingTemplateContainers_.size());
}

void SurfaceSlot::OnTemplateExpansionResolved(const std::string& containerId)
{
    pendingTemplateContainers_.erase(containerId);
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot::OnTemplateExpansionResolved: containerId=%{public}s, pendingCount=%{public}zu",
        containerId.c_str(), pendingTemplateContainers_.size());
}

void SurfaceSlot::ProcessPendingTemplateContainers()
{
    if (pendingTemplateContainers_.empty()) {
        return;
    }

    auto pending = pendingTemplateContainers_;
    for (const auto& containerId : pending) {
        auto it = allComponents_.find(containerId);
        if (it == allComponents_.end() || it->second == nullptr) {
            LOG_A2UI(
                LOG_INFO, "ProcessPending: SKIP not in allComponents, containerId=%{public}s", containerId.c_str());
            pendingTemplateContainers_.erase(containerId);
            continue;
        }
        auto& container = it->second;
        const auto& cld = container->GetChildListDescriptor();
        if (cld.type != ChildListType::TEMPLATE_PATH) {
            LOG_A2UI(LOG_INFO, "ProcessPending: SKIP not TEMPLATE_PATH, containerId=%{public}s", containerId.c_str());
            pendingTemplateContainers_.erase(containerId);
            continue;
        }

        auto itDesc = allComponentDescriptorStore_.find(cld.templateComponentId);
        if (itDesc == allComponentDescriptorStore_.end()) {
            LOG_A2UI(LOG_INFO,
                "ProcessPending: WAIT root template not in store, containerId=%{public}s, templateId=%{public}s",
                containerId.c_str(), cld.templateComponentId.c_str());
            continue;
        }

        container->BuildChildren(*this);

        auto referencedIds =
            TemplateAdapterNode::CollectReferencedDescriptorIds(cld.templateComponentId, allComponentDescriptorStore_);
        bool subtreeComplete = true;
        std::string missingId;
        for (const auto& refId : referencedIds) {
            if (allComponentDescriptorStore_.find(refId) == allComponentDescriptorStore_.end()) {
                subtreeComplete = false;
                missingId = refId;
                break;
            }
        }
        if (!subtreeComplete) {
            pendingTemplateContainers_.insert(containerId);
        }
    }
}

void SurfaceSlot::SetSurfaceId(const std::string& surfaceId)
{
    surfaceId_ = surfaceId;
    if (modalCoordinator_ != nullptr) {
        modalCoordinator_->SetOwnerContext(renderId_, surfaceId_);
    }
    LOG_A2UI(LOG_INFO, "SetSurfaceId: surfaceId=%{public}s", surfaceId.c_str());
}

void SurfaceSlot::InitializeThemeManager(const ThemeContext& context)
{
    if (themeManager_ == nullptr) {
        themeManager_ = std::make_shared<ThemeManager>(surfaceId_, renderId_, context);
        LOG_A2UI(LOG_INFO,
            "InitializeThemeManager: surfaceId=%{public}s, renderId=%{public}d, colorMode=%{public}d, "
            "breakpoint=%{public}d",
            surfaceId_.c_str(), renderId_, static_cast<int32_t>(context.colorMode),
            static_cast<int32_t>(context.breakpoint));
    }
}

const std::string& SurfaceSlot::GetSurfaceId() const
{
    return surfaceId_;
}

void SurfaceSlot::Dispose()
{
    LOG_A2UI(LOG_INFO,
        "SurfaceSlot::Dispose - renderId=%{public}d, surfaceId=%{public}s, contentHandle=%{public}p, "
        "hasRootComponent=%{public}s, componentCount=%{public}zu, parentRelationCount=%{public}zu, "
        "hasCatalog=%{public}s, hasBindingEngine=%{public}s",
        renderId_, surfaceId_.c_str(), contentHandle_, rootComponent_ != nullptr ? "true" : "false",
        allComponents_.size(), parentsRelations_.size(), catalog_ != nullptr ? "true" : "false",
        bindingEngine_ != nullptr ? "true" : "false");

    DetachRootComponentFromContent(contentHandle_, rootComponent_);
    if (modalCoordinator_ != nullptr) {
        modalCoordinator_->Dispose();
    }

    rootComponent_.reset();
    parentsRelations_.clear();
    allComponents_.clear();
    allComponentDescriptorStore_.clear();
    pendingTemplateContainers_.clear();
    ClearRuntimeStateStore();
    catalog_.reset();
    contentHandle_ = nullptr;
    surfaceId_.clear();
    surfaceCatalogId_.clear();
    hasSurfaceCatalogId_ = false;
    surfaceProtocolMode_ = SurfaceProtocolMode::UNKNOWN;
    surfaceContext_ = SurfaceContext();

    // Release BindingEngine
    if (bindingEngine_ != nullptr) {
        LOG_A2UI(LOG_INFO, "Dispose: Resetting BindingEngine");
        bindingEngine_.reset();
    }

    LOG_A2UI(LOG_INFO, "SurfaceSlot::Dispose - renderId=%{public}d completed", renderId_);
}

std::shared_ptr<Component> SurfaceSlot::BuildComponent(const std::string& componentType)
{
    bool traceButtonRouting = componentType == "Button" || componentType == "Extended.Button";
    if (catalog_ == nullptr) {
        LOG_A2UI(LOG_WARN,
            "SurfaceSlot::BuildComponent - catalog is null, surfaceId=%{public}s, componentType=%{public}s",
            surfaceId_.c_str(), componentType.c_str());
        return nullptr;
    }
    if (componentType.empty()) {
        LOG_A2UI(
            LOG_WARN, "SurfaceSlot::BuildComponent - componentType is empty, surfaceId=%{public}s", surfaceId_.c_str());
        return nullptr;
    }
    if (IsExtendedProtocolSurface()) {
        ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
        if (ShouldUseCustomExtendedRow(apiVersion_, componentType)) {
            LOG_A2UI(LOG_INFO,
                "SurfaceSlot::BuildComponent - route Row to custom factory for apiVersion=%{public}d, "
                "surfaceId=%{public}s, componentType=%{public}s",
                apiVersion_, surfaceId_.c_str(), componentType.c_str());
            return CustomComponentFactory::Create(componentType);
        }
        if (factory.IsExtendedComponent(componentType)) {
            if (traceButtonRouting) {
                LOG_A2UI(LOG_DEBUG,
                    "SurfaceSlot::BuildComponent - Button route to extended factory, surfaceId=%{public}s, "
                    "componentType=%{public}s",
                    surfaceId_.c_str(), componentType.c_str());
            }
            LOG_A2UI(LOG_DEBUG,
                "SurfaceSlot::BuildComponent - route to extended factory, surfaceId=%{public}s, "
                "componentType=%{public}s",
                surfaceId_.c_str(), componentType.c_str());
            return factory.CreateComponent(componentType);
        }

        auto catalogItem = catalog_->GetCatalogItemByName(componentType);
        if (catalogItem == nullptr) {
            LOG_A2UI(LOG_WARN,
                "SurfaceSlot::BuildComponent - extended protocol component not found in catalog, "
                "surfaceId=%{public}s, catalogId=%{public}s, componentType=%{public}s",
                surfaceId_.c_str(), catalog_->GetCatalogId().c_str(), componentType.c_str());
            return nullptr;
        }
        if (catalogItem->IsInnerNative()) {
            LOG_A2UI(LOG_WARN,
                "SurfaceSlot::BuildComponent - extended protocol inner-native component is unsupported, "
                "surfaceId=%{public}s, componentType=%{public}s",
                surfaceId_.c_str(), componentType.c_str());
            return nullptr;
        }

        LOG_A2UI(LOG_INFO,
            "SurfaceSlot::BuildComponent - route to custom factory under extended protocol, "
            "surfaceId=%{public}s, componentType=%{public}s",
            surfaceId_.c_str(), componentType.c_str());
        if (traceButtonRouting) {
            LOG_A2UI(LOG_DEBUG,
                "SurfaceSlot::BuildComponent - Button route to custom factory under extended protocol, "
                "surfaceId=%{public}s, componentType=%{public}s",
                surfaceId_.c_str(), componentType.c_str());
        }
        return CustomComponentFactory::Create(componentType);
    }
    auto catalogItem = catalog_->GetCatalogItemByName(componentType);
    if (catalogItem == nullptr) {
        LOG_A2UI(LOG_WARN,
            "SurfaceSlot::BuildComponent - component not found in catalog, surfaceId=%{public}s, "
            "catalogId=%{public}s, componentType=%{public}s",
            surfaceId_.c_str(), catalog_->GetCatalogId().c_str(), componentType.c_str());
        return nullptr;
    }
    if (catalogItem->IsInnerNative()) {
        if (traceButtonRouting) {
            LOG_A2UI(LOG_DEBUG,
                "SurfaceSlot::BuildComponent - Button route to native factory, surfaceId=%{public}s, "
                "componentType=%{public}s",
                surfaceId_.c_str(), componentType.c_str());
        }
        return NativeComponentFactory::CreateComponent(componentType);
    } else {
        if (traceButtonRouting) {
            LOG_A2UI(LOG_DEBUG,
                "SurfaceSlot::BuildComponent - Button route to custom factory, surfaceId=%{public}s, "
                "componentType=%{public}s",
                surfaceId_.c_str(), componentType.c_str());
        }
        return CustomComponentFactory::Create(componentType);
    }
}

std::shared_ptr<Component> SurfaceSlot::BuildExtendedComponent(const std::string& componentType) const
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    if (!factory.IsExtendedComponent(componentType)) {
        LOG_A2UI(LOG_WARN, "SurfaceSlot::BuildExtendedComponent: unsupported extended component, type=%{public}s",
            componentType.c_str());
        return nullptr;
    }
    return factory.CreateComponent(componentType);
}

void SurfaceSlot::DebugPrintGlobalMapsInternal() const
{
    LOG_A2UI(LOG_DEBUG, "=== DebugPrintGlobalMaps Start ===");

    LOG_A2UI(LOG_DEBUG, "parentsRelations_ size: %{public}zu", parentsRelations_.size());
    for (const auto& pair : parentsRelations_) {
        LOG_A2UI(LOG_DEBUG, "parentsRelations_[%{public}s] = %{public}s", pair.first.c_str(), pair.second.c_str());
    }

    LOG_A2UI(LOG_DEBUG, "allComponents_ size: %{public}zu", allComponents_.size());
    for (const auto& pair : allComponents_) {
        LOG_A2UI(LOG_DEBUG, "allComponents_[%{public}s] = node", pair.first.c_str());
    }

    LOG_A2UI(LOG_DEBUG, "=== DebugPrintGlobalMaps End ===");
}

void SurfaceSlot::PrepareDescriptorById(const JsonValue& componentsValue)
{
    descriptorsById_.clear();
    int rootCount = componentsValue.GetArraySize();
    for (int i = 0; i < rootCount; ++i) {
        JsonValue nodeValue = componentsValue.GetArrayItem(i);
        if (!nodeValue.IsObject()) {
            continue;
        }
        if (IsExtendedProtocolSurface()) {
            std::string componentType = nodeValue.GetString("component", "");
            std::string normalizedType = NormalizeExtendedProtocolComponentType(componentType, catalog_);
            if (!componentType.empty() && normalizedType != componentType) {
                nodeValue.ReplaceString("component", normalizedType);
            }
        }
        if (!ValidateDescriptorIdForPreparation(nodeValue, renderId_, surfaceId_)) {
            continue;
        }
        std::string nodeId = nodeValue.GetString("id", "");
        auto existingIt = descriptorsById_.find(nodeId);
        if (existingIt == descriptorsById_.end()) {
            descriptorsById_.emplace(nodeId, nodeValue);
            allComponentDescriptorStore_[nodeId] = nodeValue;
            continue;
        }
        std::string existingType = existingIt->second.GetString("component", "");
        std::string incomingType = nodeValue.GetString("component", "");
        if (!existingType.empty() && !incomingType.empty() && existingType != incomingType) {
            continue;
        }
        if (incomingType.empty() && !existingType.empty()) {
            continue;
        }
        existingIt->second = nodeValue;
        allComponentDescriptorStore_[nodeId] = nodeValue;
    }
}

std::shared_ptr<Component> SurfaceSlot::GetOrCreateComponentNode(
    const JsonValue& nodeValue, const std::string& nodeId, const std::string& componentType, bool& isNewNode)
{
    isNewNode = false;
    auto [it, inserted] = allComponents_.try_emplace(nodeId, nullptr);
    if (!inserted && it->second != nullptr) {
        if (it->second->GetType() != componentType) {
            LOG_A2UI(LOG_WARN,
                "BuildRootFromComponents: duplicate id with different component type, nodeId=%{public}s, "
                "existingType=%{public}s, incomingType=%{public}s",
                nodeId.c_str(), it->second->GetType().c_str(), componentType.c_str());
            return nullptr;
        } else {
            LOG_A2UI(LOG_DEBUG, "BuildRootFromComponents: found existing node for nodeId=%{public}s", nodeId.c_str());
            return it->second;
        }
    }

    std::shared_ptr<Component> node = BuildComponent(componentType);
    if (node == nullptr) {
        allComponents_.erase(it);
        LOG_A2UI(
            LOG_ERROR, "BuildRootFromComponents: failed to create node for nodeId=%{public}s, skipped", nodeId.c_str());
        return nullptr;
    }

    isNewNode = true;
    it->second = node;
    LOG_A2UI(LOG_DEBUG, "BuildRootFromComponents: created node for nodeId=%{public}s %{public}p ", nodeId.c_str(),
        node.get());
    return node;
}

void SurfaceSlot::RegisterComponentIfNeeded(const std::shared_ptr<Component>& node, bool isNewNode) const
{
    if (node == nullptr || bindingEngine_ == nullptr) {
        return;
    }
    if (isNewNode) {
        bindingEngine_->RegisterComponent(node);
        LOG_A2UI(LOG_DEBUG,
            "BuildRootFromComponents: Registered component %{public}s to BindingEngine, has %{public}zu bindings",
            node->GetComponentId().c_str(), node->GetDataBindings().size());
        return;
    }

    bindingEngine_->SyncComponentBindings(node);
    LOG_A2UI(LOG_DEBUG, "BuildRootFromComponents: Synced bindings for component %{public}s, has %{public}zu bindings",
        node->GetComponentId().c_str(), node->GetDataBindings().size());
}

void SurfaceSlot::RefreshLazyAdapters(const std::string& changedPath, bool refreshAll)
{
    for (const auto& [_, component] : allComponents_) {
        auto a2uiComp = std::dynamic_pointer_cast<A2UIComponent>(component);
        if (a2uiComp == nullptr) {
            continue;
        }
        auto adapter = a2uiComp->GetLazyAdapter();
        if (adapter == nullptr) {
            continue;
        }
        const std::string& adapterPath = adapter->GetDataPath();
        if (!refreshAll && !ShouldRefreshLazyAdapter(changedPath, adapterPath)) {
            continue;
        }
        a2uiComp->RefreshLazyAdapterFromDataModel();
    }
}

void SurfaceSlot::ApplyExtendedComponentDescriptor(const JsonValue& nodeValue, const std::shared_ptr<Component>& node,
    bool isNewNode, const RenderContext& renderContext) const
{
    if (node == nullptr) {
        LOG_A2UI(LOG_WARN, "SurfaceSlot::ApplyExtendedComponentDescriptor - node is null");
        return;
    }

    auto extendedNode = std::dynamic_pointer_cast<ExtendedComponent>(node);
    if (extendedNode == nullptr) {
        LOG_A2UI(LOG_DEBUG,
            "SurfaceSlot::ApplyExtendedComponentDescriptor - skip non-extended component, nodeId=%{public}s, "
            "componentType=%{public}s",
            node->GetComponentId().c_str(), node->GetType().c_str());
        return;
    }
    bool applyResult = isNewNode ? extendedNode->InitFromDescriptor(nodeValue, renderContext)
                                 : extendedNode->UpdateFromDescriptor(nodeValue, renderContext);
    LOG_A2UI(LOG_DEBUG,
        "SurfaceSlot::ApplyExtendedComponentDescriptor - completed, nodeId=%{public}s, componentType=%{public}s, "
        "operation=%{public}s, result=%{public}s",
        nodeValue.GetString("id", "").c_str(), nodeValue.GetString("component", "").c_str(),
        isNewNode ? "init" : "update", applyResult ? "true" : "false");
}

std::shared_ptr<Component> SurfaceSlot::CreateOrUpdateComponentNode(
    const JsonValue& nodeValue, const std::string& nodeId, const std::string& componentType)
{
    bool isNewNode = false;
    std::shared_ptr<Component> node = GetOrCreateComponentNode(nodeValue, nodeId, componentType, isNewNode);
    if (node == nullptr) {
        return nullptr;
    }
    // Set surfaceId renderId and componentId for component before registering to BindingEngine.
    node->SetSurfaceId(GetSurfaceId());
    node->SetRenderId(GetRenderId());
    node->SetSurfaceContext(GetSurfaceContext());
    node->SetComponentId(nodeId);
    bool isExtendedNode = std::dynamic_pointer_cast<ExtendedComponent>(node) != nullptr;
    if (!isExtendedNode) {
        node->ApplyDescriptor(nodeValue);
    }
    ThemeMode colorMode = ThemeMode::LIGHT;
    if (themeManager_ != nullptr) {
        colorMode = themeManager_->GetContext().colorMode;
    }
    RenderContext renderContext = RenderContext::Create(
        GetRenderId(), GetSurfaceId(), bindingEngine_, catalog_, fontSizeScale_, apiVersion_, colorMode);
    ApplyExtendedComponentDescriptor(nodeValue, node, isNewNode, renderContext);
    node->ApplyParentsRelations(this);

    // Register component to BindingEngine after ApplyDescriptor (when
    // componentId and bindings are set). Only register new nodes to avoid duplicate registration.
    RegisterComponentIfNeeded(node, isNewNode);
    return node;
}

std::shared_ptr<Component> SurfaceSlot::BuildRootFromComponents(
    const JsonValue& componentsValue, bool& hasProcessedNode, bool& sawRootDescriptor)
{
    hasProcessedNode = false;
    sawRootDescriptor = false;
    PrepareDescriptorById(componentsValue);
    auto root = BuildRootFromComponents("root", descriptorsById_, hasProcessedNode, sawRootDescriptor);
    // Root size behavior is controlled by forceRootFill_.
    ApplyRootDefaultSizeToContentArea(root, forceRootFill_);
    return root;
}

std::shared_ptr<Component> SurfaceSlot::BuildRootFromComponents(const std::string& rootId,
    const std::map<std::string, JsonValue>& descriptorsById, bool& hasProcessedNode, bool& sawRootDescriptor,
    bool updateSurfaceRoot)
{
    std::shared_ptr<Component> resolvedRoot = nullptr;
    std::vector<ModalCoordinator::ModalDescriptor> modalDescriptors;
    std::vector<std::shared_ptr<Component>> buildNodeCandidates;
    buildNodeCandidates.reserve(descriptorsById.size());
    const bool hasExplicitRootDescriptor = descriptorsById.find(rootId) != descriptorsById.end();
    sawRootDescriptor = hasExplicitRootDescriptor;
    std::set<std::string> staticReachableIds;
    if (hasExplicitRootDescriptor) {
        CollectStaticReachableDescriptorIds(rootId, descriptorsById, staticReachableIds);
    }
    std::set<std::string> templateRootIds;
    for (const auto& [_, descriptor] : descriptorsById) {
        CollectTemplateRootIdsFromDescriptor(descriptor, templateRootIds);
    }
    // First pass: Create or update all components
    for (auto iter = descriptorsById.begin(); iter != descriptorsById.end(); ++iter) {
        std::string nodeId = iter->first;
        JsonValue nodeValue = iter->second;
        if (nodeId.empty()) {
            LOG_A2UI(LOG_WARN, "BuildRootFromComponents: nodeId is empty, skip");
            continue;
        }
        if (!nodeValue.IsObject()) {
            LOG_A2UI(LOG_WARN, "BuildRootFromComponents: nodeValue is not an object, skip");
            continue;
        }

        ValidateRequiredCreationFields(nodeValue, renderId_, surfaceId_);

        std::string componentType = nodeValue.GetString("component", "");
        if (componentType.empty()) {
            LOG_A2UI(
                LOG_WARN, "BuildRootFromComponents: component is empty for nodeId=%{public}s, skipped", nodeId.c_str());
            continue;
        }
        if (componentType == "Modal") {
            ModalCoordinator::ModalDescriptor modalDescriptor;
            if (modalCoordinator_ != nullptr && modalCoordinator_->TryCreateDescriptor(nodeValue, modalDescriptor)) {
                modalDescriptors.push_back(modalDescriptor);
                continue;
            }
            continue;
        }
        if (nodeId != rootId && templateRootIds.count(nodeId) > 0) {
            // Template prototypes stay cached in descriptor storage and are instantiated lazily by container nodes.
            hasProcessedNode = true;
            LOG_A2UI(LOG_DEBUG, "BuildRootFromComponents: skip template prototype nodeId=%{public}s", nodeId.c_str());
            continue;
        }
        if (hasExplicitRootDescriptor && staticReachableIds.count(nodeId) == 0 &&
            !ShouldBuildDetachedCheckboxForReachableGroup(nodeId, nodeValue, descriptorsById, staticReachableIds)) {
            hasProcessedNode = true;
            LOG_A2UI(LOG_DEBUG,
                "BuildRootFromComponents: skip detached descriptor nodeId=%{public}s under explicit root",
                nodeId.c_str());
            continue;
        }

        hasProcessedNode = true;
        if (nodeId == rootId || (!hasExplicitRootDescriptor && resolvedRoot == nullptr)) {
            PrebuildExtendedTabsChildren(*this, nodeValue);
        }

        std::shared_ptr<Component> node = CreateOrUpdateComponentNode(nodeValue, nodeId, componentType);
        if (node == nullptr) {
            continue;
        }
        ValidateRequiredStructuralFields(nodeValue, renderId_, surfaceId_, surfaceProtocolMode_);
        ValidateStructuralFieldShapes(nodeValue, renderId_, surfaceId_, surfaceProtocolMode_);
        ValidateEventHandlerFields(nodeValue, renderId_, surfaceId_);
        buildNodeCandidates.push_back(node);
        if (node->GetComponentId() == rootId) {
            sawRootDescriptor = true;
            resolvedRoot = node;
            if (updateSurfaceRoot) {
                SetRootComponent(node);
            }
        } else if (!hasExplicitRootDescriptor && resolvedRoot == nullptr) {
            resolvedRoot = rootComponent_ != nullptr ? rootComponent_ : node;
            if (updateSurfaceRoot && rootComponent_ == nullptr) {
                SetRootComponent(node);
            }
        }
    }

    std::set<std::shared_ptr<Component>, BuildNodeDepthComparator> buildNodes;
    for (const auto& node : buildNodeCandidates) {
        if (node == nullptr) {
            continue;
        }
        node->SetBuildDepth(ResolveBuildDepth(node->GetComponentId(), parentsRelations_));
        buildNodes.insert(node);
    }
    BuildComponentTree(buildNodes);
    SyncExtendedCheckboxGroupState();

    if (modalCoordinator_ != nullptr) {
        modalCoordinator_->HandlePendingModalDescriptors(modalDescriptors, allComponents_, parentsRelations_);
    }

    DebugPrintGlobalMapsInternal();
    return resolvedRoot;
}

void SurfaceSlot::BuildComponentTree(const std::set<std::shared_ptr<Component>, BuildNodeDepthComparator>& buildNodes)
{
    for (const auto& node : buildNodes) {
        if (node == nullptr) {
            continue;
        }
        auto ifNode = std::dynamic_pointer_cast<IfComponent>(node);
        if (ifNode != nullptr) {
            ifNode->BuildBranchChildren(*this);
            continue;
        }
        node->BuildChildren(*this);
    }

    for (const auto& node : buildNodes) {
        if (node == nullptr) {
            continue;
        }
        const std::string& nodeId = node->GetComponentId();
        if (nodeId.empty()) {
            continue;
        }
        auto relationIt = parentsRelations_.find(nodeId);
        if (relationIt != parentsRelations_.end() && !relationIt->second.empty()) {
            node->AttachToParentIfNeeded(allComponents_, relationIt->second);
        }
    }
}

void SurfaceSlot::SyncExtendedCheckboxGroupState()
{
    struct CheckboxGroupState {
        bool selectAll = false;
        int32_t shape = 0;
    };

    std::unordered_map<std::string, CheckboxGroupState> groupStates;
    for (const auto& groupEntry : allComponents_) {
        auto group = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(groupEntry.second);
        if (group == nullptr || group->GetGroup().empty()) {
            continue;
        }

        groupStates[group->GetGroup()] = { group->GetSelectAll(), group->GetShape() };
    }
    if (groupStates.empty()) {
        return;
    }

    for (const auto& checkboxEntry : allComponents_) {
        auto checkbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(checkboxEntry.second);
        if (checkbox == nullptr) {
            continue;
        }
        auto groupIt = groupStates.find(checkbox->GetGroup());
        if (groupIt == groupStates.end()) {
            continue;
        }
        checkbox->ApplyInheritedSelect(groupIt->second.selectAll);
        checkbox->ApplyInheritedShape(groupIt->second.shape);
    }
}

void SurfaceSlot::SetCatalog(const std::shared_ptr<Catalog>& catalog)
{
    if (catalog == nullptr) {
        LOG_A2UI(LOG_ERROR, "SetCatalog catalog nullptr");
        return;
    }
    if (catalog_ && catalog_->Equals(*catalog)) {
        return;
    }
    catalog_ = catalog;
    UpdateSurfaceProtocolMode();
    SetSurfaceContext(catalog_->GetSurfaceContext());
    LOG_A2UI(LOG_INFO,
        "SurfaceSlot::SetCatalog - renderId=%{public}d, surfaceId=%{public}s, catalogUpdated=%{public}s, "
        "a2UIProtocolVersion=%{public}s, catalogId=%{public}s, "
        "surfaceProtocolMode=%{public}s",
        renderId_, surfaceId_.c_str(), catalog_ != nullptr ? "true" : "false",
        surfaceContext_.a2UIProtocolVersion.c_str(), catalog_->GetCatalogId().c_str(),
        SurfaceProtocolModeToString(surfaceProtocolMode_));

    catalog_->DebugPrint();
}

std::shared_ptr<Catalog> SurfaceSlot::GetCatalog() const
{
    return catalog_;
}

void SurfaceSlot::SetSurfaceCatalogId(const std::string& catalogId)
{
    surfaceCatalogId_ = catalogId;
    hasSurfaceCatalogId_ = true;
    UpdateSurfaceProtocolMode();
}

std::string SurfaceSlot::ResolveProtocolCatalogId() const
{
    if (hasSurfaceCatalogId_) {
        return surfaceCatalogId_;
    }
    if (catalog_ != nullptr) {
        return catalog_->GetCatalogId();
    }
    return "";
}

void SurfaceSlot::UpdateSurfaceProtocolMode()
{
    surfaceProtocolMode_ = SurfaceProtocolMode::A2UI_STANDARD;
    std::string protocolCatalogId = ResolveProtocolCatalogId();
    std::string normalizedCatalogId = ToLowerCopy(protocolCatalogId);
    if (IsExtendedCatalogId(normalizedCatalogId)) {
        surfaceProtocolMode_ = SurfaceProtocolMode::EXTENDED_PROTOCOL;
        LOG_A2UI(LOG_INFO,
            "SurfaceSlot::UpdateSurfaceProtocolMode - protocolCatalogId=%{public}s, source=%{public}s, "
            "mode=%{public}s",
            protocolCatalogId.c_str(), hasSurfaceCatalogId_ ? "createSurface" : "catalog",
            SurfaceProtocolModeToString(surfaceProtocolMode_));
        return;
    }
    if (IsBasicCatalogId(normalizedCatalogId)) {
        surfaceProtocolMode_ = SurfaceProtocolMode::A2UI_STANDARD;
    }
    LOG_A2UI(LOG_INFO,
        "SurfaceSlot::UpdateSurfaceProtocolMode - protocolCatalogId=%{public}s, source=%{public}s, "
        "mode=%{public}s",
        protocolCatalogId.c_str(), hasSurfaceCatalogId_ ? "createSurface" : "catalog",
        SurfaceProtocolModeToString(surfaceProtocolMode_));
}

bool SurfaceSlot::IsExtendedProtocolSurface() const
{
    return surfaceProtocolMode_ == SurfaceProtocolMode::EXTENDED_PROTOCOL;
}

void SurfaceSlot::SetSurfaceContext(const SurfaceContext& surfaceContext)
{
    surfaceContext_ = surfaceContext;
}

const SurfaceContext& SurfaceSlot::GetSurfaceContext() const
{
    return surfaceContext_;
}

void SurfaceSlot::SetRenderId(int32_t renderId)
{
    renderId_ = renderId;
    if (modalCoordinator_ != nullptr) {
        modalCoordinator_->SetOwnerContext(renderId_, surfaceId_);
    }
    LOG_A2UI(LOG_INFO, "SurfaceSlot::SetRenderId - renderId=%{public}d", renderId_);
}

int32_t SurfaceSlot::GetRenderId() const
{
    return renderId_;
}

std::shared_ptr<Component> SurfaceSlot::FindComponentById(const std::string& componentId) const
{
    auto iter = allComponents_.find(componentId);
    if (iter != allComponents_.end()) {
        return iter->second;
    }

    // Keep old callers working while template instance ids migrate to the canonical "/path..."
    // form derived from children.path.
    if (!componentId.empty() && componentId[0] != '/') {
        std::string canonicalId = "/" + componentId;
        iter = allComponents_.find(canonicalId);
        if (iter != allComponents_.end()) {
            return iter->second;
        }
    }
    return nullptr;
}

std::map<std::string, std::string>& SurfaceSlot::GetParentsRelations()
{
    return parentsRelations_;
}

#ifdef TDD_BUILD
int32_t SurfaceSlot::ResolveBuildDepthForTest(const std::string& nodeId) const
{
    return ResolveBuildDepth(nodeId, parentsRelations_);
}
#endif

std::map<std::string, JsonValue>& SurfaceSlot::GetDescriptorsById()
{
    return descriptorsById_;
}

const std::map<std::string, JsonValue>& SurfaceSlot::GetAllComponentDescriptorStore() const
{
    return allComponentDescriptorStore_;
}

std::map<std::string, std::shared_ptr<Component>>& SurfaceSlot::GetAllComponents()
{
    return allComponents_;
}

std::vector<std::shared_ptr<Component>> SurfaceSlot::GetAllComponents() const
{
    std::vector<std::shared_ptr<Component>> components;
    components.reserve(allComponents_.size());
    for (const auto& entry : allComponents_) {
        if (entry.second != nullptr) {
            components.push_back(entry.second);
        }
    }
    return components;
}

void SurfaceSlot::StoreRuntimeState(const std::string& scope, const std::string& key, const JsonValue& state)
{
    if (scope.empty() || key.empty() || !state.IsValid()) {
        return;
    }

    std::unique_ptr<JsonAdapter> stateAdapter = JsonAdapter::Clone(state);
    if (stateAdapter == nullptr) {
        return;
    }
    runtimeStateStore_[scope][key] = stateAdapter->GetRoot();
}

bool SurfaceSlot::GetRuntimeState(const std::string& scope, const std::string& key, JsonValue& state) const
{
    if (scope.empty() || key.empty()) {
        return false;
    }
    auto scopeIt = runtimeStateStore_.find(scope);
    if (scopeIt == runtimeStateStore_.end()) {
        return false;
    }
    auto stateIt = scopeIt->second.find(key);
    if (stateIt == scopeIt->second.end() || !stateIt->second.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> stateAdapter = JsonAdapter::Clone(stateIt->second);
    if (stateAdapter == nullptr) {
        return false;
    }
    state = stateAdapter->GetRoot();
    return true;
}

void SurfaceSlot::ForEachRuntimeState(
    const std::string& scope, const std::function<void(const std::string&, const JsonValue&)>& visitor) const
{
    if (scope.empty() || !visitor) {
        return;
    }
    auto scopeIt = runtimeStateStore_.find(scope);
    if (scopeIt == runtimeStateStore_.end()) {
        return;
    }
    for (const auto& entry : scopeIt->second) {
        if (entry.second.IsValid()) {
            visitor(entry.first, entry.second);
        }
    }
}

void SurfaceSlot::ClearRuntimeStateStore()
{
    if (runtimeStateStore_.empty()) {
        return;
    }
    LOG_A2UI(LOG_DEBUG, "SurfaceSlot::ClearRuntimeStateStore: scopeCount=%{public}zu", runtimeStateStore_.size());
    runtimeStateStore_.clear();
}

void SurfaceSlot::CaptureRuntimeStateTree(const std::shared_ptr<Component>& component)
{
    if (component == nullptr) {
        return;
    }
    const std::string scope = component->GetRuntimeStateScope();
    const std::string key = component->GetRuntimeStateKey();
    if (!scope.empty() && !key.empty()) {
        JsonValue state = component->CaptureRuntimeState();
        StoreRuntimeState(scope, key, state);
    }
    for (const auto& child : component->GetChildren()) {
        CaptureRuntimeStateTree(child);
    }
}

void SurfaceSlot::RestoreRuntimeStateTree(const std::shared_ptr<Component>& component) const
{
    if (component == nullptr) {
        return;
    }
    const std::string scope = component->GetRuntimeStateScope();
    const std::string key = component->GetRuntimeStateKey();
    if (!scope.empty() && !key.empty()) {
        JsonValue state;
        if (GetRuntimeState(scope, key, state)) {
            component->RestoreRuntimeState(state);
        }
    }
    for (const auto& child : component->GetChildren()) {
        RestoreRuntimeStateTree(child);
    }
}

std::shared_ptr<DataModel> SurfaceSlot::GetOrCreateDataModel()
{
    if (bindingEngine_ == nullptr) {
        return nullptr;
    }
    return bindingEngine_->GetOrCreateDataModel(GetSurfaceId());
}

void SurfaceSlot::SetForceRootFill(bool forceFill)
{
    if (forceRootFill_ == forceFill) {
        return;
    }
    forceRootFill_ = forceFill;
    if (rootComponent_ == nullptr) {
        return;
    }
    ApplyRootDefaultSizeToContentArea(rootComponent_, forceRootFill_);
}

void SurfaceSlot::SetFontSizeScale(float scale)
{
    if (scale <= 0.0F) {
        fontSizeScale_ = 1.0F;
    } else {
        fontSizeScale_ = scale;
    }

    for (const auto& [id, component] : allComponents_) {
        if (component == nullptr) {
            continue;
        }
        auto extended = std::dynamic_pointer_cast<ExtendedComponent>(component);
        if (extended != nullptr) {
            extended->OnFontSizeScaleChanged(fontSizeScale_);
        }
    }
}

float SurfaceSlot::GetFontSizeScale() const
{
    return fontSizeScale_;
}

void SurfaceSlot::SetApiVersion(int32_t apiVersion)
{
    apiVersion_ = apiVersion;
}

int32_t SurfaceSlot::GetApiVersion() const
{
    return apiVersion_;
}
} // namespace NativeModule
