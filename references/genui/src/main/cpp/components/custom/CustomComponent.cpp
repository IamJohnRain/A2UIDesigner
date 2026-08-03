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
#include <cmath>
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
#include "theme/ThemeManager.h"
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

constexpr const char* TYPE_KEY = "type";
constexpr const char* TABS_COMPONENT_TYPE = "Tabs";
constexpr const char* ID_KEY = "id";
constexpr const char* CUSTOM_PROPS_KEY = "customProps";
constexpr const char* PROPERTIES_KEY = "properties";
constexpr const char* DATA_MODEL_JSON_KEY = "dataModelJson";
constexpr const char* COMPONENT_THEME_KEY = "componentTheme";
constexpr const char* SIZE_KEY = "size";
constexpr const char* COLOR_KEY = "color";
constexpr const char* ACCESSIBILITY_LABEL_KEY = "label";
constexpr const char* ACCESSIBILITY_DESCRIPTION_KEY = "description";
constexpr const char* PADDING_KEY = "padding";
constexpr const char* MARGIN_KEY = "margin";
constexpr const char* WIDTH_KEY = "width";
constexpr const char* HEIGHT_KEY = "height";
constexpr const char* CONTENT_KEY = "content";
constexpr const char* CHILD_SLOT_KEY = "childSlot";
constexpr const char* CHILD_SLOTS_KEY = "childSlots";
constexpr const char* CHILD_SLOTS_OBJECT_KEY = "childSlotsObject";
constexpr const char* DISPOSE_KEY = "dispose";
constexpr const char* A2UI_BINDINGS_KEY = "__a2uiBindings";
constexpr const char* CHOICE_PICKER_TYPE = "ChoicePicker";
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

bool UsesExtendedCommonStyleValidation(const std::string& componentType)
{
    std::string shortType = GetShortComponentType(componentType);
    return IsExtendedTabsComponentType(componentType) || IsTabContentComponentType(componentType) ||
           IsWebComponentType(componentType) || shortType == "Row";
}

bool IsDynamicDescriptorObject(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool IsEmptyStringValue(const JsonValue& value)
{
    return value.IsString() && StyleApplyUtils::TrimToken(value.GetStringValue("")).empty();
}

bool ParseColorLikeValue(const JsonValue& value)
{
    if (value.IsString() && StyleApplyUtils::TrimToken(value.GetStringValue("")) == "transparent") {
        return true;
    }

    uint32_t color = 0;
    return StyleApplyUtils::ParseColor(value, color);
}

bool ParseBooleanLikeValue(const JsonValue& value)
{
    if (value.IsBool()) {
        return true;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    return token == "true" || token == "false";
}

bool ParseShadowStyleValue(const JsonValue& value)
{
    if (value.IsNumber()) {
        double numeric = value.GetNumberValue(-1.0);
        return std::isfinite(numeric) && numeric >= 0.0 && numeric <= 5.0 &&
               std::fabs(numeric - std::round(numeric)) < 0.0001;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    return token == "0" || token == "1" || token == "2" || token == "3" || token == "4" || token == "5" ||
           token == "outerDefaultXs" || token == "outerDefaultSm" || token == "outerDefaultMd" ||
           token == "outerDefaultLg" || token == "outerFloatingSm" || token == "outerFloatingMd";
}

bool ParseShadowTypeValue(const JsonValue& value)
{
    if (value.IsNumber()) {
        double numeric = value.GetNumberValue(-1.0);
        return numeric == 0.0 || numeric == 1.0;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    return token == "color" || token == "blur";
}

bool ParseGradientDirectionValue(const JsonValue& value)
{
    if (value.IsNumber()) {
        double numeric = value.GetNumberValue(-1.0);
        return std::isfinite(numeric) && numeric >= 0.0 && numeric <= 8.0 &&
               std::fabs(numeric - std::round(numeric)) < 0.0001;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return token == "left" || token == "top" || token == "right" || token == "bottom" || token == "lefttop" ||
           token == "topleft" || token == "leftbottom" || token == "bottomleft" || token == "righttop" ||
           token == "topright" || token == "rightbottom" || token == "bottomright" || token == "none";
}

bool HasAnyChildField(const JsonValue& value)
{
    return value.IsObject() && value.GetChild().IsValid();
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

void SetStringProperty(napi_env env, napi_value object, const char* key, const std::string& value)
{
    napi_value propertyValue = nullptr;
    napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &propertyValue);
    napi_set_named_property(env, object, key, propertyValue);
}

void SetDoubleProperty(napi_env env, napi_value object, const char* key, double value)
{
    napi_value propertyValue = nullptr;
    napi_create_double(env, value, &propertyValue);
    napi_set_named_property(env, object, key, propertyValue);
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

std::string BuildSizeStyleInfo(const JsonValue& descriptor)
{
    bool hasValue = descriptor.Has("width") || descriptor.Has("height");
    if (!hasValue) {
        return "";
    }

    std::ostringstream builder;
    builder << "{\"width\":" << descriptor.GetNumber("width", 0.0)
            << ",\"height\":" << descriptor.GetNumber("height", 0.0) << "}";
    return builder.str();
}

bool HasCommonStyleProps(const CommonStyleProps& properties)
{
    return properties.hasWidth || properties.hasHeight || properties.hasWeight || !properties.size.empty() ||
           !properties.padding.empty() || properties.hasMargin || properties.hasAccessibilityLabel ||
           properties.hasAccessibilityDescription;
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

CustomComponent::CustomComponent(const std::string& componentType)
    : Component(nullptr, false),
      checksEngine_(std::make_unique<ChecksEngine>(
          [this]() { return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() }; },
          [this](JsonValue& value) {
              if (!currentCheckTargetValue_.IsValid()) {
                  return false;
              }
              value = currentCheckTargetValue_;
              return true;
          }))
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

void CustomComponent::RemoveAllChildren()
{
    for (const auto& child : GetChildren()) {
        if (child == nullptr || child->GetNativeView() == nullptr) {
            continue;
        }

        if (!childSlotHandles_.empty()) {
            for (const auto& slotEntry : childSlotHandles_) {
                if (slotEntry.second != nullptr) {
                    ArkUIOHApiAdapter::NodeContentRemoveNode(slotEntry.second, child->GetNativeView());
                }
            }
            continue;
        }

        if (childSlotHandle_ != nullptr) {
            ArkUIOHApiAdapter::NodeContentRemoveNode(childSlotHandle_, child->GetNativeView());
        }
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
    properties.size = BuildSizeStyleInfo(descriptor);
    properties.padding = BuildEdgeStyleInfo(descriptor, "paddingTop", "paddingRight", "paddingBottom", "paddingLeft");
    properties.margin = BuildEdgeStyleInfo(descriptor, "marginTop", "marginRight", "marginBottom", "marginLeft");
    properties.hasMargin = !properties.margin.empty();

    if (descriptor.Has("width")) {
        properties.width = descriptor.GetNumber("width", 0.0);
        properties.hasWidth = true;
    }
    if (descriptor.Has("height")) {
        properties.height = descriptor.GetNumber("height", 0.0);
        properties.hasHeight = true;
    }

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

    currentCheckTargetValue_ = JsonValue();
    if (!targetJsonLiteral.empty()) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(targetJsonLiteral);
        if (adapter != nullptr) {
            JsonValue targetRoot = adapter->GetRoot();
            if (targetRoot.IsArray() && targetRoot.GetArraySize() == 1) {
                JsonValue firstItem = targetRoot.GetArrayItem(0);
                if (firstItem.IsString()) {
                    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateString(firstItem.GetStringValue(""));
                    if (valueAdapter != nullptr) {
                        currentCheckTargetValue_ = valueAdapter->GetRoot();
                    }
                }
            }
            if (!currentCheckTargetValue_.IsValid()) {
                currentCheckTargetValue_ = targetRoot;
            }
        }
    }

    bool result = checksEngine_->Validate(failedMessage);
    currentCheckTargetValue_ = JsonValue();
    return result;
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

    if (descriptor.Has("accessibility")) {
        JsonValue accessibilityValue = descriptor.GetItem("accessibility");
        if (accessibilityValue.IsObject()) {
            for (JsonValue child = accessibilityValue.GetChild(); child.IsValid(); child = child.GetNext()) {
                std::string key = child.GetKey();
                if (key.empty() || IsKnownAccessibilityField(key)) {
                    continue;
                }

                std::string propertyPath = "accessibility." + key;
                ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
                    "Property " + propertyPath + " is undefined in native local schema and has been ignored",
                    propertyPath);
            }
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

void CustomComponent::NormalizeExtendedCommonStyles(JsonValue& value)
{
    if (!value.IsObject()) {
        ReportCustomSchemaWarning(
            SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Property styles expects object value, field has been ignored", "styles");
        value = JsonValue();
        return;
    }

    auto reportInvalidDimensionField = [this](JsonValue& parentValue, const char* key, const std::string& path) {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }

        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            return;
        }

        StyleDimension dimension;
        if (StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
            return;
        }

        if (IsEmptyStringValue(fieldValue) || fieldValue.IsNumber()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects dimension value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    };

    auto reportInvalidDimensionStyle = [this](JsonValue& parentValue, const char* key, const std::string& path) {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }

        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            return;
        }

        StyleDimension dimension;
        if (StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
            return;
        }

        if (fieldValue.IsString() || fieldValue.IsNumber()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects dimension value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    };

    auto reportInvalidNumberField = [this](JsonValue& parentValue, const char* key, const std::string& path,
                                        bool allowNegative, bool unitInterval, bool emptyStringIsTypeMismatch) {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }

        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            return;
        }

        float parsedNumber = 0.0F;
        bool parsed = StyleApplyUtils::ParseNumber(fieldValue, parsedNumber) && std::isfinite(parsedNumber);
        bool valid = parsed;
        if (valid && !allowNegative && parsedNumber < 0.0F) {
            valid = false;
        }
        if (valid && unitInterval && (parsedNumber < 0.0F || parsedNumber > 1.0F)) {
            valid = false;
        }
        if (valid) {
            return;
        }

        if ((emptyStringIsTypeMismatch && IsEmptyStringValue(fieldValue)) ||
            (!fieldValue.IsNumber() && !fieldValue.IsString())) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects number value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        } else if (!parsed && fieldValue.IsString()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects number value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        }
        parentValue.Remove(key);
    };

    auto reportInvalidColorField = [this](JsonValue& parentValue, const char* key, const std::string& path) {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }

        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            return;
        }

        if (ParseColorLikeValue(fieldValue)) {
            return;
        }

        if (fieldValue.IsString() || fieldValue.IsNumber()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects color value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    };

    auto reportInvalidBooleanLikeField = [this](JsonValue& parentValue, const char* key, const std::string& path) {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }

        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            return;
        }

        if (ParseBooleanLikeValue(fieldValue)) {
            return;
        }

        if (fieldValue.IsString()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects boolean value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    };

    reportInvalidDimensionStyle(value, "width", "styles.width");
    reportInvalidDimensionStyle(value, "height", "styles.height");

    if (value.Has("constraintSize")) {
        JsonValue constraintSizeValue = value.GetItem("constraintSize");
        if (!IsDynamicDescriptorObject(constraintSizeValue)) {
            if (!constraintSizeValue.IsObject()) {
                ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.constraintSize has invalid value and has been reset to default",
                    "styles.constraintSize");
                value.Remove("constraintSize");
            } else {
                reportInvalidDimensionField(constraintSizeValue, "minWidth", "styles.constraintSize.minWidth");
                reportInvalidDimensionField(constraintSizeValue, "maxWidth", "styles.constraintSize.maxWidth");
                reportInvalidDimensionField(constraintSizeValue, "minHeight", "styles.constraintSize.minHeight");
                reportInvalidDimensionField(constraintSizeValue, "maxHeight", "styles.constraintSize.maxHeight");
                if (!HasAnyChildField(constraintSizeValue)) {
                    value.Remove("constraintSize");
                }
            }
        }
    }

    if (value.Has("backgroundImage")) {
        JsonValue backgroundImageValue = value.GetItem("backgroundImage");
        if (!IsDynamicDescriptorObject(backgroundImageValue)) {
            std::string backgroundImage;
            if (!StyleApplyUtils::ParseBackgroundImage(backgroundImageValue, backgroundImage)) {
                if (backgroundImageValue.IsString()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.backgroundImage has invalid value and has been reset to default",
                        "styles.backgroundImage");
                } else {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                        "Property styles.backgroundImage expects string value, got type '" +
                            std::string(backgroundImageValue.GetTypeName()) + "', fallback/reset has been applied",
                        "styles.backgroundImage");
                }
                value.Remove("backgroundImage");
            }
        }
    }

    auto normalizeBackgroundImageSizeStyle = [this, &value, &reportInvalidDimensionField](const char* styleName) {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }

        JsonValue backgroundImageSizeValue = value.GetItem(styleName);
        if (IsDynamicDescriptorObject(backgroundImageSizeValue)) {
            return;
        }

        const std::string propertyPath = std::string("styles.") + styleName;
        if (backgroundImageSizeValue.IsObject()) {
            reportInvalidDimensionField(backgroundImageSizeValue, "width", propertyPath + ".width");
            reportInvalidDimensionField(backgroundImageSizeValue, "height", propertyPath + ".height");
            if (!HasAnyChildField(backgroundImageSizeValue)) {
                value.Remove(styleName);
            }
            return;
        }

        StyleBackgroundImageSize imageSize;
        if (!StyleApplyUtils::ParseBackgroundImageSize(backgroundImageSizeValue, imageSize)) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + propertyPath + " has invalid value and has been reset to default", propertyPath);
            value.Remove(styleName);
        }
    };

    normalizeBackgroundImageSizeStyle("backgroundImageSizeWithStyle");
    normalizeBackgroundImageSizeStyle("backgroundImageSize");
    normalizeBackgroundImageSizeStyle("backgroundimageSize");

    if (value.Has("margin")) {
        JsonValue marginValue = value.GetItem("margin");
        if (!IsDynamicDescriptorObject(marginValue)) {
            if (marginValue.IsObject()) {
                reportInvalidDimensionField(marginValue, "all", "styles.margin.all");
                reportInvalidDimensionField(marginValue, "vertical", "styles.margin.vertical");
                reportInvalidDimensionField(marginValue, "horizontal", "styles.margin.horizontal");
                reportInvalidDimensionField(marginValue, "top", "styles.margin.top");
                reportInvalidDimensionField(marginValue, "right", "styles.margin.right");
                reportInvalidDimensionField(marginValue, "bottom", "styles.margin.bottom");
                reportInvalidDimensionField(marginValue, "left", "styles.margin.left");
            } else {
                StyleEdge edge;
                if (!StyleApplyUtils::ParseEdge(marginValue, edge)) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.margin has invalid value and has been reset to default", "styles.margin");
                    value.Remove("margin");
                }
            }
        }
    }

    if (value.Has("padding")) {
        JsonValue paddingValue = value.GetItem("padding");
        if (!IsDynamicDescriptorObject(paddingValue)) {
            if (paddingValue.IsObject()) {
                reportInvalidDimensionField(paddingValue, "all", "styles.padding.all");
                reportInvalidDimensionField(paddingValue, "vertical", "styles.padding.vertical");
                reportInvalidDimensionField(paddingValue, "horizontal", "styles.padding.horizontal");
                reportInvalidDimensionField(paddingValue, "top", "styles.padding.top");
                reportInvalidDimensionField(paddingValue, "right", "styles.padding.right");
                reportInvalidDimensionField(paddingValue, "bottom", "styles.padding.bottom");
                reportInvalidDimensionField(paddingValue, "left", "styles.padding.left");
            } else {
                StyleEdge edge;
                if (!StyleApplyUtils::ParseEdge(paddingValue, edge)) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.padding has invalid value and has been reset to default", "styles.padding");
                    value.Remove("padding");
                }
            }
        }
    }

    if (value.Has("borderRadius")) {
        JsonValue borderRadiusValue = value.GetItem("borderRadius");
        if (!IsDynamicDescriptorObject(borderRadiusValue)) {
            if (borderRadiusValue.IsObject()) {
                reportInvalidDimensionField(borderRadiusValue, "all", "styles.borderRadius.all");
                reportInvalidDimensionField(borderRadiusValue, "topLeft", "styles.borderRadius.topLeft");
                reportInvalidDimensionField(borderRadiusValue, "topRight", "styles.borderRadius.topRight");
                reportInvalidDimensionField(borderRadiusValue, "bottomLeft", "styles.borderRadius.bottomLeft");
                reportInvalidDimensionField(borderRadiusValue, "bottomRight", "styles.borderRadius.bottomRight");
            } else {
                StyleRadius radius;
                if (!StyleApplyUtils::ParseRadius(borderRadiusValue, radius)) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.borderRadius has invalid value and has been reset to default",
                        "styles.borderRadius");
                    value.Remove("borderRadius");
                }
            }
        }
    }

    if (value.Has("borderWidth")) {
        JsonValue borderWidthValue = value.GetItem("borderWidth");
        if (!IsDynamicDescriptorObject(borderWidthValue)) {
            if (borderWidthValue.IsObject()) {
                reportInvalidDimensionField(borderWidthValue, "all", "styles.borderWidth.all");
                reportInvalidDimensionField(borderWidthValue, "vertical", "styles.borderWidth.vertical");
                reportInvalidDimensionField(borderWidthValue, "horizontal", "styles.borderWidth.horizontal");
                reportInvalidDimensionField(borderWidthValue, "top", "styles.borderWidth.top");
                reportInvalidDimensionField(borderWidthValue, "right", "styles.borderWidth.right");
                reportInvalidDimensionField(borderWidthValue, "bottom", "styles.borderWidth.bottom");
                reportInvalidDimensionField(borderWidthValue, "left", "styles.borderWidth.left");
                if (!HasAnyChildField(borderWidthValue)) {
                    value.Remove("borderWidth");
                }
            } else {
                StyleEdge borderWidth;
                if (!StyleApplyUtils::ParseEdge(borderWidthValue, borderWidth)) {
                    if (borderWidthValue.IsString() || borderWidthValue.IsNumber()) {
                        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                            "Property styles.borderWidth has invalid value and has been reset to default",
                            "styles.borderWidth");
                    } else {
                        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                            "Property styles.borderWidth expects dimension value, got type '" +
                                std::string(borderWidthValue.GetTypeName()) + "', fallback/reset has been applied",
                            "styles.borderWidth");
                    }
                    value.Remove("borderWidth");
                }
            }
        }
    }

    if (value.Has("clip")) {
        JsonValue clipValue = value.GetItem("clip");
        if (!IsDynamicDescriptorObject(clipValue)) {
            bool clip = false;
            if (!StyleApplyUtils::ParseClip(clipValue, clip) && !ParseBooleanLikeValue(clipValue)) {
                if (clipValue.IsString()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.clip has invalid value and has been reset to default", "styles.clip");
                } else {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                        "Property styles.clip expects boolean value, got type '" +
                            std::string(clipValue.GetTypeName()) + "', fallback/reset has been applied",
                        "styles.clip");
                }
                value.Remove("clip");
            }
        }
    }

    if (value.Has("backgroundColor")) {
        reportInvalidColorField(value, "backgroundColor", "styles.backgroundColor");
    }

    if (value.Has("borderColor")) {
        JsonValue borderColorValue = value.GetItem("borderColor");
        if (!IsDynamicDescriptorObject(borderColorValue)) {
            if (borderColorValue.IsObject()) {
                reportInvalidColorField(borderColorValue, "top", "styles.borderColor.top");
                reportInvalidColorField(borderColorValue, "right", "styles.borderColor.right");
                reportInvalidColorField(borderColorValue, "bottom", "styles.borderColor.bottom");
                reportInvalidColorField(borderColorValue, "left", "styles.borderColor.left");
                if (!HasAnyChildField(borderColorValue)) {
                    value.Remove("borderColor");
                }
            } else if (!ParseColorLikeValue(borderColorValue)) {
                if (borderColorValue.IsString() || borderColorValue.IsNumber()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.borderColor has invalid value and has been reset to default",
                        "styles.borderColor");
                } else {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                        "Property styles.borderColor expects color value, got type '" +
                            std::string(borderColorValue.GetTypeName()) + "', fallback/reset has been applied",
                        "styles.borderColor");
                }
                value.Remove("borderColor");
            }
        }
    }

    if (value.Has("linearGradient")) {
        JsonValue linearGradientValue = value.GetItem("linearGradient");
        if (!IsDynamicDescriptorObject(linearGradientValue)) {
            if (!linearGradientValue.IsObject()) {
                ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.linearGradient has invalid value and has been reset to default",
                    "styles.linearGradient");
                value.Remove("linearGradient");
            } else {
                bool shouldRemoveGradient = false;
                if (!linearGradientValue.Has("colors")) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.linearGradient has invalid value and has been reset to default",
                        "styles.linearGradient");
                    shouldRemoveGradient = true;
                } else if (!IsDynamicDescriptorObject(linearGradientValue.GetItem("colors")) &&
                           (!linearGradientValue.GetItem("colors").IsArray() ||
                               linearGradientValue.GetItem("colors").GetArraySize() <= 0)) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.linearGradient.colors has invalid value and has been reset to default",
                        "styles.linearGradient.colors");
                    linearGradientValue.Remove("colors");
                    shouldRemoveGradient = true;
                }
                if (linearGradientValue.Has("angle") &&
                    !IsDynamicDescriptorObject(linearGradientValue.GetItem("angle"))) {
                    JsonValue angleValue = linearGradientValue.GetItem("angle");
                    float parsedAngle = 0.0F;
                    if (!(StyleApplyUtils::ParseNumber(angleValue, parsedAngle) ||
                            (angleValue.IsString() && !IsEmptyStringValue(angleValue)))) {
                        ReportCustomSchemaWarning(
                            angleValue.IsString() ? SCHEMA_ERROR_CODE_INVALID_VALUE : SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                            angleValue.IsString()
                                ? "Property styles.linearGradient.angle has invalid value and has been reset to default"
                                : "Property styles.linearGradient.angle expects string or number value, got type '" +
                                      std::string(angleValue.GetTypeName()) + "', fallback/reset has been applied",
                            "styles.linearGradient.angle");
                        linearGradientValue.Remove("angle");
                    }
                }
                if (linearGradientValue.Has("direction") &&
                    !IsDynamicDescriptorObject(linearGradientValue.GetItem("direction")) &&
                    !ParseGradientDirectionValue(linearGradientValue.GetItem("direction"))) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.linearGradient.direction has invalid value and has been reset to default",
                        "styles.linearGradient.direction");
                    linearGradientValue.Remove("direction");
                }
                reportInvalidBooleanLikeField(linearGradientValue, "repeating", "styles.linearGradient.repeating");
                if (linearGradientValue.Has("stops") &&
                    !IsDynamicDescriptorObject(linearGradientValue.GetItem("stops")) &&
                    !linearGradientValue.GetItem("stops").IsArray()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.linearGradient.stops has invalid value and has been reset to default",
                        "styles.linearGradient.stops");
                    linearGradientValue.Remove("stops");
                }
                if (shouldRemoveGradient || !HasAnyChildField(linearGradientValue)) {
                    value.Remove("linearGradient");
                }
            }
        }
    }

    if (value.Has("layoutWeight")) {
        JsonValue layoutWeightValue = value.GetItem("layoutWeight");
        if (!IsDynamicDescriptorObject(layoutWeightValue)) {
            float parsedNumber = 0.0F;
            bool isValid = StyleApplyUtils::ParseNumber(layoutWeightValue, parsedNumber) &&
                           std::isfinite(parsedNumber) && parsedNumber >= 0.0F;
            if (!isValid) {
                if (layoutWeightValue.IsString() || layoutWeightValue.IsNumber()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.layoutWeight has invalid value and has been reset to default",
                        "styles.layoutWeight");
                } else {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                        "Property styles.layoutWeight expects number value, got type '" +
                            std::string(layoutWeightValue.GetTypeName()) + "', fallback/reset has been applied",
                        "styles.layoutWeight");
                }
                value.Remove("layoutWeight");
            }
        }
    }

    if (value.Has("flexShrink")) {
        JsonValue flexShrinkValue = value.GetItem("flexShrink");
        if (!IsDynamicDescriptorObject(flexShrinkValue)) {
            float parsedNumber = 0.0F;
            bool isValid = StyleApplyUtils::ParseNumber(flexShrinkValue, parsedNumber) && std::isfinite(parsedNumber) &&
                           parsedNumber >= 0.0F && parsedNumber <= 1.0F;
            if (!isValid) {
                if (IsEmptyStringValue(flexShrinkValue) || flexShrinkValue.IsNumber() || flexShrinkValue.IsString()) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.flexShrink has invalid value and has been reset to default",
                        "styles.flexShrink");
                } else {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                        "Property styles.flexShrink expects number value, got type '" +
                            std::string(flexShrinkValue.GetTypeName()) + "', fallback/reset has been applied",
                        "styles.flexShrink");
                }
                value.Remove("flexShrink");
            }
        }
    }

    if (value.Has("shadow")) {
        JsonValue shadowValue = value.GetItem("shadow");
        if (!IsDynamicDescriptorObject(shadowValue)) {
            if (shadowValue.IsObject()) {
                if (shadowValue.Has("style") && !IsDynamicDescriptorObject(shadowValue.GetItem("style")) &&
                    !ParseShadowStyleValue(shadowValue.GetItem("style"))) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.shadow.style has invalid value and has been reset to default",
                        "styles.shadow.style");
                    shadowValue.Remove("style");
                }
                reportInvalidNumberField(shadowValue, "radius", "styles.shadow.radius", false, false, false);
                reportInvalidNumberField(shadowValue, "offsetX", "styles.shadow.offsetX", true, false, true);
                reportInvalidNumberField(shadowValue, "offsetY", "styles.shadow.offsetY", true, false, true);
                reportInvalidColorField(shadowValue, "color", "styles.shadow.color");
                if (shadowValue.Has("type") && !IsDynamicDescriptorObject(shadowValue.GetItem("type")) &&
                    !ParseShadowTypeValue(shadowValue.GetItem("type"))) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.shadow.type has invalid value and has been reset to default",
                        "styles.shadow.type");
                    shadowValue.Remove("type");
                }
                reportInvalidBooleanLikeField(shadowValue, "fill", "styles.shadow.fill");
                if (!HasAnyChildField(shadowValue)) {
                    value.Remove("shadow");
                }
            } else {
                StyleShadow shadow;
                if (!StyleApplyUtils::ParseShadow(shadowValue, shadow) || !shadow.valid) {
                    ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                        "Property styles.shadow has invalid value and has been reset to default", "styles.shadow");
                    value.Remove("shadow");
                }
            }
        }
    }

    if (value.Has("visibility")) {
        JsonValue visibilityValue = value.GetItem("visibility");
        if (!IsDynamicDescriptorObject(visibilityValue)) {
            A2UIVisibility visibility = A2UIVisibility::VISIBLE;
            if (!StyleApplyUtils::ParseVisibility(visibilityValue, visibility)) {
                ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.visibility has invalid value and has been reset to default", "styles.visibility");
                value.Remove("visibility");
            }
        }
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
            if (pathJson.IsValid() && pathJson.IsString()) {
                std::string path = pathJson.GetStringValue("");
                if (!path.empty()) {
                    std::string bindingKey = "tabs[" + std::to_string(i) + "].title";
                    AddBinding(bindingKey, path);
                    LOG_A2UI(LOG_INFO,
                        "CustomComponent::RegisterDataBindings: registered binding %{public}s -> %{public}s",
                        bindingKey.c_str(), path.c_str());
                }
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

    napi_value attributeValue = CreateAttributeValue();
    napi_value createFunction = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_get_reference_value(env_, createCustomComponentRef, &createFunction);
    if (status != napi_ok || createFunction == nullptr) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: get create function failed, type=%{public}s", descriptor_.type.c_str());
        napi_close_handle_scope(env_, scope);
        return false;
    }

    napi_value argv[] = { attributeValue };
    status = napi_call_function(env_, nullptr, createFunction, 1, argv, &result);
    if (status != napi_ok || result == nullptr) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: call create function failed, type=%{public}s", descriptor_.type.c_str());
        napi_close_handle_scope(env_, scope);
        return false;
    }

    if (!HasNamedProperty(env_, result, CONTENT_KEY)) {
        LOG_A2UI(
            LOG_ERROR, "CreateCustomComponent: missing content property, type=%{public}s", descriptor_.type.c_str());
        napi_close_handle_scope(env_, scope);
        return false;
    }

    napi_value componentContent = nullptr;
    napi_get_named_property(env_, result, CONTENT_KEY, &componentContent);

    ArkUI_NodeHandle handle = nullptr;
    int32_t handleResult = ArkUIOHApiAdapter::GetNodeHandleFromNapiValue(env_, componentContent, &handle);
    if (handleResult != A2UI_ERROR_CODE_NO_ERROR || handle == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "CreateCustomComponent: convert content to native node failed, code=%{public}d, "
            "type=%{public}s",
            handleResult, descriptor_.type.c_str());
        napi_close_handle_scope(env_, scope);
        return false;
    }

    A2UINodeContentHandle childSlotHandle = nullptr;
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

    customContentHandle_ = handle;
    nativeView_ = handle;
    childSlotHandle_ = childSlotHandle;
    hasCreatedCustomComponent_ = true;
    napi_close_handle_scope(env_, scope);

    LOG_A2UI(LOG_INFO, "CreateCustomComponent success, type=%{public}s, id=%{public}s, content=%{public}p",
        descriptor_.type.c_str(), descriptor_.id.c_str(), handle);
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

    if (updateFunction == nullptr || componentContent == nullptr) {
        napi_close_handle_scope(env_, scope);
        return;
    }

    napi_value argv[] = { componentContent, childSlot, childSlots, attributeValue };
    napi_status status = napi_call_function(env_, nullptr, updateFunction, 4, argv, &result);
    if (status == napi_ok && result != nullptr) {
        napi_valuetype resultType = napi_undefined;
        napi_typeof(env_, result, &resultType);
        if (resultType == napi_object) {
            napi_value updatedChildSlots = nullptr;
            if (HasNamedProperty(env_, result, CHILD_SLOTS_OBJECT_KEY)) {
                napi_get_named_property(env_, result, CHILD_SLOTS_OBJECT_KEY, &updatedChildSlots);
            } else if (HasNamedProperty(env_, result, CHILD_SLOTS_KEY)) {
                napi_get_named_property(env_, result, CHILD_SLOTS_KEY, &updatedChildSlots);
            }
            SyncChildSlots(updatedChildSlots);
        }
    }
    napi_close_handle_scope(env_, scope);
}

napi_value CustomComponent::CreateAttributeValue() const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value attributeValue = nullptr;
    napi_create_object(env_, &attributeValue);
    SetStringProperty(env_, attributeValue, TYPE_KEY, descriptor_.type);
    SetStringProperty(env_, attributeValue, ID_KEY, descriptor_.id);
    if (!descriptor_.surfaceId.empty()) {
        SetStringProperty(env_, attributeValue, "surfaceId", descriptor_.surfaceId);
    }
    napi_value renderIdValue = nullptr;
    napi_create_int32(env_, renderId_, &renderIdValue);
    napi_set_named_property(env_, attributeValue, "renderId", renderIdValue);
    napi_value customComponentHandleValue = nullptr;
    napi.CreateDouble(env_, static_cast<double>(GetCustomComponentHandle()), &customComponentHandleValue);
    napi.SetNamedProperty(env_, attributeValue, "customComponentHandle", customComponentHandleValue);
    if (!GetSurfaceContext().a2UIProtocolVersion.empty()) {
        SetStringProperty(env_, attributeValue, "protocolVersion", GetSurfaceContext().a2UIProtocolVersion);
    }
    if (!GetSurfaceContext().catalogId.empty()) {
        SetStringProperty(env_, attributeValue, "catalogId", GetSurfaceContext().catalogId);
    }
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    SetComponentThemeProperty(env_, attributeValue, themeContext);

    if (descriptor_.customProps.IsValid()) {
        napi_set_named_property(
            env_, attributeValue, CUSTOM_PROPS_KEY, JsonValueToNapiValue(env_, descriptor_.customProps));
    }

    if (HasCommonStyleProps(descriptor_.properties)) {
        napi_value propertiesValue = nullptr;
        napi_create_object(env_, &propertiesValue);
        if (!descriptor_.properties.size.empty()) {
            SetStringProperty(env_, propertiesValue, SIZE_KEY, descriptor_.properties.size);
        }
        if (!descriptor_.properties.padding.empty()) {
            SetStringProperty(env_, propertiesValue, PADDING_KEY, descriptor_.properties.padding);
        }
        if (descriptor_.properties.hasMargin) {
            SetStringProperty(env_, propertiesValue, MARGIN_KEY, descriptor_.properties.margin);
        }
        if (descriptor_.properties.hasWidth) {
            SetDoubleProperty(env_, propertiesValue, WIDTH_KEY, descriptor_.properties.width);
        }
        if (descriptor_.properties.hasHeight) {
            SetDoubleProperty(env_, propertiesValue, HEIGHT_KEY, descriptor_.properties.height);
        }
        if (descriptor_.properties.hasWeight) {
            SetDoubleProperty(env_, propertiesValue, "weight", descriptor_.properties.weight);
        }
        if (descriptor_.properties.hasAccessibilityLabel) {
            SetStringProperty(env_, propertiesValue, "accessibilityLabel", descriptor_.properties.accessibilityLabel);
        }
        if (descriptor_.properties.hasAccessibilityDescription) {
            SetStringProperty(
                env_, propertiesValue, "accessibilityDescription", descriptor_.properties.accessibilityDescription);
        }
        napi_set_named_property(env_, attributeValue, PROPERTIES_KEY, propertiesValue);
    }

    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot != nullptr) {
        std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
        if (bindingEngine != nullptr) {
            std::shared_ptr<DataModel> dataModel = bindingEngine->GetOrCreateDataModel(surfaceId);
            if (dataModel != nullptr && dataModel->GetRoot() != nullptr && dataModel->GetRoot()->IsValid()) {
                std::string dataModelJson = dataModel->GetRoot()->ToJsonLiteral();
                if (!dataModelJson.empty() && dataModelJson != "null") {
                    napi_value dataModelValue = nullptr;
                    napi_create_string_utf8(env_, dataModelJson.c_str(), NAPI_AUTO_LENGTH, &dataModelValue);
                    napi_set_named_property(env_, attributeValue, DATA_MODEL_JSON_KEY, dataModelValue);
                }
            }
        }
    }

    return attributeValue;
}

std::map<std::string, JsonValue> CustomComponent::CollectTabsProperties() const
{
    std::map<std::string, JsonValue> tabsProperties;
    for (const auto& pair : properties_) {
        if (pair.first.find("tabs[") == 0) {
            tabsProperties[pair.first] = pair.second;
        }
    }
    return tabsProperties;
}

JsonValue CustomComponent::GetOrCreateTabsArray(const std::map<std::string, JsonValue>& properties) const
{
    auto tabsIterator = properties.find("tabs");
    if (tabsIterator != properties.end() && tabsIterator->second.IsArray()) {
        JsonValue clonedTabs;
        if (CloneJsonValue(tabsIterator->second, clonedTabs) && clonedTabs.IsArray()) {
            return clonedTabs;
        }
        LOG_A2UI(LOG_ERROR, "BuildCustomProps: failed to clone original tabs array");
    }

    std::unique_ptr<JsonAdapter> tabsAdapter = JsonAdapter::CreateArray();
    if (tabsAdapter == nullptr) {
        return JsonValue();
    }
    return tabsAdapter->GetRoot();
}

void CustomComponent::UpdateTabsWithProperties(
    JsonValue& tabsArray, const std::map<std::string, JsonValue>& tabsProperties) const
{
    if (!tabsArray.IsArray()) {
        return;
    }

    for (const auto& pair : tabsProperties) {
        const std::string& key = pair.first;
        size_t startPos = key.find('[');
        size_t endPos = key.find(']');
        size_t dotPos = key.find('.');
        if (startPos == std::string::npos || endPos == std::string::npos || dotPos == std::string::npos) {
            continue;
        }

        int index = std::stoi(key.substr(startPos + 1, endPos - startPos - 1));
        std::string propPath = key.substr(dotPos + 1);
        while (tabsArray.GetArraySize() <= index) {
            std::unique_ptr<JsonAdapter> emptyObject = JsonAdapter::CreateObject();
            if (emptyObject == nullptr || !tabsArray.Append(emptyObject->GetRoot())) {
                break;
            }
        }

        JsonValue tabObj = tabsArray.GetArrayItem(index);
        if (!tabObj.IsObject()) {
            continue;
        }

        JsonValue clonedValue;
        if (!CloneJsonValue(pair.second, clonedValue)) {
            continue;
        }
        if (!tabObj.Set(propPath.c_str(), clonedValue)) {
            continue;
        }
    }
}

void CustomComponent::ResolveFunctionCallsInTabsArray(JsonValue& tabsArray) const
{
    if (!tabsArray.IsArray()) {
        return;
    }

    DynamicResolveContext context = {
        .renderId = GetRenderId(), .surfaceId = GetSurfaceId(), .componentId = GetComponentId()
    };

    int tabCount = tabsArray.GetArraySize();
    for (int i = 0; i < tabCount; ++i) {
        JsonValue tabObj = tabsArray.GetArrayItem(i);
        if (!tabObj.IsObject()) {
            continue;
        }

        JsonValue titleItem = tabObj.GetItem("title");
        if (!titleItem.IsObject()) {
            continue;
        }

        JsonValue callItem = titleItem.GetItem("call");
        if (!callItem.IsString()) {
            continue;
        }

        JsonValue clonedTitle;
        if (!CloneJsonValue(titleItem, clonedTitle)) {
            continue;
        }

        ResolvedValue result = DynamicValueResolver::Resolve(clonedTitle, context);
        if (result.success && result.source == ResolveSource::FUNCTION_CALL && result.value.IsString()) {
            std::unique_ptr<JsonAdapter> newTitle = JsonAdapter::CreateString(result.value.GetStringValue(""));
            if (newTitle != nullptr) {
                tabObj.Set("title", newTitle->GetRoot());
            }
            LOG_A2UI(LOG_INFO, "ResolveFunctionCallsInTabsArray: resolved tabs[%{public}d].title via function call", i);
        } else {
            LOG_A2UI(
                LOG_WARN, "ResolveFunctionCallsInTabsArray: failed to resolve tabs[%{public}d].title function call", i);
        }
    }
}

void CustomComponent::AddTabsToBuilder(
    std::ostringstream& builder, const JsonValue& tabsArray, bool& hasCustomProp) const
{
    (void)builder;
    (void)tabsArray;
    (void)hasCustomProp;
}

void CustomComponent::AddOtherPropertiesToBuilder(
    std::ostringstream& builder, const std::map<std::string, JsonValue>& properties, bool& hasCustomProp) const
{
    (void)builder;
    (void)properties;
    (void)hasCustomProp;
}

JsonValue BuildCustomBindingProps(const std::vector<DataBinding>& bindings)
{
    std::unique_ptr<JsonAdapter> bindingsAdapter = JsonAdapter::CreateObject();
    if (bindingsAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue bindingsObject = bindingsAdapter->GetRoot();
    for (const auto& binding : bindings) {
        if (binding.propertyName_.empty() || binding.dataPath_.empty()) {
            continue;
        }
        bindingsObject.PutString(binding.propertyName_.c_str(), binding.dataPath_);
    }
    return bindingsObject.GetChild().IsValid() ? bindingsObject : JsonValue();
}

JsonValue BuildExtendedTabsTemplateChildren(const ChildListDescriptor& childList)
{
    if (childList.type != ChildListType::TEMPLATE_PATH) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> templateAdapter = JsonAdapter::CreateObject();
    if (templateAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue templateChildren = templateAdapter->GetRoot();
    templateChildren.PutString("componentId", childList.templateComponentId);
    templateChildren.PutString("path", childList.templatePath);
    if (!childList.useDefaultIndexVar) {
        templateChildren.PutString("indexVar", childList.resolvedIndexVarName);
    }
    if (!childList.useDefaultItemVar) {
        templateChildren.PutString("itemVar", childList.resolvedItemVarName);
    }
    return templateChildren;
}

JsonValue CustomComponent::BuildCustomProps() const
{
    std::unique_ptr<JsonAdapter> customPropsAdapter = JsonAdapter::CreateObject();
    if (customPropsAdapter == nullptr) {
        return JsonValue();
    }
    JsonValue customProps = customPropsAdapter->GetRoot();
    const std::map<std::string, JsonValue>& properties = properties_;

    std::map<std::string, JsonValue> tabsProperties = CollectTabsProperties();
    JsonValue tabsArray = GetOrCreateTabsArray(properties);
    UpdateTabsWithProperties(tabsArray, tabsProperties);
    MergeTabsFromChildIds(tabsArray);
    ResolveFunctionCallsInTabsArray(tabsArray);

    auto tabsIterator = properties.find("tabs");
    bool hasRawTabs = tabsIterator != properties.end() && tabsIterator->second.IsValid();
    if (hasRawTabs) {
        if (tabsIterator->second.IsArray()) {
            customProps.Put("tabs", tabsArray);
        } else {
            JsonValue rawTabsValue;
            if (CloneJsonValue(tabsIterator->second, rawTabsValue)) {
                customProps.Put("tabs", rawTabsValue);
            }
        }
    } else if (tabsArray.IsArray() && tabsArray.GetArraySize() > 0) {
        customProps.Put("tabs", tabsArray);
    }

    std::unique_ptr<JsonAdapter> childrenAdapter = JsonAdapter::CreateArray();
    if (childrenAdapter != nullptr) {
        JsonValue childrenArray = childrenAdapter->GetRoot();
        MergeTabsChildren(childrenArray);
        MergeRowChildren(childrenArray);
        if (childrenArray.IsArray() && childrenArray.GetArraySize() > 0) {
            customProps.Put("children", childrenArray);
        } else if (IsExtendedTabsType() && childListDescriptor_.type == ChildListType::STATIC_IDS) {
            customProps.Put("children", childrenArray);
        } else if (IsExtendedTabsType() && childListDescriptor_.type == ChildListType::TEMPLATE_PATH) {
            JsonValue templateChildren = BuildExtendedTabsTemplateChildren(childListDescriptor_);
            if (templateChildren.IsValid()) {
                customProps.Put("children", templateChildren);
            }
        }
    }

    for (const std::string& key : customPropertyNames_) {
        if (key == "tabs" || key.find("tabs[") == 0) {
            continue;
        }
        if (key == "children" && IsExtendedTabsType() && !tabChildIds_.empty()) {
            continue;
        }

        auto iterator = properties.find(key);
        if (iterator == properties.end() || !iterator->second.IsValid()) {
            continue;
        }
        customProps.Put(key.c_str(), iterator->second);
    }

    JsonValue bindingsProps = BuildCustomBindingProps(GetDataBindings());
    if (bindingsProps.IsValid()) {
        customProps.Put(A2UI_BINDINGS_KEY, bindingsProps);
    }

    if (!customProps.GetChild().IsValid()) {
        if (descriptor_.type == TABS_COMPONENT_TYPE) {
            return customProps;
        }
        return JsonValue();
    }
    if (IsExtendedEtsExpressionScope()) {
        return ResolveExpressionsInValue(customProps, "");
    }
    return customProps;
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

        CustomDynamicDescriptorKind descriptorKind = ResolveCustomDynamicDescriptorKind(child);
        if (descriptorKind != CustomDynamicDescriptorKind::NONE) {
            JsonValue pathValue = child.GetItem("path");
            bool shouldReportCompatibilityWarning = false;
            if (descriptorKind == CustomDynamicDescriptorKind::PATH && pathValue.IsString()) {
                std::string bindingPath = pathValue.GetStringValue("");
                if (IsValidDataPath(bindingPath) &&
                    ShouldReportCustomDynamicDescriptorCompatibilityWarning(descriptor_.type, key)) {
                    shouldReportCompatibilityWarning = true;
                }
            } else if (descriptorKind == CustomDynamicDescriptorKind::CALL &&
                       ShouldReportCustomDynamicDescriptorCompatibilityWarning(descriptor_.type, key)) {
                shouldReportCompatibilityWarning = true;
            }

            if (shouldReportCompatibilityWarning) {
                DispatchCustomDynamicDescriptorCompatibilityWarning(
                    renderId_, surfaceId_, componentId_, descriptor_.type, key, descriptorKind);
            }

            SetPropertyFromDescriptor(key, descriptor);
            continue;
        }

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

bool CustomComponent::IsExtendedEtsExpressionScope() const
{
    // Only ETS-implemented Extended components (registered in A2UIExtendedComponents.ets)
    // are in scope. Base A2UI ETS custom components (Tabs/Icon/...) must keep literal behavior.
    static const std::set<std::string> scopedTypes = { "TabContent", "Select", "Tabs", "Web" };
    return scopedTypes.find(descriptor_.type) != scopedTypes.end();
}

JsonValue CustomComponent::ResolveExpressionsInValue(const JsonValue& node, const std::string& path) const
{
    if (!node.IsValid()) {
        return JsonValue();
    }
#ifdef ENABLE_EXPRESSION_ENGINE
    if (node.IsString()) {
        std::string raw = node.GetStringValue("");
        if (ExpressionEngine::IsExpression(raw)) {
            JsonValue resolved = EvaluateCustomExpression(raw);
            if (resolved.IsValid()) {
                return resolved; // type-preserved value (bool/number/string/object/array)
            }
            LOG_A2UI(LOG_WARN,
                "CustomComponent: expression resolve failed, raw value kept, type=%{public}s, path=%{public}s",
                descriptor_.type.c_str(), path.c_str());
        }
        std::unique_ptr<JsonAdapter> cloned = JsonAdapter::Clone(node);
        return cloned != nullptr ? cloned->GetRoot() : JsonValue();
    }
    if (node.IsObject()) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
        if (adapter == nullptr) {
            return JsonValue();
        }
        JsonValue obj = adapter->GetRoot();
        for (JsonValue child = node.GetChild(); child.IsValid(); child = child.GetNext()) {
            JsonValue resolvedChild = ResolveExpressionsInValue(child, path + "." + child.GetKey());
            if (resolvedChild.IsValid()) {
                obj.Put(child.GetKey().c_str(), resolvedChild);
            }
        }
        return obj;
    }
    if (node.IsArray()) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateArray();
        if (adapter == nullptr) {
            return JsonValue();
        }
        JsonValue arr = adapter->GetRoot();
        for (int i = 0; i < node.GetArraySize(); ++i) {
            JsonValue resolvedItem =
                ResolveExpressionsInValue(node.GetArrayItem(i), path + "[" + std::to_string(i) + "]");
            if (resolvedItem.IsValid()) {
                arr.Append(resolvedItem);
            }
        }
        return arr;
    }
#endif
    // number/bool/null/raw, or expression engine disabled: clone as-is.
    std::unique_ptr<JsonAdapter> cloned = JsonAdapter::Clone(node);
    return cloned != nullptr ? cloned->GetRoot() : JsonValue();
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
    return GetShortType(descriptor_.type) == "Tabs";
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
    if (!arrayValueOpt.has_value() || !arrayValueOpt.value().IsArray()) {
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

        size_t keySize = 0;
        if (napi.GetValueStringUtf8(env_, key, nullptr, 0, &keySize) != napi_ok) {
            continue;
        }
        std::string keyStr(keySize + 1, '\0');
        if (napi.GetValueStringUtf8(env_, key, &keyStr[0], keySize + 1, &keySize) != napi_ok) {
            continue;
        }
        keyStr.resize(keySize);
        if (keyStr.empty()) {
            continue;
        }

        napi_value slotValue = nullptr;
        napi_get_named_property(env_, childSlots, keyStr.c_str(), &slotValue);

        A2UINodeContentHandle slotHandle = nullptr;
        ArkUIOHApiAdapter::GetNodeContentFromNapiValue(env_, slotValue, &slotHandle);
        if (slotHandle == nullptr) {
            LOG_A2UI(
                LOG_ERROR, "SyncChildSlots: Failed to extract NodeContentHandle for key=%{public}s", keyStr.c_str());
            continue;
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

JsonValue CustomComponent::GetCustomProperty(const std::string& propertyName) const
{
    auto iter = properties_.find(propertyName);
    if (iter == properties_.end()) {
        return JsonValue();
    }
    JsonValue clonedValue;
    if (!CloneJsonValue(iter->second, clonedValue)) {
        return JsonValue();
    }
    return clonedValue;
}

bool CustomComponent::SetRuntimeCustomProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName.empty() || !value.IsValid()) {
        return false;
    }

    JsonValue clonedValue;
    if (!CloneJsonValue(value, clonedValue)) {
        return false;
    }

    properties_[propertyName] = clonedValue;
    customPropertyNames_.insert(propertyName);
    descriptor_.customProps = BuildCustomProps();
    return true;
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
        if (!IsValidDataPath(path)) {
            NapiBridge::GetInstance().Provider().DeleteReference(env, callbackRef);
            if (errorMessage != nullptr) {
                *errorMessage = "path is invalid";
            }
            return false;
        }
        callbackInfo.dataPath = path;
        dynamicValueCallbacks_[propertyName] = callbackInfo;
        dynamicResolverBindingKeys_.insert(propertyName);
        RemoveBindingsForProperty(propertyName);
        AddBinding(propertyName, path);
        DispatchCurrentDynamicValue(propertyName, path);
        MarkDescriptorDynamicBindingsResolved();
        SyncDynamicValueBindings();
        return true;
    }

    DynamicResolveContext context = { .renderId = GetRenderId(),
        .surfaceId = GetSurfaceId(),
        .componentId = GetComponentId(),
        .allowExpression = true,
        .localVariables = GetLocalVariables() };
    ResolvedValue resolved = DynamicValueResolver::Resolve(descriptor, context);
    if (!resolved.success || !resolved.value.IsValid()) {
        NapiBridge::GetInstance().Provider().DeleteReference(env, callbackRef);
        if (errorMessage != nullptr) {
            *errorMessage = resolved.errorMessage.empty() ? "dynamic value resolve failed" : resolved.errorMessage;
        }
        return false;
    }

    dynamicValueCallbacks_[propertyName] = callbackInfo;
    bool callbackSuccess = DispatchDynamicValueCallback(propertyName, resolved.value);
    ClearDynamicValueCallback(propertyName);
    if (!callbackSuccess && errorMessage != nullptr) {
        *errorMessage = "failed to dispatch resolved value";
    }
    return callbackSuccess;
}

void CustomComponent::SyncCheckedToBoundDataModel(const std::string& bindingPath, bool value)
{
    if (bindingPath.empty()) {
        return;
    }
    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr) {
        return;
    }
    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        return;
    }
    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateBool(value);
    if (valueAdapter == nullptr) {
        return;
    }
    bindingEngine->UpdateDataModelByPath(surfaceId, bindingPath, valueAdapter->GetRoot());
}

} // namespace NativeModule
