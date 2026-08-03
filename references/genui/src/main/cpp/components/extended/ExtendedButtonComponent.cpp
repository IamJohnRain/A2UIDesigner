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

#include "ExtendedButtonComponent.h"

#include <cctype>
#include <cmath>
#include <functional>
#include <map>

#include "components/extended/ExtendedStyleResolver.h"
#include "functions/ActionParser.h"
#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_BUTTON_PADDING_VERTICAL = 8.0F;
constexpr float DEFAULT_BUTTON_PADDING_HORIZONTAL = 12.0F;
constexpr float DEFAULT_BUTTON_FONT_SIZE = 16.0F;
constexpr A2UIFontWeight DEFAULT_BUTTON_FONT_WEIGHT = A2UIFontWeight::W500;
constexpr uint32_t DEFAULT_BUTTON_LIGHT_TEXT_COLOR = 0xFF0A59F7u;
constexpr uint32_t DEFAULT_BUTTON_DARK_TEXT_COLOR = 0xFF5291FFu;
constexpr uint32_t DEFAULT_BUTTON_LIGHT_BACKGROUND_COLOR = 0x0C000000u;
constexpr uint32_t DEFAULT_BUTTON_DARK_BACKGROUND_COLOR = 0x19FFFFFFu;
constexpr char DEFAULT_FONT_SCALE_MODE[] = "followSystem";
float ClampMinFontScale(double value)
{
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    if (value < 0.0) {
        return 0.0F;
    }
    if (value > 1.0) {
        return 1.0F;
    }
    return static_cast<float>(value);
}

float ClampMaxFontScale(double value)
{
    if (!std::isfinite(value)) {
        return 0.0F;
    }
    if (value < 1.0) {
        return 1.0F;
    }
    return static_cast<float>(value);
}

uint32_t GetDefaultButtonFontColor(ThemeMode colorMode)
{
    return colorMode == ThemeMode::DARK ? DEFAULT_BUTTON_DARK_TEXT_COLOR : DEFAULT_BUTTON_LIGHT_TEXT_COLOR;
}

uint32_t GetDefaultButtonBackgroundColor(ThemeMode colorMode)
{
    return colorMode == ThemeMode::DARK ? DEFAULT_BUTTON_DARK_BACKGROUND_COLOR : DEFAULT_BUTTON_LIGHT_BACKGROUND_COLOR;
}

bool IsValidButtonFontWeightString(const std::string& value)
{
    std::string token = StyleApplyUtils::TrimToken(value);
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return token == "normal" || token == "regular" || token == "medium" || token == "bold" || token == "bolder";
}

bool IsValidButtonFontWeightSchemaValue(const JsonValue& value)
{
    if (value.IsString()) {
        return IsValidButtonFontWeightString(value.GetStringValue(""));
    }
    int32_t fontWeight = static_cast<int32_t>(A2UIFontWeight::NORMAL);
    return StyleApplyUtils::ParseFontWeight(value, fontWeight);
}

} // namespace

ExtendedButtonComponent::ExtendedButtonComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::BUTTON), true, true)
{
    if (nativeView_ != nullptr) {
        ArkUIOHApiAdapter::SetCrossLanguageOption(nativeView_, true);
    }
    SetLabel("");
    SetFontSize(fontSize_);
    SetFontWeight(fontWeight_);
    SetPadding(DEFAULT_BUTTON_PADDING_VERTICAL, DEFAULT_BUTTON_PADDING_HORIZONTAL, DEFAULT_BUTTON_PADDING_VERTICAL,
        DEFAULT_BUTTON_PADDING_HORIZONTAL);
}

std::string ExtendedButtonComponent::GetType() const
{
    return "Button";
}

void ExtendedButtonComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    if (descriptor.IsObject() && descriptor.Has("text") && !descriptor.Has("label")) {
        ApplyDeclaredPropertyOrFallback(descriptor, "text");
    } else {
        ApplyDeclaredPropertyOrFallback(descriptor, "label");
    }
    ApplyDeclaredPropertyOrFallback(descriptor, "enabled");
    if (descriptor.IsObject() && descriptor.Has("action")) {
        SetPropertyFromDescriptor("action", descriptor);
    } else {
        RemoveProperty("action");
    }
}

void ExtendedButtonComponent::ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor)
{
    static_cast<void>(normalizedDescriptor);
    LOG_A2UI(LOG_DEBUG,
        "ExtendedButtonComponent::ApplyComponentSpecificAttributes - componentId=%{public}s, renderId=%{public}d",
        GetComponentId().c_str(), GetRenderId());
}

PropertyDeclaration ExtendedButtonComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ExtendedButtonComponent&)>> declarations = {
        { "text",
            [](ExtendedButtonComponent& component) {
                return PropertyDeclaration { .name = "text",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetLabel(value.GetStringValue("")); } };
            } },
        { "label",
            [](ExtendedButtonComponent& component) {
                return PropertyDeclaration { .name = "label",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackString = "",
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetLabel(value.GetStringValue("")); } };
            } },
        { "enabled",
            [](ExtendedButtonComponent& component) {
                return PropertyDeclaration { .name = "enabled",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .allowExpression = true,
                    .fallbackBool = true,
                    .applyValue = [&component](
                                      const JsonValue& value) { component.SetEnabled(value.GetBoolValue(true)); } };
            } },
        { "action",
            [](ExtendedButtonComponent& component) {
                return PropertyDeclaration { .name = "action",
                    .type = PropertyValueType::OBJECT,
                    .allowDynamic = false,
                    .applyValue = [&component](const JsonValue& value) { component.SetAction(value); } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

std::vector<std::string> ExtendedButtonComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "label" };
}

void ExtendedButtonComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (!styles.IsObject()) {
        return;
    }

    auto reportInvalidNumber = [this, &styles](const char* propertyName) {
        if (propertyName == nullptr || !styles.Has(propertyName)) {
            return;
        }
        JsonValue value = styles.GetItem(propertyName);
        if (IsDynamicValueDescriptor(value)) {
            return;
        }
        float parsed = 0.0F;
        if (!StyleApplyUtils::ParseNumber(value, parsed)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles." + std::string(propertyName) +
                    " expects positive number, fallback/reset has been applied",
                "styles." + std::string(propertyName));
            return;
        }
        if (parsed <= 0.0F) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles." + std::string(propertyName) +
                    " expects positive number, fallback/reset has been applied",
                "styles." + std::string(propertyName));
        }
    };

    reportInvalidNumber("fontSize");
    reportInvalidNumber("minFontSize");
    reportInvalidNumber("maxFontSize");

    if (styles.Has("fontWeight")) {
        JsonValue fontWeightValue = styles.GetItem("fontWeight");
        if (IsDynamicValueDescriptor(fontWeightValue)) {
            // Dynamic styles are validated after resolution by DFX fallback/reset paths.
        } else if (!fontWeightValue.IsString() && !fontWeightValue.IsNumber()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.fontWeight expects string or number, fallback/reset has been applied",
                "styles.fontWeight");
        } else if (!IsValidButtonFontWeightSchemaValue(fontWeightValue)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.fontWeight expects valid font weight, fallback/reset has been applied",
                "styles.fontWeight");
        }
    }

    auto reportMinFontScale = [this, &styles]() {
        constexpr const char* propertyName = "minFontScale";
        if (!styles.Has(propertyName)) {
            return;
        }
        JsonValue value = styles.GetItem(propertyName);
        if (IsDynamicValueDescriptor(value)) {
            return;
        }
        if (!value.IsNumber()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.minFontScale expects number, fallback/reset has been applied", "styles.minFontScale");
            return;
        }
        double number = value.GetNumberValue(0.0);
        if (!std::isfinite(number) || number < 0.0 || number > 1.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.minFontScale expects number in range [0, 1], fallback/reset has been applied",
                "styles.minFontScale");
        }
    };
    auto reportMaxFontScale = [this, &styles]() {
        constexpr const char* propertyName = "maxFontScale";
        if (!styles.Has(propertyName)) {
            return;
        }
        JsonValue value = styles.GetItem(propertyName);
        if (IsDynamicValueDescriptor(value)) {
            return;
        }
        if (!value.IsNumber()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.maxFontScale expects number, fallback/reset has been applied", "styles.maxFontScale");
            return;
        }
        double number = value.GetNumberValue(0.0);
        if (!std::isfinite(number) || number < 1.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.maxFontScale expects number greater than or equal to 1, fallback/reset has been "
                "applied",
                "styles.maxFontScale");
        }
    };
    reportMinFontScale();
    reportMaxFontScale();

    if (styles.Has("fontScaleMode")) {
        JsonValue fontScaleModeValue = styles.GetItem("fontScaleMode");
        if (IsDynamicValueDescriptor(fontScaleModeValue)) {
            // Dynamic styles are validated after resolution by DFX fallback/reset paths.
        } else if (!fontScaleModeValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.fontScaleMode expects string, fallback to followSystem", "styles.fontScaleMode");
        } else {
            std::string mode = StyleApplyUtils::TrimToken(fontScaleModeValue.GetStringValue(""));
            if (mode != "custom" && mode != "followSystem") {
                ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.fontScaleMode expects custom/followSystem, fallback to followSystem",
                    "styles.fontScaleMode");
            }
        }
    }

    if (styles.Has("fontColor")) {
        uint32_t color = 0;
        JsonValue fontColorValue = styles.GetItem("fontColor");
        if (IsDynamicValueDescriptor(fontColorValue)) {
            return;
        }
        if (!fontColorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.fontColor expects string color, fallback/reset has been applied", "styles.fontColor");
            return;
        }
        if (!ExtendedStyleResolver::ParseColor(fontColorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.fontColor expects color value, fallback/reset has been applied", "styles.fontColor");
        }
    }
}

void ExtendedButtonComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedButtonComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();

    float fontSize = 0.0F;
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("fontSize"))) {
        if (StyleApplyUtils::ParseNumber(styles.GetItem("fontSize"), fontSize) && fontSize > 0.0F) {
            LOG_A2UI(LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontSize=%{public}f",
                fontSize);
            SetFontSize(fontSize);
        } else {
            SetFontSize(DEFAULT_BUTTON_FONT_SIZE);
        }
    }

    int32_t fontWeight = static_cast<int32_t>(A2UIFontWeight::NORMAL);
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("fontWeight"))) {
        if (StyleApplyUtils::ParseFontWeight(styles.GetItem("fontWeight"), fontWeight)) {
            LOG_A2UI(LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontWeight=%{public}d",
                fontWeight);
            SetFontWeight(static_cast<A2UIFontWeight>(fontWeight));
        } else {
            SetFontWeight(DEFAULT_BUTTON_FONT_WEIGHT);
        }
    }

    float minFontSize = 0.0F;
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("minFontSize"))) {
        if (StyleApplyUtils::ParseNumber(styles.GetItem("minFontSize"), minFontSize) && minFontSize > 0.0F) {
            SetMinFontSize(minFontSize);
        } else {
            SetMinFontSize(0.0F);
        }
    }

    float maxFontSize = 0.0F;
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("maxFontSize"))) {
        if (StyleApplyUtils::ParseNumber(styles.GetItem("maxFontSize"), maxFontSize) && maxFontSize > 0.0F) {
            SetMaxFontSize(maxFontSize);
        } else {
            SetMaxFontSize(0.0F);
        }
    }

    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("minFontScale"))) {
        JsonValue minFontScaleValue = styles.GetItem("minFontScale");
        if (minFontScaleValue.IsNumber()) {
            SetMinFontScale(ClampMinFontScale(minFontScaleValue.GetNumberValue(0.0)));
        } else {
            SetMinFontScale(0.0F);
        }
    }

    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("maxFontScale"))) {
        JsonValue maxFontScaleValue = styles.GetItem("maxFontScale");
        if (maxFontScaleValue.IsNumber()) {
            SetMaxFontScale(ClampMaxFontScale(maxFontScaleValue.GetNumberValue(1.0)));
        } else {
            SetMaxFontScale(0.0F);
        }
    }

    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("fontScaleMode"))) {
        JsonValue fontScaleModeValue = styles.GetItem("fontScaleMode");
        if (fontScaleModeValue.IsString()) {
            std::string mode = StyleApplyUtils::TrimToken(fontScaleModeValue.GetStringValue(""));
            if (mode == "custom" || mode == "followSystem") {
                SetFontScaleMode(mode);
            } else {
                SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
            }
        } else {
            SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
        }
    }

    uint32_t fontColor = 0;
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("fontColor"))) {
        JsonValue fontColorValue = styles.GetItem("fontColor");
        if (ExtendedStyleResolver::ParseColor(fontColorValue, fontColor)) {
            LOG_A2UI(LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontColor=%{public}u",
                fontColor);
            SetFontColor(fontColor, true);
        } else {
            ApplyDefaultFontColor();
        }
    }

    uint32_t backgroundColor = 0;
    if (!isDeltaUpdate || (styles.IsObject() && styles.Has("backgroundColor"))) {
        JsonValue backgroundColorValue = styles.GetItem("backgroundColor");
        if (ExtendedStyleResolver::ParseColor(backgroundColorValue, backgroundColor)) {
            LOG_A2UI(LOG_DEBUG,
                "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply backgroundColor=%{public}u",
                backgroundColor);
            SetBackgroundColor(backgroundColor, true);
        } else {
            ApplyDefaultBackgroundColor();
        }
    }
}

void ExtendedButtonComponent::OnConfigChange(const ThemeContext& context)
{
    if (!hasFontColor_) {
        SetFontColor(GetDefaultButtonFontColor(context.colorMode), false);
    }
    if (!hasBackgroundColor_) {
        SetBackgroundColor(GetDefaultButtonBackgroundColor(context.colorMode), false);
    }
}

void ExtendedButtonComponent::RegisterClickHandler()
{
    ClickHandler onClick;
    if (actionInfo_ != nullptr && actionInfo_->IsValid()) {
        onClick = [this](const JsonValue& context) { DispatchActionInfo("action", actionInfo_, context); };
    } else if (HasEventHandler("onClick")) {
        onClick = [this](const JsonValue& context) { DispatchEvent("onClick", context); };
    }

    if (!onClick) {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedButtonComponent::RegisterClickHandler - clear click listener, componentId=%{public}s",
            GetComponentId().c_str());
        RegisterOnClickWithContext(nullptr);
        return;
    }

    LOG_A2UI(LOG_DEBUG,
        "ExtendedButtonComponent::RegisterClickHandler - register click listener, componentId=%{public}s",
        GetComponentId().c_str());
    RegisterOnClickWithContext(onClick);
}

void ExtendedButtonComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "text" || propertyName == "label") {
        SetLabel("");
        return;
    }
    if (propertyName == "enabled") {
        SetEnabled(true);
        return;
    }
    if (propertyName == "action") {
        ClearAction();
        return;
    }
}

void ExtendedButtonComponent::SetLabel(const std::string& label)
{
    labelValue_ = label;
    ArkUINodeApiAdapter::SetNodeButtonLabel(nativeView_, labelValue_);
}

void ExtendedButtonComponent::SetFontSize(float fontSize)
{
    fontSize_ = fontSize;
    float effectiveSize = ComputeEffectiveFontSize(fontSize_);
    ArkUINodeApiAdapter::SetNodeFontSize(nativeView_, effectiveSize);
}

void ExtendedButtonComponent::SetFontWeight(A2UIFontWeight fontWeight)
{
    fontWeight_ = fontWeight;
    ArkUINodeApiAdapter::SetNodeFontWeight(nativeView_, fontWeight_);
}

void ExtendedButtonComponent::SetEnabled(bool enabled)
{
    enabled_ = enabled;
    ArkUINodeApiAdapter::SetNodeEnabled(nativeView_, enabled_);
}

void ExtendedButtonComponent::SetFontColor(uint32_t color, bool userOverride)
{
    hasFontColor_ = userOverride;
    fontColor_ = color;
    ArkUINodeApiAdapter::SetNodeFontColor(nativeView_, color);
}

void ExtendedButtonComponent::ApplyDefaultFontColor()
{
    ThemeMode colorMode = GetRenderContext().colorMode;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        colorMode = themeManager->GetContext().colorMode;
    }
    SetFontColor(GetDefaultButtonFontColor(colorMode), false);
}

void ExtendedButtonComponent::ResetFontColor()
{
    hasFontColor_ = false;
    fontColor_ = 0;
    ResetNodeFontColor();
}

void ExtendedButtonComponent::SetBackgroundColor(uint32_t color, bool userOverride)
{
    hasBackgroundColor_ = userOverride;
    backgroundColor_ = color;
    ArkUINodeApiAdapter::SetNodeBackgroundColor(nativeView_, color);
}

void ExtendedButtonComponent::ApplyDefaultBackgroundColor()
{
    ThemeMode colorMode = GetRenderContext().colorMode;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        colorMode = themeManager->GetContext().colorMode;
    }
    SetBackgroundColor(GetDefaultButtonBackgroundColor(colorMode), false);
}

void ExtendedButtonComponent::SetMinFontSize(float minFontSize)
{
    minFontSize_ = minFontSize > 0.0F ? minFontSize : 0.0F;
    if (minFontSize_ <= 0.0F) {
        ResetNodeTextMinFontSize();
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextMinFontSize(nativeView_, minFontSize_);
}

void ExtendedButtonComponent::SetMaxFontSize(float maxFontSize)
{
    maxFontSize_ = maxFontSize > 0.0F ? maxFontSize : 0.0F;
    if (maxFontSize_ <= 0.0F) {
        ResetNodeTextMaxFontSize();
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextMaxFontSize(nativeView_, maxFontSize_);
}

void ExtendedButtonComponent::SetMinFontScale(float minFontScale)
{
    minFontScale_ = minFontScale > 0.0F ? minFontScale : 0.0F;
    if (minFontScale_ <= 0.0F) {
        ResetNodeButtonMinFontScale();
        return;
    }

    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeButtonMinFontScale(nativeView_, minFontScale_);
}

void ExtendedButtonComponent::SetMaxFontScale(float maxFontScale)
{
    maxFontScale_ = maxFontScale > 0.0F ? maxFontScale : 0.0F;
    if (maxFontScale_ <= 0.0F) {
        ResetNodeButtonMaxFontScale();
        return;
    }

    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeButtonMaxFontScale(nativeView_, maxFontScale_);
}

void ExtendedButtonComponent::ResetNodeFontColor()
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeFontColor(nativeView_);
}

void ExtendedButtonComponent::ResetNodeTextMinFontSize()
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeTextMinFontSize(nativeView_);
}

void ExtendedButtonComponent::ResetNodeTextMaxFontSize()
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(nativeView_);
}

void ExtendedButtonComponent::ResetNodeButtonMinFontScale()
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeButtonMinFontScale(nativeView_);
}

void ExtendedButtonComponent::ResetNodeButtonMaxFontScale()
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::ResetNodeButtonMaxFontScale(nativeView_);
}

void ExtendedButtonComponent::SetFontScaleMode(const std::string& mode)
{
    fontScaleMode_ = mode == "custom" ? "custom" : DEFAULT_FONT_SCALE_MODE;
    SetFontSize(fontSize_);
}

void ExtendedButtonComponent::SetAction(const JsonValue& actionValue)
{
    actionInfo_ = ActionParser::Parse(actionValue);
    if (actionInfo_ == nullptr || !actionInfo_->IsValid()) {
        LOG_A2UI(LOG_WARN, "ExtendedButtonComponent::SetAction - invalid action, componentId=%{public}s",
            GetComponentId().c_str());
        ClearAction();
    }
}

void ExtendedButtonComponent::ClearAction()
{
    actionInfo_.reset();
}

void ExtendedButtonComponent::OnFontSizeScaleChanged(float newScale)
{
    ExtendedComponent::OnFontSizeScaleChanged(newScale);
    SetFontSize(fontSize_);
}

float ExtendedButtonComponent::ComputeEffectiveFontSize(float baseFontSize) const
{
    if (fontScaleMode_ == "custom") {
        const RenderContext& ctx = GetRenderContext();
        float scale = ctx.fontSizeScale > 0.0F ? ctx.fontSizeScale : 1.0F;
        return baseFontSize * scale;
    }
    return baseFontSize;
}

} // namespace NativeModule
