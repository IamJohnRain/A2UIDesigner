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

#include "ExtendedTextInputComponent.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>

#include "components/extended/ExtendedStyleResolver.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "ArkUINodeApiAdapter.h"
#include "ArkUIOHApiAdapter.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr A2UITextInputType TEXT_INPUT_TYPE_PHONE_NUMBER = A2UITextInputType::PHONE_NUMBER;
constexpr A2UITextInputType TEXT_INPUT_TYPE_EMAIL = A2UITextInputType::EMAIL;
constexpr A2UITextInputType TEXT_INPUT_TYPE_NUMBER_PASSWORD = A2UITextInputType::NUMBER_PASSWORD;
constexpr A2UITextInputType TEXT_INPUT_TYPE_SCREEN_LOCK_PASSWORD = A2UITextInputType::SCREEN_LOCK_PASSWORD;
constexpr A2UITextInputType TEXT_INPUT_TYPE_USER_NAME = A2UITextInputType::USER_NAME;
constexpr A2UITextInputType TEXT_INPUT_TYPE_NEW_PASSWORD = A2UITextInputType::NEW_PASSWORD;
constexpr A2UITextInputType TEXT_INPUT_TYPE_NUMBER_DECIMAL = A2UITextInputType::NUMBER_DECIMAL;
constexpr A2UITextInputType TEXT_INPUT_TYPE_ONE_TIME_CODE = A2UITextInputType::ONE_TIME_CODE;

constexpr int32_t DEFAULT_TEXT_ALIGN = 0;
constexpr A2UIWordBreak DEFAULT_WORD_BREAK = A2UIWordBreak::NORMAL;
constexpr int32_t DEFAULT_MAX_LENGTH = std::numeric_limits<int32_t>::max();
constexpr uint32_t TEXT_INPUT_LIGHT_FONT_COLOR = 0xFF182431u;
constexpr uint32_t TEXT_INPUT_DARK_FONT_COLOR = 0xE5FFFFFFu;
constexpr uint32_t TEXT_INPUT_LIGHT_PLACEHOLDER_COLOR = 0x99182431u;
constexpr uint32_t TEXT_INPUT_DARK_PLACEHOLDER_COLOR = 0x99FFFFFFu;
constexpr uint32_t TEXT_INPUT_LIGHT_CARET_COLOR = 0xFF007DFFu;
constexpr uint32_t TEXT_INPUT_DARK_CARET_COLOR = 0xFF5291FFu;
constexpr uint32_t TEXT_INPUT_LIGHT_BACKGROUND_COLOR = 0x0C000000u;
constexpr uint32_t TEXT_INPUT_DARK_BACKGROUND_COLOR = 0x19FFFFFFu;
constexpr uint32_t TEXT_INPUT_LIGHT_SELECTED_BACKGROUND_COLOR = 0x33007DFFu;
constexpr uint32_t TEXT_INPUT_DARK_SELECTED_BACKGROUND_COLOR = 0x33006CDEu;
constexpr uint32_t TEXT_INPUT_LIGHT_UNDERLINE_COLOR = 0x33182431u;
constexpr uint32_t TEXT_INPUT_DARK_UNDERLINE_COLOR = 0x33FFFFFFu;
constexpr char DEFAULT_FONT_SCALE_MODE[] = "followSystem";
JsonValue BuildTextInputChangeEventContext(const std::string& value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }
    JsonValue root = adapter->GetRoot();
    root.PutString("value", value);
    return root;
}

uint32_t GetDefaultTextInputFontColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_FONT_COLOR : TEXT_INPUT_LIGHT_FONT_COLOR;
}

uint32_t GetDefaultTextInputPlaceholderColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_PLACEHOLDER_COLOR : TEXT_INPUT_LIGHT_PLACEHOLDER_COLOR;
}

uint32_t GetDefaultTextInputCaretColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_CARET_COLOR : TEXT_INPUT_LIGHT_CARET_COLOR;
}

uint32_t GetDefaultTextInputBackgroundColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_BACKGROUND_COLOR : TEXT_INPUT_LIGHT_BACKGROUND_COLOR;
}

uint32_t GetDefaultTextInputSelectedBackgroundColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_SELECTED_BACKGROUND_COLOR
                                   : TEXT_INPUT_LIGHT_SELECTED_BACKGROUND_COLOR;
}

uint32_t GetDefaultTextInputUnderlineColor(ThemeMode mode)
{
    return mode == ThemeMode::DARK ? TEXT_INPUT_DARK_UNDERLINE_COLOR : TEXT_INPUT_LIGHT_UNDERLINE_COLOR;
}

bool IsSupportedCancelButtonMember(const std::string& key)
{
    return key == "style" || key == "fontSize" || key == "fontColor";
}

bool IsSupportedUnderlineColorMember(const std::string& key)
{
    return key == "typing" || key == "normal" || key == "error" || key == "disable";
}

float GetFontDensityScale()
{
    float densityPixels = 0.0F;
    if (ArkUIOHApiAdapter::GetDefaultDisplayDensityPixels(&densityPixels) != DISPLAY_MANAGER_OK) {
        return 1.0F;
    }
    float scaledDensity = 0.0F;
    if (ArkUIOHApiAdapter::GetDefaultDisplayScaledDensity(&scaledDensity) != DISPLAY_MANAGER_OK) {
        return 1.0F;
    }
    if (!std::isfinite(densityPixels) || densityPixels <= 0.0F) {
        return 1.0F;
    }
    if (!std::isfinite(scaledDensity) || scaledDensity <= 0.0F) {
        return 1.0F;
    }
    return scaledDensity / densityPixels;
}

bool ConvertDimensionToFloat(const StyleDimension& dimension, float& value)
{
    switch (dimension.unit) {
        case StyleDimensionUnit::VP:
            value = dimension.value;
            return true;
        case StyleDimensionUnit::FP:
            value = dimension.value * GetFontDensityScale();
            return std::isfinite(value);
        default:
            return false;
    }
}

std::string NormalizeToken(const std::string& rawValue)
{
    std::string trimmed = StyleApplyUtils::TrimToken(rawValue);
    std::string normalized;
    normalized.reserve(trimmed.size());
    for (char ch : trimmed) {
        if (ch == '-' || ch == '_' || ch == ' ') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool IsValidFontWeightNumber(double value)
{
    constexpr int32_t minFontWeight = 100;
    constexpr int32_t maxFontWeight = 900;
    constexpr int32_t fontWeightStep = 100;
    constexpr double epsilon = 0.0001;

    if (!std::isfinite(value)) {
        return false;
    }
    int32_t normalized = static_cast<int32_t>(std::lround(value));
    return std::fabs(value - static_cast<double>(normalized)) <= epsilon && normalized >= minFontWeight &&
           normalized <= maxFontWeight && normalized % fontWeightStep == 0;
}

bool IsValidTextInputFontWeight(const JsonValue& value)
{
    if (value.IsNumber()) {
        return IsValidFontWeightNumber(value.GetNumberValue(0.0));
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = NormalizeToken(value.GetStringValue(""));
    if (token == "lighter" || token == "normal" || token == "regular" || token == "medium" || token == "bold" ||
        token == "bolder") {
        return true;
    }

    float parsedNumber = 0.0F;
    return StyleApplyUtils::ParseNumber(value, parsedNumber) && IsValidFontWeightNumber(parsedNumber);
}

bool IsValidCancelButtonStyleToken(const JsonValue& value)
{
    if (!value.IsString()) {
        return false;
    }

    std::string token = NormalizeToken(value.GetStringValue(""));
    return token == "constant" || token == "invisible" || token == "input";
}

bool TryParseCancelButtonFontSize(const JsonValue& value, float& fontSize)
{
    StyleDimension dimension;
    return StyleApplyUtils::ParseDimension(value, dimension) && ConvertDimensionToFloat(dimension, fontSize);
}

bool IsNestedDynamicDescriptor(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

A2UITextInputType ParseTextInputTypeToken(const std::string& rawValue)
{
    static const std::map<std::string, A2UITextInputType> inputTypes = {
        { "number", A2UITextInputType::NUMBER },
        { "phonenumber", TEXT_INPUT_TYPE_PHONE_NUMBER },
        { "email", TEXT_INPUT_TYPE_EMAIL },
        { "password", A2UITextInputType::PASSWORD },
        { "numberpassword", TEXT_INPUT_TYPE_NUMBER_PASSWORD },
        { "screenlockpassword", TEXT_INPUT_TYPE_SCREEN_LOCK_PASSWORD },
        { "username", TEXT_INPUT_TYPE_USER_NAME },
        { "newpassword", TEXT_INPUT_TYPE_NEW_PASSWORD },
        { "numberdecimal", TEXT_INPUT_TYPE_NUMBER_DECIMAL },
        { "onetimecode", TEXT_INPUT_TYPE_ONE_TIME_CODE },
    };
    std::string token = NormalizeToken(rawValue);
    auto it = inputTypes.find(token);
    return it == inputTypes.end() ? A2UITextInputType::NORMAL : it->second;
}

bool ParseWordBreak(const JsonValue& value, A2UIWordBreak& wordBreak)
{
    if (!value.IsString()) {
        return false;
    }
    std::string token = NormalizeToken(value.GetStringValue(""));
    if (token == "normal") {
        wordBreak = A2UIWordBreak::NORMAL;
        return true;
    }
    if (token == "breakall") {
        wordBreak = A2UIWordBreak::BREAK_ALL;
        return true;
    }
    if (token == "breakword") {
        wordBreak = A2UIWordBreak::BREAK_WORD;
        return true;
    }
    if (token == "hyphenation") {
        wordBreak = A2UIWordBreak::HYPHENATION;
        return true;
    }
    return false;
}

bool ParseCancelButtonStyle(const JsonValue& value, A2UICancelButtonStyle& style)
{
    if (value.IsNumber()) {
        switch (static_cast<int32_t>(value.GetNumberValue(static_cast<double>(static_cast<int32_t>(style))))) {
            case 0:
                style = A2UICancelButtonStyle::CONSTANT;
                return true;
            case 1:
                style = A2UICancelButtonStyle::INVISIBLE;
                return true;
            case 2:
                style = A2UICancelButtonStyle::INPUT;
                return true;
            default:
                return false;
        }
    }

    if (!value.IsString()) {
        return false;
    }

    std::string token = NormalizeToken(value.GetStringValue(""));
    if (token == "constant") {
        style = A2UICancelButtonStyle::CONSTANT;
        return true;
    }
    if (token == "invisible") {
        style = A2UICancelButtonStyle::INVISIBLE;
        return true;
    }
    if (token == "input") {
        style = A2UICancelButtonStyle::INPUT;
        return true;
    }
    return false;
}

bool ParseCancelButton(const JsonValue& value, A2UICancelButtonStyle& style, bool& hasIconSize, float& iconSize,
    bool& hasIconColor, uint32_t& iconColor, bool& hasIconSrc, std::string& iconSrc)
{
    if (!value.IsObject()) {
        return false;
    }

    style = A2UICancelButtonStyle::INPUT;
    hasIconSize = false;
    iconSize = 0.0F;
    hasIconColor = false;
    iconColor = 0;
    // Use ArkUI TextInput's built-in clear icon. Protocol icon.src is fixed and not user-configurable here.
    hasIconSrc = false;
    iconSrc.clear();

    A2UICancelButtonStyle parsedStyle = style;
    if (ParseCancelButtonStyle(value.GetItem("style"), parsedStyle)) {
        style = parsedStyle;
    }

    StyleDimension iconSizeDimension;
    float parsedIconSize = 0.0F;
    if (StyleApplyUtils::ParseDimension(value.GetItem("fontSize"), iconSizeDimension) &&
        ConvertDimensionToFloat(iconSizeDimension, parsedIconSize) && std::isfinite(parsedIconSize) &&
        parsedIconSize > 0.0F) {
        hasIconSize = true;
        iconSize = parsedIconSize;
    }

    uint32_t parsedIconColor = 0;
    if (ExtendedStyleResolver::ParseColor(value.GetItem("fontColor"), parsedIconColor)) {
        hasIconColor = true;
        iconColor = parsedIconColor;
    }

    return true;
}

bool ParseUnderlineColorGroup(const JsonValue& value, uint32_t& typingColor, uint32_t& normalColor,
    uint32_t& errorColor, uint32_t& disableColor, uint32_t defaultColor)
{
    typingColor = defaultColor;
    normalColor = defaultColor;
    errorColor = defaultColor;
    disableColor = defaultColor;

    uint32_t parsedColor = 0;
    if (ExtendedStyleResolver::ParseColor(value, parsedColor)) {
        normalColor = parsedColor;
        return true;
    }

    if (!value.IsObject()) {
        return false;
    }

    if (ExtendedStyleResolver::ParseColor(value.GetItem("typing"), parsedColor)) {
        typingColor = parsedColor;
    }
    if (ExtendedStyleResolver::ParseColor(value.GetItem("normal"), parsedColor)) {
        normalColor = parsedColor;
    }
    if (ExtendedStyleResolver::ParseColor(value.GetItem("error"), parsedColor)) {
        errorColor = parsedColor;
    }
    if (ExtendedStyleResolver::ParseColor(value.GetItem("disable"), parsedColor)) {
        disableColor = parsedColor;
    }
    return true;
}

int32_t NormalizeMaxLength(double value)
{
    if (!std::isfinite(value) || value < 0.0) {
        return DEFAULT_MAX_LENGTH;
    }
    double clamped = std::min(value, static_cast<double>(std::numeric_limits<int32_t>::max()));
    return static_cast<int32_t>(clamped);
}

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

} // namespace

ExtendedTextInputComponent::ExtendedTextInputComponent()
    : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT_INPUT))
{
    LOG_A2UI(LOG_DEBUG, "ExtendedTextInputComponent::ExtendedTextInputComponent - create text input component");
    if (nativeView_ != nullptr) {
        ArkUINodeApiAdapter::AddNodeEventReceiver(nativeView_, ExtendedTextInputComponent::NodeEventReceiver);
        LOG_A2UI(LOG_DEBUG, "ExtendedTextInputComponent::ExtendedTextInputComponent - event receiver registered");
        ArkUIOHApiAdapter::SetCrossLanguageOption(nativeView_, true);
    }
}

ExtendedTextInputComponent::~ExtendedTextInputComponent()
{
    if (nativeView_ == nullptr) {
        return;
    }
    if (changeEventRegistered_) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, A2UINodeEventType::TEXT_INPUT_ON_CHANGE);
    }
    ArkUINodeApiAdapter::RemoveNodeEventReceiver(nativeView_, ExtendedTextInputComponent::NodeEventReceiver);
    LOG_A2UI(LOG_DEBUG,
        "ExtendedTextInputComponent::~ExtendedTextInputComponent - cleanup completed, changeEventRegistered=%{public}s",
        changeEventRegistered_ ? "true" : "false");
}

std::string ExtendedTextInputComponent::GetType() const
{
    return "TextInput";
}

void ExtendedTextInputComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    if (descriptor.Has("text")) {
        SetPropertyFromDescriptor("text", descriptor);
    } else {
        ResetTextPropertyIfMissing();
    }

    if (descriptor.Has("maxLength")) {
        JsonValue maxLengthValue = descriptor.GetItem("maxLength");
        if (!IsDynamicValueDescriptor(maxLengthValue) && maxLengthValue.IsNumber()) {
            double maxLength = maxLengthValue.GetNumberValue(0.0);
            if (!std::isfinite(maxLength) || maxLength < 0.0) {
                ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property maxLength expects number in [0, inf), fallback/reset has been applied", "maxLength");
            }
        }
    }

    ApplyDeclaredPropertyOrFallback(descriptor, "placeholder");
    ApplyDeclaredPropertyOrFallback(descriptor, "enabled");
    ApplyDeclaredPropertyOrFallback(descriptor, "maxLength");
    ApplyDeclaredPropertyOrFallback(descriptor, "type");
}

PropertyDeclaration ExtendedTextInputComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName == "text") {
        return CreateTextPropertyDeclaration();
    }
    if (propertyName == "placeholder") {
        return CreatePlaceholderPropertyDeclaration();
    }
    if (propertyName == "enabled") {
        return CreateEnabledPropertyDeclaration();
    }
    if (propertyName == "maxLength") {
        return CreateMaxLengthPropertyDeclaration();
    }
    if (propertyName == "type") {
        return CreateTypePropertyDeclaration();
    }
    return {};
}

PropertyDeclaration ExtendedTextInputComponent::CreateTextPropertyDeclaration()
{
    return PropertyDeclaration { .name = "text",
        .type = PropertyValueType::STRING,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackString = "",
        .applyValue = [this](const JsonValue& value) { SetText(value.GetStringValue("")); } };
}

PropertyDeclaration ExtendedTextInputComponent::CreatePlaceholderPropertyDeclaration()
{
    return PropertyDeclaration { .name = "placeholder",
        .type = PropertyValueType::STRING,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackString = "",
        .applyValue = [this](const JsonValue& value) { SetPlaceholder(value.GetStringValue("")); } };
}

PropertyDeclaration ExtendedTextInputComponent::CreateEnabledPropertyDeclaration()
{
    return PropertyDeclaration { .name = "enabled",
        .type = PropertyValueType::BOOLEAN,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackBool = true,
        .applyValue = [this](const JsonValue& value) { SetEnabled(value.GetBoolValue(true)); } };
}

PropertyDeclaration ExtendedTextInputComponent::CreateMaxLengthPropertyDeclaration()
{
    return PropertyDeclaration { .name = "maxLength",
        .type = PropertyValueType::NUMBER,
        .allowDynamic = true,
        .allowExpression = true,
        .reportDynamicNumberRange = true,
        .dynamicNumberMin = 0.0,
        .dynamicNumberMinExclusive = false,
        .fallbackNumber = DEFAULT_MAX_LENGTH,
        .applyValue = [this](const JsonValue& value) {
            SetMaxLength(NormalizeMaxLength(value.GetNumberValue(DEFAULT_MAX_LENGTH)));
        } };
}

PropertyDeclaration ExtendedTextInputComponent::CreateTypePropertyDeclaration()
{
    return PropertyDeclaration { .name = "type",
        .type = PropertyValueType::ENUM_STRING,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackString = "normal",
        .enumAllowed = { "normal", "number", "phoneNumber", "email", "password", "numberPassword", "screenLockPassword",
            "userName", "newPassword", "numberDecimal", "oneTimeCode" },
        .enumFallback = "normal",
        .applyValue = [this](const JsonValue& value) {
            SetInputType(ParseTextInputTypeToken(value.GetStringValue("normal")));
        } };
}

void ExtendedTextInputComponent::ValidateStylesSchema(const JsonValue& styles)
{
    if (IsApplyingStyleDeltaUpdate() || !styles.IsObject()) {
        return;
    }

    ValidateColorStyle(styles, "fontColor");
    ValidateColorStyle(styles, "placeholderColor");
    ValidateColorStyle(styles, "caretColor");
    ValidateColorStyle(styles, "selectedBackgroundColor");
    ValidatePositiveNumberStyle(styles, "fontSize");
    ValidatePositiveNumberStyle(styles, "minFontSize");
    ValidatePositiveNumberStyle(styles, "maxFontSize");
    ValidateMaxLinesStyle(styles);
    ValidateFontWeightStyle(styles);
    ValidateTextAlignStyle(styles);
    ValidateFontScaleRangeStyle(styles, "minFontScale", 0.0, 1.0, true);
    ValidateFontScaleRangeStyle(styles, "maxFontScale", 1.0, 0.0, false);
    ValidateFontScaleModeStyle(styles);
    ValidateShowUnderlineStyle(styles);
    ValidateCancelButtonStyleProperty(styles);
    ValidateUnderlineColorStyle(styles);
    ValidateWordBreakStyle(styles);
}

void ExtendedTextInputComponent::ValidatePositiveNumberStyle(const JsonValue& styles, const char* propertyName)
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

void ExtendedTextInputComponent::ValidateColorStyle(const JsonValue& styles, const char* propertyName)
{
    if (propertyName == nullptr || !styles.Has(propertyName)) {
        return;
    }
    JsonValue value = styles.GetItem(propertyName);
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles." + std::string(propertyName) + " expects string color, fallback/reset has been applied",
            "styles." + std::string(propertyName));
        return;
    }
    uint32_t color = 0;
    if (!ExtendedStyleResolver::ParseColor(value, color)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles." + std::string(propertyName) + " expects color value, fallback/reset has been applied",
            "styles." + std::string(propertyName));
    }
}

void ExtendedTextInputComponent::ValidateMaxLinesStyle(const JsonValue& styles)
{
    if (!styles.Has("maxLines")) {
        return;
    }
    JsonValue value = styles.GetItem("maxLines");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsNumber()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.maxLines expects number, fallback/reset has been applied", "styles.maxLines");
        return;
    }
    double maxLines = value.GetNumberValue(0.0);
    if (!std::isfinite(maxLines) || maxLines <= 0.0) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.maxLines expects positive integer, fallback/reset has been applied", "styles.maxLines");
    }
}

void ExtendedTextInputComponent::ValidateFontWeightStyle(const JsonValue& styles)
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
    if (!IsValidTextInputFontWeight(value)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.fontWeight expects valid font weight, fallback/reset has been applied",
            "styles.fontWeight");
    }
}

void ExtendedTextInputComponent::ValidateTextAlignStyle(const JsonValue& styles)
{
    if (!styles.Has("textAlign")) {
        return;
    }
    JsonValue value = styles.GetItem("textAlign");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.textAlign expects string enum, fallback/reset has been applied", "styles.textAlign");
        return;
    }
    int32_t textAlign = DEFAULT_TEXT_ALIGN;
    if (!StyleApplyUtils::ParseTextAlign(value, textAlign)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.textAlign expects valid enum value, fallback/reset has been applied", "styles.textAlign");
    }
}

void ExtendedTextInputComponent::ValidateFontScaleRangeStyle(
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
    if (!std::isfinite(number) || number < minValue || (hasMaxValue && number > maxValue)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles." + std::string(propertyName) +
                " expects number in supported range, fallback/reset has been applied",
            "styles." + std::string(propertyName));
    }
}

void ExtendedTextInputComponent::ValidateFontScaleModeStyle(const JsonValue& styles)
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

void ExtendedTextInputComponent::ValidateShowUnderlineStyle(const JsonValue& styles)
{
    if (!styles.Has("showUnderline")) {
        return;
    }
    JsonValue value = styles.GetItem("showUnderline");
    if (IsDynamicValueDescriptor(value) || value.IsBool()) {
        return;
    }
    ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
        "Property styles.showUnderline expects boolean, fallback/reset has been applied", "styles.showUnderline");
}

void ExtendedTextInputComponent::ValidateCancelButtonStyleProperty(const JsonValue& styles)
{
    if (!styles.Has("cancelButton")) {
        return;
    }
    JsonValue value = styles.GetItem("cancelButton");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsObject()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.cancelButton expects object, fallback/reset has been applied", "styles.cancelButton");
        return;
    }
    ValidateCancelButtonSchema(value);
}

void ExtendedTextInputComponent::ValidateUnderlineColorStyle(const JsonValue& styles)
{
    if (!styles.Has("underlineColor")) {
        return;
    }
    JsonValue value = styles.GetItem("underlineColor");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    uint32_t color = 0;
    if (value.IsString() && ExtendedStyleResolver::ParseColor(value, color)) {
        return;
    }
    if (!value.IsObject()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.underlineColor expects color or object, fallback/reset has been applied",
            "styles.underlineColor");
        return;
    }
    ValidateUnderlineColorSchema(value);
}

void ExtendedTextInputComponent::ValidateWordBreakStyle(const JsonValue& styles)
{
    if (!styles.Has("wordBreak")) {
        return;
    }
    JsonValue value = styles.GetItem("wordBreak");
    if (IsDynamicValueDescriptor(value)) {
        return;
    }
    if (!value.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.wordBreak expects string enum, fallback/reset has been applied", "styles.wordBreak");
        return;
    }
    A2UIWordBreak wordBreak = DEFAULT_WORD_BREAK;
    if (!ParseWordBreak(value, wordBreak)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.wordBreak expects valid enum value, fallback/reset has been applied", "styles.wordBreak");
    }
}

void ExtendedTextInputComponent::ValidateCancelButtonSchema(const JsonValue& cancelButtonValue)
{
    for (JsonValue child = cancelButtonValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (!key.empty() && !IsSupportedCancelButtonMember(key)) {
            std::string path = "styles.cancelButton." + key;
            ReportExtendedSchemaWarning(
                SCHEMA_ERROR_CODE_UNDEFINED_FIELD, "Property " + path + " is undefined and has been ignored", path);
        }
    }

    if (cancelButtonValue.Has("style")) {
        ValidateCancelButtonStyleValue(cancelButtonValue.GetItem("style"), true);
    }
    if (cancelButtonValue.Has("fontSize")) {
        ValidateCancelButtonFontSizeValue(cancelButtonValue.GetItem("fontSize"), true);
    }
    if (cancelButtonValue.Has("fontColor")) {
        ValidateCancelButtonFontColorValue(cancelButtonValue.GetItem("fontColor"), true);
    }
}

void ExtendedTextInputComponent::ValidateCancelButtonStyleValue(const JsonValue& styleValue, bool allowDynamic)
{
    if (allowDynamic && IsDynamicValueDescriptor(styleValue)) {
        return;
    }
    if (!styleValue.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.cancelButton.style expects string enum, fallback/reset has been applied",
            "styles.cancelButton.style");
    } else if (!IsValidCancelButtonStyleToken(styleValue)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.cancelButton.style expects valid enum value, fallback/reset has been applied",
            "styles.cancelButton.style");
    }
}

void ExtendedTextInputComponent::ValidateCancelButtonFontSizeValue(const JsonValue& fontSizeValue, bool allowDynamic)
{
    if (allowDynamic && IsDynamicValueDescriptor(fontSizeValue)) {
        return;
    }
    if (fontSizeValue.IsNumber()) {
        double number = fontSizeValue.GetNumberValue(0.0);
        if (!std::isfinite(number) || number <= 0.0) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.cancelButton.fontSize expects positive number, fallback/reset has been applied",
                "styles.cancelButton.fontSize");
        }
        return;
    }
    if (!fontSizeValue.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.cancelButton.fontSize expects number or dimension, fallback/reset has been applied",
            "styles.cancelButton.fontSize");
        return;
    }

    float fontSize = 0.0F;
    if (!TryParseCancelButtonFontSize(fontSizeValue, fontSize)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.cancelButton.fontSize expects valid positive dimension, fallback/reset has been applied",
            "styles.cancelButton.fontSize");
    } else if (!std::isfinite(fontSize) || fontSize <= 0.0F) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.cancelButton.fontSize expects positive number, fallback/reset has been applied",
            "styles.cancelButton.fontSize");
    }
}

void ExtendedTextInputComponent::ValidateCancelButtonFontColorValue(const JsonValue& fontColorValue, bool allowDynamic)
{
    if (allowDynamic && IsDynamicValueDescriptor(fontColorValue)) {
        return;
    }
    if (!fontColorValue.IsString()) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.cancelButton.fontColor expects string color, fallback/reset has been applied",
            "styles.cancelButton.fontColor");
        return;
    }

    uint32_t color = 0;
    if (!ExtendedStyleResolver::ParseColor(fontColorValue, color)) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.cancelButton.fontColor expects color value, fallback/reset has been applied",
            "styles.cancelButton.fontColor");
    }
}

void ExtendedTextInputComponent::ValidateUnderlineColorSchema(const JsonValue& underlineColorValue)
{
    uint32_t color = 0;
    const char* colorKeys[] = { "typing", "normal", "error", "disable" };
    for (JsonValue child = underlineColorValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (!key.empty() && !IsSupportedUnderlineColorMember(key)) {
            std::string path = "styles.underlineColor." + key;
            ReportExtendedSchemaWarning(
                SCHEMA_ERROR_CODE_UNDEFINED_FIELD, "Property " + path + " is undefined and has been ignored", path);
        }
    }
    for (const char* key : colorKeys) {
        if (key == nullptr || !underlineColorValue.Has(key)) {
            continue;
        }
        JsonValue colorValue = underlineColorValue.GetItem(key);
        if (IsDynamicValueDescriptor(colorValue)) {
            continue;
        }
        if (!colorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.underlineColor." + std::string(key) +
                    " expects string color, fallback/reset has been applied",
                "styles.underlineColor." + std::string(key));
        } else if (!ExtendedStyleResolver::ParseColor(colorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.underlineColor." + std::string(key) +
                    " expects color value, fallback/reset has been applied",
                "styles.underlineColor." + std::string(key));
        }
    }
}

void ExtendedTextInputComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStylesSchema(styles);
}

namespace {
bool TryPutResolvedStyleMember(JsonValue& resolvedRoot, const std::string& key, const JsonValue& child,
    const RenderContext& renderContext, const DynamicResolveContext& context)
{
    if (child.Has("path") && renderContext.bindingEngine != nullptr) {
        std::string path = child.GetString("path", "");
        std::shared_ptr<DataModel> dataModel =
            renderContext.bindingEngine->GetOrCreateDataModel(renderContext.surfaceId);
        if (dataModel != nullptr) {
            std::optional<JsonValue> value = dataModel->GetNode(path);
            if (value.has_value()) {
                resolvedRoot.Put(key.c_str(), value.value());
                return true;
            }
        }
    }

    ResolvedValue resolvedValue = DynamicValueResolver::Resolve(child, context);
    if (!resolvedValue.success || !resolvedValue.value.IsValid()) {
        return false;
    }
    resolvedRoot.Put(key.c_str(), resolvedValue.value);
    return true;
}
} // namespace

std::unique_ptr<JsonAdapter> ExtendedTextInputComponent::ResolveStyleObjectDynamicMembers(
    const JsonValue& styleObjectValue) const
{
    if (!styleObjectValue.IsObject() || IsNestedDynamicDescriptor(styleObjectValue)) {
        return nullptr;
    }

    std::unique_ptr<JsonAdapter> resolvedAdapter = JsonAdapter::CreateObject();
    if (resolvedAdapter == nullptr) {
        return nullptr;
    }
    JsonValue resolvedRoot = resolvedAdapter->GetRoot();
    bool hasDynamicMember = false;
    const RenderContext& renderContext = GetRenderContext();
    DynamicResolveContext context = { .renderId = renderContext.renderId,
        .surfaceId = renderContext.surfaceId,
        .componentId = GetComponentId(),
        .dataModel = renderContext.dataModel,
        .allowExpression = true,
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
    for (JsonValue child = styleObjectValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        if (IsNestedDynamicDescriptor(child)) {
            hasDynamicMember = true;
            if (TryPutResolvedStyleMember(resolvedRoot, key, child, renderContext, context)) {
                continue;
            }
        }
        resolvedRoot.Put(key.c_str(), child);
    }
    if (!hasDynamicMember) {
        return nullptr;
    }
    return resolvedAdapter;
}

std::unique_ptr<JsonAdapter> ExtendedTextInputComponent::ResolveCancelButtonDynamicMembers(
    const JsonValue& cancelButtonValue) const
{
    return ResolveStyleObjectDynamicMembers(cancelButtonValue);
}

std::unique_ptr<JsonAdapter> ExtendedTextInputComponent::ResolveUnderlineColorDynamicMembers(
    const JsonValue& underlineColorValue) const
{
    return ResolveStyleObjectDynamicMembers(underlineColorValue);
}

void ExtendedTextInputComponent::ValidateResolvedCancelButtonDfx(const JsonValue& cancelButtonValue)
{
    if (!cancelButtonValue.IsObject()) {
        return;
    }
    if (cancelButtonValue.Has("style")) {
        ValidateCancelButtonStyleValue(cancelButtonValue.GetItem("style"), false);
    }
    if (cancelButtonValue.Has("fontSize")) {
        ValidateCancelButtonFontSizeValue(cancelButtonValue.GetItem("fontSize"), false);
    }
    if (cancelButtonValue.Has("fontColor")) {
        ValidateCancelButtonFontColorValue(cancelButtonValue.GetItem("fontColor"), false);
    }
}

void ExtendedTextInputComponent::ValidateResolvedUnderlineColorDfx(const JsonValue& underlineColorValue)
{
    if (!underlineColorValue.IsObject()) {
        return;
    }

    const char* colorKeys[] = { "typing", "normal", "error", "disable" };
    for (const char* key : colorKeys) {
        if (key == nullptr || !underlineColorValue.Has(key)) {
            continue;
        }
        JsonValue colorValue = underlineColorValue.GetItem(key);
        uint32_t color = 0;
        if (!colorValue.IsString()) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.underlineColor." + std::string(key) +
                    " expects string color, fallback/reset has been applied",
                "styles.underlineColor." + std::string(key));
        } else if (!ExtendedStyleResolver::ParseColor(colorValue, color)) {
            ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.underlineColor." + std::string(key) +
                    " expects color value, fallback/reset has been applied",
                "styles.underlineColor." + std::string(key));
        }
    }
}

void ExtendedTextInputComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    ThemeMode themeMode = ResolveThemeMode();
    uint32_t defaultFontColor = GetDefaultTextInputFontColor(themeMode);
    uint32_t defaultPlaceholderColor = GetDefaultTextInputPlaceholderColor(themeMode);
    uint32_t defaultCaretColor = GetDefaultTextInputCaretColor(themeMode);
    uint32_t defaultSelectedBackgroundColor = GetDefaultTextInputSelectedBackgroundColor(themeMode);
    uint32_t defaultUnderlineColor = GetDefaultTextInputUnderlineColor(themeMode);

    if (!styles.IsObject()) {
        if (IsApplyingStyleDeltaUpdate()) {
            return;
        }
        ResetStylesToDefaults(defaultFontColor, defaultPlaceholderColor, defaultCaretColor,
            defaultSelectedBackgroundColor, defaultUnderlineColor);
        return;
    }

    ApplyBackgroundColorStyle(styles, themeMode);
    ApplyColorStyles(
        styles, defaultFontColor, defaultPlaceholderColor, defaultCaretColor, defaultSelectedBackgroundColor);
    ApplyCancelButtonStyle(styles);
    ApplyFontSizeStyle(styles, applier);
    ApplyTextInputStyle(styles, applier);
    ApplyMaxLinesStyle(styles, applier);
    ApplyFontWeightStyle(styles);
    ApplyTextAlignStyle(styles);
    ApplyMinFontSizeStyle(styles);
    ApplyMaxFontSizeStyle(styles);
    ApplyFontScaleLimits(styles);
    ApplyFontScaleModeStyle(styles);
    ApplyShowUnderlineStyle(styles);
    ApplyUnderlineColorStyle(styles, defaultUnderlineColor);
    ApplyWordBreakStyle(styles);
}

void ExtendedTextInputComponent::ApplyBackgroundColorStyle(const JsonValue& styles, ThemeMode themeMode)
{
    if (!ShouldApplyStyle(styles, "backgroundColor")) {
        return;
    }
    uint32_t color = 0;
    bool parsed = ExtendedStyleResolver::ParseColor(styles.GetItem("backgroundColor"), color);
    SetBackgroundColor(parsed ? color : GetDefaultTextInputBackgroundColor(themeMode), parsed);
}

bool ExtendedTextInputComponent::ShouldApplyStyle(const JsonValue& styles, const char* styleName) const
{
    return !IsApplyingStyleDeltaUpdate() || (styleName != nullptr && styles.Has(styleName));
}

void ExtendedTextInputComponent::ResetStylesToDefaults(uint32_t defaultFontColor, uint32_t defaultPlaceholderColor,
    uint32_t defaultCaretColor, uint32_t defaultSelectedBackgroundColor, uint32_t defaultUnderlineColor)
{
    SetFontColor(defaultFontColor, false);
    SetPlaceholderColor(defaultPlaceholderColor, false);
    SetCaretColor(defaultCaretColor, false);
    SetSelectedBackgroundColor(defaultSelectedBackgroundColor, false);
    ResetCancelButton();
    ResetNodeFontSize();
    ArkUINodeApiAdapter::SetNodeTextInputStyle(nativeView_, false);
    ResetNodeTextInputNumberOfLines();
    ResetNodeFontWeight();
    ResetNodeTextAlign();
    ResetNodeTextInputShowUnderline();
    ResetUnderlineColor();
    ResetNodeTextInputWordBreak();
    SetMinFontSize(0.0F);
    SetMaxFontSize(0.0F);
    fontWeight_ = A2UIFontWeight::NORMAL;
    textAlign_ = DEFAULT_TEXT_ALIGN;
    showUnderline_ = false;
    wordBreak_ = DEFAULT_WORD_BREAK;
    SetUnderlineColor(
        defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, false);
    SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
}

void ExtendedTextInputComponent::ApplyColorStyles(const JsonValue& styles, uint32_t defaultFontColor,
    uint32_t defaultPlaceholderColor, uint32_t defaultCaretColor, uint32_t defaultSelectedBackgroundColor)
{
    uint32_t color = 0;
    if (ShouldApplyStyle(styles, "fontColor")) {
        bool parsed = ExtendedStyleResolver::ParseColor(styles.GetItem("fontColor"), color);
        SetFontColor(parsed ? color : defaultFontColor, parsed);
    }
    if (ShouldApplyStyle(styles, "placeholderColor")) {
        bool parsed = ExtendedStyleResolver::ParseColor(styles.GetItem("placeholderColor"), color);
        SetPlaceholderColor(parsed ? color : defaultPlaceholderColor, parsed);
    }
    if (ShouldApplyStyle(styles, "caretColor")) {
        bool parsed = ExtendedStyleResolver::ParseColor(styles.GetItem("caretColor"), color);
        SetCaretColor(parsed ? color : defaultCaretColor, parsed);
    }
    if (ShouldApplyStyle(styles, "selectedBackgroundColor")) {
        bool parsed = ExtendedStyleResolver::ParseColor(styles.GetItem("selectedBackgroundColor"), color);
        SetSelectedBackgroundColor(parsed ? color : defaultSelectedBackgroundColor, parsed);
    }
}

void ExtendedTextInputComponent::ApplyCancelButtonStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "cancelButton")) {
        return;
    }
    JsonValue value = styles.GetItem("cancelButton");
    std::unique_ptr<JsonAdapter> resolvedAdapter = ResolveCancelButtonDynamicMembers(value);
    if (resolvedAdapter != nullptr) {
        value = resolvedAdapter->GetRoot();
        ValidateResolvedCancelButtonDfx(value);
    }
    A2UICancelButtonStyle style = A2UICancelButtonStyle::INPUT;
    bool hasIconSize = false;
    float iconSize = 0.0F;
    bool hasIconColor = false;
    uint32_t iconColor = 0;
    bool hasIconSrc = false;
    std::string iconSrc;
    if (ParseCancelButton(value, style, hasIconSize, iconSize, hasIconColor, iconColor, hasIconSrc, iconSrc)) {
        SetCancelButton(style, hasIconSize, iconSize, hasIconColor, iconColor, hasIconSrc, iconSrc);
        return;
    }
    ResetCancelButton();
}

void ExtendedTextInputComponent::ApplyFontSizeStyle(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    if (!ShouldApplyStyle(styles, "fontSize")) {
        return;
    }
    float fontSize = 0.0F;
    if (StyleApplyUtils::ParseNumber(styles.GetItem("fontSize"), fontSize) && fontSize > 0.0F) {
        fontSize_ = fontSize;
        applier.SetNodeFontSize(nativeView_, ComputeEffectiveFontSize(fontSize));
        return;
    }
    fontSize_ = 0.0F;
    ResetNodeFontSize();
}

void ExtendedTextInputComponent::ApplyTextInputStyle(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    bool hasMaxLines = styles.IsObject() && styles.Has("maxLines");
    bool hasWordBreak = styles.IsObject() && styles.Has("wordBreak");
    if (IsApplyingStyleDeltaUpdate() && !hasMaxLines && !hasWordBreak) {
        return;
    }

    int32_t maxLines = 0;
    A2UIWordBreak wordBreak = DEFAULT_WORD_BREAK;
    bool hasValidMaxLines = false;
    if (hasMaxLines) {
        JsonValue maxLinesValue = styles.GetItem("maxLines");
        double maxLinesNumber = maxLinesValue.GetNumberValue(0.0);
        hasValidMaxLines = maxLinesValue.IsNumber() && std::isfinite(maxLinesNumber) && maxLinesNumber > 0.0 &&
                           maxLinesNumber <= static_cast<double>(std::numeric_limits<int32_t>::max()) &&
                           std::floor(maxLinesNumber) == maxLinesNumber &&
                           StyleApplyUtils::ParseMaxLines(maxLinesValue, maxLines) && maxLines > 0;
    }
    bool requiresInlineStyle =
        hasValidMaxLines ||
        (hasWordBreak && ParseWordBreak(styles.GetItem("wordBreak"), wordBreak) && wordBreak != A2UIWordBreak::NORMAL);
    applier.SetNodeTextInputStyle(nativeView_, requiresInlineStyle);
}

void ExtendedTextInputComponent::ApplyMaxLinesStyle(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    if (!ShouldApplyStyle(styles, "maxLines")) {
        return;
    }
    int32_t maxLines = 0;
    if (StyleApplyUtils::ParseMaxLines(styles.GetItem("maxLines"), maxLines)) {
        applier.SetNodeTextInputNumberOfLines(nativeView_, maxLines);
        return;
    }
    ResetNodeTextInputNumberOfLines();
}

void ExtendedTextInputComponent::ApplyFontWeightStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "fontWeight")) {
        return;
    }
    int32_t fontWeight = static_cast<int32_t>(A2UIFontWeight::NORMAL);
    if (StyleApplyUtils::ParseFontWeight(styles.GetItem("fontWeight"), fontWeight)) {
        SetFontWeight(static_cast<A2UIFontWeight>(fontWeight));
        return;
    }
    fontWeight_ = A2UIFontWeight::NORMAL;
    ResetNodeFontWeight();
}

void ExtendedTextInputComponent::ApplyTextAlignStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "textAlign")) {
        return;
    }
    int32_t textAlign = DEFAULT_TEXT_ALIGN;
    if (StyleApplyUtils::ParseTextAlign(styles.GetItem("textAlign"), textAlign)) {
        SetTextAlign(textAlign);
        return;
    }
    textAlign_ = DEFAULT_TEXT_ALIGN;
    ResetNodeTextAlign();
}

void ExtendedTextInputComponent::ApplyMinFontSizeStyle(const JsonValue& styles)
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

void ExtendedTextInputComponent::ApplyMaxFontSizeStyle(const JsonValue& styles)
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

void ExtendedTextInputComponent::ApplyFontScaleLimits(const JsonValue& styles)
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

void ExtendedTextInputComponent::ApplyFontScaleModeStyle(const JsonValue& styles)
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

void ExtendedTextInputComponent::ApplyShowUnderlineStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "showUnderline")) {
        return;
    }
    JsonValue value = styles.GetItem("showUnderline");
    if (value.IsBool()) {
        SetShowUnderline(value.GetBoolValue(false));
        return;
    }
    showUnderline_ = false;
    ResetNodeTextInputShowUnderline();
}

void ExtendedTextInputComponent::ApplyUnderlineColorStyle(const JsonValue& styles, uint32_t defaultUnderlineColor)
{
    if (!ShouldApplyStyle(styles, "underlineColor")) {
        return;
    }
    JsonValue value = styles.GetItem("underlineColor");
    std::unique_ptr<JsonAdapter> resolvedAdapter = ResolveUnderlineColorDynamicMembers(value);
    if (resolvedAdapter != nullptr) {
        value = resolvedAdapter->GetRoot();
        ValidateResolvedUnderlineColorDfx(value);
    }
    uint32_t typingColor = defaultUnderlineColor;
    uint32_t normalColor = defaultUnderlineColor;
    uint32_t errorColor = defaultUnderlineColor;
    uint32_t disableColor = defaultUnderlineColor;
    bool parsed =
        ParseUnderlineColorGroup(value, typingColor, normalColor, errorColor, disableColor, defaultUnderlineColor);
    if (parsed) {
        SetUnderlineColor(typingColor, normalColor, errorColor, disableColor, true);
        return;
    }
    SetUnderlineColor(
        defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, false);
}

void ExtendedTextInputComponent::ApplyWordBreakStyle(const JsonValue& styles)
{
    if (!ShouldApplyStyle(styles, "wordBreak")) {
        return;
    }
    A2UIWordBreak wordBreak = DEFAULT_WORD_BREAK;
    if (ParseWordBreak(styles.GetItem("wordBreak"), wordBreak)) {
        SetWordBreak(wordBreak);
        return;
    }
    wordBreak_ = DEFAULT_WORD_BREAK;
    ResetNodeTextInputWordBreak();
}

void ExtendedTextInputComponent::OnConfigChange(const ThemeContext& context)
{
    if (!hasFontColorOverride_) {
        SetFontColor(GetDefaultTextInputFontColor(context.colorMode), false);
    }
    if (!hasPlaceholderColorOverride_) {
        SetPlaceholderColor(GetDefaultTextInputPlaceholderColor(context.colorMode), false);
    }
    if (!hasBackgroundColorOverride_) {
        SetBackgroundColor(GetDefaultTextInputBackgroundColor(context.colorMode), false);
    }
    if (!hasCaretColorOverride_) {
        SetCaretColor(GetDefaultTextInputCaretColor(context.colorMode), false);
    }
    if (!hasSelectedBackgroundColorOverride_) {
        SetSelectedBackgroundColor(GetDefaultTextInputSelectedBackgroundColor(context.colorMode), false);
    }
    if (!hasUnderlineColor_) {
        uint32_t defaultUnderlineColor = GetDefaultTextInputUnderlineColor(context.colorMode);
        SetUnderlineColor(
            defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, defaultUnderlineColor, false);
    }
}

void ExtendedTextInputComponent::RegisterComponentSpecificListeners()
{
    LOG_A2UI(LOG_DEBUG, "ExtendedTextInputComponent::RegisterComponentSpecificListeners - componentId=%{public}s",
        GetComponentId().c_str());
    UpdateChangeEventRegistration();
}

void ExtendedTextInputComponent::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == "text") {
        SetText("");
        UpdateChangeEventRegistration();
        return;
    }
    if (propertyName == "placeholder") {
        SetPlaceholder("");
        return;
    }
    if (propertyName == "enabled") {
        SetEnabled(true);
        return;
    }
    if (propertyName == "maxLength") {
        SetMaxLength(DEFAULT_MAX_LENGTH);
        return;
    }
    if (propertyName == "type") {
        SetInputType(A2UITextInputType::NORMAL);
        return;
    }
}

void ExtendedTextInputComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* component = reinterpret_cast<ExtendedTextInputComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (component != nullptr) {
        component->HandleNodeEvent(event);
    }
}

void ExtendedTextInputComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    if (ArkUIOHApiAdapter::NodeEventGetEventType(event) != A2UINodeEventType::TEXT_INPUT_ON_CHANGE) {
        return;
    }

    A2UIStringAsyncEvent* stringEvent = ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(event);
    if (stringEvent == nullptr || stringEvent->pStr == nullptr) {
        return;
    }
    HandleInputValueChange(stringEvent->pStr);
}

void ExtendedTextInputComponent::HandleInputValueChange(const std::string& nextValue)
{
    if (nextValue == textValue_) {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedTextInputComponent::HandleInputValueChange - skip unchanged value, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    SetText(nextValue);
    SyncTextToBoundDataModel(nextValue);
    DispatchEvent("onChange", BuildTextInputChangeEventContext(nextValue));
}

void ExtendedTextInputComponent::UpdateChangeEventRegistration()
{
    bool shouldRegister = HasEventHandler("onChange");
    if (!shouldRegister) {
        shouldRegister = !ResolveTextBindingPath().empty();
    }
    if (nativeView_ == nullptr) {
        changeEventRegistered_ = false;
        LOG_A2UI(LOG_WARN,
            "ExtendedTextInputComponent::UpdateChangeEventRegistration - native view unavailable, "
            "componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }
    LOG_A2UI(LOG_DEBUG,
        "ExtendedTextInputComponent::UpdateChangeEventRegistration - componentId=%{public}s, "
        "shouldRegister=%{public}s, "
        "currentlyRegistered=%{public}s",
        GetComponentId().c_str(), shouldRegister ? "true" : "false", changeEventRegistered_ ? "true" : "false");
    if (shouldRegister == changeEventRegistered_) {
        return;
    }
    if (shouldRegister) {
        RegisterChangeEvent();
        return;
    }
    UnregisterChangeEvent();
}

void ExtendedTextInputComponent::RegisterChangeEvent()
{
    ArkUINodeApiAdapter::RegisterNodeEvent(nativeView_, A2UINodeEventType::TEXT_INPUT_ON_CHANGE, 0, this);
    changeEventRegistered_ = true;
    LOG_A2UI(LOG_DEBUG,
        "ExtendedTextInputComponent::UpdateChangeEventRegistration - registered change event, componentId=%{public}s",
        GetComponentId().c_str());
}

void ExtendedTextInputComponent::UnregisterChangeEvent()
{
    ArkUINodeApiAdapter::UnregisterNodeEvent(nativeView_, A2UINodeEventType::TEXT_INPUT_ON_CHANGE);
    changeEventRegistered_ = false;
    LOG_A2UI(LOG_DEBUG,
        "ExtendedTextInputComponent::UpdateChangeEventRegistration - unregistered change event, componentId=%{public}s",
        GetComponentId().c_str());
}

void ExtendedTextInputComponent::ResetTextPropertyIfMissing()
{
    LOG_A2UI(LOG_DEBUG, "ExtendedTextInputComponent::ResetTextPropertyIfMissing - componentId=%{public}s",
        GetComponentId().c_str());
    RemoveBindingsForProperty("text");
    ApplyRuntimeProperty("text", JsonValue(), false);
    UpdateChangeEventRegistration();
}

#define DEFINE_TEXT_INPUT_RESET_METHOD(NAME)               \
    void ExtendedTextInputComponent::ResetNode##NAME()     \
    {                                                      \
        if (nativeView_ == nullptr) {                      \
            return;                                        \
        }                                                  \
        ArkUINodeApiAdapter::ResetNode##NAME(nativeView_); \
    }

DEFINE_TEXT_INPUT_RESET_METHOD(FontSize)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputNumberOfLines)
DEFINE_TEXT_INPUT_RESET_METHOD(FontWeight)
DEFINE_TEXT_INPUT_RESET_METHOD(TextAlign)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputShowUnderline)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputWordBreak)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputMaxLength)
DEFINE_TEXT_INPUT_RESET_METHOD(TextMinFontSize)
DEFINE_TEXT_INPUT_RESET_METHOD(TextMaxFontSize)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputCancelButton)
DEFINE_TEXT_INPUT_RESET_METHOD(TextInputUnderlineColor)

#undef DEFINE_TEXT_INPUT_RESET_METHOD

ThemeMode ExtendedTextInputComponent::ResolveThemeMode() const
{
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        return themeManager->GetContext().colorMode;
    }
    return GetRenderContext().colorMode;
}

void ExtendedTextInputComponent::SetText(const std::string& text)
{
    textValue_ = text;
    ArkUINodeApiAdapter::SetNodeTextInputText(nativeView_, textValue_);
}

void ExtendedTextInputComponent::SetPlaceholder(const std::string& placeholder)
{
    placeholderValue_ = placeholder;
    ArkUINodeApiAdapter::SetNodeTextInputPlaceholder(nativeView_, placeholderValue_);
}

void ExtendedTextInputComponent::SetEnabled(bool enabled)
{
    enabled_ = enabled;
    ArkUINodeApiAdapter::SetNodeEnabled(nativeView_, enabled_);
}

void ExtendedTextInputComponent::SetMaxLength(int32_t maxLength)
{
    maxLength_ = maxLength >= 0 ? maxLength : DEFAULT_MAX_LENGTH;
    if (maxLength_ == DEFAULT_MAX_LENGTH) {
        ResetNodeTextInputMaxLength();
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextInputMaxLength(nativeView_, maxLength_);
}

void ExtendedTextInputComponent::SetInputType(A2UITextInputType inputType)
{
    inputType_ = inputType;
    ArkUINodeApiAdapter::SetNodeTextInputType(nativeView_, inputType_);
}

void ExtendedTextInputComponent::SetFontColor(uint32_t color, bool userOverride)
{
    fontColor_ = color;
    hasFontColorOverride_ = userOverride;
    ArkUINodeApiAdapter::SetNodeFontColor(nativeView_, fontColor_);
}

void ExtendedTextInputComponent::SetPlaceholderColor(uint32_t color, bool userOverride)
{
    placeholderColor_ = color;
    hasPlaceholderColorOverride_ = userOverride;
    ArkUINodeApiAdapter::SetNodeTextInputPlaceholderColor(nativeView_, placeholderColor_);
}

void ExtendedTextInputComponent::SetFontWeight(A2UIFontWeight fontWeight)
{
    fontWeight_ = fontWeight;
    ArkUINodeApiAdapter::SetNodeFontWeight(nativeView_, fontWeight_);
}

void ExtendedTextInputComponent::SetTextAlign(int32_t textAlign)
{
    textAlign_ = textAlign;
    ArkUINodeApiAdapter::SetNodeTextAlign(nativeView_, textAlign_);
}

void ExtendedTextInputComponent::SetMinFontSize(float minFontSize)
{
    minFontSize_ = minFontSize > 0.0F ? minFontSize : 0.0F;
    if (minFontSize_ <= 0.0F) {
        ResetNodeTextMinFontSize();
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextMinFontSize(nativeView_, minFontSize_);
}

void ExtendedTextInputComponent::SetMaxFontSize(float maxFontSize)
{
    maxFontSize_ = maxFontSize > 0.0F ? maxFontSize : 0.0F;
    if (maxFontSize_ <= 0.0F) {
        ResetNodeTextMaxFontSize();
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextMaxFontSize(nativeView_, maxFontSize_);
}

void ExtendedTextInputComponent::SetCaretColor(uint32_t color, bool userOverride)
{
    hasCaretColorOverride_ = userOverride;
    caretColor_ = color;
    ArkUINodeApiAdapter::SetNodeTextInputCaretColor(nativeView_, caretColor_);
}

void ExtendedTextInputComponent::SetBackgroundColor(uint32_t color, bool userOverride)
{
    hasBackgroundColorOverride_ = userOverride;
    backgroundColor_ = color;
    ArkUINodeApiAdapter::SetNodeBackgroundColor(nativeView_, backgroundColor_);
}

void ExtendedTextInputComponent::SetSelectedBackgroundColor(uint32_t color, bool userOverride)
{
    hasSelectedBackgroundColorOverride_ = userOverride;
    selectedBackgroundColor_ = color;
    ArkUINodeApiAdapter::SetNodeTextInputSelectedBackgroundColor(nativeView_, selectedBackgroundColor_);
}

void ExtendedTextInputComponent::ResetCancelButton()
{
    hasCancelButton_ = false;
    cancelButtonStyle_ = A2UICancelButtonStyle::INPUT;
    hasCancelButtonIconSize_ = false;
    cancelButtonIconSize_ = 0.0F;
    hasCancelButtonIconColor_ = false;
    cancelButtonIconColor_ = 0;
    hasCancelButtonIconSrc_ = false;
    cancelButtonIconSrc_.clear();
    ResetNodeTextInputCancelButton();
}

void ExtendedTextInputComponent::SetCancelButton(A2UICancelButtonStyle style, bool hasIconSize, float iconSize,
    bool hasIconColor, uint32_t iconColor, bool hasIconSrc, const std::string& iconSrc)
{
    hasCancelButton_ = true;
    cancelButtonStyle_ = style;
    hasCancelButtonIconSize_ = hasIconSize;
    cancelButtonIconSize_ = hasIconSize ? iconSize : 0.0F;
    hasCancelButtonIconColor_ = hasIconColor;
    cancelButtonIconColor_ = hasIconColor ? iconColor : 0;
    hasCancelButtonIconSrc_ = hasIconSrc;
    cancelButtonIconSrc_ = hasIconSrc ? iconSrc : "";

    ArkUINodeApiAdapter::SetNodeTextInputCancelButton(nativeView_, cancelButtonStyle_, hasCancelButtonIconSize_,
        cancelButtonIconSize_, hasCancelButtonIconColor_, cancelButtonIconColor_, hasCancelButtonIconSrc_,
        cancelButtonIconSrc_);
}

void ExtendedTextInputComponent::SetShowUnderline(bool showUnderline)
{
    showUnderline_ = showUnderline;
    ArkUINodeApiAdapter::SetNodeTextInputShowUnderline(nativeView_, showUnderline_);
}

void ExtendedTextInputComponent::ResetUnderlineColor()
{
    hasUnderlineColor_ = false;
    underlineColorTyping_ = TEXT_INPUT_LIGHT_UNDERLINE_COLOR;
    underlineColorNormal_ = TEXT_INPUT_LIGHT_UNDERLINE_COLOR;
    underlineColorError_ = TEXT_INPUT_LIGHT_UNDERLINE_COLOR;
    underlineColorDisable_ = TEXT_INPUT_LIGHT_UNDERLINE_COLOR;
    ResetNodeTextInputUnderlineColor();
}

void ExtendedTextInputComponent::SetUnderlineColor(
    uint32_t typingColor, uint32_t normalColor, uint32_t errorColor, uint32_t disableColor, bool userOverride)
{
    hasUnderlineColor_ = userOverride;
    underlineColorTyping_ = typingColor;
    underlineColorNormal_ = normalColor;
    underlineColorError_ = errorColor;
    underlineColorDisable_ = disableColor;

    ArkUINodeApiAdapter::SetNodeTextInputUnderlineColor(
        nativeView_, underlineColorTyping_, underlineColorNormal_, underlineColorError_, underlineColorDisable_);
}

void ExtendedTextInputComponent::SetWordBreak(A2UIWordBreak wordBreak)
{
    wordBreak_ = wordBreak;
    ArkUINodeApiAdapter::SetNodeTextInputWordBreak(nativeView_, static_cast<int32_t>(wordBreak_));
}

std::string ExtendedTextInputComponent::ResolveTextBindingPath() const
{
    const auto& bindings = GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "text" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

void ExtendedTextInputComponent::SyncTextToBoundDataModel(const std::string& value)
{
    std::string textBindingPath = ResolveTextBindingPath();
    if (textBindingPath.empty()) {
        LOG_A2UI(LOG_DEBUG,
            "ExtendedTextInputComponent::SyncTextToBoundDataModel - no text binding, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    const RenderContext& renderContext = GetRenderContext();
    std::shared_ptr<BindingEngine> bindingEngine = renderContext.bindingEngine;
    if (bindingEngine == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedTextInputComponent::SyncTextToBoundDataModel: binding engine is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    if (renderContext.surfaceId.empty()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedTextInputComponent::SyncTextToBoundDataModel: surfaceId is empty, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateString(value);
    if (valueAdapter == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedTextInputComponent::SyncTextToBoundDataModel - create string adapter failed, "
            "componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }
    LOG_A2UI(LOG_DEBUG,
        "ExtendedTextInputComponent::SyncTextToBoundDataModel - update data model, componentId=%{public}s, "
        "surfaceId=%{public}s, path=%{public}s",
        GetComponentId().c_str(), renderContext.surfaceId.c_str(), textBindingPath.c_str());
    bindingEngine->UpdateDataModelByPath(renderContext.surfaceId, textBindingPath, valueAdapter->GetRoot());
}

void ExtendedTextInputComponent::SetFontScaleMode(const std::string& mode)
{
    fontScaleMode_ = mode == "custom" ? "custom" : DEFAULT_FONT_SCALE_MODE;
    if (fontSize_ > 0.0F) {
        if (nativeView_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeFontSize(nativeView_, ComputeEffectiveFontSize(fontSize_));
        }
    }
}

void ExtendedTextInputComponent::OnFontSizeScaleChanged(float newScale)
{
    ExtendedComponent::OnFontSizeScaleChanged(newScale);
    if (fontSize_ > 0.0F) {
        if (nativeView_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeFontSize(nativeView_, ComputeEffectiveFontSize(fontSize_));
        }
    }
}

float ExtendedTextInputComponent::ComputeEffectiveFontSize(float baseFontSize) const
{
    if (fontScaleMode_ == "custom") {
        const RenderContext& ctx = GetRenderContext();
        float scale = ctx.fontSizeScale > 0.0F ? ctx.fontSizeScale : 1.0F;
        return baseFontSize * scale;
    }
    return baseFontSize;
}

void ExtendedTextInputComponent::SetMinFontScale(float minFontScale)
{
    minFontScale_ = minFontScale > 0.0F ? minFontScale : 0.0F;
    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = GetRenderId(),
        .componentId = GetComponentId(),
        .nodeUniqueId = GetNativeNodeUniqueId(),
        .componentType = "TextInput",
        .attributeName = "minFontScale",
        .floatValue = minFontScale_,
        .reset = minFontScale_ <= 0.0F });
}

void ExtendedTextInputComponent::SetMaxFontScale(float maxFontScale)
{
    maxFontScale_ = maxFontScale > 0.0F ? maxFontScale : 0.0F;
    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = GetRenderId(),
        .componentId = GetComponentId(),
        .nodeUniqueId = GetNativeNodeUniqueId(),
        .componentType = "TextInput",
        .attributeName = "maxFontScale",
        .floatValue = maxFontScale_,
        .reset = maxFontScale_ <= 0.0F });
}

} // namespace NativeModule
