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
#include <limits>
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

bool ParseButtonFontWeight(const JsonValue& value, int32_t& fontWeight)
{
    if (value.IsString()) {
        if (!IsValidButtonFontWeightString(value.GetStringValue(""))) {
            return false;
        }
        return StyleApplyUtils::ParseFontWeight(value, fontWeight);
    }
    if (!value.IsNumber()) {
        return false;
    }

    constexpr int32_t minFontWeight = 100;
    constexpr int32_t maxFontWeight = 900;
    constexpr int32_t fontWeightStep = 100;
    double rawValue = value.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(rawValue)) {
        return false;
    }
    double flooredValue = std::floor(rawValue);
    if (flooredValue < minFontWeight || flooredValue > maxFontWeight) {
        return false;
    }
    int32_t normalized = static_cast<int32_t>(flooredValue);
    if (normalized % fontWeightStep != 0) {
        return false;
    }
    fontWeight = (normalized / fontWeightStep) - 1;
    return true;
}

bool IsValidButtonFontWeightSchemaValue(const JsonValue& value)
{
    int32_t fontWeight = static_cast<int32_t>(DEFAULT_BUTTON_FONT_WEIGHT);
    return ParseButtonFontWeight(value, fontWeight);
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

    ValidatePositiveNumberStyle(styles, "fontSize");
    ValidatePositiveNumberStyle(styles, "minFontSize");
    ValidatePositiveNumberStyle(styles, "maxFontSize");
    ValidateFontWeightStyle(styles);
    ValidateFontScaleStyle(styles, "minFontScale", 0.0, 1.0, true);
    ValidateFontScaleStyle(styles, "maxFontScale", 1.0, 0.0, false);
    ValidateFontScaleModeStyle(styles);
    ValidateFontColorStyle(styles);
}

void ExtendedButtonComponent::ValidatePositiveNumberStyle(const JsonValue& styles, const char* propertyName)
{
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
}

void ExtendedButtonComponent::ValidateFontWeightStyle(const JsonValue& styles)
{
    if (!styles.Has("fontWeight")) {
        return;
    }
    JsonValue value = styles.GetItem("fontWeight");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString() && !value.IsNumber()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.fontWeight expects string or number, fallback/reset has been applied",
            "styles.fontWeight");
        return;
    }
    if (!IsValidButtonFontWeightSchemaValue(value)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.fontWeight expects valid font weight, fallback/reset has been applied",
            "styles.fontWeight");
    }
}

void ExtendedButtonComponent::ValidateFontScaleStyle(
    const JsonValue& styles, const char* propertyName, double minValue, double maxValue, bool hasMaxValue)
{
    if (propertyName == nullptr || !styles.Has(propertyName)) {
        return;
    }
    JsonValue value = styles.GetItem(propertyName);
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsNumber()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles." + std::string(propertyName) + " expects number, fallback/reset has been applied",
            "styles." + std::string(propertyName));
        return;
    }
    double number = value.GetNumberValue(0.0);
    if (std::isfinite(number) && number >= minValue && (!hasMaxValue || number <= maxValue)) {
        return;
    }
    std::string message = "Property styles.maxFontScale expects number greater than or equal to 1, "
                          "fallback/reset has been applied";
    if (hasMaxValue) {
        message = "Property styles.minFontScale expects number in range [0, 1], fallback/reset has been applied";
    }
    ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, message, "styles." + std::string(propertyName));
}

void ExtendedButtonComponent::ValidateFontScaleModeStyle(const JsonValue& styles)
{
    if (!styles.Has("fontScaleMode")) {
        return;
    }
    JsonValue value = styles.GetItem("fontScaleMode");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.fontScaleMode expects string, fallback to followSystem", "styles.fontScaleMode");
        return;
    }
    std::string mode = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    if (mode != "custom" && mode != "followSystem") {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.fontScaleMode expects custom/followSystem, fallback to followSystem",
            "styles.fontScaleMode");
    }
}

void ExtendedButtonComponent::ValidateFontColorStyle(const JsonValue& styles)
{
    if (!styles.Has("fontColor")) {
        return;
    }
    JsonValue value = styles.GetItem("fontColor");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.fontColor expects string color, fallback/reset has been applied", "styles.fontColor");
        return;
    }
    uint32_t color = 0;
    if (!ExtendedStyleResolver::ParseColor(value, color)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.fontColor expects color value, fallback/reset has been applied", "styles.fontColor");
    }
}

void ExtendedButtonComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

void ExtendedButtonComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    ApplyFontSizeStyle(styles);
    ApplyFontWeightStyle(styles);
    ApplyMinFontSizeStyle(styles);
    ApplyMaxFontSizeStyle(styles);
    ApplyFontScaleLimits(styles);
    ApplyFontScaleModeStyle(styles);
    ApplyFontColorStyle(styles);
    ApplyBackgroundColorStyle(styles);
}

bool ExtendedButtonComponent::ShouldApplyStyle(const JsonValue& styles, const char* styleName) const
{
    return !IsApplyingStyleDeltaUpdate() || (styles.IsObject() && styleName != nullptr && styles.Has(styleName));
}

void ExtendedButtonComponent::ApplyFontSizeStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "fontSize")) {
        return;
    }
    float fontSize = 0.0F;
    if (!StyleApplyUtils::ParseNumber(styles.GetItem("fontSize"), fontSize) || fontSize <= 0.0F) {
        SetFontSize(DEFAULT_BUTTON_FONT_SIZE);
        return;
    }
    LOG_A2UI(LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontSize=%{public}f", fontSize);
    SetFontSize(fontSize);
}

void ExtendedButtonComponent::ApplyFontWeightStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "fontWeight")) {
        return;
    }
    int32_t fontWeight = static_cast<int32_t>(DEFAULT_BUTTON_FONT_WEIGHT);
    if (!ParseButtonFontWeight(styles.GetItem("fontWeight"), fontWeight)) {
        SetFontWeight(DEFAULT_BUTTON_FONT_WEIGHT);
        return;
    }
    LOG_A2UI(
        LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontWeight=%{public}d", fontWeight);
    SetFontWeight(static_cast<A2UIFontWeight>(fontWeight));
}

void ExtendedButtonComponent::ApplyMinFontSizeStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "minFontSize")) {
        return;
    }
    float minFontSize = 0.0F;
    if (StyleApplyUtils::ParseNumber(styles.GetItem("minFontSize"), minFontSize) && minFontSize > 0.0F) {
        SetMinFontSize(minFontSize);
        return;
    }
    SetMinFontSize(0.0F);
}

void ExtendedButtonComponent::ApplyMaxFontSizeStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "maxFontSize")) {
        return;
    }
    float maxFontSize = 0.0F;
    if (StyleApplyUtils::ParseNumber(styles.GetItem("maxFontSize"), maxFontSize) && maxFontSize > 0.0F) {
        SetMaxFontSize(maxFontSize);
        return;
    }
    SetMaxFontSize(0.0F);
}

void ExtendedButtonComponent::ApplyFontScaleLimits(const JsonValue& styles)
{
    if (ShouldApplyStyle(styles, "minFontScale")) {
        JsonValue value = styles.GetItem("minFontScale");
        if (value.IsNumber()) {
            SetMinFontScale(ClampMinFontScale(value.GetNumberValue(0.0)));
        } else if (value.IsValid()) {
            SetMinFontScale(0.0F);
        }
    }
    if (ShouldApplyStyle(styles, "maxFontScale")) {
        JsonValue value = styles.GetItem("maxFontScale");
        if (value.IsNumber()) {
            SetMaxFontScale(ClampMaxFontScale(value.GetNumberValue(1.0)));
        } else if (value.IsValid()) {
            SetMaxFontScale(0.0F);
        }
    }
}

void ExtendedButtonComponent::ApplyFontScaleModeStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "fontScaleMode")) {
        return;
    }
    JsonValue value = styles.GetItem("fontScaleMode");
    if (!value.IsString()) {
        SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
        return;
    }
    std::string mode = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    if (mode != "custom" && mode != "followSystem") {
        mode = DEFAULT_FONT_SCALE_MODE;
    }
    SetFontScaleMode(mode);
}

void ExtendedButtonComponent::ApplyFontColorStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "fontColor")) {
        return;
    }
    uint32_t color = 0;
    if (!ExtendedStyleResolver::ParseColor(styles.GetItem("fontColor"), color)) {
        ApplyDefaultFontColor();
        return;
    }
    LOG_A2UI(LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply fontColor=%{public}u", color);
    SetFontColor(color, true);
}

void ExtendedButtonComponent::ApplyBackgroundColorStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "backgroundColor")) {
        return;
    }
    uint32_t color = 0;
    if (!ExtendedStyleResolver::ParseColor(styles.GetItem("backgroundColor"), color)) {
        ApplyDefaultBackgroundColor();
        return;
    }
    LOG_A2UI(
        LOG_DEBUG, "ExtendedButtonComponent::ApplyComponentSpecificStyles - apply backgroundColor=%{public}u", color);
    SetBackgroundColor(color, true);
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
