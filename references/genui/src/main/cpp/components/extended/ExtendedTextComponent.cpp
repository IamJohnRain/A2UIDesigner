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

#include "ExtendedTextComponent.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>

#include "components/extended/ExtendedStyleResolver.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"

#include "ExtendedTextTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr float DEFAULT_TEXT_FONT_SIZE = 16.0F;
constexpr A2UIFontWeight DEFAULT_TEXT_FONT_WEIGHT = A2UIFontWeight::W400;
constexpr int32_t DEFAULT_TEXT_ALIGN = 0;
constexpr int32_t DEFAULT_TEXT_OVERFLOW = 1;
constexpr A2UIWordBreak DEFAULT_TEXT_WORD_BREAK = A2UIWordBreak::BREAK_WORD;
constexpr int32_t DEFAULT_TEXT_MAX_LINES = std::numeric_limits<int32_t>::max();
constexpr char DEFAULT_FONT_SCALE_MODE[] = "followSystem";
const char* DescribeStyleResolution(bool isDeltaUpdate)
{
    static_cast<void>(isDeltaUpdate);
    return "fallback to default value";
}

bool HasNonObjectStylePayload(const JsonValue& styles)
{
    return styles.IsValid() && !styles.IsObject();
}

bool IsBindingDescriptorValue(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

TextDecorationState BuildDefaultDecorationState(uint32_t defaultColor)
{
    TextDecorationState defaultDecoration;
    defaultDecoration.type = 0;
    defaultDecoration.color = defaultColor;
    defaultDecoration.hasColor = true;
    defaultDecoration.style = 0;
    defaultDecoration.hasStyle = true;
    defaultDecoration.thicknessScale = 1.0F;
    defaultDecoration.hasThicknessScale = true;
    return defaultDecoration;
}

bool TryParseFiniteNumberStyleValue(const JsonValue& value, float& parsed)
{
    if (!value.IsNumber()) {
        return false;
    }

    double rawValue = value.GetNumberValue(0.0);
    if (!std::isfinite(rawValue)) {
        return false;
    }

    parsed = static_cast<float>(rawValue);
    return true;
}

bool TryParseHexColorStringStyleValue(const JsonValue& value, uint32_t& color)
{
    return value.IsString() && StyleApplyUtils::ParseHexColorString(value.GetStringValue(""), color);
}

std::string NormalizeTextToken(const std::string& value)
{
    std::string token = StyleApplyUtils::TrimToken(value);
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return token;
}

bool ParseDecorationTypeToken(const std::string& value, int32_t& type)
{
    static const std::map<std::string, int32_t> decorationTypeMap = { { "none", 0 }, { "underline", 1 },
        { "overline", 2 }, { "linethrough", 3 } };

    auto iter = decorationTypeMap.find(NormalizeTextToken(value));
    if (iter == decorationTypeMap.end()) {
        return false;
    }
    type = iter->second;
    return true;
}

bool ParseDecorationStyleToken(const std::string& value, int32_t& style)
{
    static const std::map<std::string, int32_t> decorationStyleMap = { { "solid", 0 }, { "double", 1 }, { "dotted", 2 },
        { "dashed", 3 }, { "wavy", 4 } };

    auto iter = decorationStyleMap.find(NormalizeTextToken(value));
    if (iter == decorationStyleMap.end()) {
        return false;
    }
    style = iter->second;
    return true;
}

bool IsValidFontWeightNumberValue(double value)
{
    constexpr int32_t minFontWeight = 100;
    constexpr int32_t maxFontWeight = 900;
    constexpr int32_t fontWeightStep = 100;
    constexpr double epsilon = 0.0001;

    if (!std::isfinite(value)) {
        return false;
    }

    int32_t normalized = static_cast<int32_t>(std::lround(value));
    return normalized >= minFontWeight && normalized <= maxFontWeight && normalized % fontWeightStep == 0 &&
           std::fabs(value - static_cast<double>(normalized)) <= epsilon;
}

bool IsValidTextFontWeight(const JsonValue& value)
{
    if (value.IsNumber()) {
        return IsValidFontWeightNumberValue(value.GetNumberValue(0.0));
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (token == "lighter" || token == "normal" || token == "regular" || token == "medium" || token == "bold" ||
        token == "bolder") {
        return true;
    }

    float parsedNumber = 0.0F;
    return StyleApplyUtils::ParseNumber(value, parsedNumber) && IsValidFontWeightNumberValue(parsedNumber);
}

bool TryParseTextFontWeight(const JsonValue& value, int32_t& fontWeight)
{
    if (!IsValidTextFontWeight(value)) {
        return false;
    }
    if (!StyleApplyUtils::ParseFontWeight(value, fontWeight)) {
        return false;
    }
    return true;
}

bool TryParsePositiveFiniteStyleScalar(const JsonValue& value, float& parsed)
{
    if (!TryParseFiniteNumberStyleValue(value, parsed)) {
        return false;
    }
    return parsed > 0.0F;
}

bool TryParseMaxLinesNumber(const JsonValue& value, int32_t& maxLines)
{
    return value.IsNumber() && StyleApplyUtils::ParseMaxLines(value, maxLines);
}

bool ShouldWarnTextOverflowWithoutMaxLines(const JsonValue& textOverflowValue, const JsonValue& maxLinesValue)
{
    if (!textOverflowValue.IsValid() || maxLinesValue.IsValid()) {
        return false;
    }

    int32_t textOverflow = 0;
    return StyleApplyUtils::ParseTextOverflow(textOverflowValue, textOverflow) && textOverflow != DEFAULT_TEXT_OVERFLOW;
}

bool HasConflictingAdaptiveFontSizes(
    const JsonValue& minFontSizeValue, const JsonValue& maxFontSizeValue, float minFontSize, float maxFontSize)
{
    return minFontSizeValue.IsValid() && maxFontSizeValue.IsValid() && minFontSize > 0.0F && maxFontSize > 0.0F &&
           minFontSize >= maxFontSize;
}

bool TryParseValidMinFontScale(const JsonValue& value, float& parsed, bool* clamped = nullptr)
{
    if (clamped != nullptr) {
        *clamped = false;
    }
    if (!value.IsNumber()) {
        return false;
    }
    double rawValue = value.GetNumberValue(0.0);
    if (!std::isfinite(rawValue)) {
        return false;
    }
    if (rawValue < 0.0) {
        rawValue = 0.0;
        if (clamped != nullptr) {
            *clamped = true;
        }
    } else if (rawValue > 1.0) {
        rawValue = 1.0;
        if (clamped != nullptr) {
            *clamped = true;
        }
    }
    parsed = static_cast<float>(rawValue);
    return true;
}

bool TryParseValidMaxFontScale(const JsonValue& value, float& parsed, bool* clamped = nullptr)
{
    if (clamped != nullptr) {
        *clamped = false;
    }
    if (!value.IsNumber()) {
        return false;
    }
    double rawValue = value.GetNumberValue(0.0);
    if (!std::isfinite(rawValue)) {
        return false;
    }
    if (rawValue < 1.0) {
        rawValue = 1.0;
        if (clamped != nullptr) {
            *clamped = true;
        }
    }
    parsed = static_cast<float>(rawValue);
    return true;
}

} // namespace

ExtendedTextComponent::ExtendedTextComponent() : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT))
{
    SetText("");
    ApplyDefaultTextStyles();
}

std::string ExtendedTextComponent::GetType() const
{
    return "Text";
}

void ExtendedTextComponent::ReportStyleWarning(
    const std::string& code, const std::string& styleName, const std::string& message) const
{
    ReportSchemaWarning(code, message, "styles." + styleName);
}

void ExtendedTextComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    bool hasContent = descriptor.IsObject() && descriptor.Has("content");
    bool hasTextAlias = descriptor.IsObject() && descriptor.Has("text");
    if (!hasContent && !hasTextAlias) {
        ReportSchemaWarning(
            SCHEMA_ERROR_CODE_INVALID_VALUE, "Property content is missing, fallback to empty string", "content");
        ApplyRuntimeProperty("content", JsonValue(), false);
        return;
    }

    const std::string descriptorKey = hasContent ? "content" : "text";
    JsonValue contentValue = descriptor.GetItem(descriptorKey.c_str());
    if (contentValue.IsString() || IsBindingDescriptorValue(contentValue)) {
        if (hasContent) {
            ApplyDeclaredPropertyOrFallback(descriptor, "content");
        } else {
            SetPropertyFromDescriptor("text", descriptor, "content");
        }
        return;
    }

    ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
        std::string("Property ") + descriptorKey + " expects string value, got type '" + contentValue.GetTypeName() +
            "', fallback to empty string",
        descriptorKey);
    RemoveBindingsForProperty("content");
    ApplyRuntimeProperty("content", JsonValue(), false);
}

PropertyDeclaration ExtendedTextComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName != "content" && propertyName != "text") {
        return {};
    }

    return PropertyDeclaration { .name = propertyName,
        .type = PropertyValueType::STRING,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackString = "",
        .applyValue = [this](const JsonValue& value) { SetText(value.GetStringValue("")); } };
}

void ExtendedTextComponent::ApplyFontWeightAndColorStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue fontWeightValue = styles.GetItem("fontWeight");
    if (fontWeightValue.IsValid()) {
        int32_t parsedFontWeight = static_cast<int32_t>(DEFAULT_TEXT_FONT_WEIGHT);
        bool isValidFontWeight = TryParseTextFontWeight(fontWeightValue, parsedFontWeight);
        if (!isValidFontWeight) {
            if (!fontWeightValue.IsString() && !fontWeightValue.IsNumber()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "fontWeight",
                    std::string("Property fontWeight expects string or number, got type '") +
                        fontWeightValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "fontWeight",
                    "Property fontWeight got invalid value, " + std::string(DescribeStyleResolution(isDeltaUpdate)));
            }
            SetFontWeight(DEFAULT_TEXT_FONT_WEIGHT);
        } else {
            SetFontWeight(static_cast<A2UIFontWeight>(parsedFontWeight));
        }
    }
    uint32_t parsedColor = 0;
    JsonValue fontColorValue = styles.GetItem("fontColor");
    if (TryParseHexColorStringStyleValue(fontColorValue, parsedColor)) {
        useDefaultFontColor_ = false;
        SetFontColor(parsedColor);
    } else {
        if (fontColorValue.IsValid()) {
            if (fontColorValue.IsString()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "fontColor",
                    "Property fontColor got invalid color value, " +
                        std::string(DescribeStyleResolution(isDeltaUpdate)));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "fontColor",
                    std::string("Property fontColor expects string value, got type '") + fontColorValue.GetTypeName() +
                        "', " + DescribeStyleResolution(isDeltaUpdate));
            }
        }
        useDefaultFontColor_ = true;
        SetFontColor(ResolveDefaultFontColor());
    }
}

void ExtendedTextComponent::ApplyFontScaleRangeStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue minFontScaleValue = styles.GetItem("minFontScale");
    float minFontScale = 0.0F;
    bool minFontScaleClamped = false;
    if (TryParseValidMinFontScale(minFontScaleValue, minFontScale, &minFontScaleClamped)) {
        SetMinFontScale(minFontScale);
        if (minFontScaleClamped) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "minFontScale",
                "Property minFontScale got out-of-range number value, clamped to supported range");
        }
    } else if (minFontScaleValue.IsValid()) {
        if (minFontScaleValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "minFontScale",
                "Property minFontScale got invalid number value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "minFontScale",
                std::string("Property minFontScale expects number value, got type '") +
                    minFontScaleValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        }
        SetMinFontScale(0.0F);
    } else if (!isDeltaUpdate) {
        SetMinFontScale(0.0F);
    }
    JsonValue maxFontScaleValue = styles.GetItem("maxFontScale");
    float maxFontScale = 0.0F;
    bool maxFontScaleClamped = false;
    if (TryParseValidMaxFontScale(maxFontScaleValue, maxFontScale, &maxFontScaleClamped)) {
        SetMaxFontScale(maxFontScale);
        if (maxFontScaleClamped) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "maxFontScale",
                "Property maxFontScale got out-of-range number value, clamped to supported range");
        }
    } else if (maxFontScaleValue.IsValid()) {
        if (maxFontScaleValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "maxFontScale",
                "Property maxFontScale got invalid number value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "maxFontScale",
                std::string("Property maxFontScale expects number value, got type '") +
                    maxFontScaleValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        }
        SetMaxFontScale(0.0F);
    } else if (!isDeltaUpdate) {
        SetMaxFontScale(0.0F);
    }
}

void ExtendedTextComponent::ApplyFontScaleModeAndSizeStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue fontScaleModeValue = styles.GetItem("fontScaleMode");
    if (fontScaleModeValue.IsString()) {
        std::string mode = StyleApplyUtils::TrimToken(fontScaleModeValue.GetStringValue(""));
        if (mode == "custom" || mode == DEFAULT_FONT_SCALE_MODE) {
            SetFontScaleMode(mode);
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "fontScaleMode",
                "Property fontScaleMode got invalid enum value '" + mode + "', " +
                    DescribeStyleResolution(isDeltaUpdate));
            SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
        }
    } else if (fontScaleModeValue.IsValid()) {
        ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "fontScaleMode",
            std::string("Property fontScaleMode expects string enum value, got type '") +
                fontScaleModeValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
    } else if (!isDeltaUpdate) {
        SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
    }
    float fontSize = 0.0F;
    JsonValue fontSizeValue = styles.GetItem("fontSize");
    if (TryParsePositiveFiniteStyleScalar(fontSizeValue, fontSize)) {
        SetFontSize(fontSize);
    } else if (fontSizeValue.IsValid()) {
        if (fontSizeValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "fontSize",
                "Property fontSize got invalid number value, " + std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "fontSize",
                std::string("Property fontSize expects number value, got type '") + fontSizeValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
        }
        SetFontSize(DEFAULT_TEXT_FONT_SIZE);
    } else if (!isDeltaUpdate) {
        SetFontSize(DEFAULT_TEXT_FONT_SIZE);
    }
}

void ExtendedTextComponent::ApplyMaxLinesAndOverflowStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue maxLinesValue = styles.GetItem("maxLines");
    if (maxLinesValue.IsValid()) {
        int32_t maxLines = 0;
        if (!TryParseMaxLinesNumber(maxLinesValue, maxLines)) {
            if (maxLinesValue.IsNumber()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "maxLines",
                    "Property maxLines got invalid number value, " +
                        std::string(DescribeStyleResolution(isDeltaUpdate)));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "maxLines",
                    std::string("Property maxLines expects number value, got type '") + maxLinesValue.GetTypeName() +
                        "', " + DescribeStyleResolution(isDeltaUpdate));
            }
            SetMaxLines(DEFAULT_TEXT_MAX_LINES);
        }
    }
    JsonValue textOverflowValue = styles.GetItem("textOverflow");
    if (textOverflowValue.IsValid()) {
        int32_t textOverflow = 0;
        if (!StyleApplyUtils::ParseTextOverflow(textOverflowValue, textOverflow)) {
            if (textOverflowValue.IsString()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "textOverflow",
                    "Property textOverflow got invalid enum value '" +
                        StyleApplyUtils::TrimToken(textOverflowValue.GetStringValue("")) + "', " +
                        DescribeStyleResolution(isDeltaUpdate));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "textOverflow",
                    std::string("Property textOverflow expects string enum value, got type '") +
                        textOverflowValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
            }
            SetTextOverflow(DEFAULT_TEXT_OVERFLOW);
        }
    }
    if (ShouldWarnTextOverflowWithoutMaxLines(textOverflowValue, maxLinesValue)) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property textOverflow is set but maxLines is not configured; "
            "textOverflow effect may not be visible without a finite maxLines value",
            "styles.textOverflow");
    }
}

void ExtendedTextComponent::ApplyTextAlignAndBreakStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue textAlignValue = styles.GetItem("textAlign");
    if (textAlignValue.IsValid()) {
        int32_t textAlign = 0;
        if (!StyleApplyUtils::ParseTextAlign(textAlignValue, textAlign)) {
            if (textAlignValue.IsString()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "textAlign",
                    "Property textAlign got invalid enum value '" +
                        StyleApplyUtils::TrimToken(textAlignValue.GetStringValue("")) + "', " +
                        DescribeStyleResolution(isDeltaUpdate));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "textAlign",
                    std::string("Property textAlign expects string enum value, got type '") +
                        textAlignValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
            }
            SetTextAlign(DEFAULT_TEXT_ALIGN);
        }
    }
    JsonValue wordBreakValue = styles.GetItem("wordBreak");
    if (wordBreakValue.IsValid()) {
        int32_t wordBreak = 0;
        if (!StyleApplyUtils::ParseWordBreak(wordBreakValue, wordBreak)) {
            if (wordBreakValue.IsString()) {
                ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "wordBreak",
                    "Property wordBreak got invalid enum value '" +
                        StyleApplyUtils::TrimToken(wordBreakValue.GetStringValue("")) + "', " +
                        DescribeStyleResolution(isDeltaUpdate));
            } else {
                ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "wordBreak",
                    std::string("Property wordBreak expects string enum value, got type '") +
                        wordBreakValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
            }
            SetWordBreak(DEFAULT_TEXT_WORD_BREAK);
        }
    }
}

void ExtendedTextComponent::ApplyMinFontSizeStyle(const JsonValue& styles, bool isDeltaUpdate, float& minFontSize)
{
    minFontSize = -1.0F;
    JsonValue minFontSizeValue = styles.GetItem("minFontSize");
    if (minFontSizeValue.IsValid()) {
        if (TryParsePositiveFiniteStyleScalar(minFontSizeValue, minFontSize)) {
            // valid, applied by ExtendedStyleResolver above
        } else if (minFontSizeValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "minFontSize",
                "Property minFontSize got invalid number value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
            if (nativeView_ != nullptr) {
                ArkUINodeApiAdapter::ResetNodeTextMinFontSize(nativeView_);
            }
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "minFontSize",
                std::string("Property minFontSize expects number value, got type '") + minFontSizeValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
            minFontSize = -1.0F;
            if (nativeView_ != nullptr) {
                ArkUINodeApiAdapter::ResetNodeTextMinFontSize(nativeView_);
            }
        }
    } else if (!isDeltaUpdate && nativeView_ != nullptr) {
        ArkUINodeApiAdapter::ResetNodeTextMinFontSize(nativeView_);
    }
}

void ExtendedTextComponent::ApplyMaxFontSizeStyle(const JsonValue& styles, bool isDeltaUpdate, float minFontSize)
{
    float maxFontSize = -1.0F;
    JsonValue maxFontSizeValue = styles.GetItem("maxFontSize");
    JsonValue minFontSizeValue = styles.GetItem("minFontSize");
    if (maxFontSizeValue.IsValid()) {
        if (TryParsePositiveFiniteStyleScalar(maxFontSizeValue, maxFontSize)) {
            // valid, applied by ExtendedStyleResolver above
        } else if (maxFontSizeValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "maxFontSize",
                "Property maxFontSize got invalid number value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
            if (nativeView_ != nullptr) {
                ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(nativeView_);
            }
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "maxFontSize",
                std::string("Property maxFontSize expects number value, got type '") + maxFontSizeValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
            maxFontSize = -1.0F;
            if (nativeView_ != nullptr) {
                ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(nativeView_);
            }
        }
    } else if (!isDeltaUpdate && nativeView_ != nullptr) {
        ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(nativeView_);
    }
    if (HasConflictingAdaptiveFontSizes(minFontSizeValue, maxFontSizeValue, minFontSize, maxFontSize)) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property minFontSize (" + std::to_string(minFontSize) + ") must be less than maxFontSize (" +
                std::to_string(maxFontSize) + "); adaptive font sizing has been ignored",
            "styles.minFontSize");
    }
}

void ExtendedTextComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    ExtendedStyleResolver::ApplyTextComponentStyles(styles, applier);
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (HasNonObjectStylePayload(styles)) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles expects object value, got type '" + std::string(styles.GetTypeName()) + "', " +
                DescribeStyleResolution(isDeltaUpdate),
            "styles");
        SetMinFontScale(0.0F);
        SetMaxFontScale(0.0F);
        SetFontScaleMode(DEFAULT_FONT_SCALE_MODE);
        useDefaultFontColor_ = true;
        SetFontColor(ResolveDefaultFontColor());
        SetFontSize(DEFAULT_TEXT_FONT_SIZE);
        SetFontWeight(DEFAULT_TEXT_FONT_WEIGHT);
        SetMaxLines(DEFAULT_TEXT_MAX_LINES);
        SetTextOverflow(DEFAULT_TEXT_OVERFLOW);
        SetTextAlign(DEFAULT_TEXT_ALIGN);
        SetWordBreak(DEFAULT_TEXT_WORD_BREAK);
        if (nativeView_ != nullptr) {
            ArkUINodeApiAdapter::ResetNodeTextMinFontSize(nativeView_);
            ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(nativeView_);
        }
        TextDecorationState defaultDecoration = BuildDefaultDecorationState(ResolveDefaultDecorationColor());
        hasAppliedDecoration_ = true;
        useDefaultDecorationColor_ = true;
        appliedDecoration_ = defaultDecoration;
        ApplyDecorationState(defaultDecoration);
#ifdef TDD_BUILD
        ApplyTextStyleStateForTest(styles);
#endif
        return;
    }

    ApplyFontWeightAndColorStyle(styles, isDeltaUpdate);
    ApplyFontScaleRangeStyle(styles, isDeltaUpdate);
    ApplyFontScaleModeAndSizeStyle(styles, isDeltaUpdate);
    ApplyMaxLinesAndOverflowStyle(styles, isDeltaUpdate);
    ApplyTextAlignAndBreakStyle(styles, isDeltaUpdate);
    float minFontSize = -1.0F;
    ApplyMinFontSizeStyle(styles, isDeltaUpdate, minFontSize);
    ApplyMaxFontSizeStyle(styles, isDeltaUpdate, minFontSize);

    ApplyDecorationStyleWithFallback(styles);
#ifdef TDD_BUILD
    ApplyTextStyleStateForTest(styles);
#endif
}

void ExtendedTextComponent::SetText(const std::string& text)
{
    textValue_ = text;
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextContent(nativeView_, textValue_);
}

void ExtendedTextComponent::ApplyDefaultTextStyles()
{
    useDefaultFontColor_ = true;
    SetFontColor(ResolveDefaultFontColor());
    SetFontSize(DEFAULT_TEXT_FONT_SIZE);
    SetFontWeight(DEFAULT_TEXT_FONT_WEIGHT);
    SetMaxLines(DEFAULT_TEXT_MAX_LINES);
    SetTextAlign(DEFAULT_TEXT_ALIGN);
    SetTextOverflow(DEFAULT_TEXT_OVERFLOW);
    SetWordBreak(DEFAULT_TEXT_WORD_BREAK);
}

uint32_t ExtendedTextComponent::ResolveDefaultFontColor() const
{
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    ExtendedTextTheme theme(themeContext);
    return theme.GetDefaultFontColor();
}

uint32_t ExtendedTextComponent::ResolveDefaultDecorationColor() const
{
    ThemeContext themeContext;
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager != nullptr) {
        themeContext = themeManager->GetContext();
    }
    ExtendedTextTheme theme(themeContext);
    return theme.GetDefaultDecorationColor();
}

bool ExtendedTextComponent::ResolveDecorationWithFallback(
    const JsonValue& styles, TextDecorationState& decoration, bool* usedDefaultColor) const
{
    if (usedDefaultColor != nullptr) {
        *usedDefaultColor = false;
    }
    JsonValue decorationValue = styles.GetItem("decoration");
    if (!decorationValue.IsObject() || decorationValue.Has("path") || decorationValue.Has("call")) {
        return false;
    }

    TextDecorationState nextDecoration;

    JsonValue typeValue = decorationValue.GetItem("type");
    if (typeValue.IsString()) {
        int32_t parsedType = 0;
        if (ParseDecorationTypeToken(typeValue.GetStringValue(""), parsedType)) {
            nextDecoration.type = parsedType;
        }
    }

    JsonValue colorValue = decorationValue.GetItem("color");
    uint32_t parsedColor = 0;
    if (TryParseHexColorStringStyleValue(colorValue, parsedColor)) {
        nextDecoration.color = parsedColor;
        nextDecoration.hasColor = true;
    } else {
        nextDecoration.color = ResolveDefaultDecorationColor();
        nextDecoration.hasColor = true;
        if (usedDefaultColor != nullptr) {
            *usedDefaultColor = true;
        }
    }

    JsonValue styleValue = decorationValue.GetItem("style");
    if (styleValue.IsString()) {
        int32_t parsedStyle = 0;
        if (ParseDecorationStyleToken(styleValue.GetStringValue(""), parsedStyle)) {
            nextDecoration.style = parsedStyle;
        }
    }
    nextDecoration.hasStyle = true;

    JsonValue thicknessScaleValue = decorationValue.GetItem("thicknessScale");
    float parsedThicknessScale = 0.0F;
    if (TryParseFiniteNumberStyleValue(thicknessScaleValue, parsedThicknessScale)) {
        nextDecoration.thicknessScale = parsedThicknessScale;
    } else {
        nextDecoration.thicknessScale = 1.0F;
    }
    nextDecoration.hasThicknessScale = true;

    decoration = nextDecoration;
    return true;
}

void ExtendedTextComponent::ValidateDecorationFields(const JsonValue& styles, bool isDeltaUpdate)
{
    JsonValue decorationValue = styles.GetItem("decoration");
    JsonValue typeValue = decorationValue.GetItem("type");
    if (typeValue.IsValid()) {
        int32_t parsedType = 0;
        if (!typeValue.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "decoration.type",
                std::string("Property decoration.type expects string enum value, got type '") +
                    typeValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        } else if (!ParseDecorationTypeToken(typeValue.GetStringValue(""), parsedType)) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "decoration.type",
                "Property decoration.type got invalid enum value '" +
                    StyleApplyUtils::TrimToken(typeValue.GetStringValue("")) + "', " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        }
    }
    JsonValue colorValue = decorationValue.GetItem("color");
    uint32_t parsedColor = 0;
    if (colorValue.IsValid() && !TryParseHexColorStringStyleValue(colorValue, parsedColor)) {
        if (colorValue.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "decoration.color",
                "Property decoration.color got invalid color value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "decoration.color",
                std::string("Property decoration.color expects string value, got type '") + colorValue.GetTypeName() +
                    "', " + DescribeStyleResolution(isDeltaUpdate));
        }
    }
    JsonValue styleValue = decorationValue.GetItem("style");
    if (styleValue.IsValid()) {
        int32_t parsedStyle = 0;
        if (!styleValue.IsString()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "decoration.style",
                std::string("Property decoration.style expects string enum value, got type '") +
                    styleValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        } else if (!ParseDecorationStyleToken(styleValue.GetStringValue(""), parsedStyle)) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "decoration.style",
                "Property decoration.style has invalid value, " + std::string(DescribeStyleResolution(isDeltaUpdate)));
        }
    }
}

void ExtendedTextComponent::ApplyDecorationStyleWithFallback(const JsonValue& styles)
{
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    JsonValue decorationValue = styles.GetItem("decoration");
    TextDecorationState decoration;
    bool usedDefaultColor = false;
    if (!ResolveDecorationWithFallback(styles, decoration, &usedDefaultColor)) {
        if (decorationValue.IsValid()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "decoration",
                std::string("Property decoration expects valid object with type field, got type '") +
                    decorationValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
            TextDecorationState defaultDecoration = BuildDefaultDecorationState(ResolveDefaultDecorationColor());
            hasAppliedDecoration_ = true;
            useDefaultDecorationColor_ = true;
            appliedDecoration_ = defaultDecoration;
            ApplyDecorationState(defaultDecoration);
        } else if (!isDeltaUpdate) {
            hasAppliedDecoration_ = false;
            useDefaultDecorationColor_ = false;
            appliedDecoration_ = TextDecorationState();
        }
        return;
    }

    ValidateDecorationFields(styles, isDeltaUpdate);

    JsonValue thicknessScaleValue = decorationValue.GetItem("thicknessScale");
    float parsedThicknessScale = 0.0F;
    if (thicknessScaleValue.IsValid() && !TryParseFiniteNumberStyleValue(thicknessScaleValue, parsedThicknessScale)) {
        if (thicknessScaleValue.IsNumber()) {
            ReportStyleWarning(SCHEMA_ERROR_CODE_INVALID_VALUE, "decoration.thicknessScale",
                "Property decoration.thicknessScale has invalid value, " +
                    std::string(DescribeStyleResolution(isDeltaUpdate)));
        } else {
            ReportStyleWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH, "decoration.thicknessScale",
                std::string("Property decoration.thicknessScale expects number value, got type '") +
                    thicknessScaleValue.GetTypeName() + "', " + DescribeStyleResolution(isDeltaUpdate));
        }
    }
    hasAppliedDecoration_ = true;
    useDefaultDecorationColor_ = usedDefaultColor;
    appliedDecoration_ = decoration;
    ApplyDecorationState(decoration);
}

void ExtendedTextComponent::ApplyDecorationState(const TextDecorationState& decoration)
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextDecoration(nativeView_, decoration.type, decoration.hasColor, decoration.color,
        decoration.hasStyle, decoration.style, decoration.hasThicknessScale, decoration.thicknessScale);
}

void ExtendedTextComponent::OnConfigChange(const ThemeContext& context)
{
    ExtendedTextTheme theme(context);
    if (useDefaultFontColor_) {
        SetFontColor(theme.GetDefaultFontColor());
    }
    if (!hasAppliedDecoration_ || !useDefaultDecorationColor_) {
        return;
    }

    appliedDecoration_.color = theme.GetDefaultDecorationColor();
    appliedDecoration_.hasColor = true;
    ApplyDecorationState(appliedDecoration_);
#ifdef TDD_BUILD
    decoration_ = appliedDecoration_;
#endif
}

void ExtendedTextComponent::SetFontColor(uint32_t fontColor)
{
#ifdef TDD_BUILD
    fontColor_ = fontColor;
#endif
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFontColor(nativeView_, fontColor);
}

void ExtendedTextComponent::SetFontSize(float fontSize)
{
    fontSize_ = fontSize;
    float effectiveSize = ComputeEffectiveFontSize(fontSize_);
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFontSize(nativeView_, effectiveSize);
}

void ExtendedTextComponent::SetFontWeight(A2UIFontWeight fontWeight)
{
#ifdef TDD_BUILD
    fontWeight_ = fontWeight;
#endif
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFontWeight(nativeView_, fontWeight);
}

void ExtendedTextComponent::SetMaxLines(int32_t maxLines)
{
    maxLines_ = maxLines >= 0 ? maxLines : DEFAULT_TEXT_MAX_LINES;
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextMaxLines(nativeView_, maxLines_);
}

void ExtendedTextComponent::SetTextAlign(int32_t textAlign)
{
#ifdef TDD_BUILD
    textAlign_ = textAlign;
#endif
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextAlign(nativeView_, textAlign);
}

void ExtendedTextComponent::SetTextOverflow(int32_t textOverflow)
{
#ifdef TDD_BUILD
    textOverflow_ = textOverflow;
#endif
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextOverflow(nativeView_, textOverflow);
}

void ExtendedTextComponent::SetWordBreak(A2UIWordBreak wordBreak)
{
#ifdef TDD_BUILD
    wordBreak_ = wordBreak;
#endif
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeTextWordBreak(nativeView_, static_cast<int32_t>(wordBreak));
}
#ifdef TDD_BUILD
void ExtendedTextComponent::ApplyFontColorFontSizeWeightForTest(const JsonValue& styles, bool isDeltaUpdate)
{
    uint32_t parsedColor = 0;
    JsonValue fontColorValue = styles.GetItem("fontColor");
    if (TryParseHexColorStringStyleValue(fontColorValue, parsedColor)) {
        fontColor_ = parsedColor;
    } else if (fontColorValue.IsValid()) {
        fontColor_ = ResolveDefaultFontColor();
    } else if (!isDeltaUpdate) {
        fontColor_ = ResolveDefaultFontColor();
    }
    float parsedFontSize = 0.0F;
    JsonValue fontSizeValue = styles.GetItem("fontSize");
    if (TryParsePositiveFiniteStyleScalar(fontSizeValue, parsedFontSize)) {
        fontSize_ = parsedFontSize;
    } else if (fontSizeValue.IsValid()) {
        fontSize_ = DEFAULT_TEXT_FONT_SIZE;
    } else if (!isDeltaUpdate) {
        fontSize_ = DEFAULT_TEXT_FONT_SIZE;
    }
    int32_t parsedFontWeight = static_cast<int32_t>(A2UIFontWeight::NORMAL);
    JsonValue fontWeightValue = styles.GetItem("fontWeight");
    if (TryParseTextFontWeight(fontWeightValue, parsedFontWeight)) {
        fontWeight_ = static_cast<A2UIFontWeight>(parsedFontWeight);
    } else if (fontWeightValue.IsValid()) {
        fontWeight_ = DEFAULT_TEXT_FONT_WEIGHT;
    } else if (!isDeltaUpdate) {
        fontWeight_ = DEFAULT_TEXT_FONT_WEIGHT;
    }
}

void ExtendedTextComponent::ApplyFontScaleRangeAndModeForTest(const JsonValue& styles, bool isDeltaUpdate)
{
    float parsedMinFontScale = 0.0F;
    JsonValue minFontScaleValue = styles.GetItem("minFontScale");
    if (TryParseValidMinFontScale(minFontScaleValue, parsedMinFontScale)) {
        minFontScale_ = parsedMinFontScale;
    } else if (minFontScaleValue.IsValid()) {
        minFontScale_ = 0.0F;
    } else if (!isDeltaUpdate) {
        minFontScale_ = 0.0F;
    }
    float parsedMaxFontScale = 0.0F;
    JsonValue maxFontScaleValue = styles.GetItem("maxFontScale");
    if (TryParseValidMaxFontScale(maxFontScaleValue, parsedMaxFontScale)) {
        maxFontScale_ = parsedMaxFontScale;
    } else if (maxFontScaleValue.IsValid()) {
        maxFontScale_ = 0.0F;
    } else if (!isDeltaUpdate) {
        maxFontScale_ = 0.0F;
    }
    JsonValue fontScaleModeValue = styles.GetItem("fontScaleMode");
    if (fontScaleModeValue.IsString()) {
        std::string mode = StyleApplyUtils::TrimToken(fontScaleModeValue.GetStringValue(""));
        if (mode == "custom" || mode == DEFAULT_FONT_SCALE_MODE) {
            fontScaleMode_ = mode;
        } else {
            fontScaleMode_ = DEFAULT_FONT_SCALE_MODE;
        }
    } else if (fontScaleModeValue.IsValid()) {
        fontScaleMode_ = DEFAULT_FONT_SCALE_MODE;
    } else if (!isDeltaUpdate) {
        fontScaleMode_ = DEFAULT_FONT_SCALE_MODE;
    }
}

void ExtendedTextComponent::ApplyMaxLinesAndAdaptiveFontForTest(const JsonValue& styles, bool isDeltaUpdate)
{
    int32_t parsedInt = 0;
    JsonValue maxLinesValue = styles.GetItem("maxLines");
    if (TryParseMaxLinesNumber(maxLinesValue, parsedInt)) {
        maxLines_ = parsedInt;
    } else if (maxLinesValue.IsValid()) {
        maxLines_ = -1;
    } else if (!isDeltaUpdate) {
        maxLines_ = -1;
    }
    float parsedMinFontSize = 0.0F;
    JsonValue minFontSizeValue = styles.GetItem("minFontSize");
    if (TryParsePositiveFiniteStyleScalar(minFontSizeValue, parsedMinFontSize)) {
        minFontSize_ = parsedMinFontSize;
    } else if (minFontSizeValue.IsValid()) {
        minFontSize_ = -1.0F;
    } else if (!isDeltaUpdate) {
        minFontSize_ = -1.0F;
    }
    float parsedMaxFontSize = 0.0F;
    JsonValue maxFontSizeValue = styles.GetItem("maxFontSize");
    if (TryParsePositiveFiniteStyleScalar(maxFontSizeValue, parsedMaxFontSize)) {
        maxFontSize_ = parsedMaxFontSize;
    } else if (maxFontSizeValue.IsValid()) {
        maxFontSize_ = -1.0F;
    } else if (!isDeltaUpdate) {
        maxFontSize_ = -1.0F;
    }
}

void ExtendedTextComponent::ApplyOverflowAlignBreakForTest(const JsonValue& styles, bool isDeltaUpdate)
{
    int32_t parsedInt = 0;
    JsonValue textOverflowValue = styles.GetItem("textOverflow");
    if (StyleApplyUtils::ParseTextOverflow(textOverflowValue, parsedInt)) {
        textOverflow_ = parsedInt;
    } else if (textOverflowValue.IsValid()) {
        textOverflow_ = DEFAULT_TEXT_OVERFLOW;
    } else if (!isDeltaUpdate) {
        textOverflow_ = DEFAULT_TEXT_OVERFLOW;
    }
    JsonValue textAlignValue = styles.GetItem("textAlign");
    if (StyleApplyUtils::ParseTextAlign(textAlignValue, parsedInt)) {
        textAlign_ = parsedInt;
    } else if (textAlignValue.IsValid()) {
        textAlign_ = DEFAULT_TEXT_ALIGN;
    } else if (!isDeltaUpdate) {
        textAlign_ = DEFAULT_TEXT_ALIGN;
    }
    JsonValue wordBreakValue = styles.GetItem("wordBreak");
    if (StyleApplyUtils::ParseWordBreak(wordBreakValue, parsedInt)) {
        wordBreak_ = static_cast<A2UIWordBreak>(parsedInt);
    } else if (wordBreakValue.IsValid()) {
        wordBreak_ = DEFAULT_TEXT_WORD_BREAK;
    } else if (!isDeltaUpdate) {
        wordBreak_ = DEFAULT_TEXT_WORD_BREAK;
    }
}

void ExtendedTextComponent::ApplyTextStyleStateForTest(const JsonValue& styles)
{
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (HasNonObjectStylePayload(styles)) {
        fontColor_ = ResolveDefaultFontColor();
        fontSize_ = DEFAULT_TEXT_FONT_SIZE;
        fontWeight_ = DEFAULT_TEXT_FONT_WEIGHT;
        minFontScale_ = 0.0F;
        maxFontScale_ = 0.0F;
        fontScaleMode_ = DEFAULT_FONT_SCALE_MODE;
        maxLines_ = -1;
        minFontSize_ = -1.0F;
        maxFontSize_ = -1.0F;
        textOverflow_ = DEFAULT_TEXT_OVERFLOW;
        textAlign_ = DEFAULT_TEXT_ALIGN;
        wordBreak_ = DEFAULT_TEXT_WORD_BREAK;
        decoration_ = BuildDefaultDecorationState(ResolveDefaultDecorationColor());
        return;
    }
    if (!isDeltaUpdate) {
        fontColor_ = ResolveDefaultFontColor();
        fontSize_ = DEFAULT_TEXT_FONT_SIZE;
        fontWeight_ = DEFAULT_TEXT_FONT_WEIGHT;
        minFontScale_ = 0.0F;
        maxFontScale_ = 0.0F;
        fontScaleMode_ = DEFAULT_FONT_SCALE_MODE;
        maxLines_ = -1;
        minFontSize_ = -1.0F;
        maxFontSize_ = -1.0F;
        textOverflow_ = DEFAULT_TEXT_OVERFLOW;
        textAlign_ = DEFAULT_TEXT_ALIGN;
        wordBreak_ = DEFAULT_TEXT_WORD_BREAK;
        decoration_ = TextDecorationState();
    }

    ApplyFontColorFontSizeWeightForTest(styles, isDeltaUpdate);
    ApplyFontScaleRangeAndModeForTest(styles, isDeltaUpdate);
    ApplyMaxLinesAndAdaptiveFontForTest(styles, isDeltaUpdate);
    ApplyOverflowAlignBreakForTest(styles, isDeltaUpdate);

    TextDecorationState decoration;
    JsonValue decorationValue = styles.GetItem("decoration");
    if (ResolveDecorationWithFallback(styles, decoration, nullptr)) {
        decoration_ = decoration;
    } else if (decorationValue.IsValid()) {
        decoration_ = BuildDefaultDecorationState(ResolveDefaultDecorationColor());
    } else if (!isDeltaUpdate) {
        decoration_ = TextDecorationState();
    }
}
#endif

void ExtendedTextComponent::SetFontScaleMode(const std::string& mode)
{
    fontScaleMode_ = mode == "custom" ? "custom" : DEFAULT_FONT_SCALE_MODE;
    SetFontSize(fontSize_);
}

void ExtendedTextComponent::OnFontSizeScaleChanged(float newScale)
{
    ExtendedComponent::OnFontSizeScaleChanged(newScale);
    SetFontSize(fontSize_);
}

float ExtendedTextComponent::ComputeEffectiveFontSize(float baseFontSize) const
{
    if (fontScaleMode_ == "custom") {
        const RenderContext& ctx = GetRenderContext();
        float scale = ctx.fontSizeScale > 0.0F ? ctx.fontSizeScale : 1.0F;
        if (minFontScale_ > 0.0F && scale < minFontScale_) {
            scale = minFontScale_;
        }
        if (maxFontScale_ > 0.0F && scale > maxFontScale_) {
            scale = maxFontScale_;
        }
        return baseFontSize * scale;
    }
    return baseFontSize;
}

void ExtendedTextComponent::SetMinFontScale(float minFontScale)
{
    minFontScale_ = minFontScale > 0.0F ? minFontScale : 0.0F;
    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = GetRenderId(),
        .componentId = GetComponentId(),
        .nodeUniqueId = GetNativeNodeUniqueId(),
        .componentType = "Text",
        .attributeName = "minFontScale",
        .floatValue = minFontScale_,
        .reset = minFontScale_ <= 0.0F });
}

void ExtendedTextComponent::SetMaxFontScale(float maxFontScale)
{
    maxFontScale_ = maxFontScale > 0.0F ? maxFontScale : 0.0F;
    const RenderContext& ctx = GetRenderContext();
    if (ctx.apiVersion < MIN_API_VERSION_FONT_SCALE) {
        return;
    }

    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = GetRenderId(),
        .componentId = GetComponentId(),
        .nodeUniqueId = GetNativeNodeUniqueId(),
        .componentType = "Text",
        .attributeName = "maxFontScale",
        .floatValue = maxFontScale_,
        .reset = maxFontScale_ <= 0.0F });
}

} // namespace NativeModule
