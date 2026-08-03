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

#include "ExtendedRadioComponent.h"

#include <functional>
#include <map>
#include <memory>

#include "components/extended/ExtendedStyleResolver.h"
#include "data/BindingEngine.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SchemaErrorCodes.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

constexpr uint32_t RADIO_LIGHT_CHECKED_BACKGROUND_COLOR = 0xFF0A59F7u;
constexpr uint32_t RADIO_DARK_CHECKED_BACKGROUND_COLOR = 0xFF317AF7u;
constexpr uint32_t DEFAULT_RADIO_UNCHECKED_BORDER_COLOR = 0x33FFFFFFu;
constexpr uint32_t DEFAULT_RADIO_INDICATOR_COLOR = 0xFFFFFFFFu;
JsonValue BuildRadioChangeEventContext(bool isChecked)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = adapter->GetRoot();
    root.PutBool("isChecked", isChecked);
    return root;
}

uint32_t GetDefaultRadioCheckedBackgroundColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? RADIO_DARK_CHECKED_BACKGROUND_COLOR : RADIO_LIGHT_CHECKED_BACKGROUND_COLOR;
}

SurfaceSlot* FindOwningSurface(const RenderContext& renderContext)
{
    if (renderContext.surfaceId.empty()) {
        return nullptr;
    }
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderContext.renderId);
    if (renderSlot == nullptr) {
        return nullptr;
    }
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return nullptr;
    }
    return surfaceManager->FindSurface(renderContext.surfaceId);
}

} // namespace

ExtendedRadioComponent::ExtendedRadioComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::RADIO))
{
    if (nativeView_ != nullptr) {
        ArkUIOHApiAdapter::SetCrossLanguageOption(nativeView_, true);
        ArkUINodeApiAdapter::AddNodeEventReceiver(nativeView_, ExtendedRadioComponent::NodeEventReceiver);
    }
    ApplyRadioStyle();
}

ExtendedRadioComponent::~ExtendedRadioComponent()
{
    if (nativeView_ == nullptr) {
        return;
    }
    if (changeEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, A2UINodeEventType::RADIO_ON_CHANGE);
    }
    ArkUINodeApiAdapter::RemoveNodeEventReceiver(nativeView_, ExtendedRadioComponent::NodeEventReceiver);
}

std::string ExtendedRadioComponent::GetType() const
{
    return "Radio";
}

void ExtendedRadioComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ApplyDeclaredPropertyOrFallback(descriptor, "value");
    ApplyDeclaredPropertyOrFallback(descriptor, "group");
    ApplyDeclaredPropertyOrFallback(descriptor, "checked");
    if (checked_) {
        SyncSiblingCheckedState();
    }
}

PropertyDeclaration ExtendedRadioComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedRadioComponent&)>> declarations = {
        { "value",
            [](ExtendedRadioComponent& component) {
                return PropertyDeclaration { .name = "value",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetValue(value.GetStringValue("")); } };
            } },
        { "checked",
            [](ExtendedRadioComponent& component) {
                return PropertyDeclaration { .name = "checked",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackBool = false,
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetChecked(value.GetBoolValue(false)); } };
            } },
        { "group",
            [](ExtendedRadioComponent& component) {
                return PropertyDeclaration { .name = "group",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetGroup(value.GetStringValue("")); } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

void ExtendedRadioComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (IsApplyingStyleDeltaUpdate() || !styles.IsObject()) {
        return;
    }
    if (styles.Has("uncheckedBorderColor")) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property styles.uncheckedBorderColor is undefined for Radio and has been ignored",
            "styles.uncheckedBorderColor");
    }
    const char* colorKeys[] = { "checkedBackgroundColor", "unCheckedBorderColor", "indicatorColor" };
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

void ExtendedRadioComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedRadioComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    ThemeMode themeMode = ResolveThemeMode();

    ApplyStyleColor(styles, "checkedBackgroundColor", GetDefaultRadioCheckedBackgroundColor(themeMode),
        checkedBackgroundColor_, checkedBackgroundColorOverridden_);
    ApplyStyleColor(styles, "unCheckedBorderColor", DEFAULT_RADIO_UNCHECKED_BORDER_COLOR, uncheckedBackgroundColor_,
        uncheckedBackgroundColorOverridden_);
    ApplyStyleColor(
        styles, "indicatorColor", DEFAULT_RADIO_INDICATOR_COLOR, indicatorColor_, indicatorColorOverridden_);
    ApplyRadioStyle();
}

void ExtendedRadioComponent::OnConfigChange(const ThemeContext& context)
{
    if (!checkedBackgroundColorOverridden_) {
        checkedBackgroundColor_ = GetDefaultRadioCheckedBackgroundColor(context.colorMode);
    }
    if (!uncheckedBackgroundColorOverridden_) {
        uncheckedBackgroundColor_ = DEFAULT_RADIO_UNCHECKED_BORDER_COLOR;
    }
    if (!indicatorColorOverridden_) {
        indicatorColor_ = DEFAULT_RADIO_INDICATOR_COLOR;
    }
    ApplyRadioStyle();
}

void ExtendedRadioComponent::RegisterComponentSpecificListeners()
{
    UpdateChangeEventRegistration();
}

void ExtendedRadioComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "value") {
        SetValue("");
        return;
    }
    if (propertyName == "group") {
        SetGroup("");
        return;
    }
    if (propertyName == "checked") {
        SetChecked(false);
        UpdateChangeEventRegistration();
        return;
    }
}

void ExtendedRadioComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* component = reinterpret_cast<ExtendedRadioComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (component != nullptr) {
        component->HandleNodeEvent(event);
    }
}

ThemeMode ExtendedRadioComponent::ResolveThemeMode() const
{
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        return themeManager->GetContext().colorMode;
    }
    return GetRenderContext().colorMode;
}

void ExtendedRadioComponent::ApplyStyleColor(
    const JsonValue& styles, const char* propertyName, uint32_t defaultColor, uint32_t& outColor, bool& overridden)
{
    bool hasProperty = styles.IsObject() && propertyName != nullptr && styles.Has(propertyName);
    if (!hasProperty && IsApplyingStyleDeltaUpdate()) {
        return;
    }

    if (hasProperty) {
        uint32_t color = outColor;
        if (ExtendedStyleResolver::ParseColor(styles.GetItem(propertyName), color)) {
            outColor = color;
            overridden = true;
        } else {
            outColor = defaultColor;
            overridden = false;
        }
        return;
    }

    outColor = defaultColor;
    overridden = false;
}

void ExtendedRadioComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    if (ArkUIOHApiAdapter::NodeEventGetEventType(event) != A2UINodeEventType::RADIO_ON_CHANGE) {
        return;
    }

    A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
    if (componentEvent == nullptr) {
        return;
    }
    HandleRadioChange(componentEvent->data[0].i32 == 1);
}

void ExtendedRadioComponent::HandleRadioChange(bool checked)
{
    if (checked == checked_) {
        return;
    }

    SetChecked(checked);
    if (checked_) {
        SyncSiblingCheckedState();
    }
    SyncCheckedToBoundDataModel(checked);
    DispatchEvent("onChange", BuildRadioChangeEventContext(checked));
}

void ExtendedRadioComponent::UpdateChangeEventRegistration()
{
    if (nativeView_ == nullptr) {
        changeEventRegistered_ = false;
        return;
    }
    // Radio selection changes must stay in sync with internal state even without listeners/bindings,
    // otherwise same-group selection and getRadioValue() will observe stale checked flags.
    if (!changeEventRegistered_) {
        ArkUINodeApiAdapter::RegisterNodeEvent(nativeView_, A2UINodeEventType::RADIO_ON_CHANGE, 0, this);
        changeEventRegistered_ = true;
        return;
    }
}

void ExtendedRadioComponent::SetChecked(bool checked)
{
    checked_ = checked;
    ArkUINodeApiAdapter::SetNodeRadioChecked(nativeView_, checked_);
}

void ExtendedRadioComponent::SetValue(const std::string& value)
{
    value_ = value;
    ArkUINodeApiAdapter::SetNodeRadioValue(nativeView_, value_);
}

void ExtendedRadioComponent::SetGroup(const std::string& group)
{
    group_ = group;
    ArkUINodeApiAdapter::SetNodeRadioGroup(nativeView_, group_);
}

void ExtendedRadioComponent::ApplyRadioStyle()
{
    ArkUINodeApiAdapter::SetNodeRadioStyle(
        nativeView_, checkedBackgroundColor_, uncheckedBackgroundColor_, indicatorColor_);
}

void ExtendedRadioComponent::SyncSiblingCheckedState()
{
    SurfaceSlot* surface = FindOwningSurface(GetRenderContext());
    if (surface == nullptr) {
        return;
    }

    surface->ForEachComponent([this](const std::shared_ptr<Component>& component) {
        std::shared_ptr<ExtendedRadioComponent> peer = std::dynamic_pointer_cast<ExtendedRadioComponent>(component);
        if (peer == nullptr || peer.get() == this) {
            return;
        }
        if (peer->group_ != group_ || !peer->checked_) {
            return;
        }
        peer->SetChecked(false);
        peer->SyncCheckedToBoundDataModel(false);
    });
}

std::string ExtendedRadioComponent::ResolveCheckedBindingPath() const
{
    const auto& bindings = GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "checked" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

void ExtendedRadioComponent::SyncCheckedToBoundDataModel(bool value)
{
    std::string bindingPath = ResolveCheckedBindingPath();
    if (bindingPath.empty()) {
        return;
    }

    const RenderContext& renderContext = GetRenderContext();
    std::shared_ptr<BindingEngine> bindingEngine = renderContext.bindingEngine;
    if (bindingEngine == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedRadioComponent::SyncCheckedToBoundDataModel: binding engine is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    if (renderContext.surfaceId.empty()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedRadioComponent::SyncCheckedToBoundDataModel: surfaceId is empty, componentId=%{public}s",
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
