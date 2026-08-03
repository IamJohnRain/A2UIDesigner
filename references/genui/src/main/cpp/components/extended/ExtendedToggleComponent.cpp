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

#include "ExtendedToggleComponent.h"

#include <functional>
#include <map>
#include <memory>

#include "components/extended/ExtendedStyleResolver.h"
#include "data/BindingEngine.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr uint32_t TOGGLE_LIGHT_SELECTED_COLOR = 0xFF007DFFu;
constexpr uint32_t TOGGLE_DARK_SELECTED_COLOR = 0xFF006CDEu;
constexpr uint32_t TOGGLE_LIGHT_UNSELECTED_COLOR = 0x19000000u;
constexpr uint32_t TOGGLE_DARK_UNSELECTED_COLOR = 0x19FFFFFFu;
constexpr uint32_t TOGGLE_LIGHT_SWITCH_POINT_COLOR = 0xFFFFFFFFu;
constexpr uint32_t TOGGLE_DARK_SWITCH_POINT_COLOR = 0xFFE5E5E5u;
constexpr float DEFAULT_TOGGLE_LABEL_SPACING = 12.0F;
JsonValue BuildToggleChangeEventContext(bool isOn)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = adapter->GetRoot();
    root.PutBool("isOn", isOn);
    return root;
}

uint32_t GetDefaultToggleSelectedColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TOGGLE_DARK_SELECTED_COLOR : TOGGLE_LIGHT_SELECTED_COLOR;
}

uint32_t GetDefaultToggleUnSelectedColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TOGGLE_DARK_UNSELECTED_COLOR : TOGGLE_LIGHT_UNSELECTED_COLOR;
}

uint32_t GetDefaultToggleSwitchPointColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TOGGLE_DARK_SWITCH_POINT_COLOR : TOGGLE_LIGHT_SWITCH_POINT_COLOR;
}

} // namespace

ExtendedToggleComponent::ExtendedToggleComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::ROW))
{
    if (nativeView_ == nullptr) {
        return;
    }

    toggleNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TOGGLE);
    textNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);

    if (toggleNode_ == nullptr || textNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetUserData(toggleNode_, this);
    ArkUINodeApiAdapter::AddChild(nativeView_, textNode_);
    ArkUINodeApiAdapter::AddChild(nativeView_, toggleNode_);

    ArkUINodeApiAdapter::AddNodeEventReceiver(toggleNode_, ExtendedToggleComponent::NodeEventReceiver);

    ArkUINodeApiAdapter::SetNodeMargin(toggleNode_, 0.0F, 0.0F, 0.0F, DEFAULT_TOGGLE_LABEL_SPACING);

    SetIsOn(isOn_);
    SetEnabled(enabled_);
    SetSelectedColor(selectedColor_);
    SetUnSelectedColor(unSelectedColor_);
    SetSwitchPointColor(switchPointColor_);
    SetLabel(label_);
}

ExtendedToggleComponent::~ExtendedToggleComponent()
{
    if (toggleNode_ != nullptr) {
        if (changeEventRegistered_) {
            ArkUINodeApiAdapter::UnregisterNodeEvent(toggleNode_, A2UINodeEventType::TOGGLE_ON_CHANGE);
        }
        ArkUINodeApiAdapter::RemoveNodeEventReceiver(toggleNode_, ExtendedToggleComponent::NodeEventReceiver);
    }
}

std::string ExtendedToggleComponent::GetType() const
{
    return "Toggle";
}

void ExtendedToggleComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplyDeclaredPropertyOrFallback(descriptor, "label");
    ApplyDeclaredPropertyOrFallback(descriptor, "isOn");
    ApplyDeclaredPropertyOrFallback(descriptor, "enabled");
}

PropertyDeclaration ExtendedToggleComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedToggleComponent&)>> declarations = {
        { "label",
            [](ExtendedToggleComponent& component) {
                return PropertyDeclaration { .name = "label",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetLabel(value.GetStringValue("")); } };
            } },
        { "isOn",
            [](ExtendedToggleComponent& component) {
                return PropertyDeclaration { .name = "isOn",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackBool = false,
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetIsOn(value.GetBoolValue(false)); } };
            } },
        { "enabled",
            [](ExtendedToggleComponent& component) {
                return PropertyDeclaration { .name = "enabled",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackBool = true,
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetEnabled(value.GetBoolValue(true)); } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

void ExtendedToggleComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (IsApplyingStyleDeltaUpdate() || !styles.IsObject()) {
        return;
    }
    const char* colorKeys[] = { "selectedColor", "unSelectedColor", "switchPointColor" };
    for (const char* key : colorKeys) {
        if (key == nullptr || !styles.Has(key)) {
            continue;
        }
        uint32_t color = 0;
        JsonValue colorValue = styles.GetItem(key);
        if (IsDynamicValueDescriptor(colorValue)) {
            continue;
        }
        if (!colorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles." + std::string(key) + " expects string color, fallback/reset has been applied",
                "styles." + std::string(key));
            continue;
        }
        if (!ExtendedStyleResolver::ParseColor(colorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles." + std::string(key) + " expects color value, fallback/reset has been applied",
                "styles." + std::string(key));
        }
    }
}

void ExtendedToggleComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedToggleComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    ThemeMode themeMode = ResolveThemeMode();
    ApplyStyleColor(styles, "selectedColor", GetDefaultToggleSelectedColor(themeMode), selectedColor_,
        selectedColorOverridden_, &ExtendedToggleComponent::SetSelectedColor);
    ApplyStyleColor(styles, "unSelectedColor", GetDefaultToggleUnSelectedColor(themeMode), unSelectedColor_,
        unSelectedColorOverridden_, &ExtendedToggleComponent::SetUnSelectedColor);
    ApplyStyleColor(styles, "switchPointColor", GetDefaultToggleSwitchPointColor(themeMode), switchPointColor_,
        switchPointColorOverridden_, &ExtendedToggleComponent::SetSwitchPointColor);
}

void ExtendedToggleComponent::OnConfigChange(const ThemeContext& context)
{
    if (!selectedColorOverridden_) {
        SetSelectedColor(GetDefaultToggleSelectedColor(context.colorMode));
    }
    if (!unSelectedColorOverridden_) {
        SetUnSelectedColor(GetDefaultToggleUnSelectedColor(context.colorMode));
    }
    if (!switchPointColorOverridden_) {
        SetSwitchPointColor(GetDefaultToggleSwitchPointColor(context.colorMode));
    }
}

void ExtendedToggleComponent::RegisterComponentSpecificListeners()
{
    UpdateChangeEventRegistration();
}

void ExtendedToggleComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "isOn") {
        SetIsOn(false);
        UpdateChangeEventRegistration();
        return;
    }
    if (propertyName == "enabled") {
        SetEnabled(true);
        return;
    }
    if (propertyName == "label") {
        SetLabel("");
        return;
    }
}

void ExtendedToggleComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* component = reinterpret_cast<ExtendedToggleComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (component != nullptr) {
        component->HandleNodeEvent(event);
    }
}

ThemeMode ExtendedToggleComponent::ResolveThemeMode() const
{
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        return themeManager->GetContext().colorMode;
    }
    return GetRenderContext().colorMode;
}

void ExtendedToggleComponent::ApplyStyleColor(const JsonValue& styles, const char* propertyName, uint32_t defaultColor,
    uint32_t& outColor, bool& overridden, void (ExtendedToggleComponent::*setter)(uint32_t))
{
    bool hasProperty = styles.IsObject() && propertyName != nullptr && styles.Has(propertyName);
    if (!hasProperty && IsApplyingStyleDeltaUpdate()) {
        return;
    }

    if (hasProperty) {
        uint32_t color = outColor;
        if (ExtendedStyleResolver::ParseColor(styles.GetItem(propertyName), color)) {
            overridden = true;
            outColor = color;
            (this->*setter)(outColor);
        } else {
            overridden = false;
            outColor = defaultColor;
            (this->*setter)(outColor);
        }
        return;
    }

    overridden = false;
    outColor = defaultColor;
    (this->*setter)(outColor);
}

void ExtendedToggleComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    if (ArkUIOHApiAdapter::NodeEventGetEventType(event) != A2UINodeEventType::TOGGLE_ON_CHANGE) {
        return;
    }

    A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
    if (componentEvent == nullptr) {
        return;
    }
    HandleToggleChange(componentEvent->data[0].i32 == 1);
}

void ExtendedToggleComponent::HandleToggleChange(bool isOn)
{
    if (isOn == isOn_) {
        return;
    }

    SetIsOn(isOn);
    SyncIsOnToBoundDataModel(isOn);
    DispatchEvent("onChange", BuildToggleChangeEventContext(isOn));
}

void ExtendedToggleComponent::UpdateChangeEventRegistration()
{
    // Keep native state in sync even when the component is queried only by getToggleValue.
    bool shouldRegister = true;
    if (toggleNode_ == nullptr) {
        changeEventRegistered_ = false;
        return;
    }
    if (shouldRegister && !changeEventRegistered_) {
        ArkUINodeApiAdapter::RegisterNodeEvent(toggleNode_, A2UINodeEventType::TOGGLE_ON_CHANGE, 0, this);
        changeEventRegistered_ = true;
        return;
    }

    if (!shouldRegister && changeEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(toggleNode_, A2UINodeEventType::TOGGLE_ON_CHANGE);
        changeEventRegistered_ = false;
    }
}

void ExtendedToggleComponent::SetIsOn(bool isOn)
{
    isOn_ = isOn;
    if (toggleNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeToggleValue(toggleNode_, isOn_);
}

void ExtendedToggleComponent::SetEnabled(bool enabled)
{
    enabled_ = enabled;
    if (toggleNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeEnabled(toggleNode_, enabled_);
}

void ExtendedToggleComponent::SetSelectedColor(uint32_t color)
{
    selectedColor_ = color;
    if (toggleNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeToggleSelectedColor(toggleNode_, selectedColor_);
}

void ExtendedToggleComponent::SetUnSelectedColor(uint32_t color)
{
    unSelectedColor_ = color;
    if (toggleNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeToggleUnselectedColor(toggleNode_, unSelectedColor_);
}

void ExtendedToggleComponent::SetSwitchPointColor(uint32_t color)
{
    switchPointColor_ = color;
    if (toggleNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeToggleSwitchPointColor(toggleNode_, switchPointColor_);
}

void ExtendedToggleComponent::SetLabel(const std::string& label)
{
    label_ = label;
    if (textNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextContent(textNode_, label_);
}

std::string ExtendedToggleComponent::ResolveIsOnBindingPath() const
{
    const auto& bindings = GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "isOn" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

void ExtendedToggleComponent::SyncIsOnToBoundDataModel(bool value)
{
    std::string bindingPath = ResolveIsOnBindingPath();
    if (bindingPath.empty()) {
        return;
    }

    const RenderContext& renderContext = GetRenderContext();
    std::shared_ptr<BindingEngine> bindingEngine = renderContext.bindingEngine;
    if (bindingEngine == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedToggleComponent::SyncIsOnToBoundDataModel: binding engine is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    if (renderContext.surfaceId.empty()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedToggleComponent::SyncIsOnToBoundDataModel: surfaceId is empty, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateBool(value);
    if (valueAdapter == nullptr) {
        return;
    }
    bindingEngine->UpdateDataModelByPath(renderContext.surfaceId, bindingPath, valueAdapter->GetRoot());
}

} // namespace NativeModule
