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

#include "ExtendedStyleResolver.h"

#include <array>
#include <cfloat>
#include <cmath>
#include <initializer_list>
#include <sstream>

#include "functions/CrossLanguageAttributeBridge.h"
#include "styles/StyleApplyUtils.h"
#include "utils/LogA2UI.h"

#include "A2UIArkUITypeConverter.h"
#include "ArkUIOHApiAdapter.h"
#include "ExtendedCommonTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

std::string FormatStyleInput(const JsonValue& value);

void PushStyleValidationIssue(
    std::vector<DescriptorValidationIssue>& issues, const char* propertyName, const std::string& message)
{
    if (propertyName != nullptr) {
        issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE, message, "styles." + std::string(propertyName) });
    }
}

bool IsDynamicDescriptorObject(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool HasInvalidKnownDimensionField(const JsonValue& value, std::initializer_list<const char*> keys)
{
    if (!value.IsObject()) {
        return false;
    }

    for (const char* key : keys) {
        if (key == nullptr || !value.Has(key)) {
            continue;
        }
        JsonValue fieldValue = value.GetItem(key);
        if (IsDynamicDescriptorObject(fieldValue)) {
            continue;
        }

        StyleDimension dimension;
        if (!StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
            return true;
        }
    }
    return false;
}

void PushNestedDimensionValidationIssue(
    std::vector<DescriptorValidationIssue>& issues, const char* propertyName, const JsonValue& inputValue)
{
    if (propertyName == nullptr) {
        return;
    }
    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver - style=%{public}s applied with invalid nested dimension field(s), input=%{public}s",
        propertyName, FormatStyleInput(inputValue).c_str());
    PushStyleValidationIssue(issues, propertyName,
        "Property styles." + std::string(propertyName) +
            " contains invalid nested value and fallback/reset has been applied");
}

bool ConvertDimensionToFloat(const StyleDimension& dimension, float& value)
{
    switch (dimension.unit) {
        case StyleDimensionUnit::VP:
            if (!std::isfinite(dimension.value) || dimension.value < 0.0F) {
                value = 0.0F;
                return true;
            }
            value = dimension.value;
            return true;
        case StyleDimensionUnit::FP: {
            float densityScale = 1.0F;
            float densityPixels = 0.0F;
            float scaledDensity = 0.0F;
            if (ArkUIOHApiAdapter::GetDefaultDisplayDensityPixels(&densityPixels) == DISPLAY_MANAGER_OK &&
                ArkUIOHApiAdapter::GetDefaultDisplayScaledDensity(&scaledDensity) == DISPLAY_MANAGER_OK &&
                std::isfinite(densityPixels) && densityPixels > 0.0F && std::isfinite(scaledDensity) &&
                scaledDensity > 0.0F) {
                densityScale = scaledDensity / densityPixels;
            }
            value = dimension.value * densityScale;
            if (!std::isfinite(value) || value < 0.0F) {
                value = 0.0F;
            }
            return true;
        }
        case StyleDimensionUnit::PERCENT:
        case StyleDimensionUnit::MATCH_PARENT:
            if (!std::isfinite(dimension.value) || dimension.value < 0.0F) {
                value = 0.0F;
                return true;
            }
            value = dimension.value;
            return true;
        case StyleDimensionUnit::WRAP_CONTENT:
        case StyleDimensionUnit::FIX_AT_IDEAL_SIZE:
            value = 0.0F;
            return true;
        default:
            value = 0.0F;
            return true;
    }
}

float ConvertDimensionToPercentRatio(const StyleDimension& dimension, float value)
{
    if (dimension.unit == StyleDimensionUnit::MATCH_PARENT) {
        return 1.0F;
    }
    return value / 100.0F;
}

bool ConvertDimensionToLayoutPolicy(const StyleDimension& dimension, A2UILayoutPolicy& policy)
{
    switch (dimension.unit) {
        case StyleDimensionUnit::MATCH_PARENT:
            policy = A2UILayoutPolicy::MATCH_PARENT;
            return true;
        case StyleDimensionUnit::WRAP_CONTENT:
            policy = A2UILayoutPolicy::WRAP_CONTENT;
            return true;
        case StyleDimensionUnit::FIX_AT_IDEAL_SIZE:
            policy = A2UILayoutPolicy::FIX_AT_IDEAL_SIZE;
            return true;
        default:
            return false;
    }
}

const char* LayoutPolicyToString(A2UILayoutPolicy policy)
{
    switch (policy) {
        case A2UILayoutPolicy::MATCH_PARENT:
            return "MATCH_PARENT";
        case A2UILayoutPolicy::WRAP_CONTENT:
            return "WRAP_CONTENT";
        case A2UILayoutPolicy::FIX_AT_IDEAL_SIZE:
            return "FIX_AT_IDEAL_SIZE";
    }
    return "UNKNOWN";
}

// Routes layout-semantic size units (match_parent/wrap_content/fix_at_ideal_size) to the ArkUI layout-policy API.
// Returns true when the policy path fully handled the dimension (caller returns); returns false when the caller
// should fall back to the legacy direct-dimension switch (non-mappable units only).
bool ApplyLayoutPolicyDimension(ArkUI_NodeHandle nodeHandle, const StyleDimension& dimension,
    ArkUINodeApiAdapter& applier, bool isWidth, std::optional<ConstraintDispatchContext> dispatchContext)
{
    // isRootNode is derived from componentId (equivalent to Component::IsRootNode()); see MR338 §13.
    bool isRootNode = dispatchContext.has_value() && dispatchContext->componentId == "root";
    uint32_t apiVersion = dispatchContext.has_value() ? dispatchContext->apiVersion : 0;

    A2UILayoutPolicy policy = A2UILayoutPolicy::WRAP_CONTENT;
    if (!ConvertDimensionToLayoutPolicy(dimension, policy)) {
        return false;
    }
    if (dimension.unit == StyleDimensionUnit::MATCH_PARENT && isRootNode) {
        if (isWidth) {
            applier.SetWidthPercent(1.0F);
        } else {
            applier.SetHeightPercent(1.0F);
        }
        return true;
    }
    if (apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY) {
        LOG_A2UI(LOG_INFO,
            "ApplyLayoutPolicyDimension - old apiVersion=%{public}d, dispatch to ArkTS, policy=%{public}s", apiVersion,
            LayoutPolicyToString(policy));
        // Native layout-policy C-API is unavailable below MIN_API_VERSION_LAYOUT_POLICY: route the
        // unit to the ArkTS declarative LayoutPolicy via the cross-language bridge (mirrors the
        // constraintSize percent dispatch path). has_value() is guaranteed here: apiVersion>0 above
        // implies a dispatch context was supplied (nullopt degrades apiVersion to 0).
        const char* axis = isWidth ? "width" : "height";
        std::string payloadJson = R"({"axis":")" + std::string(axis) + R"(","policy":)" +
                                  std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(policy)) + "}";
        CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
            .componentId = dispatchContext->componentId,
            .nodeUniqueId = dispatchContext->nodeUniqueId,
            .componentType = dispatchContext->componentType,
            .attributeName = "layoutPolicy",
            .payloadJson = payloadJson });
        return true;
    }
    if (isWidth) {
        applier.SetNodeWidthLayoutPolicy(
            nodeHandle, A2UIArkUITypeConverter::ToArkUILayoutPolicy(policy), static_cast<int32_t>(apiVersion));
    } else {
        applier.SetNodeHeightLayoutPolicy(
            nodeHandle, A2UIArkUITypeConverter::ToArkUILayoutPolicy(policy), static_cast<int32_t>(apiVersion));
    }
    return true;
}

// Single-pass: parse all fields and collect payload for ETS when percent fields exist.
// - Pure VP/FP: outPercentJson is empty, VP/FP values written to outputs for C++ path.
// - Has percent: outPercentJson contains ALL fields (VP as "Nvp", percent as "N%"),
//   VP/FP outputs are still filled but C++ SetNodeConstraintSize should be skipped.
// Returns true if constraintSize is valid.
bool ParseConstraintSizeStyle(const JsonValue& constraintSize, float& minWidth, float& maxWidth, float& minHeight,
    float& maxHeight, std::string& outPercentJson)
{
    if (!constraintSize.IsObject()) {
        return false;
    }

    minWidth = 0.0F;
    maxWidth = FLT_MAX;
    minHeight = 0.0F;
    maxHeight = FLT_MAX;
    outPercentJson.clear();

    static const char* fields[] = { "minWidth", "maxWidth", "minHeight", "maxHeight" };
    float* outputs[] = { &minWidth, &maxWidth, &minHeight, &maxHeight };

    // First pass: parse all dimensions
    struct ParsedField {
        bool present;
        StyleDimensionUnit unit;
        float value;
    };
    ParsedField parsed[] = { { false, StyleDimensionUnit::INVALID, 0.0F }, { false, StyleDimensionUnit::INVALID, 0.0F },
        { false, StyleDimensionUnit::INVALID, 0.0F }, { false, StyleDimensionUnit::INVALID, 0.0F } };

    bool hasPercent = false;
    for (size_t i = 0; i < 4; ++i) {
        if (!constraintSize.Has(fields[i])) {
            continue;
        }
        StyleDimension dimension;
        if (!StyleApplyUtils::ParseDimension(constraintSize.GetItem(fields[i]), dimension)) {
            return false;
        }
        if (dimension.unit != StyleDimensionUnit::VP && dimension.unit != StyleDimensionUnit::FP &&
            dimension.unit != StyleDimensionUnit::PERCENT) {
            return false;
        }
        float converted = 0.0F;
        if (!ConvertDimensionToFloat(dimension, converted)) {
            return false;
        }
        if (!std::isfinite(converted) || converted < 0.0F) {
            return false;
        }
        parsed[i].present = true;
        parsed[i].unit = dimension.unit;
        parsed[i].value = converted;
        if (dimension.unit == StyleDimensionUnit::PERCENT) {
            hasPercent = true;
        } else {
            *outputs[i] = converted;
        }
    }

    // If any percent field: build full payload with ALL fields for ETS
    if (hasPercent) {
        std::ostringstream stream;
        stream << "{";
        bool first = true;
        for (size_t i = 0; i < 4; ++i) {
            if (!parsed[i].present) {
                continue;
            }
            if (!first) {
                stream << ",";
            }
            first = false;
            if (parsed[i].unit == StyleDimensionUnit::PERCENT) {
                stream << "\"" << fields[i] << "\":\"" << parsed[i].value << "%\"";
            } else {
                stream << "\"" << fields[i] << "\":\"" << parsed[i].value << "vp\"";
            }
        }
        stream << "}";
        outPercentJson = stream.str();
    }
    return true;
}

// Single-pass: parse backgroundImageSize {width, height} and detect percent fields.
// - Pure VP/FP: outPercentJson is empty, numeric values written to outputs for C++ path.
// - Has percent: outPercentJson contains ALL fields (VP as "Nvp", percent as "N%").
// Returns true if backgroundImageSize is valid and should be applied.
bool ParseBackgroundImageSizeWithPercent(
    const JsonValue& backgroundImageSize, float& outWidth, float& outHeight, std::string& outPercentJson)
{
    if (!backgroundImageSize.IsObject()) {
        return false;
    }

    outWidth = 0.0F;
    outHeight = 0.0F;
    outPercentJson.clear();

    static const char* fields[] = { "width", "height" };
    float* outputs[] = { &outWidth, &outHeight };

    struct ParsedField {
        bool present;
        StyleDimensionUnit unit;
        float value;
    };
    ParsedField parsed[] = { { false, StyleDimensionUnit::INVALID, 0.0F },
        { false, StyleDimensionUnit::INVALID, 0.0F } };

    bool hasPercent = false;
    for (size_t i = 0; i < 2; ++i) {
        if (!backgroundImageSize.Has(fields[i])) {
            continue;
        }
        StyleDimension dimension;
        if (!StyleApplyUtils::ParseDimension(backgroundImageSize.GetItem(fields[i]), dimension)) {
            return false;
        }
        if (dimension.unit != StyleDimensionUnit::VP && dimension.unit != StyleDimensionUnit::FP &&
            dimension.unit != StyleDimensionUnit::PERCENT) {
            return false;
        }
        float converted = 0.0F;
        if (!ConvertDimensionToFloat(dimension, converted)) {
            return false;
        }
        if (!std::isfinite(converted) || converted < 0.0F) {
            return false;
        }
        parsed[i].present = true;
        parsed[i].unit = dimension.unit;
        parsed[i].value = converted;
        if (dimension.unit == StyleDimensionUnit::PERCENT) {
            hasPercent = true;
        } else {
            *outputs[i] = converted;
        }
    }

    if (!parsed[0].present && !parsed[1].present) {
        return false;
    }

    if (hasPercent) {
        std::ostringstream stream;
        stream << "{";
        bool first = true;
        for (size_t i = 0; i < 2; ++i) {
            if (!parsed[i].present) {
                continue;
            }
            if (!first) {
                stream << ",";
            }
            first = false;
            if (parsed[i].unit == StyleDimensionUnit::PERCENT) {
                stream << "\"" << fields[i] << "\":\"" << parsed[i].value << "%\"";
            } else {
                stream << "\"" << fields[i] << "\":\"" << parsed[i].value << "vp\"";
            }
        }
        stream << "}";
        outPercentJson = stream.str();
    }
    return true;
}

bool HasSameRadius(const StyleRadius& radius, float& value)
{
    float topLeft = 0.0F;
    float topRight = 0.0F;
    float bottomRight = 0.0F;
    float bottomLeft = 0.0F;
    if (!ConvertDimensionToFloat(radius.topLeft, topLeft) || !ConvertDimensionToFloat(radius.topRight, topRight) ||
        !ConvertDimensionToFloat(radius.bottomRight, bottomRight) ||
        !ConvertDimensionToFloat(radius.bottomLeft, bottomLeft)) {
        return false;
    }
    if (topLeft != topRight || topLeft != bottomRight || topLeft != bottomLeft) {
        return false;
    }
    value = topLeft;
    return true;
}

bool ParseTextComponentFontWeight(const JsonValue& value, int32_t& fontWeight)
{
    return StyleApplyUtils::ParseFontWeight(value, fontWeight);
}

bool TryParseFiniteTextStyleNumber(const JsonValue& value, float& parsed)
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

bool TryParsePositiveTextStyleNumber(const JsonValue& value, float& parsed)
{
    if (!TryParseFiniteTextStyleNumber(value, parsed)) {
        return false;
    }
    return parsed > 0.0F;
}

bool TryParseTextComponentMaxLines(const JsonValue& value, int32_t& maxLines)
{
    return value.IsNumber() && StyleApplyUtils::ParseMaxLines(value, maxLines);
}

std::string FormatStyleInput(const JsonValue& value)
{
    if (!value.IsValid()) {
        return "type=invalid";
    }

    std::ostringstream stream;
    stream << "type=" << value.GetTypeName();
    if (value.IsString()) {
        stream << ", length=" << value.GetStringValue("").size();
    } else if (value.IsArray()) {
        stream << ", size=" << value.GetArraySize();
    } else if (value.IsObject()) {
        int32_t fieldCount = 0;
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            ++fieldCount;
        }
        stream << ", fieldCount=" << fieldCount;
    }
    return stream.str();
}

const char* BoolToString(bool value)
{
    return value ? "true" : "false";
}

const char* DimensionUnitToString(StyleDimensionUnit unit)
{
    switch (unit) {
        case StyleDimensionUnit::VP:
            return "vp";
        case StyleDimensionUnit::FP:
            return "fp";
        case StyleDimensionUnit::PERCENT:
            return "%";
        case StyleDimensionUnit::MATCH_PARENT:
            return "match_parent";
        case StyleDimensionUnit::WRAP_CONTENT:
            return "wrap_content";
        case StyleDimensionUnit::FIX_AT_IDEAL_SIZE:
            return "fix_at_ideal_size";
        case StyleDimensionUnit::INVALID:
        default:
            return "invalid";
    }
}

const char* ShadowKindToString(StyleShadowKind kind)
{
    switch (kind) {
        case StyleShadowKind::STYLE:
            return "style";
        case StyleShadowKind::CUSTOM:
            return "custom";
        default:
            return "unknown";
    }
}

bool HasAnyField(const JsonValue& styles, std::initializer_list<const char*> keys)
{
    if (!styles.IsObject()) {
        return false;
    }

    for (const char* key : keys) {
        if (key != nullptr && styles.Has(key)) {
            return true;
        }
    }
    return false;
}

std::string FormatDimension(const StyleDimension& dimension)
{
    if (dimension.unit == StyleDimensionUnit::MATCH_PARENT || dimension.unit == StyleDimensionUnit::WRAP_CONTENT ||
        dimension.unit == StyleDimensionUnit::FIX_AT_IDEAL_SIZE || dimension.unit == StyleDimensionUnit::INVALID) {
        return DimensionUnitToString(dimension.unit);
    }

    std::ostringstream stream;
    stream << dimension.value << DimensionUnitToString(dimension.unit);
    return stream.str();
}

std::string FormatEdge(const StyleEdge& edge)
{
    std::ostringstream stream;
    stream << "[top=" << FormatDimension(edge.top) << ", right=" << FormatDimension(edge.right)
           << ", bottom=" << FormatDimension(edge.bottom) << ", left=" << FormatDimension(edge.left) << "]";
    return stream.str();
}

std::string FormatRadius(const StyleRadius& radius)
{
    std::ostringstream stream;
    stream << "[topLeft=" << FormatDimension(radius.topLeft) << ", topRight=" << FormatDimension(radius.topRight)
           << ", bottomRight=" << FormatDimension(radius.bottomRight)
           << ", bottomLeft=" << FormatDimension(radius.bottomLeft) << "]";
    return stream.str();
}

std::string FormatNamedStyleInputs(const JsonValue& styles, std::initializer_list<const char*> keys)
{
    if (!styles.IsObject()) {
        return "<not_object>";
    }

    std::ostringstream stream;
    stream << "{";
    bool hasAny = false;
    for (const char* key : keys) {
        if (key == nullptr || !styles.Has(key)) {
            continue;
        }
        if (hasAny) {
            stream << ", ";
        }
        stream << key << "=" << FormatStyleInput(styles.GetItem(key));
        hasAny = true;
    }
    if (!hasAny) {
        return "{}";
    }
    stream << "}";
    return stream.str();
}

} // namespace

void ExtendedStyleResolver::ResolveAndApply(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    if (!styles.IsObject()) {
        if (styles.IsValid()) {
            LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ResolveAndApply - styles is not object, type=%{public}s",
                styles.GetTypeName());
        }
        return;
    }

    ApplySizeStyles(styles, applier, issues, dispatchContext);
    ApplyColorStyles(styles, applier, issues);
    ApplyTextStyles(styles, applier);
    ApplyEdgeStyles(styles, applier, issues);
    ApplyDecorationStyles(styles, applier, dispatchContext, issues);
    ApplyCommonNodeStyles(styles, applier, dispatchContext, issues);
}

void ExtendedStyleResolver::ResolveAndApply(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::optional<ConstraintDispatchContext> dispatchContext)
{
    std::vector<DescriptorValidationIssue> unusedIssues;
    ResolveAndApply(styles, applier, dispatchContext, unusedIssues);
}

void ExtendedStyleResolver::ApplyTextComponentStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    if (!styles.IsObject()) {
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - root node is null, styleInputs=%{public}s",
            FormatNamedStyleInputs(styles, { "fontWeight", "maxLines", "textOverflow", "textAlign", "decoration" })
                .c_str());
        return;
    }

    JsonValue fontWeightValue = styles.GetItem("fontWeight");
    int32_t fontWeight = 0;
    if (ParseTextComponentFontWeight(fontWeightValue, fontWeight)) {
        applier.SetNodeFontWeight(nodeHandle, static_cast<A2UIFontWeight>(fontWeight));
    } else if (fontWeightValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=fontWeight ignored, invalid input=%{public}s",
            FormatStyleInput(fontWeightValue).c_str());
    }

    JsonValue maxLinesValue = styles.GetItem("maxLines");
    int32_t number = 0;
    if (TryParseTextComponentMaxLines(maxLinesValue, number)) {
        applier.SetNodeTextMaxLines(nodeHandle, number);
    } else if (maxLinesValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=maxLines ignored, invalid input=%{public}s",
            FormatStyleInput(maxLinesValue).c_str());
    }

    float fontSize = 0.0F;
    JsonValue minFontSizeValue = styles.GetItem("minFontSize");
    if (TryParsePositiveTextStyleNumber(minFontSizeValue, fontSize)) {
        applier.SetNodeTextMinFontSize(nodeHandle, fontSize);
    }

    JsonValue maxFontSizeValue = styles.GetItem("maxFontSize");
    if (TryParsePositiveTextStyleNumber(maxFontSizeValue, fontSize)) {
        applier.SetNodeTextMaxFontSize(nodeHandle, fontSize);
    }

    JsonValue textOverflowValue = styles.GetItem("textOverflow");
    if (StyleApplyUtils::ParseTextOverflow(textOverflowValue, number)) {
        applier.SetNodeTextOverflow(nodeHandle, number);
    } else if (textOverflowValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=textOverflow ignored, invalid input=%{public}s",
            FormatStyleInput(textOverflowValue).c_str());
    }

    JsonValue textAlignValue = styles.GetItem("textAlign");
    if (StyleApplyUtils::ParseTextAlign(textAlignValue, number)) {
        applier.SetNodeTextAlign(nodeHandle, number);
    } else if (textAlignValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=textAlign ignored, invalid input=%{public}s",
            FormatStyleInput(textAlignValue).c_str());
    }
    if (StyleApplyUtils::ParseWordBreak(styles.GetItem("wordBreak"), number)) {
        applier.SetNodeTextWordBreak(nodeHandle, number);
    }

    ApplyTextDecoration(styles.GetItem("decoration"), applier);
}

void ExtendedStyleResolver::ApplySizeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::vector<DescriptorValidationIssue>& issues, std::optional<ConstraintDispatchContext> dispatchContext)
{
    ApplyDimension(styles.GetItem("width"), applier, true, issues, dispatchContext);
    ApplyDimension(styles.GetItem("height"), applier, false, issues, dispatchContext);
}

void ExtendedStyleResolver::ApplyColorStyles(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    JsonValue backgroundColorValue = styles.GetItem("backgroundColor");
    uint32_t color = 0;
    if (StyleApplyUtils::ParseColor(backgroundColorValue, color)) {
        applier.SetBackgroundColor(color);
    } else if (backgroundColorValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyColorStyles - style=backgroundColor reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(backgroundColorValue).c_str());
        PushStyleValidationIssue(issues, "backgroundColor",
            "Property styles.backgroundColor has invalid value and has been reset to default");
        Reset({ .rawName = "backgroundColor", .name = StylePropertyName::BACKGROUND_COLOR }, applier);
    }

    JsonValue borderColorValue = styles.GetItem("borderColor");
    if (StyleApplyUtils::ParseColor(borderColorValue, color)) {
        applier.SetNodeBorderColor(applier.GetRootNode(), color);
    } else if (borderColorValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyColorStyles - style=borderColor reset to default, invalid input=%{public}s",
            FormatStyleInput(borderColorValue).c_str());
        PushStyleValidationIssue(
            issues, "borderColor", "Property styles.borderColor has invalid value and has been reset to default");
        Reset({ .rawName = "borderColor", .name = StylePropertyName::BORDER_COLOR }, applier);
    }
}

void ExtendedStyleResolver::ApplyTextStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    JsonValue fontColorValue = styles.GetItem("fontColor");
    uint32_t color = 0;
    if (StyleApplyUtils::ParseColor(fontColorValue, color)) {
        applier.SetNodeFontColor(applier.GetRootNode(), color);
    } else if (fontColorValue.IsValid()) {
        LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyTextStyles - style=fontColor ignored, invalid input=%{public}s",
            FormatStyleInput(fontColorValue).c_str());
    }

    JsonValue fontSizeValue = styles.GetItem("fontSize");
    float number = 0.0F;
    if (StyleApplyUtils::ParseNumber(fontSizeValue, number)) {
        applier.SetNodeFontSize(applier.GetRootNode(), number);
    } else if (fontSizeValue.IsValid()) {
        LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyTextStyles - style=fontSize ignored, invalid input=%{public}s",
            FormatStyleInput(fontSizeValue).c_str());
    }

    JsonValue fontWeightValue = styles.GetItem("fontWeight");
    int32_t fontWeight = 0;
    if (StyleApplyUtils::ParseFontWeight(fontWeightValue, fontWeight)) {
        applier.SetNodeFontWeight(applier.GetRootNode(), static_cast<A2UIFontWeight>(fontWeight));
    } else if (fontWeightValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextStyles - style=fontWeight ignored, invalid input=%{public}s",
            FormatStyleInput(fontWeightValue).c_str());
    }
}

void ExtendedStyleResolver::ApplyEdgeStyles(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    StyleEdge edge;
    bool hasPaddingInput = HasAnyField(styles, { "padding", "top", "right", "bottom", "left" });
    JsonValue paddingValue = styles.GetItem("padding");
    bool paddingHasInvalidNestedField = HasInvalidKnownDimensionField(
        paddingValue, { "all", "vertical", "horizontal", "top", "right", "bottom", "left" });
    if (ParseEdgeStyle(styles, "padding", "top", "right", "bottom", "left", edge)) {
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        float left = 0.0F;
        bool paddingApplied = false;
        if (DimensionToFloat(edge.top, top) && DimensionToFloat(edge.right, right) &&
            DimensionToFloat(edge.bottom, bottom) && DimensionToFloat(edge.left, left)) {
            bool hasPercent = false;
            bool hasAbsolute = false;
            const std::array<StyleDimension, 4> dimensions = { edge.top, edge.right, edge.bottom, edge.left };
            for (const auto& dimension : dimensions) {
                if (dimension.value == 0.0F) {
                    continue;
                }
                if (dimension.unit == StyleDimensionUnit::PERCENT) {
                    hasPercent = true;
                } else {
                    hasAbsolute = true;
                }
            }
            if (hasPercent && hasAbsolute) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, mixed units are not "
                    "supported, input=%{public}s",
                    FormatEdge(edge).c_str());
                PushStyleValidationIssue(issues, "padding",
                    "Property styles.padding has mixed units which are not supported and has been reset to default");
                Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
                // continue to margin processing instead of return
            } else if (hasPercent) {
                applier.SetPaddingPercent(top / 100.0F, right / 100.0F, bottom / 100.0F, left / 100.0F);
                paddingApplied = true;
            } else {
                applier.SetPadding(top, right, bottom, left);
                paddingApplied = true;
            }
        } else {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, dimension conversion failed, "
                "input=%{public}s",
                FormatEdge(edge).c_str());
            PushStyleValidationIssue(
                issues, "padding", "Property styles.padding has invalid value and has been reset to default");
            Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
        }
        if (paddingApplied && paddingHasInvalidNestedField) {
            PushNestedDimensionValidationIssue(issues, "padding", paddingValue);
        }
    } else if (hasPaddingInput) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, invalid input=%{public}s",
            FormatNamedStyleInputs(styles, { "padding", "top", "right", "bottom", "left" }).c_str());
        PushStyleValidationIssue(
            issues, "padding", "Property styles.padding has invalid value and has been reset to default");
        Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
    }

    bool hasMarginInput = HasAnyField(styles, { "margin", "marginTop", "marginRight", "marginBottom", "marginLeft" });
    JsonValue marginValue = styles.GetItem("margin");
    bool marginHasInvalidNestedField = HasInvalidKnownDimensionField(
        marginValue, { "all", "vertical", "horizontal", "top", "right", "bottom", "left" });
    if (ParseEdgeStyle(styles, "margin", "marginTop", "marginRight", "marginBottom", "marginLeft", edge)) {
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        float left = 0.0F;
        bool marginApplied = false;
        if (DimensionToFloat(edge.top, top) && DimensionToFloat(edge.right, right) &&
            DimensionToFloat(edge.bottom, bottom) && DimensionToFloat(edge.left, left)) {
            bool hasPercent = false;
            bool hasAbsolute = false;
            const std::array<StyleDimension, 4> dimensions = { edge.top, edge.right, edge.bottom, edge.left };
            for (const auto& dimension : dimensions) {
                if (dimension.value == 0.0F) {
                    continue;
                }
                if (dimension.unit == StyleDimensionUnit::PERCENT) {
                    hasPercent = true;
                } else {
                    hasAbsolute = true;
                }
            }
            if (hasPercent && hasAbsolute) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, mixed units are not "
                    "supported, input=%{public}s",
                    FormatEdge(edge).c_str());
                PushStyleValidationIssue(issues, "margin",
                    "Property styles.margin has mixed units which are not supported and has been reset to default");
                Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
            } else if (hasPercent) {
                applier.SetMarginPercent(top / 100.0F, right / 100.0F, bottom / 100.0F, left / 100.0F);
                marginApplied = true;
            } else {
                applier.SetMargin(top, right, bottom, left);
                marginApplied = true;
            }
        } else {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, dimension conversion failed, "
                "input=%{public}s",
                FormatEdge(edge).c_str());
            PushStyleValidationIssue(
                issues, "margin", "Property styles.margin has invalid value and has been reset to default");
            Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
        }
        if (marginApplied && marginHasInvalidNestedField) {
            PushNestedDimensionValidationIssue(issues, "margin", marginValue);
        }
    } else if (hasMarginInput) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, invalid input=%{public}s",
            FormatNamedStyleInputs(styles, { "margin", "marginTop", "marginRight", "marginBottom", "marginLeft" })
                .c_str());
        PushStyleValidationIssue(
            issues, "margin", "Property styles.margin has invalid value and has been reset to default");
        Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
    }
}

void ExtendedStyleResolver::ApplyDecorationStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    float number = 0.0F;
    JsonValue borderWidthValue = styles.GetItem("borderWidth");
    StyleDimension borderWidthDimension;
    if (StyleApplyUtils::ParseDimension(borderWidthValue, borderWidthDimension)) {
        if (borderWidthDimension.unit == StyleDimensionUnit::VP ||
            borderWidthDimension.unit == StyleDimensionUnit::FP) {
            if (DimensionToFloat(borderWidthDimension, number)) {
                applier.ResetNodeBorderWidthPercent(applier.GetRootNode());
                applier.SetNodeBorderWidth(applier.GetRootNode(), number);
            } else if (borderWidthValue.IsValid()) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyDecorationStyles - style=borderWidth reset to default, conversion "
                    "failed, input=%{public}s",
                    FormatStyleInput(borderWidthValue).c_str());
                PushStyleValidationIssue(issues, "borderWidth",
                    "Property styles.borderWidth conversion failed and has been reset to default");
                Reset({ .rawName = "borderWidth", .name = StylePropertyName::BORDER_WIDTH }, applier);
            }
        } else if (borderWidthDimension.unit == StyleDimensionUnit::PERCENT) {
            if (DimensionToFloat(borderWidthDimension, number)) {
                applier.ResetNodeBorderWidth(applier.GetRootNode());
                applier.SetBorderWidthPercent(number / 100.0F);
            } else if (borderWidthValue.IsValid()) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyDecorationStyles - style=borderWidth reset to default, percent "
                    "conversion failed, input=%{public}s",
                    FormatStyleInput(borderWidthValue).c_str());
                PushStyleValidationIssue(issues, "borderWidth",
                    "Property styles.borderWidth percent conversion failed and has been reset to default");
                Reset({ .rawName = "borderWidth", .name = StylePropertyName::BORDER_WIDTH }, applier);
            }
        } else if (borderWidthValue.IsValid()) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyDecorationStyles - style=borderWidth reset to default, invalid "
                "unit=%{public}s, input=%{public}s",
                DimensionUnitToString(borderWidthDimension.unit), FormatStyleInput(borderWidthValue).c_str());
            PushStyleValidationIssue(issues, "borderWidth",
                "Property styles.borderWidth has unsupported unit and has been reset to default");
            Reset({ .rawName = "borderWidth", .name = StylePropertyName::BORDER_WIDTH }, applier);
        }
    } else if (borderWidthValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDecorationStyles - style=borderWidth reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(borderWidthValue).c_str());
        PushStyleValidationIssue(
            issues, "borderWidth", "Property styles.borderWidth has invalid value and has been reset to default");
        Reset({ .rawName = "borderWidth", .name = StylePropertyName::BORDER_WIDTH }, applier);
    }

    JsonValue opacityValue = styles.GetItem("opacity");
    if (StyleApplyUtils::ParseNumber(opacityValue, number)) {
        applier.SetNodeOpacity(applier.GetRootNode(), number);
    } else if (opacityValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDecorationStyles - style=opacity ignored, invalid input=%{public}s",
            FormatStyleInput(opacityValue).c_str());
    }

    JsonValue visibilityValue = styles.GetItem("visibility");
    A2UIVisibility visibility = A2UIVisibility::VISIBLE;
    if (StyleApplyUtils::ParseVisibility(visibilityValue, visibility)) {
        applier.SetNodeVisibility(applier.GetRootNode(), visibility);
    } else if (visibilityValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDecorationStyles - style=visibility reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(visibilityValue).c_str());
        PushStyleValidationIssue(
            issues, "visibility", "Property styles.visibility has invalid value and has been reset to default");
        Reset({ .rawName = "visibility", .name = StylePropertyName::VISIBILITY }, applier);
    }

    ApplyRadius(styles.GetItem("borderRadius"), applier, dispatchContext, issues);

    ApplyShadow(styles.GetItem("shadow"), applier, issues,
        dispatchContext.has_value() ? dispatchContext->commonTheme : nullptr);
    ApplyBackgroundImageSize(
        styles.GetItem("backgroundImageSize"), "backgroundImageSize", applier, dispatchContext, issues);
    ApplyBackgroundImageSize(
        styles.GetItem("backgroundimageSize"), "backgroundimageSize", applier, dispatchContext, issues);
    ApplyBackgroundImageSize(styles.GetItem("backgroundImageSizeWithStyle"), "backgroundImageSizeWithStyle", applier,
        dispatchContext, issues);
    ApplyBackgroundImageSize(styles.GetItem("backgroundimageSizeWithStyle"), "backgroundimageSizeWithStyle", applier,
        dispatchContext, issues);
    ApplyLinearGradient(styles.GetItem("linearGradient"), applier, issues);
}

void ExtendedStyleResolver::ApplyCommonNodeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyCommonNodeStyles - root node is null, styleInputs=%{public}s",
            FormatNamedStyleInputs(styles,
                { "flexShrink", "backgroundImage", "backgroundimage", "clip", "layoutWeight", "constraintSize" })
                .c_str());
        return;
    }

    JsonValue flexShrinkValue = styles.GetItem("flexShrink");
    float flexShrink = 0.0F;
    if (flexShrinkValue.IsValid() && StyleApplyUtils::ParseFlexShrink(flexShrinkValue, flexShrink)) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=flexShrink, input=%{public}s, "
            "applied=%{public}f, attribute=NODE_FLEX_SHRINK",
            FormatStyleInput(flexShrinkValue).c_str(), flexShrink);
        applier.SetNodeFlexShrink(nodeHandle, flexShrink);
    } else if (flexShrinkValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=flexShrink reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(flexShrinkValue).c_str());
        PushStyleValidationIssue(
            issues, "flexShrink", "Property styles.flexShrink has invalid value and has been reset to default");
        Reset({ .rawName = "flexShrink", .name = StylePropertyName::FLEX_SHRINK }, applier);
    }

    ApplyBackgroundImage("backgroundImage", styles.GetItem("backgroundImage"), applier, issues);
    ApplyBackgroundImage("backgroundimage", styles.GetItem("backgroundimage"), applier, issues);

    JsonValue clipValue = styles.GetItem("clip");
    bool clip = false;
    if (clipValue.IsValid() && StyleApplyUtils::ParseClip(clipValue, clip)) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=clip, input=%{public}s, applied=%{public}s, "
            "attribute=NODE_CLIP",
            FormatStyleInput(clipValue).c_str(), BoolToString(clip));
        applier.SetNodeClip(nodeHandle, clip);
    } else if (clipValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=clip reset to default, invalid input=%{public}s",
            FormatStyleInput(clipValue).c_str());
        PushStyleValidationIssue(
            issues, "clip", "Property styles.clip has invalid value and has been reset to default");
        Reset({ .rawName = "clip", .name = StylePropertyName::CLIP }, applier);
    }

    JsonValue layoutWeightValue = styles.GetItem("layoutWeight");
    float layoutWeight = 0.0F;
    if (layoutWeightValue.IsValid() && StyleApplyUtils::ParseNumber(layoutWeightValue, layoutWeight) &&
        std::isfinite(layoutWeight) && layoutWeight >= 0.0F) {
        if (layoutWeight > 0.0F) {
            applier.SetNodeLayoutWeight(nodeHandle, static_cast<uint32_t>(layoutWeight));
        }
    } else if (layoutWeightValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=layoutWeight reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(layoutWeightValue).c_str());
        PushStyleValidationIssue(
            issues, "layoutWeight", "Property styles.layoutWeight has invalid value and has been reset to default");
        Reset({ .rawName = "layoutWeight", .name = StylePropertyName::LAYOUT_WEIGHT }, applier);
    }

    JsonValue constraintSizeValue = styles.GetItem("constraintSize");
    float minWidth = 0.0F;
    float maxWidth = FLT_MAX;
    float minHeight = 0.0F;
    float maxHeight = FLT_MAX;
    std::string percentJson;
    if (constraintSizeValue.IsValid() &&
        ParseConstraintSizeStyle(constraintSizeValue, minWidth, maxWidth, minHeight, maxHeight, percentJson)) {
        if (percentJson.empty()) {
            // Pure VP/FP: C++ fast path
            applier.SetNodeConstraintSize(nodeHandle, minWidth, maxWidth, minHeight, maxHeight);
        } else {
            // Has percent: dispatch all fields to ETS via cross-language bridge
            if (dispatchContext.has_value()) {
                LOG_A2UI(LOG_INFO,
                    "ExtendedStyleResolver::ApplyCommonNodeStyles - constraintSize percent dispatched to ETS, "
                    "componentId=%{public}s, payload=%{public}s",
                    dispatchContext->componentId.c_str(), percentJson.c_str());
                CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
                    .componentId = dispatchContext->componentId,
                    .nodeUniqueId = dispatchContext->nodeUniqueId,
                    .componentType = dispatchContext->componentType,
                    .attributeName = "constraintSize",
                    .payloadJson = percentJson });
            }
        }
    } else if (constraintSizeValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=constraintSize reset to default, invalid "
            "input=%{public}s",
            FormatStyleInput(constraintSizeValue).c_str());
        PushStyleValidationIssue(
            issues, "constraintSize", "Property styles.constraintSize has invalid value and has been reset to default");
        Reset({ .rawName = "constraintSize", .name = StylePropertyName::CONSTRAINT_SIZE }, applier);
    }
}

void ExtendedStyleResolver::ApplyBackgroundImage(const char* propertyName, const JsonValue& value,
    ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    std::string backgroundImage;
    if (!value.IsValid()) {
        return;
    }
    if (!StyleApplyUtils::ParseBackgroundImage(value, backgroundImage)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=%{public}s reset to default, invalid "
            "input=%{public}s",
            propertyName, FormatStyleInput(value).c_str());
        PushStyleValidationIssue(issues, propertyName,
            "Property styles." + std::string(propertyName) + " has invalid value and has been reset to default");
        Reset({ .rawName = propertyName, .name = StylePropertyName::BACKGROUND_IMAGE }, applier);
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=%{public}s skipped, root node is null, "
            "input=%{public}s",
            propertyName, FormatStyleInput(value).c_str());
        return;
    }

    if (backgroundImage.empty()) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=%{public}s, input=%{public}s, "
            "applied=<reset>, attribute=NODE_BACKGROUND_IMAGE",
            propertyName, FormatStyleInput(value).c_str());
        applier.ResetNodeBackgroundImage(nodeHandle);
        return;
    }

    LOG_A2UI(LOG_INFO,
        "ExtendedStyleResolver::ApplyCommonNodeStyles - style=%{public}s, input=%{public}s, "
        "appliedLength=%{public}zu, attribute=NODE_BACKGROUND_IMAGE",
        propertyName, FormatStyleInput(value).c_str(), backgroundImage.size());
    applier.SetNodeBackgroundImage(nodeHandle, backgroundImage);
}

void ExtendedStyleResolver::ApplyTextDecoration(const JsonValue& value, ArkUINodeApiAdapter& applier)
{
    StyleTextDecoration decoration;
    if (!StyleApplyUtils::ParseTextDecoration(value, decoration)) {
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        return;
    }

    applier.SetNodeTextDecoration(nodeHandle, decoration.type, decoration.hasColor, decoration.color,
        decoration.hasStyle, decoration.style, decoration.hasThicknessScale, decoration.thicknessScale);
}

void ExtendedStyleResolver::Reset(const StyleResetProperty& property, ArkUINodeApiAdapter& applier, int32_t apiVersion)
{
    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr || property.rawName.empty()) {
        return;
    }

    switch (property.name) {
        case StylePropertyName::WIDTH:
            applier.ResetNodeWidth(nodeHandle);
            applier.ResetNodeWidthPercent(nodeHandle);
            applier.ResetNodeWidthLayoutPolicy(nodeHandle, apiVersion);
            break;
        case StylePropertyName::HEIGHT:
            applier.ResetNodeHeight(nodeHandle);
            applier.ResetNodeHeightPercent(nodeHandle);
            applier.ResetNodeHeightLayoutPolicy(nodeHandle, apiVersion);
            break;
        case StylePropertyName::PADDING:
            applier.ResetNodePadding(nodeHandle);
            applier.ResetNodePaddingPercent(nodeHandle);
            break;
        case StylePropertyName::MARGIN:
            applier.ResetNodeMargin(nodeHandle);
            applier.ResetNodeMarginPercent(nodeHandle);
            break;
        case StylePropertyName::BACKGROUND_COLOR:
            applier.ResetNodeBackgroundColor(nodeHandle);
            break;
        case StylePropertyName::BORDER_RADIUS:
            applier.ResetNodeBorderRadius(nodeHandle);
            applier.ResetNodeBorderRadiusPercent(nodeHandle);
            break;
        case StylePropertyName::BORDER_WIDTH:
            applier.ResetNodeBorderWidth(nodeHandle);
            applier.ResetNodeBorderWidthPercent(nodeHandle);
            break;
        case StylePropertyName::BORDER_COLOR:
            applier.ResetNodeBorderColor(nodeHandle);
            break;
        case StylePropertyName::FONT_COLOR:
            applier.ResetNodeFontColor(nodeHandle);
            break;
        case StylePropertyName::FONT_SIZE:
            applier.ResetNodeFontSize(nodeHandle);
            break;
        case StylePropertyName::FONT_WEIGHT:
            applier.ResetNodeFontWeight(nodeHandle);
            break;
        case StylePropertyName::TEXT_ALIGN:
            applier.ResetNodeTextAlign(nodeHandle);
            break;
        case StylePropertyName::MAX_LINES:
            applier.ResetNodeTextMaxLines(nodeHandle);
            applier.ResetNodeTextInputNumberOfLines(nodeHandle);
            break;
        case StylePropertyName::TEXT_MIN_FONT_SIZE:
            applier.ResetNodeTextMinFontSize(nodeHandle);
            break;
        case StylePropertyName::TEXT_MAX_FONT_SIZE:
            applier.ResetNodeTextMaxFontSize(nodeHandle);
            break;
        case StylePropertyName::TEXT_OVERFLOW:
            applier.ResetNodeTextOverflow(nodeHandle);
            break;
        case StylePropertyName::WORD_BREAK:
            applier.ResetNodeTextWordBreak(nodeHandle);
            break;
        case StylePropertyName::DECORATION:
            applier.ResetNodeTextDecoration(nodeHandle);
            break;
        case StylePropertyName::VISIBILITY:
            applier.ResetNodeVisibility(nodeHandle);
            break;
        case StylePropertyName::OPACITY:
            applier.ResetNodeOpacity(nodeHandle);
            break;
        case StylePropertyName::SHADOW:
            applier.ResetNodeShadow(nodeHandle);
            applier.ResetNodeCustomShadow(nodeHandle);
            break;
        case StylePropertyName::FLEX_SHRINK:
            applier.ResetNodeFlexShrink(nodeHandle);
            break;
        case StylePropertyName::BACKGROUND_IMAGE:
            applier.ResetNodeBackgroundImage(nodeHandle);
            break;
        case StylePropertyName::BACKGROUND_IMAGE_SIZE:
            applier.ResetNodeBackgroundImageSize(nodeHandle);
            applier.ResetNodeBackgroundImageSizeWithStyle(nodeHandle);
            break;
        case StylePropertyName::LINEAR_GRADIENT:
            applier.ResetNodeLinearGradient(nodeHandle);
            break;
        case StylePropertyName::CLIP:
            applier.ResetNodeClip(nodeHandle);
            break;
        case StylePropertyName::PLACEHOLDER_COLOR:
            applier.ResetNodeTextInputPlaceholderColor(nodeHandle);
            break;
        case StylePropertyName::LAYOUT_WEIGHT:
            applier.ResetNodeLayoutWeight(nodeHandle);
            break;
        case StylePropertyName::CONSTRAINT_SIZE:
            applier.ResetNodeConstraintSize(nodeHandle);
            break;
        default:
            break;
    }
}

bool ExtendedStyleResolver::ParseColor(const JsonValue& value, uint32_t& color)
{
    return StyleApplyUtils::ParseColor(value, color);
}

bool ExtendedStyleResolver::ParseEdgeStyle(const JsonValue& styles, const char* allKey, const char* topKey,
    const char* rightKey, const char* bottomKey, const char* leftKey, StyleEdge& edge)
{
    if (!styles.IsObject()) {
        return false;
    }

    StyleDimension zeroDimension;
    zeroDimension.unit = StyleDimensionUnit::VP;
    zeroDimension.value = 0.0F;
    edge.top = zeroDimension;
    edge.right = zeroDimension;
    edge.bottom = zeroDimension;
    edge.left = zeroDimension;

    bool hasValue = false;
    StyleEdge parsedEdge;
    if (StyleApplyUtils::ParseEdge(styles.GetItem(allKey), parsedEdge)) {
        edge = parsedEdge;
        hasValue = true;
    }

    static const std::array<StyleDimension StyleEdge::*, 4> edgeMembers = { &StyleEdge::top, &StyleEdge::right,
        &StyleEdge::bottom, &StyleEdge::left };
    const std::array<const char*, 4> edgeKeys = { topKey, rightKey, bottomKey, leftKey };

    for (size_t i = 0; i < edgeKeys.size(); ++i) {
        StyleDimension parsedDimension;
        if (!StyleApplyUtils::ParseDimension(styles.GetItem(edgeKeys[i]), parsedDimension)) {
            continue;
        }
        edge.*(edgeMembers[i]) = parsedDimension;
        hasValue = true;
    }

    return hasValue;
}

bool ExtendedStyleResolver::DimensionToFloat(const StyleDimension& dimension, float& value)
{
    return ConvertDimensionToFloat(dimension, value);
}

void ExtendedStyleResolver::ApplyDimension(const JsonValue& value, ArkUINodeApiAdapter& applier, bool isWidth,
    std::vector<DescriptorValidationIssue>& issues, std::optional<ConstraintDispatchContext> dispatchContext)
{
    const char* propName = isWidth ? "width" : "height";
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, invalid input=%{public}s",
                propName, FormatStyleInput(value).c_str());
            PushStyleValidationIssue(issues, propName,
                "Property styles." + std::string(propName) + " has invalid value and has been reset to default");
            Reset({ .rawName = propName, .name = isWidth ? StylePropertyName::WIDTH : StylePropertyName::HEIGHT },
                applier, dispatchContext.has_value() ? dispatchContext->apiVersion : 0);
        }
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDimension - style=%{public}s skipped, root node is null, input=%{public}s",
            propName, FormatStyleInput(value).c_str());
        return;
    }

    if (ApplyLayoutPolicyDimension(nodeHandle, dimension, applier, isWidth, dispatchContext)) {
        return;
    }

    switch (dimension.unit) {
        case StyleDimensionUnit::WRAP_CONTENT:
        case StyleDimensionUnit::FIX_AT_IDEAL_SIZE:
            if (isWidth) {
                applier.ResetNodeWidth(nodeHandle);
                applier.ResetNodeWidthPercent(nodeHandle);
            } else {
                applier.ResetNodeHeight(nodeHandle);
                applier.ResetNodeHeightPercent(nodeHandle);
            }
            return;
        case StyleDimensionUnit::PERCENT:
        case StyleDimensionUnit::MATCH_PARENT: {
            float resolvedValue = 0.0F;
            if (!DimensionToFloat(dimension, resolvedValue)) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, percent conversion "
                    "failed, input=%{public}s",
                    propName, FormatStyleInput(value).c_str());
                PushStyleValidationIssue(issues, propName,
                    "Property styles." + std::string(propName) +
                        " percent conversion failed and has been reset to default");
                Reset({ .rawName = propName, .name = isWidth ? StylePropertyName::WIDTH : StylePropertyName::HEIGHT },
                    applier);
                return;
            }
            float percentRatio = ConvertDimensionToPercentRatio(dimension, resolvedValue);
            if (isWidth) {
                applier.ResetNodeWidth(nodeHandle);
                applier.SetWidthPercent(percentRatio);
                return;
            }
            applier.ResetNodeHeight(nodeHandle);
            applier.SetHeightPercent(percentRatio);
            return;
        }
        case StyleDimensionUnit::VP:
        case StyleDimensionUnit::FP: {
            float resolvedValue = 0.0F;
            if (!DimensionToFloat(dimension, resolvedValue)) {
                LOG_A2UI(LOG_WARN,
                    "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, absolute conversion "
                    "failed, input=%{public}s",
                    propName, FormatStyleInput(value).c_str());
                PushStyleValidationIssue(issues, propName,
                    "Property styles." + std::string(propName) +
                        " absolute conversion failed and has been reset to default");
                Reset({ .rawName = propName, .name = isWidth ? StylePropertyName::WIDTH : StylePropertyName::HEIGHT },
                    applier);
                return;
            }
            if (isWidth) {
                applier.ResetNodeWidthPercent(nodeHandle);
                applier.SetWidth(resolvedValue);
                return;
            }
            applier.ResetNodeHeightPercent(nodeHandle);
            applier.SetHeight(resolvedValue);
            return;
        }
        default:
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, unsupported "
                "unit=%{public}s",
                propName, DimensionUnitToString(dimension.unit));
            PushStyleValidationIssue(issues, propName,
                "Property styles." + std::string(propName) + " has unsupported unit and has been reset to default");
            Reset({ .rawName = propName, .name = isWidth ? StylePropertyName::WIDTH : StylePropertyName::HEIGHT },
                applier);
            return;
    }
}

void ExtendedStyleResolver::ApplyRadius(const JsonValue& value, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    bool hasInvalidNestedField =
        HasInvalidKnownDimensionField(value, { "all", "topLeft", "topRight", "bottomLeft", "bottomRight" });
    StyleRadius radius;
    if (!StyleApplyUtils::ParseRadius(value, radius)) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyRadius - reset to default, invalid input=%{public}s",
                FormatStyleInput(value).c_str());
            PushStyleValidationIssue(
                issues, "borderRadius", "Property styles.borderRadius has invalid value and has been reset to default");
            Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        }
        return;
    }

    if (dispatchContext.has_value() && dispatchContext->componentType == "Button") {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyRadius - Button borderRadius dispatched to ETS, componentId=%{public}s, "
            "payload=%{public}s",
            dispatchContext->componentId.c_str(), value.ToJsonLiteral().c_str());
        CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
            .componentId = dispatchContext->componentId,
            .nodeUniqueId = dispatchContext->nodeUniqueId,
            .componentType = dispatchContext->componentType,
            .attributeName = "borderRadius",
            .payloadJson = value.ToJsonLiteral() });
        if (hasInvalidNestedField) {
            PushNestedDimensionValidationIssue(issues, "borderRadius", value);
        }
        return;
    }

    bool hasPercent = false;
    bool hasAbsolute = false;
    const std::array<StyleDimension, 4> dimensions = { radius.topLeft, radius.topRight, radius.bottomRight,
        radius.bottomLeft };
    for (const auto& dimension : dimensions) {
        if (dimension.value == 0.0F) {
            continue;
        }
        if (dimension.unit == StyleDimensionUnit::PERCENT) {
            hasPercent = true;
        } else {
            hasAbsolute = true;
        }
    }
    if (hasPercent && hasAbsolute) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyRadius - style=borderRadius reset to default, mixed units are not supported, "
            "input=%{public}s",
            FormatRadius(radius).c_str());
        PushStyleValidationIssue(issues, "borderRadius",
            "Property styles.borderRadius has mixed units which are not supported and has been reset to default");
        Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        return;
    }

    if (hasPercent) {
        float topLeft = 0.0F;
        float topRight = 0.0F;
        float bottomRight = 0.0F;
        float bottomLeft = 0.0F;
        if (!DimensionToFloat(radius.topLeft, topLeft) || !DimensionToFloat(radius.topRight, topRight) ||
            !DimensionToFloat(radius.bottomRight, bottomRight) || !DimensionToFloat(radius.bottomLeft, bottomLeft)) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyRadius - style=borderRadius reset to default, percent conversion failed, "
                "input=%{public}s",
                FormatRadius(radius).c_str());
            PushStyleValidationIssue(issues, "borderRadius",
                "Property styles.borderRadius percent conversion failed and has been reset to default");
            Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
            return;
        }
        applier.ResetNodeBorderRadius(applier.GetRootNode());
        applier.SetBorderRadiusPercent(topLeft, topRight, bottomLeft, bottomRight);
        if (hasInvalidNestedField) {
            PushNestedDimensionValidationIssue(issues, "borderRadius", value);
        }
        return;
    }

    float sameRadius = 0.0F;
    if (HasSameRadius(radius, sameRadius)) {
        applier.ResetNodeBorderRadiusPercent(applier.GetRootNode());
        applier.SetBorderRadius(sameRadius);
        if (hasInvalidNestedField) {
            PushNestedDimensionValidationIssue(issues, "borderRadius", value);
        }
        return;
    }

    float topLeft = 0.0F;
    float topRight = 0.0F;
    float bottomRight = 0.0F;
    float bottomLeft = 0.0F;
    if (!DimensionToFloat(radius.topLeft, topLeft) || !DimensionToFloat(radius.topRight, topRight) ||
        !DimensionToFloat(radius.bottomRight, bottomRight) || !DimensionToFloat(radius.bottomLeft, bottomLeft)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyRadius - reset to default, dimension conversion failed, input=%{public}s, "
            "parsed=%{public}s",
            FormatStyleInput(value).c_str(), FormatRadius(radius).c_str());
        PushStyleValidationIssue(issues, "borderRadius",
            "Property styles.borderRadius dimension conversion failed and has been reset to default");
        Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        return;
    }

    applier.ResetNodeBorderRadiusPercent(applier.GetRootNode());
    applier.SetNodeBorderRadius(applier.GetRootNode(), topLeft, topRight, bottomLeft, bottomRight);
    if (hasInvalidNestedField) {
        PushNestedDimensionValidationIssue(issues, "borderRadius", value);
    }
}

void ExtendedStyleResolver::ApplyShadow(const JsonValue& value, ArkUINodeApiAdapter& applier,
    std::vector<DescriptorValidationIssue>& issues, std::shared_ptr<ExtendedCommonTheme> commonTheme)
{
    StyleShadow shadow;
    if (!StyleApplyUtils::ParseShadow(value, shadow) || !shadow.valid) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyShadow - reset to default, invalid input=%{public}s",
                FormatStyleInput(value).c_str());
            PushStyleValidationIssue(
                issues, "shadow", "Property styles.shadow has invalid value and has been reset to default");
            Reset({ .rawName = "shadow", .name = StylePropertyName::SHADOW }, applier);
        }
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyShadow - root node is null, input=%{public}s, kind=%{public}s",
            FormatStyleInput(value).c_str(), ShadowKindToString(shadow.kind));
        return;
    }

    if (shadow.kind == StyleShadowKind::STYLE) {
        applier.ResetNodeCustomShadow(nodeHandle);
        applier.SetNodeShadow(nodeHandle, shadow.style);
        return;
    }

    if (!shadow.hasColor && commonTheme != nullptr) {
        shadow.color = commonTheme->GetShadowColor();
    }

    applier.ResetNodeShadow(nodeHandle);
    applier.SetNodeCustomShadow(nodeHandle, shadow.radius, shadow.useColorStrategy, shadow.offsetX, shadow.offsetY,
        shadow.type, shadow.color, shadow.fill);
}

void ExtendedStyleResolver::ApplyBackgroundImageSize(const JsonValue& value, const char* propertyName,
    ArkUINodeApiAdapter& applier, std::optional<ConstraintDispatchContext> dispatchContext,
    std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    // String tokens: cover, contain, auto, fill → IMAGE_SIZE via C++ path
    if (value.IsString()) {
        StyleBackgroundImageSize imageSize;
        if (StyleApplyUtils::ParseBackgroundImageSize(value, imageSize) &&
            imageSize.kind == StyleBackgroundImageSizeKind::IMAGE_SIZE) {
            applier.SetNodeBackgroundImageSizeWithStyle(applier.GetRootNode(), imageSize.imageSize);
            return;
        }
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyBackgroundImageSize - reset to default, invalid string input=%{public}s",
                FormatStyleInput(value).c_str());
            if (propertyName != nullptr) {
                issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles." + std::string(propertyName) + " has invalid value and has been reset to default",
                    "styles." + std::string(propertyName) });
                Reset({ .rawName = propertyName, .name = StylePropertyName::BACKGROUND_IMAGE_SIZE }, applier);
            }
        }
        return;
    }

    // Object {width, height}: use percent-aware parser to detect percent fields.
    // NOTE: Do NOT use StyleApplyUtils::ParseBackgroundImageSize for objects — its internal
    // TryParseImageSizeDimension accepts PERCENT but discards the unit info, so percent values
    // would silently be treated as absolute VP values.
    float width = 0.0F;
    float height = 0.0F;
    std::string percentJson;
    if (!ParseBackgroundImageSizeWithPercent(value, width, height, percentJson)) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyBackgroundImageSize - reset to default, invalid input=%{public}s",
                FormatStyleInput(value).c_str());
            if (propertyName != nullptr) {
                issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles." + std::string(propertyName) + " has invalid value and has been reset to default",
                    "styles." + std::string(propertyName) });
                Reset({ .rawName = propertyName, .name = StylePropertyName::BACKGROUND_IMAGE_SIZE }, applier);
            }
        }
        return;
    }

    if (percentJson.empty()) {
        // Pure VP/FP — C++ fast path
        applier.SetNodeBackgroundImageSize(applier.GetRootNode(), width, height);
        return;
    }

    // Has percent — dispatch to ETS via cross-language bridge
    if (dispatchContext.has_value()) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyBackgroundImageSize - percent dispatched to ETS, "
            "componentId=%{public}s, payload=%{public}s",
            dispatchContext->componentId.c_str(), percentJson.c_str());
        CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
            .componentId = dispatchContext->componentId,
            .nodeUniqueId = dispatchContext->nodeUniqueId,
            .componentType = dispatchContext->componentType,
            .attributeName = "backgroundImageSize",
            .payloadJson = percentJson });
    } else {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyBackgroundImageSize - has percent but no dispatchContext, "
            "fallback to C++ path, input=%{public}s",
            FormatStyleInput(value).c_str());
        applier.SetNodeBackgroundImageSize(applier.GetRootNode(), width, height);
    }
}

void ExtendedStyleResolver::ApplyLinearGradient(
    const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    StyleLinearGradient gradient;
    if (!StyleApplyUtils::ParseLinearGradient(value, gradient)) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyLinearGradient - reset to default, invalid input=%{public}s",
                FormatStyleInput(value).c_str());
            PushStyleValidationIssue(issues, "linearGradient",
                "Property styles.linearGradient has invalid value and has been reset to default");
            Reset({ .rawName = "linearGradient", .name = StylePropertyName::LINEAR_GRADIENT }, applier);
        }
        return;
    }
    applier.SetNodeLinearGradient(applier.GetRootNode(), gradient.angle, gradient.direction, gradient.repeating,
        gradient.colors.data(), static_cast<int32_t>(gradient.colors.size()), gradient.stops.data(),
        static_cast<int32_t>(gradient.stops.size()));
}

} // namespace NativeModule
