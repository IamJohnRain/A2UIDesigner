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

#include "CustomComponent.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "catalog/CatalogConstants.h"
#include "components/ChildListSchemaValidationUtils.h"
#include "components/TypeValidation.h"
#include "components/actions/EventHandlerChainExecutor.h"
#include "components/custom/CustomComponentExpressionBinding.h"
#include "components/custom/ExtendedTabsPrebuildHelper.h"
#include "composition/ChildListParser.h"
#include "composition/TemplateInstantiator.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "data/PathValidator.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/EventContextResolver.h"
#include "functions/FunctionBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "styles/StyleApplyUtils.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"
#include "utils/NapiUtils.h"
#include "utils/RequiredStringPropertyUtils.h"

#include "NapiBridge.h"
#include "NapiResourceManager.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SchemaErrorCodes.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/ExpressionEngine.h"
#endif

namespace NativeModule {

std::string ResolveCustomPropertyWarningPath(const std::string& componentId, const std::string& propertyKey)
{
    std::string prefix = componentId;
    size_t start = prefix.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        prefix = "customProps";
    } else {
        size_t end = prefix.find_last_not_of(" \t\r\n");
        prefix = prefix.substr(start, end - start + 1);
    }

    if (propertyKey.empty()) {
        return prefix;
    }
    return prefix + "." + propertyKey;
}

namespace {

std::string ToLowerCopy(const std::string& value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return normalized;
}

std::string BuildTemplateInstanceRootId(
    const std::string& templateComponentId, const std::string& templatePath, int32_t itemIndex)
{
    return templatePath + templateComponentId + ":" + std::to_string(itemIndex) + ":" + templateComponentId;
}

std::mutex g_customComponentRegistryMutex;
std::set<CustomComponent*> g_activeCustomComponents;

void RegisterActiveCustomComponent(CustomComponent* component)
{
    if (component == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_customComponentRegistryMutex);
    g_activeCustomComponents.insert(component);
}

void UnregisterActiveCustomComponent(CustomComponent* component)
{
    if (component == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_customComponentRegistryMutex);
    g_activeCustomComponents.erase(component);
}

// Helper function to get SurfaceManager by renderId
// This is used for custom component callbacks
std::shared_ptr<SurfaceManager> GetCustomComponentSurfaceManager(int32_t renderId)
{
    auto& renderManager = RenderManager::GetInstance();
    if (renderManager.GetRenderSlotCount() == 0) {
        LOG_A2UI(LOG_ERROR, "GetCustomComponentSurfaceManager: No RenderSlots available");
        return nullptr;
    }

    RenderSlot* renderSlot = renderManager.FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        LOG_A2UI(LOG_ERROR, "GetCustomComponentSurfaceManager: RenderSlot not found for renderId=%{public}d", renderId);
        return nullptr;
    }

    return renderSlot->GetSurfaceManager();
}

// Helper function to get NapiResourceManager
// This is used for custom component callbacks
NapiResourceManager* GetNapiResourceManager()
{
    return RenderManager::GetInstance().GetNapiResourceManager();
}

constexpr const char* COMPONENT_THEME_KEY = "componentTheme";
constexpr const char* ACCESSIBILITY_LABEL_KEY = "label";
constexpr const char* ACCESSIBILITY_DESCRIPTION_KEY = "description";
constexpr const char* CONTENT_KEY = "content";
constexpr const char* CHILD_SLOT_KEY = "childSlot";
constexpr const char* CHILD_SLOTS_KEY = "childSlots";
constexpr const char* CHILD_SLOTS_OBJECT_KEY = "childSlotsObject";
constexpr const char* DISPOSE_KEY = "dispose";
constexpr const char* CHOICE_PICKER_TYPE = "ChoicePicker";
constexpr const char* TABS_COMPONENT_TYPE = "Tabs";
constexpr const char* VALUE_KEY = "value";
constexpr const char* PATH_KEY = "path";
constexpr char COMPONENT_TYPE_SEPARATOR = '.';

std::string ResolveDynamicPath(const JsonValue& value)
{
    if (!value.IsValid() || !value.IsObject()) {
        return "";
    }

    JsonValue pathValue = value.GetItem(PATH_KEY);
    if (!pathValue.IsValid() || !pathValue.IsString()) {
        return "";
    }
    return pathValue.GetStringValue("");
}

std::string ResolveChoicePickerValueBindingPath(const JsonValue& descriptor)
{
    if (!descriptor.IsValid() || !descriptor.IsObject() || !descriptor.Has(VALUE_KEY)) {
        return "";
    }

    JsonValue value = descriptor.GetItem(VALUE_KEY);
    std::string directPath = ResolveDynamicPath(value);
    if (!directPath.empty()) {
        return directPath;
    }

    if (!value.IsValid() || !value.IsArray() || value.GetArraySize() != 1) {
        return "";
    }
    return ResolveDynamicPath(value.GetArrayItem(0));
}

std::string GetShortComponentType(const std::string& componentType)
{
    size_t separatorIndex = componentType.find_last_of(COMPONENT_TYPE_SEPARATOR);
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= componentType.size()) {
        return componentType;
    }
    return componentType.substr(separatorIndex + 1);
}

bool IsTabContentComponentType(const std::string& componentType)
{
    return GetShortComponentType(componentType) == "TabContent";
}

bool IsWebComponentType(const std::string& componentType)
{
    return componentType == "Extended.Web" || GetShortComponentType(componentType) == "Web";
}

bool IsKnownAccessibilityField(const std::string& key)
{
    return key == ACCESSIBILITY_LABEL_KEY || key == ACCESSIBILITY_DESCRIPTION_KEY;
}

std::string FormatArgbColorString(uint32_t argb)
{
    std::ostringstream builder;
    builder << "#" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << argb;
    return builder.str();
}

} // namespace

std::unique_ptr<JsonAdapter> BuildComponentThemeJson(const ThemeContext& themeContext)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return nullptr;
    }

    JsonValue root = adapter->GetRoot();
    root.PutNumber("colorMode", static_cast<double>(static_cast<int32_t>(themeContext.colorMode)));
    root.PutNumber("breakpoint", static_cast<double>(static_cast<int32_t>(themeContext.breakpoint)));
    if (themeContext.hasPrimaryColor) {
        root.PutString("primaryColor", FormatArgbColorString(themeContext.primaryColorArgb));
    }
    if (themeContext.hasDarkPrimaryColor) {
        root.PutString("darkPrimaryColor", FormatArgbColorString(themeContext.darkPrimaryColorArgb));
    }
    if (themeContext.hasBrandColor) {
        root.PutString("brandColor", FormatArgbColorString(themeContext.brandColor));
    }
    if (!themeContext.iconUrl.empty()) {
        root.PutString("iconUrl", themeContext.iconUrl);
    }
    if (!themeContext.agentDisplayName.empty()) {
        root.PutString("agentDisplayName", themeContext.agentDisplayName);
    }
    return adapter;
}

void SetComponentThemeProperty(napi_env env, napi_value object, const ThemeContext& themeContext)
{
    std::unique_ptr<JsonAdapter> componentThemeAdapter = BuildComponentThemeJson(themeContext);
    if (componentThemeAdapter == nullptr) {
        return;
    }

    auto& napi = NapiBridge::GetInstance().Provider();
    napi.SetNamedProperty(
        env, object, COMPONENT_THEME_KEY, JsonValueToNapiValue(env, componentThemeAdapter->GetRoot()));
}

namespace {
bool HasNamedProperty(napi_env env, napi_value object, const char* key)
{
    bool hasProperty = false;
    napi_has_named_property(env, object, key, &hasProperty);
    return hasProperty;
}

std::string BuildEdgeStyleInfo(
    const JsonValue& descriptor, const char* topKey, const char* rightKey, const char* bottomKey, const char* leftKey)
{
    bool hasValue =
        descriptor.Has(topKey) || descriptor.Has(rightKey) || descriptor.Has(bottomKey) || descriptor.Has(leftKey);
    if (!hasValue) {
        return "";
    }

    std::ostringstream builder;
    builder << "{\"top\":" << descriptor.GetNumber(topKey, 0.0) << ",\"right\":" << descriptor.GetNumber(rightKey, 0.0)
            << ",\"bottom\":" << descriptor.GetNumber(bottomKey, 0.0)
            << ",\"left\":" << descriptor.GetNumber(leftKey, 0.0) << "}";
    return builder.str();
}

/**
 * Parse a width/height value from the component descriptor using
 * StyleApplyUtils::ParseDimension for strict schema validation.
 *
 * @param descriptor   The parent JSON object that may contain @p key.
 * @param key          "width" or "height".
 * @param outValue     On success, set to the parsed numeric value.
 * @param reportFn     Callable(code, message, path) for schema warnings.
 * @return             true when a valid dimension was extracted.
 */
template<typename ReportFn>
bool ParseCommonDimensionField(const JsonValue& descriptor, const char* key, double& outValue, ReportFn&& reportFn)
{
    if (key == nullptr || !descriptor.Has(key)) {
        return false;
    }

    JsonValue fieldValue = descriptor.GetItem(key);
    if (IsDynamicDescriptorObject(fieldValue) || IsExpressionStringValue(fieldValue)) {
        // Dynamic / data-bound / expression values cannot be validated statically.
        // They will be resolved at runtime by the binding engine.
        return false;
    }

    StyleDimension dimension;
    if (StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
        outValue = static_cast<double>(dimension.value);
        return true;
    }

    // Validation failed — report the appropriate warning.
    std::string path(key);
    if (fieldValue.IsString() || fieldValue.IsNumber()) {
        reportFn(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + path + " has invalid value and has been reset to default", path);
    } else {
        reportFn(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + path + " expects dimension value (number or string), got type '" +
                std::string(fieldValue.GetTypeName()) + "', fallback/reset has been applied",
            path);
    }
    return false;
}

bool IsReservedDescriptorKey(const std::string& key)
{
    static const std::set<std::string> reservedKeys = { "id", "component", "type", "children", "checks", "width",
        "height", "paddingTop", "paddingRight", "paddingBottom", "paddingLeft", "marginTop", "marginRight",
        "marginBottom", "marginLeft" };
    return reservedKeys.find(key) != reservedKeys.end() || EventHandlerParser::KNOWN_EVENT_NAMES.count(key) > 0;
}

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

enum class CustomDynamicDescriptorKind { NONE = 0, PATH, CALL };

CustomDynamicDescriptorKind ResolveCustomDynamicDescriptorKind(const JsonValue& value)
{
    if (!value.IsObject()) {
        return CustomDynamicDescriptorKind::NONE;
    }
    if (value.Has("path")) {
        return CustomDynamicDescriptorKind::PATH;
    }
    if (value.Has("call")) {
        return CustomDynamicDescriptorKind::CALL;
    }
    return CustomDynamicDescriptorKind::NONE;
}

bool ShouldReportCustomDynamicDescriptorCompatibilityWarning(
    const std::string& componentType, const std::string& propertyKey)
{
    if (componentType == "Icon") {
        return propertyKey == "name";
    }
    if (componentType == "Divider") {
        return propertyKey == "axis";
    }
    if (componentType == "DateTimeInput") {
        return propertyKey == "enableDate" || propertyKey == "enableTime" || propertyKey == "min" ||
               propertyKey == "max";
    }
    if (componentType == "ChoicePicker") {
        return propertyKey == "variant" || propertyKey == "displayStyle" || propertyKey == "filterable";
    }
    return false;
}

void DispatchCustomDynamicDescriptorCompatibilityWarning(int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, const std::string& componentType, const std::string& propertyKey,
    CustomDynamicDescriptorKind kind)
{
    if (renderId < 0 || propertyKey.empty()) {
        return;
    }

    std::string message =
        "Property " + propertyKey + " does not support dynamic descriptor, compatibility resolution has been preserved";
    if (kind == CustomDynamicDescriptorKind::PATH) {
        message = "Property " + propertyKey +
                  " does not support local path binding, compatibility resolution has been preserved";
    } else if (kind == CustomDynamicDescriptorKind::CALL) {
        message = "Property " + propertyKey +
                  " does not support function-call descriptor, compatibility resolution has been preserved";
    }

    WarningDispatchBridge::GetInstance().Dispatch(renderId, surfaceId, componentId, SCHEMA_ERROR_CODE_INVALID_VALUE,
        message, ResolveCustomPropertyWarningPath(componentId, propertyKey), "component",
        componentType.empty() ? "component" : componentType);
}

} // namespace

CustomComponent::CustomComponent(const std::string& componentType, bool preserveDynamicDescriptors)
    : Component(nullptr, false),
      checksEngine_(std::make_unique<ChecksEngine>(
          [this]() { return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() }; },
          [this](JsonValue& value) {
              if (!currentCheckTargetValue_.IsValid()) {
                  return false;
              }
              value = currentCheckTargetValue_;
              return true;
          })),
      preserveDynamicDescriptors_(preserveDynamicDescriptors)
{
    descriptor_.type = componentType;
    RegisterActiveCustomComponent(this);
    LOG_A2UI(LOG_INFO, "CustomComponent: no-wrapper mode, type=%{public}s", componentType.c_str());
}

CustomComponent::~CustomComponent()
{
    ClearDynamicValueCallbacks();
    UnregisterActiveCustomComponent(this);
    ClearChildren();
    DisposeComponentContent();
    ResetReferences();
    childSlotHandle_ = nullptr;
    hasCreatedCustomComponent_ = false;
    properties_.clear();
    rawDynamicProperties_.clear();
}

CustomComponent* CustomComponent::FindByHandle(uintptr_t handle)
{
    if (handle == 0U) {
        return nullptr;
    }
    CustomComponent* component = reinterpret_cast<CustomComponent*>(handle);
    std::lock_guard<std::mutex> lock(g_customComponentRegistryMutex);
    if (g_activeCustomComponents.find(component) == g_activeCustomComponents.end()) {
        return nullptr;
    }
    return component;
}

uintptr_t CustomComponent::GetCustomComponentHandle() const
{
    return reinterpret_cast<uintptr_t>(this);
}

std::string CustomComponent::GetType() const
{
    return descriptor_.type;
}

bool CustomComponent::ShouldValidateUnknownDescriptorFields() const
{
    return false;
}

bool CustomComponent::AcceptsChild(const std::shared_ptr<Component>& child) const
{
    if (child == nullptr) {
        return false;
    }
    if (IsExtendedTabsType()) {
        return IsExtendedTabsChildComponentType(child->GetType());
    }
    return Component::AcceptsChild(child);
}

bool CustomComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    return ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
}

void CustomComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListDescriptor();
    using ChildListDescriptorParseHandler = void (CustomComponent::*)(const JsonValue&);
    static const std::unordered_map<std::string, ChildListDescriptorParseHandler> shortTypeParserTable = {
        { "Tabs", &CustomComponent::ParseTabsChildListDescriptor },
        { "Row", &CustomComponent::ParseRowChildListDescriptor },
        { "TabContent", &CustomComponent::ParseTabContentChildListDescriptor }
    };

    if (IsExtendedTabsType()) {
        ParseExtendedTabsChildListDescriptor(descriptor);
        return;
    }

    std::string shortType = GetShortType(descriptor_.type);
    auto shortParserIt = shortTypeParserTable.find(shortType);
    if (shortParserIt != shortTypeParserTable.end()) {
        (this->*(shortParserIt->second))(descriptor);
        return;
    }

    Component::CollectChildListDescriptor(descriptor);
}

void CustomComponent::ParseExtendedTabsChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ParseExtendedTabsChildList(descriptor);
}

void CustomComponent::ParseTabsChildListDescriptor(const JsonValue& descriptor)
{
    JsonValue tabsValue = descriptor.GetItem("tabs");
    if (!tabsValue.IsValid() || !tabsValue.IsArray()) {
        Component::CollectChildListDescriptor(descriptor);
        return;
    }

    childListDescriptor_.type = ChildListType::STATIC_IDS;
    for (size_t i = 0; i < tabsValue.GetArraySize(); ++i) {
        JsonValue tabItem = tabsValue.GetArrayItem(i);
        if (!tabItem.IsObject()) {
            continue;
        }
        std::string childId = tabItem.GetString("child", "");
        if (!childId.empty()) {
            childListDescriptor_.staticChildIds.push_back(childId);
        }
    }
    if (childListDescriptor_.staticChildIds.empty()) {
        Component::CollectChildListDescriptor(descriptor);
    }
}

void CustomComponent::ParseRowChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

void CustomComponent::ParseTabContentChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

void CustomComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    if (child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }

    // For multi-slot components (e.g., Tabs)
    if (!childToSlotMapping_.empty() && !childSlotHandles_.empty()) {
        std::string childId = child->GetComponentId();
        auto mappingIt = childToSlotMapping_.find(childId);
        if (mappingIt != childToSlotMapping_.end()) {
            std::string slotKey = mappingIt->second;
            auto slotIt = childSlotHandles_.find(slotKey);
            if (slotIt != childSlotHandles_.end() && slotIt->second != nullptr) {
                ArkUIOHApiAdapter::NodeContentInsertNode(
                    slotIt->second, child->GetNativeView(), static_cast<int32_t>(index));
                LOG_A2UI(LOG_INFO,
                    "CustomComponent::OnAddChild: Added child=%{public}s to slot=%{public}s, type=%{public}s, "
                    "index=%{public}zu",
                    childId.c_str(), slotKey.c_str(), descriptor_.type.c_str(), index);
                return;
            }
        }
    }

    // Fallback to single slot
    if (childSlotHandle_ != nullptr) {
        ArkUIOHApiAdapter::NodeContentInsertNode(childSlotHandle_, child->GetNativeView(), static_cast<int32_t>(index));
        LOG_A2UI(LOG_INFO,
            "CustomComponent::OnAddChild: Added child to single slot, type=%{public}s, index=%{public}zu",
            descriptor_.type.c_str(), index);
    } else {
        LOG_A2UI(LOG_WARN, "CustomComponent::OnAddChild: No slot available, type=%{public}s", descriptor_.type.c_str());
    }
}

void CustomComponent::OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    static_cast<void>(currentIndex);
    if (child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }

    // For multi-slot components (e.g., Tabs)
    if (!childToSlotMapping_.empty() && !childSlotHandles_.empty()) {
        std::string childId = child->GetComponentId();
        auto mappingIt = childToSlotMapping_.find(childId);
        if (mappingIt != childToSlotMapping_.end()) {
            std::string slotKey = mappingIt->second;
            auto slotIt = childSlotHandles_.find(slotKey);
            if (slotIt != childSlotHandles_.end() && slotIt->second != nullptr) {
                ArkUIOHApiAdapter::NodeContentInsertNode(
                    slotIt->second, child->GetNativeView(), static_cast<int32_t>(targetIndex));
                LOG_A2UI(LOG_INFO,
                    "CustomComponent::OnMoveChild: Moved child=%{public}s to slot=%{public}s, type=%{public}s, "
                    "index=%{public}zu",
                    childId.c_str(), slotKey.c_str(), descriptor_.type.c_str(), targetIndex);
                return;
            }
        }
    }

    // Fallback to single slot
    if (childSlotHandle_ != nullptr) {
        ArkUIOHApiAdapter::NodeContentInsertNode(
            childSlotHandle_, child->GetNativeView(), static_cast<int32_t>(targetIndex));
        LOG_A2UI(LOG_INFO,
            "CustomComponent::OnMoveChild: Moved child to single slot, type=%{public}s, index=%{public}zu",
            descriptor_.type.c_str(), targetIndex);
    } else {
        LOG_A2UI(
            LOG_WARN, "CustomComponent::OnMoveChild: No slot available, type=%{public}s", descriptor_.type.c_str());
    }
}

void CustomComponent::DetachChildFromSlots(const std::shared_ptr<Component>& child)
{
    ArkUI_NodeHandle nativeView = child->GetNativeView();
    if (!childSlotHandles_.empty()) {
        for (const auto& slotEntry : childSlotHandles_) {
            if (slotEntry.second != nullptr) {
                ArkUIOHApiAdapter::NodeContentRemoveNode(slotEntry.second, nativeView);
            }
        }
        return;
    }
    if (childSlotHandle_ != nullptr) {
        ArkUIOHApiAdapter::NodeContentRemoveNode(childSlotHandle_, nativeView);
    }
}

void CustomComponent::CacheRawDynamicProperty(const std::string& key, const JsonValue& child)
{
    JsonValue rawValue;
    if (CloneJsonValue(child, rawValue)) {
        rawDynamicProperties_[key] = rawValue;
    } else {
        rawDynamicProperties_.erase(key);
    }
}

void CustomComponent::RemoveAllChildren()
{
    for (const auto& child : GetChildren()) {
        if (child == nullptr || child->GetNativeView() == nullptr) {
            continue;
        }
        DetachChildFromSlots(child);
    }

    Component::RemoveAllChildren();
}

void CustomComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    if (childSlotHandle_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    ArkUIOHApiAdapter::NodeContentRemoveNode(childSlotHandle_, child->GetNativeView());
}

void CustomComponent::ApplyCommonAttributes(const JsonValue& descriptor)
{
    if (!descriptor.IsObject() || !descriptor.Has("weight")) {
        RemoveProperty("weight");
    }
    Component::ApplyCommonAttributes(descriptor);

    CommonStyleProps properties;

    // Validate and extract width/height with strict schema checking.
    // Uses ParseDimension which accepts:
    //   - number:  [0, +∞), unit defaults to vp; negative / non-finite is rejected
    //   - string:  "Nvp" | "Nfp" | "N%" | "matchParent" | "wrapContent" | "fixAtIdealSize"
    //   - other types: TYPE_MISMATCH warning
    auto reportWarning = [this](const std::string& code, const std::string& message, const std::string& propertyPath) {
        ReportCustomSchemaWarning(code, message, propertyPath);
    };

    if (ParseCommonDimensionField(descriptor, "width", properties.width, reportWarning)) {
        properties.hasWidth = true;
    }
    if (ParseCommonDimensionField(descriptor, "height", properties.height, reportWarning)) {
        properties.hasHeight = true;
    }

    // Build the size JSON string from validated width/height for ArkUI consumption.
    if (properties.hasWidth || properties.hasHeight) {
        std::ostringstream builder;
        builder << "{\"width\":" << properties.width << ",\"height\":" << properties.height << "}";
        properties.size = builder.str();
    }

    properties.padding = BuildEdgeStyleInfo(descriptor, "paddingTop", "paddingRight", "paddingBottom", "paddingLeft");
    properties.margin = BuildEdgeStyleInfo(descriptor, "marginTop", "marginRight", "marginBottom", "marginLeft");
    properties.hasMargin = !properties.margin.empty();

    descriptor_.properties = properties;

    const auto& props = properties_;
    auto weightIt = props.find("weight");
    if (weightIt != props.end() && weightIt->second.IsValid()) {
        descriptor_.properties.weight = weightIt->second.GetNumberValue(0.0);
        descriptor_.properties.hasWeight = true;
        LOG_A2UI(LOG_INFO, "CustomComponent::ApplyCommonAttributes: weight=%{public}f, type=%{public}s",
            descriptor_.properties.weight, descriptor_.type.c_str());
    }
    auto labelIt = props.find("accessibility.label");
    if (labelIt != props.end() && labelIt->second.IsValid()) {
        descriptor_.properties.accessibilityLabel = labelIt->second.ToString("");
        descriptor_.properties.hasAccessibilityLabel = true;
    }
    auto descriptionIt = props.find("accessibility.description");
    if (descriptionIt != props.end() && descriptionIt->second.IsValid()) {
        descriptor_.properties.accessibilityDescription = descriptionIt->second.ToString("");
        descriptor_.properties.hasAccessibilityDescription = true;
    }
}

void CustomComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    descriptor_.id = componentId_;
    descriptor_.surfaceId = surfaceId_;
    if (IsExtendedTabsType()) {
        CollectChildListDescriptor(descriptor);
    }
    ParseChecks(descriptor);
    ApplyCustomProperties(descriptor);
    ParseListeners(descriptor);

    // Refresh tabs data bindings before rebuilding customProps so stale tabs[i].title
    // values do not override newly provided static titles during updateComponents.
    RegisterDataBindings(descriptor);

    // Parse slot mapping before serializing customProps so generated children/tabs
    // can be reflected in the payload passed to ArkTS.
    ParseTabsMapping(descriptor);

    // Always rebuild serialized customProps after the descriptor/property map is refreshed.
    descriptor_.customProps = BuildCustomProps();

    if (hasCreatedCustomComponent_) {
        UpdateCustomComponent();
        return;
    }
    CreateCustomComponent();
}

void CustomComponent::ParseChecks(const JsonValue& descriptor)
{
    if (checksEngine_ == nullptr) {
        return;
    }
    checksEngine_->ParseChecks(descriptor);
}

bool CustomComponent::ValidateChecks(const std::string& targetJsonLiteral, std::string* failedMessage) const
{
    if (checksEngine_ == nullptr) {
        return true;
    }

    currentCheckTargetValue_ = ParseCheckTargetValue(targetJsonLiteral);
    bool result = checksEngine_->Validate(failedMessage);
    currentCheckTargetValue_ = JsonValue();
    return result;
}

JsonValue CustomComponent::ParseCheckTargetValue(const std::string& targetJsonLiteral) const
{
    if (targetJsonLiteral.empty()) {
        return JsonValue();
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(targetJsonLiteral);
    if (adapter == nullptr) {
        return JsonValue();
    }

    JsonValue targetRoot = adapter->GetRoot();
    if (!targetRoot.IsArray() || targetRoot.GetArraySize() != 1) {
        return targetRoot;
    }
    JsonValue firstItem = targetRoot.GetArrayItem(0);
    if (!firstItem.IsString()) {
        return targetRoot;
    }
    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateString(firstItem.GetStringValue(""));
    return valueAdapter == nullptr ? targetRoot : valueAdapter->GetRoot();
}

void CustomComponent::SetMargin(float top, float right, float bottom, float left)
{
    std::ostringstream builder;
    builder << "{\"top\":" << top << ",\"right\":" << right << ",\"bottom\":" << bottom << ",\"left\":" << left << "}";
    descriptor_.properties.margin = builder.str();
    descriptor_.properties.hasMargin = true;

    LOG_A2UI(LOG_INFO,
        "CustomComponent::SetMargin: top=%{public}f, right=%{public}f, bottom=%{public}f, left=%{public}f, "
        "type=%{public}s",
        top, right, bottom, left, descriptor_.type.c_str());

    if (hasCreatedCustomComponent_) {
        UpdateCustomComponent();
    }
}

void CustomComponent::OnConfigChange(const ThemeContext& context)
{
    if (hasCreatedCustomComponent_) {
        UpdateCustomComponent();
    }
}

void CustomComponent::OnAttachToParent()
{
    SyncFlexShrinkParentDefaultProperties();
    bool needsParentDefault = flexShrinkStyleState_ == FlexShrinkStyleState::DYNAMIC_VALUE ||
                              flexShrinkStyleState_ == FlexShrinkStyleState::PARENT_DEFAULT;
    if (hasCreatedCustomComponent_ && needsParentDefault) {
        UpdateCustomComponent();
    }
}

void CustomComponent::SyncFlexShrinkParentDefaultProperties()
{
    CommonStyleProps& properties = descriptor_.properties;
    properties.hasFlexShrinkParentDefault = false;
    properties.flexShrinkParentDefault = 0.0;
    properties.resetFlexShrinkToParentDefault = flexShrinkStyleState_ == FlexShrinkStyleState::PARENT_DEFAULT;
    if (flexShrinkStyleState_ != FlexShrinkStyleState::DYNAMIC_VALUE &&
        flexShrinkStyleState_ != FlexShrinkStyleState::PARENT_DEFAULT) {
        return;
    }

    std::shared_ptr<Component> parent = GetParent();
    std::string parentType = parent == nullptr ? "" : GetShortType(parent->GetType());
    if (parentType == "Column" || parentType == "Row") {
        properties.flexShrinkParentDefault = 0.0;
        properties.hasFlexShrinkParentDefault = true;
    } else if (parentType == "Flex") {
        properties.flexShrinkParentDefault = 1.0;
        properties.hasFlexShrinkParentDefault = true;
    }
}

void CustomComponent::OnPropertyApplied(const std::string& propertyName, const JsonValue& value)
{
    JsonValue storedValue;
    if (!value.IsValid() || !CloneJsonValue(value, storedValue)) {
        properties_.erase(propertyName);
        return;
    }
    properties_[propertyName] = storedValue;
}

std::optional<JsonValue> CustomComponent::GetProperty(const std::string& key) const
{
    auto it = properties_.find(key);
    if (it == properties_.end() || !it->second.IsValid()) {
        return std::nullopt;
    }
    return it->second;
}

void CustomComponent::ReportCustomSchemaWarning(
    const std::string& code, const std::string& message, const std::string& propertyPath) const
{
    if (renderId_ < 0 || code.empty() || propertyPath.empty()) {
        return;
    }

    WarningDispatchBridge::GetInstance().Dispatch(renderId_, surfaceId_, componentId_, code, message,
        ResolveCustomPropertyWarningPath(componentId_, propertyPath), "component",
        descriptor_.type.empty() ? "component" : descriptor_.type);
}

void CustomComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    if (!descriptor.IsObject()) {
        return;
    }

    JsonValue accessibilityValue = descriptor.GetItem("accessibility");
    if (accessibilityValue.IsObject()) {
        for (JsonValue child = accessibilityValue.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (key.empty() || IsKnownAccessibilityField(key)) {
                continue;
            }

            std::string propertyPath = "accessibility." + key;
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
                "Property " + propertyPath + " is undefined in native local schema and has been ignored", propertyPath);
        }
    }

    if (IsExtendedTabsType()) {
        for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
            ReportCustomSchemaWarning(issue.code, issue.message, issue.propertyPath);
        }
        return;
    }

    if (IsTabContentComponentType(descriptor_.type)) {
        for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
            ReportCustomSchemaWarning(issue.code, issue.message, issue.propertyPath);
        }

        if (!descriptor.Has("child")) {
            return;
        }

        const RequiredStringPropertyState childState = GetRequiredStringPropertyState(descriptor, "child");
        if (childState == RequiredStringPropertyState::TYPE_MISMATCH) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property child expects string child id, field has been ignored", "child");
        } else if (childState == RequiredStringPropertyState::EMPTY) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property child is required", "child");
        }
        return;
    }

    if (IsWebComponentType(descriptor_.type) && !descriptor.Has("url")) {
        ReportCustomSchemaWarning(
            SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property url is required, fallback to default value", "url");
    }
}

void CustomComponent::NormalizeCustomProperty(const std::string& propertyName, JsonValue& value)
{
    if (!value.IsValid()) {
        return;
    }

    if (propertyName == "styles" &&
        (IsExtendedTabsType() || IsTabContentComponentType(descriptor_.type) || IsWebComponentType(descriptor_.type) ||
            GetShortType(descriptor_.type) == "Row")) {
        NormalizeExtendedCommonStyles(value);
        if (!value.IsValid()) {
            return;
        }
    }

    if (IsExtendedTabsType()) {
        NormalizeExtendedTabsProperty(propertyName, value);
        if (!value.IsValid()) {
            return;
        }
    }

    if (IsTabContentComponentType(descriptor_.type)) {
        NormalizeExtendedTabContentProperty(propertyName, value);
        if (!value.IsValid()) {
            return;
        }
        if (propertyName == "styles") {
            return;
        }
    }

    if (IsWebComponentType(descriptor_.type) && propertyName == "url" && !value.IsString()) {
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property url expects string value, got type '" + std::string(value.GetTypeName()) +
                "', fallback/reset has been applied",
            "url");
        value = JsonValue();
    }
}

void CustomComponent::RegisterDataBindings(const JsonValue& descriptor)
{
    if (descriptor_.type == CHOICE_PICKER_TYPE) {
        std::string valuePath = ResolveChoicePickerValueBindingPath(descriptor);
        RemoveBindingsForProperty(VALUE_KEY);
        if (!valuePath.empty()) {
            AddBinding(VALUE_KEY, valuePath);
            LOG_A2UI(LOG_INFO,
                "CustomComponent::RegisterDataBindings: registered ChoicePicker value binding -> %{public}s",
                valuePath.c_str());
        }
        return;
    }

    if (descriptor_.type != "Tabs") {
        return;
    }

    auto bindings = GetDataBindings();
    for (const auto& binding : bindings) {
        if (binding.propertyName_.find("tabs[") == 0) {
            RemoveBindingsForProperty(binding.propertyName_);
            RemoveProperty(binding.propertyName_);
            LOG_A2UI(LOG_INFO, "RegisterDataBindings: removed old binding '%{public}s'", binding.propertyName_.c_str());
        }
    }

    JsonValue tabsJson = descriptor.GetItem("tabs");
    if (!tabsJson.IsValid() || !tabsJson.IsArray()) {
        return;
    }

    for (int i = 0; i < tabsJson.GetArraySize(); i++) {
        JsonValue tabJson = tabsJson.GetArrayItem(i);
        if (!tabJson.IsValid() || !tabJson.IsObject()) {
            continue;
        }

        JsonValue titleJson = tabJson.GetItem("title");
        if (!titleJson.IsValid()) {
            continue;
        }

        // 濡傛灉 title 鏄暟鎹粦瀹氭牸寮?{"path": "/xxx"}
        if (titleJson.IsObject()) {
            JsonValue pathJson = titleJson.GetItem("path");
            std::string path = pathJson.IsString() ? pathJson.GetStringValue("") : "";
            if (pathJson.IsValid() && !path.empty()) {
                std::string bindingKey = "tabs[" + std::to_string(i) + "].title";
                AddBinding(bindingKey, path);
                LOG_A2UI(LOG_INFO, "CustomComponent::RegisterDataBindings: registered binding %{public}s -> %{public}s",
                    bindingKey.c_str(), path.c_str());
            }
        }
    }
}

bool CustomComponent::CreateCustomComponent()
{
    NapiResourceManager* resourceManager = GetNapiResourceManager();
    if (resourceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "CreateCustomComponent: NapiResourceManager not available, type=%{public}s",
            descriptor_.type.c_str());
        return false;
    }

    env_ = resourceManager->GetNapiEnv();
    napi_ref createCustomComponentRef = resourceManager->GetCreateCustomComponentRef();
    if (env_ == nullptr || createCustomComponentRef == nullptr) {
        LOG_A2UI(LOG_ERROR, "CreateCustomComponent: custom component callback not registered, type=%{public}s",
            descriptor_.type.c_str());
        return false;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env_, &scope);

    napi_value result = nullptr;
    if (!InvokeCreateFunction(createCustomComponentRef, result)) {
        napi_close_handle_scope(env_, scope);
        return false;
    }

    ArkUI_NodeHandle handle = nullptr;
    A2UINodeContentHandle childSlotHandle = nullptr;
    if (!ExtractContentAndCreateRefs(result, handle, childSlotHandle)) {
        napi_close_handle_scope(env_, scope);
        return false;
    }

    customContentHandle_ = handle;
    nativeView_ = handle;
    childSlotHandle_ = childSlotHandle;
    hasCreatedCustomComponent_ = true;
    napi_close_handle_scope(env_, scope);

    LOG_A2UI(LOG_INFO, "CreateCustomComponent success, type=%{public}s, id=%{public}s, content=%{public}p",
        descriptor_.type.c_str(), descriptor_.id.c_str(), handle);
    return true;
}

bool CustomComponent::InvokeCreateFunction(napi_ref createCustomComponentRef, napi_value& result)
{
    napi_value attributeValue = CreateAttributeValue();
    napi_value createFunction = nullptr;
    napi_status status = napi_get_reference_value(env_, createCustomComponentRef, &createFunction);
    if (status != napi_ok || createFunction == nullptr) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: get create function failed, type=%{public}s", descriptor_.type.c_str());
        return false;
    }

    napi_value argv[] = { attributeValue };
    status = napi_call_function(env_, nullptr, createFunction, 1, argv, &result);
    if (status != napi_ok || result == nullptr) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: call create function failed, type=%{public}s", descriptor_.type.c_str());
        return false;
    }

    if (!HasNamedProperty(env_, result, CONTENT_KEY)) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: missing content property, type=%{public}s", descriptor_.type.c_str());
        return false;
    }
    return true;
}

bool CustomComponent::ExtractContentAndCreateRefs(
    napi_value result, ArkUI_NodeHandle& handle, A2UINodeContentHandle& childSlotHandle)
{
    napi_value componentContent = nullptr;
    napi_get_named_property(env_, result, CONTENT_KEY, &componentContent);

    handle = nullptr;
    int32_t handleResult = ArkUIOHApiAdapter::GetNodeHandleFromNapiValue(env_, componentContent, &handle);
    if (handleResult != A2UI_ERROR_CODE_NO_ERROR || handle == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "CreateCustomComponent: convert content to native node failed, code=%{public}d, "
            "type=%{public}s",
            handleResult, descriptor_.type.c_str());
        return false;
    }

    childSlotHandle = nullptr;
    napi_value childSlot = nullptr;
    if (HasNamedProperty(env_, result, CHILD_SLOT_KEY)) {
        napi_get_named_property(env_, result, CHILD_SLOT_KEY, &childSlot);
        napi_valuetype childSlotType = napi_undefined;
        napi_typeof(env_, childSlot, &childSlotType);
        if (childSlotType != napi_undefined && childSlotType != napi_null) {
            ArkUIOHApiAdapter::GetNodeContentFromNapiValue(env_, childSlot, &childSlotHandle);
        }
    }

    // Handle multiple slots for components like Tabs
    napi_value childSlots = nullptr;
    if (HasNamedProperty(env_, result, CHILD_SLOTS_OBJECT_KEY)) {
        napi_get_named_property(env_, result, CHILD_SLOTS_OBJECT_KEY, &childSlots);
    } else if (HasNamedProperty(env_, result, CHILD_SLOTS_KEY)) {
        napi_get_named_property(env_, result, CHILD_SLOTS_KEY, &childSlots);
    }
    SyncChildSlots(childSlots);

    napi_create_reference(env_, componentContent, 1, &componentContentRef_);
    if (childSlot != nullptr) {
        napi_valuetype childSlotType = napi_undefined;
        napi_typeof(env_, childSlot, &childSlotType);
        if (childSlotType != napi_undefined && childSlotType != napi_null) {
            napi_create_reference(env_, childSlot, 1, &childSlotRef_);
        }
    }
    return true;
}

void CustomComponent::UpdateCustomComponent()
{
    NapiResourceManager* resourceManager = GetNapiResourceManager();
    if (resourceManager == nullptr) {
        LOG_A2UI(LOG_ERROR, "UpdateCustomComponent: NapiResourceManager not available, type=%{public}s",
            descriptor_.type.c_str());
        return;
    }

    env_ = resourceManager->GetNapiEnv();
    napi_ref updateCustomComponentRef = resourceManager->GetUpdateCustomComponentRef();
    if (env_ == nullptr || updateCustomComponentRef == nullptr || componentContentRef_ == nullptr) {
        return;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env_, &scope);

    napi_value updateFunction = nullptr;
    napi_value componentContent = nullptr;
    napi_value childSlot = nullptr;
    napi_value childSlots = nullptr;
    napi_value attributeValue = CreateAttributeValue();
    napi_value result = nullptr;

    napi_get_reference_value(env_, updateCustomComponentRef, &updateFunction);
    napi_get_reference_value(env_, componentContentRef_, &componentContent);
    ResolveChildSlotValues(childSlot, childSlots);

    if (updateFunction == nullptr || componentContent == nullptr) {
        napi_close_handle_scope(env_, scope);
        return;
    }

    napi_value argv[] = { componentContent, childSlot, childSlots, attributeValue };
    napi_status status = napi_call_function(env_, nullptr, updateFunction, 4, argv, &result);
    if (status == napi_ok && result != nullptr) {
        SyncUpdatedChildSlots(result);
    }
    napi_close_handle_scope(env_, scope);
}

void CustomComponent::SyncUpdatedChildSlots(napi_value result)
{
    napi_valuetype resultType = napi_undefined;
    napi_typeof(env_, result, &resultType);
    if (resultType != napi_object) {
        return;
    }
    napi_value updatedChildSlots = nullptr;
    if (HasNamedProperty(env_, result, CHILD_SLOTS_OBJECT_KEY)) {
        napi_get_named_property(env_, result, CHILD_SLOTS_OBJECT_KEY, &updatedChildSlots);
    } else if (HasNamedProperty(env_, result, CHILD_SLOTS_KEY)) {
        napi_get_named_property(env_, result, CHILD_SLOTS_KEY, &updatedChildSlots);
    }
    SyncChildSlots(updatedChildSlots);
}

void CustomComponent::ResolveChildSlotValues(napi_value& childSlot, napi_value& childSlots)
{
    if (childSlotRef_ != nullptr) {
        napi_get_reference_value(env_, childSlotRef_, &childSlot);
    } else {
        napi_get_undefined(env_, &childSlot);
    }

    // Create childSlots object for multi-slot components
    if (!childSlotRefs_.empty()) {
        napi_create_object(env_, &childSlots);
        for (const auto& pair : childSlotRefs_) {
            napi_value slotValue = nullptr;
            napi_get_reference_value(env_, pair.second, &slotValue);
            if (slotValue != nullptr) {
                napi_set_named_property(env_, childSlots, pair.first.c_str(), slotValue);
            }
        }
    } else {
        napi_get_undefined(env_, &childSlots);
    }
}

void CustomComponent::ApplyCustomProperties(const JsonValue& descriptor)
{
    std::set<std::string> currentCustomPropertyNames;
    if (!descriptor.IsObject()) {
        for (const std::string& propertyName : customPropertyNames_) {
            RemoveProperty(propertyName);
        }
        customPropertyNames_.clear();
        return;
    }

    for (JsonValue child = descriptor.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty() || IsReservedDescriptorKey(key)) {
            continue;
        }
        currentCustomPropertyNames.insert(key);
        RemoveBindingsForProperty(BuildCustomExpressionBindingKey(key));

        if (ApplyDynamicCustomProperty(key, child, descriptor)) {
            continue;
        }

        rawDynamicProperties_.erase(key);

        RemoveBindingsForProperty(key);

        JsonValue storedValue;
        if (!CloneJsonValue(child, storedValue)) {
            RemoveProperty(key);
            continue;
        }
        NormalizeCustomProperty(key, storedValue);
        if (!storedValue.IsValid()) {
            RemoveProperty(key);
            continue;
        }

        OnPropertyApplied(key, storedValue);
        RefreshCustomExpressionBindings(*this, key, storedValue);
    }

    for (const std::string& propertyName : customPropertyNames_) {
        if (currentCustomPropertyNames.find(propertyName) == currentCustomPropertyNames.end()) {
            RemoveProperty(propertyName);
        }
    }
    customPropertyNames_ = currentCustomPropertyNames;
}

bool CustomComponent::ApplyDynamicCustomProperty(
    const std::string& key, const JsonValue& child, const JsonValue& descriptor)
{
    CustomDynamicDescriptorKind descriptorKind = ResolveCustomDynamicDescriptorKind(child);
    if (descriptorKind == CustomDynamicDescriptorKind::NONE) {
        return false;
    }
    if (preserveDynamicDescriptors_) {
        CacheRawDynamicProperty(key, child);
    }
    JsonValue pathValue = child.GetItem("path");
    bool shouldReportCompatibilityWarning = false;
    std::string bindingPath = pathValue.IsString() ? pathValue.GetStringValue("") : "";
    if (descriptorKind == CustomDynamicDescriptorKind::PATH && !bindingPath.empty() && IsValidDataPath(bindingPath) &&
        ShouldReportCustomDynamicDescriptorCompatibilityWarning(descriptor_.type, key)) {
        shouldReportCompatibilityWarning = true;
    } else if (descriptorKind == CustomDynamicDescriptorKind::CALL &&
               ShouldReportCustomDynamicDescriptorCompatibilityWarning(descriptor_.type, key)) {
        shouldReportCompatibilityWarning = true;
    }

    if (shouldReportCompatibilityWarning) {
        DispatchCustomDynamicDescriptorCompatibilityWarning(
            renderId_, surfaceId_, componentId_, descriptor_.type, key, descriptorKind);
    }

    SetPropertyFromDescriptor(key, descriptor);
    return true;
}

bool CustomComponent::IsExtendedEtsExpressionScope() const
{
    if (preserveDynamicDescriptors_) {
        return false;
    }
    if (IsTabContentComponentType(descriptor_.type) && !GetLocalVariables().empty()) {
        return true;
    }
    // Only ETS-implemented Extended components (registered in A2UIExtendedComponents.ets)
    // and template TabContent instances are in scope. Base A2UI ETS custom components
    // without template-local variables must keep literal behavior.
    static const std::set<std::string> scopedTypes = { "Select", "Web" };
    return scopedTypes.find(descriptor_.type) != scopedTypes.end();
}

void CustomComponent::DisposeComponentContent()
{
    if (env_ == nullptr || componentContentRef_ == nullptr) {
        return;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env_, &scope);

    napi_value componentContent = nullptr;
    napi_value disposeFunction = nullptr;
    napi_status status = napi_get_reference_value(env_, componentContentRef_, &componentContent);
    if (status == napi_ok && componentContent != nullptr && HasNamedProperty(env_, componentContent, DISPOSE_KEY)) {
        napi_get_named_property(env_, componentContent, DISPOSE_KEY, &disposeFunction);
        if (disposeFunction != nullptr) {
            napi_call_function(env_, componentContent, disposeFunction, 0, nullptr, nullptr);
        }
    }

    napi_close_handle_scope(env_, scope);

    // The nativeView_ was obtained from the ComponentContent node which has been
    // disposed above via the ArkTS dispose() call. Clear the stale pointer to
    // prevent use-after-free. Since ownsNativeView_ is false, the base class
    // destructor will not attempt a second disposeNode.
    nativeView_ = nullptr;
}

void CustomComponent::ResetReferences()
{
    if (env_ != nullptr && componentContentRef_ != nullptr) {
        napi_delete_reference(env_, componentContentRef_);
    }
    if (env_ != nullptr && childSlotRef_ != nullptr) {
        napi_delete_reference(env_, childSlotRef_);
    }

    // Clean up multi-slot references
    for (auto& pair : childSlotRefs_) {
        if (env_ != nullptr && pair.second != nullptr) {
            napi_delete_reference(env_, pair.second);
        }
    }
    childSlotRefs_.clear();
    childSlotHandles_.clear();
    childToSlotMapping_.clear();
    tabChildIds_.clear();
    rowChildIds_.clear();
    componentContentRef_ = nullptr;
    childSlotRef_ = nullptr;
}

bool CustomComponent::IsTabsType() const
{
    return descriptor_.type == TABS_COMPONENT_TYPE;
}

bool CustomComponent::IsExtendedTabsType() const
{
    return IsTabsType() && IsExtendedProtocolSurface();
}

bool CustomComponent::IsExtendedProtocolSurface() const
{
    return ToLowerCopy(GetSurfaceContext().catalogId) == ToLowerCopy(A2UI_EXTENDED_CATALOG_ID);
}

bool CustomComponent::IsRowType() const
{
    return GetShortType(descriptor_.type) == "Row";
}

std::string CustomComponent::GetShortType(const std::string& type) const
{
    return GetShortComponentType(type);
}

std::list<std::string> CustomComponent::ResolveTemplateChildIds(
    const std::string& templateComponentId, const std::string& templatePath) const
{
    std::list<std::string> childIds;
    if (templateComponentId.empty() || templatePath.empty()) {
        return childIds;
    }

    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }

    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr) {
        return childIds;
    }

    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        return childIds;
    }

    std::shared_ptr<DataModel> dataModel = bindingEngine->GetOrCreateDataModel(surfaceId);
    if (dataModel == nullptr) {
        return childIds;
    }

    std::optional<JsonValue> arrayValueOpt = dataModel->GetNode(templatePath);
    if (!arrayValueOpt.has_value()) {
        DynamicResolveContext context = { .renderId = GetRenderId(),
            .surfaceId = GetSurfaceId(),
            .componentId = GetComponentId(),
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        DynamicValueResolver::ReportMissingPath(context, templatePath);
        return childIds;
    }
    if (!arrayValueOpt.value().IsArray()) {
        return childIds;
    }

    JsonValue arrayValue = arrayValueOpt.value();
    int itemCount = arrayValue.GetArraySize();
    for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
        childIds.push_back(BuildTemplateInstanceRootId(templateComponentId, templatePath, itemIndex));
    }
    return childIds;
}

std::list<std::string> CustomComponent::ResolveTabsChildIds(const JsonValue& descriptor) const
{
    if (IsExtendedTabsType()) {
        return ResolveExtendedTabsChildIds(
            descriptor, [this](const std::string& templateComponentId, const std::string& templatePath) {
                return ResolveTemplateChildIds(templateComponentId, templatePath);
            });
    }

    std::list<std::string> childIds;
    JsonValue tabsValue = descriptor.GetItem("tabs");
    if (tabsValue.IsValid() && tabsValue.IsArray()) {
        for (int i = 0; i < tabsValue.GetArraySize(); ++i) {
            JsonValue tabItem = tabsValue.GetArrayItem(i);
            if (!tabItem.IsObject()) {
                continue;
            }
            std::string childId = tabItem.GetString("child", "");
            if (!childId.empty()) {
                childIds.push_back(childId);
            }
        }
        if (!childIds.empty()) {
            return childIds;
        }
    }

    ChildListDescriptor childList = ChildListParser::ParseChildren(descriptor.GetItem("children"));
    if (childList.type == ChildListType::STATIC_IDS) {
        return childList.staticChildIds;
    }
    if (childList.type == ChildListType::TEMPLATE_PATH) {
        return ResolveTemplateChildIds(childList.templateComponentId, childList.templatePath);
    }

    return childIds;
}

std::list<std::string> CustomComponent::ResolveRowChildIds(const JsonValue& descriptor) const
{
    ChildListDescriptor childList = ChildListParser::ParseChildren(descriptor.GetItem("children"));
    if (childList.type == ChildListType::STATIC_IDS) {
        return childList.staticChildIds;
    }
    if (childList.type == ChildListType::TEMPLATE_PATH) {
        return ResolveTemplateChildIds(childList.templateComponentId, childList.templatePath);
    }
    return {};
}

void CustomComponent::MergeTabsFromChildIds(JsonValue& tabsArray) const
{
    if (IsExtendedTabsType()) {
        return;
    }
    if (!IsTabsType() || !tabsArray.IsArray() || tabsArray.GetArraySize() > 0 || tabChildIds_.empty()) {
        return;
    }

    for (const auto& childId : tabChildIds_) {
        if (childId.empty()) {
            continue;
        }
        std::unique_ptr<JsonAdapter> tabObjectAdapter = JsonAdapter::CreateObject();
        if (tabObjectAdapter == nullptr) {
            continue;
        }
        JsonValue tabObject = tabObjectAdapter->GetRoot();
        tabObject.PutString("child", childId);
        tabsArray.Append(tabObject);
    }
}

void CustomComponent::MergeRowChildren(JsonValue& childrenArray) const
{
    if (!IsRowType() || !childrenArray.IsArray() || childrenArray.GetArraySize() > 0 || rowChildIds_.empty()) {
        return;
    }

    for (const auto& childId : rowChildIds_) {
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

void CustomComponent::MergeTabsChildren(JsonValue& childrenArray) const
{
    if (!IsExtendedTabsType()) {
        return;
    }
    MergeExtendedTabsChildIds(tabChildIds_, childrenArray);
}

void CustomComponent::ParseTabsMapping(const JsonValue& descriptor)
{
    childToSlotMapping_.clear();

    tabChildIds_.clear();
    rowChildIds_.clear();

    std::list<std::string> childIds;
    std::string slotPrefix;
    if (IsTabsType() || IsExtendedTabsType()) {
        childIds = ResolveTabsChildIds(descriptor);
        slotPrefix = "tab-";
    } else if (IsRowType()) {
        childIds = ResolveRowChildIds(descriptor);
        slotPrefix = "row-";
    } else {
        return;
    }

    size_t slotIndex = 0;
    for (const auto& childId : childIds) {
        if (childId.empty()) {
            continue;
        }
        std::string slotKey = slotPrefix + std::to_string(slotIndex++);
        childToSlotMapping_[childId] = slotKey;
        if (IsRowType()) {
            rowChildIds_.push_back(childId);
        } else {
            tabChildIds_.push_back(childId);
        }
        LOG_A2UI(
            LOG_INFO, "ParseTabsMapping: Mapped child=%{public}s to slot=%{public}s", childId.c_str(), slotKey.c_str());
    }
}

void CustomComponent::SyncChildSlots(napi_value childSlots)
{
    if (env_ == nullptr || childSlots == nullptr) {
        return;
    }
    auto& napi = NapiBridge::GetInstance().Provider();

    napi_valuetype childSlotsType = napi_undefined;
    napi_typeof(env_, childSlots, &childSlotsType);
    if (childSlotsType != napi_object) {
        return;
    }

    napi_value keys = nullptr;
    napi_get_property_names(env_, childSlots, &keys);
    if (keys == nullptr) {
        return;
    }

    uint32_t keysLength = 0;
    napi_get_array_length(env_, keys, &keysLength);
    for (uint32_t i = 0; i < keysLength; ++i) {
        napi_value key = nullptr;
        napi_get_element(env_, keys, i, &key);
        SyncChildSlotEntry(childSlots, key);
    }
}

void CustomComponent::SyncChildSlotEntry(napi_value childSlots, napi_value key)
{
    auto& napi = NapiBridge::GetInstance().Provider();

    size_t keySize = 0;
    if (napi.GetValueStringUtf8(env_, key, nullptr, 0, &keySize) != napi_ok) {
        return;
    }
    std::string keyStr(keySize + 1, '\0');
    if (napi.GetValueStringUtf8(env_, key, &keyStr[0], keySize + 1, &keySize) != napi_ok) {
        return;
    }
    keyStr.resize(keySize);
    if (keyStr.empty()) {
        return;
    }

    napi_value slotValue = nullptr;
    napi_get_named_property(env_, childSlots, keyStr.c_str(), &slotValue);

    A2UINodeContentHandle slotHandle = nullptr;
    ArkUIOHApiAdapter::GetNodeContentFromNapiValue(env_, slotValue, &slotHandle);
    if (slotHandle == nullptr) {
        LOG_A2UI(LOG_ERROR, "SyncChildSlots: Failed to extract NodeContentHandle for key=%{public}s", keyStr.c_str());
        return;
    }

    childSlotHandles_[keyStr] = slotHandle;
    if (childSlotRefs_.find(keyStr) == childSlotRefs_.end()) {
        napi_ref slotRef = nullptr;
        napi_create_reference(env_, slotValue, 1, &slotRef);
        if (slotRef != nullptr) {
            childSlotRefs_[keyStr] = slotRef;
        }
    }
}

void CustomComponent::ParseListeners(const JsonValue& descriptor)
{
    eventHandlers_.clear();
    if (!descriptor.IsObject()) {
        return;
    }
    eventHandlers_ = EventHandlerParser::Parse(descriptor);
    if (!eventHandlers_.empty()) {
        LOG_A2UI(LOG_INFO, "CustomComponent::ParseListeners: parsed %{public}zu event handlers, type=%{public}s",
            eventHandlers_.size(), descriptor_.type.c_str());
    }
}

void CustomComponent::DispatchEvent(const std::string& listenerName, const JsonValue& extraContext)
{
    DispatchEventToHandlers({ eventHandlers_, listenerName, GetSurfaceId(), GetComponentId(), GetRenderId(),
        extraContext, &GetLocalVariables() });
}

void CustomComponent::ClearDynamicValueCallback(const std::string& propertyName)
{
    auto callbackIterator = dynamicValueCallbacks_.find(propertyName);
    if (callbackIterator != dynamicValueCallbacks_.end()) {
        DynamicValueCallbackInfo& callbackInfo = callbackIterator->second;
        if (callbackInfo.env != nullptr && callbackInfo.callbackRef != nullptr) {
            NapiBridge::GetInstance().Provider().DeleteReference(callbackInfo.env, callbackInfo.callbackRef);
        }
        dynamicValueCallbacks_.erase(callbackIterator);
    }

    if (dynamicResolverBindingKeys_.erase(propertyName) > 0U) {
        RemoveBindingsForProperty(propertyName);
    }
}

void CustomComponent::ClearDynamicValueCallbacks()
{
    std::vector<std::string> propertyNames;
    propertyNames.reserve(dynamicValueCallbacks_.size());
    for (const auto& callbackPair : dynamicValueCallbacks_) {
        propertyNames.push_back(callbackPair.first);
    }
    for (const std::string& propertyName : propertyNames) {
        ClearDynamicValueCallback(propertyName);
    }
}

bool CustomComponent::DispatchDynamicValueCallback(const std::string& propertyName, const JsonValue& value)
{
    auto callbackIterator = dynamicValueCallbacks_.find(propertyName);
    if (callbackIterator == dynamicValueCallbacks_.end()) {
        return false;
    }

    const DynamicValueCallbackInfo& callbackInfo = callbackIterator->second;
    if (callbackInfo.env == nullptr || callbackInfo.callbackRef == nullptr) {
        return false;
    }

    auto& napi = NapiBridge::GetInstance().Provider();
    napi_handle_scope scope = nullptr;
    if (napi.OpenHandleScope(callbackInfo.env, &scope) != napi_ok) {
        return false;
    }

    napi_value callback = nullptr;
    napi_value global = nullptr;
    napi_value argument = JsonValueToNapiValue(callbackInfo.env, value);
    napi_value result = nullptr;
    bool success = napi.GetReferenceValue(callbackInfo.env, callbackInfo.callbackRef, &callback) == napi_ok &&
                   callback != nullptr && napi.GetGlobal(callbackInfo.env, &global) == napi_ok && global != nullptr &&
                   napi.CallFunction(callbackInfo.env, global, callback, 1, &argument, &result) == napi_ok;
    napi.CloseHandleScope(callbackInfo.env, scope);
    if (!success) {
        LOG_A2UI(LOG_WARN, "CustomComponent::DispatchDynamicValueCallback failed, property=%{public}s",
            propertyName.c_str());
    }
    return success;
}

void CustomComponent::DispatchCurrentDynamicValue(const std::string& propertyName, const std::string& path)
{
    if (propertyName.empty() || path.empty()) {
        return;
    }

    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr || surfaceSlot->GetBindingEngine() == nullptr) {
        return;
    }

    std::shared_ptr<DataModel> dataModel = surfaceSlot->GetBindingEngine()->GetOrCreateDataModel(surfaceId);
    if (dataModel == nullptr) {
        return;
    }

    std::optional<JsonValue> currentValue = dataModel->GetNode(path);
    if (!currentValue.has_value()) {
        DynamicResolveContext context = { .renderId = GetRenderId(),
            .surfaceId = GetSurfaceId(),
            .componentId = GetComponentId(),
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        DynamicValueResolver::ReportMissingPath(context, path);
        LOG_A2UI(LOG_INFO, "CustomComponent::DispatchCurrentDynamicValue path not found, property=%{public}s",
            propertyName.c_str());
        return;
    }
    DispatchDynamicValueCallback(propertyName, currentValue.value());
}

void CustomComponent::SyncDynamicValueBindings()
{
    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr || surfaceSlot->GetBindingEngine() == nullptr) {
        return;
    }

    try {
        surfaceSlot->GetBindingEngine()->SyncComponentBindings(shared_from_this());
    } catch (const std::bad_weak_ptr&) {
        LOG_A2UI(LOG_WARN, "CustomComponent::SyncDynamicValueBindings skipped because component is not shared");
    }
}

bool CustomComponent::RegisterPathDynamicValueCallback(const std::string& propertyName, const std::string& path,
    const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage)
{
    if (!IsValidDataPath(path)) {
        NapiBridge::GetInstance().Provider().DeleteReference(callbackInfo.env, callbackInfo.callbackRef);
        if (errorMessage != nullptr) {
            *errorMessage = "path is invalid";
        }
        return false;
    }

    DynamicValueCallbackInfo pathCallbackInfo = callbackInfo;
    pathCallbackInfo.dataPath = path;
    dynamicValueCallbacks_[propertyName] = pathCallbackInfo;
    dynamicResolverBindingKeys_.insert(propertyName);
    RemoveBindingsForProperty(propertyName);
    AddBinding(propertyName, path);
    DispatchCurrentDynamicValue(propertyName, path);
    MarkDescriptorDynamicBindingsResolved();
    SyncDynamicValueBindings();
    return true;
}

bool CustomComponent::RegisterFunctionCallDynamicBindings(
    const std::string& propertyName, const JsonValue& descriptor, const DynamicValueDependencies& dependencies)
{
    bool registeredBinding = false;
    for (const auto& path : dependencies.dataPaths) {
        DataBinding binding(propertyName, path, BindingType::FUNCTION_CALL, descriptor);
        binding.globalVarDeps_ = dependencies.globalVariables;
        dataBindings_.push_back(std::move(binding));
        registeredBinding = true;
    }
    if (dependencies.dataPaths.empty()) {
        DataBinding binding(propertyName, "", BindingType::FUNCTION_CALL, descriptor);
        binding.globalVarDeps_ = dependencies.globalVariables;
        dataBindings_.push_back(std::move(binding));
        registeredBinding = true;
    }
    return registeredBinding;
}

bool CustomComponent::RegisterExpressionDynamicBindings(
    const std::string& propertyName, const JsonValue& descriptor, const DynamicValueDependencies& dependencies)
{
#ifdef ENABLE_EXPRESSION_ENGINE
    std::string expression =
        descriptor.IsString() ? ExpressionEngine::ExtractExpression(descriptor.GetStringValue("")) : "";
    if (expression.empty()) {
        return false;
    }

    std::vector<std::string> expressionDependencies = dependencies.globalVariables;
    if (!dependencies.dataPaths.empty() && std::find(expressionDependencies.begin(), expressionDependencies.end(),
                                               "__dataModel") == expressionDependencies.end()) {
        expressionDependencies.emplace_back("__dataModel");
    }

    bool registeredBinding = false;
    for (const auto& path : dependencies.dataPaths) {
        DataBinding binding(propertyName, expression, expressionDependencies);
        binding.dataPath_ = path;
        dataBindings_.push_back(std::move(binding));
        registeredBinding = true;
    }
    if (dependencies.dataPaths.empty()) {
        dataBindings_.emplace_back(propertyName, expression, expressionDependencies);
        registeredBinding = true;
    }
    return registeredBinding;
#else
    (void)propertyName;
    (void)descriptor;
    (void)dependencies;
    return false;
#endif
}

bool CustomComponent::RegisterPersistentDynamicValueCallback(
    const PersistentDynamicValueRegistrationContext& registration)
{
    dynamicValueCallbacks_[registration.propertyName] = registration.callbackInfo;
    dynamicResolverBindingKeys_.insert(registration.propertyName);
    RemoveBindingsForProperty(registration.propertyName);

    bool registeredBinding = registration.resolved.source == ResolveSource::FUNCTION_CALL
                                 ? RegisterFunctionCallDynamicBindings(
                                       registration.propertyName, registration.descriptor, registration.dependencies)
                                 : RegisterExpressionDynamicBindings(
                                       registration.propertyName, registration.descriptor, registration.dependencies);
    if (!registeredBinding) {
        ClearDynamicValueCallback(registration.propertyName);
        if (registration.errorMessage != nullptr) {
            *registration.errorMessage = "failed to dispatch resolved value";
        }
        return false;
    }

    if (!DispatchDynamicValueCallback(registration.propertyName, registration.resolved.value)) {
        ClearDynamicValueCallback(registration.propertyName);
        if (registration.errorMessage != nullptr) {
            *registration.errorMessage = "failed to dispatch resolved value";
        }
        return false;
    }
    MarkDescriptorDynamicBindingsResolved();
    SyncDynamicValueBindings();
    return true;
}

bool CustomComponent::DispatchOneShotDynamicValueCallback(const std::string& propertyName,
    const ResolvedValue& resolved, const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage)
{
    dynamicValueCallbacks_[propertyName] = callbackInfo;
    bool callbackSuccess = DispatchDynamicValueCallback(propertyName, resolved.value);
    ClearDynamicValueCallback(propertyName);
    if (!callbackSuccess && errorMessage != nullptr) {
        *errorMessage = "failed to dispatch resolved value";
    }
    return callbackSuccess;
}

bool CustomComponent::ResolveAndRegisterDynamicValueCallback(const std::string& propertyName,
    const JsonValue& descriptor, const DynamicValueCallbackInfo& callbackInfo, std::string* errorMessage)
{
    DynamicResolveContext context = { .renderId = GetRenderId(),
        .surfaceId = GetSurfaceId(),
        .componentId = GetComponentId(),
        .allowExpression = true,
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE,
        .localVariables = GetLocalVariables() };
    ResolvedValue resolved = DynamicValueResolver::Resolve(descriptor, context);
    if (!resolved.success || !resolved.value.IsValid()) {
        NapiBridge::GetInstance().Provider().DeleteReference(callbackInfo.env, callbackInfo.callbackRef);
        if (errorMessage != nullptr) {
            *errorMessage = resolved.errorMessage.empty() ? "dynamic value resolve failed" : resolved.errorMessage;
        }
        return false;
    }

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor);
    bool hasPersistentDependencies = !dependencies.dataPaths.empty() || !dependencies.globalVariables.empty();
    if (hasPersistentDependencies &&
        (resolved.source == ResolveSource::FUNCTION_CALL || resolved.source == ResolveSource::EXPRESSION)) {
        PersistentDynamicValueRegistrationContext registration = { propertyName, descriptor, resolved, dependencies,
            callbackInfo, errorMessage };
        return RegisterPersistentDynamicValueCallback(registration);
    }
    return DispatchOneShotDynamicValueCallback(propertyName, resolved, callbackInfo, errorMessage);
}

bool CustomComponent::RegisterDynamicValueCallback(const std::string& propertyName, const JsonValue& descriptor,
    napi_env env, napi_value callback, std::string* errorMessage)
{
    if (propertyName.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "propertyName is empty";
        }
        return false;
    }
    if (env == nullptr || callback == nullptr || !NapiIsFunctionValue(env, callback)) {
        if (errorMessage != nullptr) {
            *errorMessage = "callback is not a function";
        }
        return false;
    }

    ClearDynamicValueCallback(propertyName);

    napi_ref callbackRef = nullptr;
    if (NapiBridge::GetInstance().Provider().CreateReference(env, callback, 1, &callbackRef) != napi_ok ||
        callbackRef == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = "failed to create callback reference";
        }
        return false;
    }

    DynamicValueCallbackInfo callbackInfo;
    callbackInfo.env = env;
    callbackInfo.callbackRef = callbackRef;

    std::string path = ResolveDynamicPath(descriptor);
    if (!path.empty()) {
        return RegisterPathDynamicValueCallback(propertyName, path, callbackInfo, errorMessage);
    }
    return ResolveAndRegisterDynamicValueCallback(propertyName, descriptor, callbackInfo, errorMessage);
}

} // namespace NativeModule
