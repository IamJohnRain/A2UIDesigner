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

#include "components/TypeValidation.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "styles/StyleApplyUtils.h"
#include "utils/LogA2UI.h"

#include "A2UIArkUITypeConverter.h"
#include "ArkUIConstraintSizeAdapter.h"
#include "ArkUIOHApiAdapter.h"
#include "ExtendedCommonTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

std::string FormatStyleInput(const JsonValue& value);

constexpr float DEFAULT_ASPECT_RATIO = 1.0F;

void PushStyleValidationIssue(
    std::vector<DescriptorValidationIssue>& issues, const char* propertyName, const std::string& message)
{
    if (propertyName != nullptr) {
        issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE, message, "styles." + std::string(propertyName) });
    }
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

float NormalizeNonNegativeDimension(float value)
{
    if (!std::isfinite(value) || value < 0.0F) {
        return 0.0F;
    }
    return value;
}

float GetStyleDensityScale()
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
    if (dimension.unit == StyleDimensionUnit::FP) {
        value = NormalizeNonNegativeDimension(dimension.value * GetStyleDensityScale());
        return true;
    }
    if (dimension.unit == StyleDimensionUnit::VP || dimension.unit == StyleDimensionUnit::PERCENT ||
        dimension.unit == StyleDimensionUnit::MATCH_PARENT) {
        value = NormalizeNonNegativeDimension(dimension.value);
        return true;
    }
    value = 0.0F;
    return true;
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

bool IsRootDispatchContext(const std::optional<ConstraintDispatchContext>& dispatchContext)
{
    if (!dispatchContext.has_value()) {
        return false;
    }
    return dispatchContext->componentId == "root";
}

void ApplyRootMatchParent(ArkUINodeApiAdapter& applier, bool isWidth)
{
    if (isWidth) {
        applier.SetWidthPercent(1.0F);
        return;
    }
    applier.SetHeightPercent(1.0F);
}

bool ShouldDispatchLayoutPolicy(uint32_t apiVersion)
{
    return apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY;
}

void DispatchLayoutPolicy(A2UILayoutPolicy policy, bool isWidth, const ConstraintDispatchContext& dispatchContext)
{
    const char* axis = isWidth ? "width" : "height";
    std::string payloadJson = R"({"axis":")" + std::string(axis) + R"(","policy":)" +
                              std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(policy)) + "}";
    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext.renderId,
        .componentId = dispatchContext.componentId,
        .nodeUniqueId = dispatchContext.nodeUniqueId,
        .componentType = dispatchContext.componentType,
        .attributeName = "layoutPolicy",
        .payloadJson = payloadJson });
}

void ApplyNativeLayoutPolicy(ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier, A2UILayoutPolicy policy,
    bool isWidth, uint32_t apiVersion)
{
    int32_t convertedPolicy = A2UIArkUITypeConverter::ToArkUILayoutPolicy(policy);
    if (isWidth) {
        applier.SetNodeWidthLayoutPolicy(nodeHandle, convertedPolicy, static_cast<int32_t>(apiVersion));
        return;
    }
    applier.SetNodeHeightLayoutPolicy(nodeHandle, convertedPolicy, static_cast<int32_t>(apiVersion));
}

// Routes layout-semantic size units (match_parent/wrap_content/fix_at_ideal_size) to the ArkUI layout-policy API.
// Returns true when the policy path fully handled the dimension (caller returns); returns false when the caller
// should fall back to the legacy direct-dimension switch (non-mappable units only).
bool ApplyLayoutPolicyDimension(ArkUI_NodeHandle nodeHandle, const StyleDimension& dimension,
    ArkUINodeApiAdapter& applier, bool isWidth, std::optional<ConstraintDispatchContext> dispatchContext)
{
    uint32_t apiVersion = dispatchContext.has_value() ? dispatchContext->apiVersion : 0;
    A2UILayoutPolicy policy = A2UILayoutPolicy::WRAP_CONTENT;
    if (!ConvertDimensionToLayoutPolicy(dimension, policy)) {
        return false;
    }
    if (dimension.unit == StyleDimensionUnit::MATCH_PARENT && IsRootDispatchContext(dispatchContext)) {
        ApplyRootMatchParent(applier, isWidth);
        return true;
    }
    if (ShouldDispatchLayoutPolicy(apiVersion)) {
        LOG_A2UI(LOG_INFO,
            "ApplyLayoutPolicyDimension - old apiVersion=%{public}d, dispatch to ArkTS, policy=%{public}s", apiVersion,
            LayoutPolicyToString(policy));
        DispatchLayoutPolicy(policy, isWidth, dispatchContext.value());
        return true;
    }
    ApplyNativeLayoutPolicy(nodeHandle, applier, policy, isWidth, apiVersion);
    return true;
}

struct ParsedConstraintField {
    bool present = false;
    StyleDimensionUnit unit = StyleDimensionUnit::INVALID;
    float value = 0.0F;
};

bool ParseConstraintField(const JsonValue& value, ParsedConstraintField& parsed)
{
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        return false;
    }
    if (dimension.unit != StyleDimensionUnit::VP && dimension.unit != StyleDimensionUnit::FP &&
        dimension.unit != StyleDimensionUnit::PERCENT) {
        return false;
    }
    float converted = 0.0F;
    if (!ConvertDimensionToFloat(dimension, converted) || !std::isfinite(converted) || converted < 0.0F) {
        return false;
    }
    parsed = { true, dimension.unit, converted };
    return true;
}

std::string BuildConstraintPercentJson(
    const std::array<const char*, 4>& fields, const std::array<ParsedConstraintField, 4>& parsed)
{
    std::ostringstream stream;
    stream << "{";
    bool first = true;
    for (size_t index = 0; index < parsed.size(); ++index) {
        if (!parsed[index].present) {
            continue;
        }
        if (!first) {
            stream << ",";
        }
        first = false;
        const char* unit = parsed[index].unit == StyleDimensionUnit::PERCENT ? "%" : "vp";
        stream << "\"" << fields[index] << "\":\"" << parsed[index].value << unit << "\"";
    }
    stream << "}";
    return stream.str();
}

struct ConstraintFieldParseResult {
    std::array<ParsedConstraintField, 4> fields;
    bool hasPresentField = false;
    bool hasValidField = false;
    bool hasPercent = false;
};

struct ConstraintSizeParseResult {
    A2UIConstraintSizeSpec nativeSpec;
    std::string percentJson;
};

ConstraintFieldParseResult ParseConstraintFields(
    const JsonValue& constraintSize, const std::array<const char*, 4>& fieldNames, const std::array<float*, 4>& outputs)
{
    ConstraintFieldParseResult result;
    for (size_t index = 0; index < fieldNames.size(); ++index) {
        if (!constraintSize.Has(fieldNames[index])) {
            continue;
        }
        result.hasPresentField = true;
        if (!ParseConstraintField(constraintSize.GetItem(fieldNames[index]), result.fields[index])) {
            continue;
        }
        result.hasValidField = true;
        if (result.fields[index].unit == StyleDimensionUnit::PERCENT) {
            result.hasPercent = true;
        } else {
            *outputs[index] = result.fields[index].value;
        }
    }
    return result;
}

bool ParseConstraintSizeStyle(const JsonValue& constraintSize, ConstraintSizeParseResult& output)
{
    if (!constraintSize.IsObject()) {
        return false;
    }

    output = {};

    const std::array<const char*, 4> fields = { "minWidth", "maxWidth", "minHeight", "maxHeight" };
    const std::array<float*, 4> outputs = { &output.nativeSpec.minWidth.value, &output.nativeSpec.maxWidth.value,
        &output.nativeSpec.minHeight.value, &output.nativeSpec.maxHeight.value };
    ConstraintFieldParseResult result = ParseConstraintFields(constraintSize, fields, outputs);
    if (result.hasPresentField && !result.hasValidField) {
        return false;
    }
    if (result.hasPercent) {
        output.percentJson = BuildConstraintPercentJson(fields, result.fields);
    }
    const std::array<A2UIConstraintDimension*, 4> nativeFields = { &output.nativeSpec.minWidth,
        &output.nativeSpec.maxWidth, &output.nativeSpec.minHeight, &output.nativeSpec.maxHeight };
    for (size_t index = 0; index < result.fields.size(); ++index) {
        if (!result.fields[index].present) {
            continue;
        }
        nativeFields[index]->value = result.fields[index].value;
        nativeFields[index]->isPercent = result.fields[index].unit == StyleDimensionUnit::PERCENT;
    }
    return true;
}

void ReportInvalidConstraintSize(const JsonValue& value, std::vector<DescriptorValidationIssue>& issues)
{
    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyCommonNodeStyles - style=constraintSize reset to default, invalid "
        "input=%{public}s",
        FormatStyleInput(value).c_str());
    PushStyleValidationIssue(
        issues, "constraintSize", "Property styles.constraintSize has invalid value and has been reset to default");
}

struct ParsedBackgroundImageSizeField {
    bool present = false;
    StyleDimensionUnit unit = StyleDimensionUnit::INVALID;
    float value = 0.0F;
};

struct BackgroundImageSizeParseResult {
    float width = 0.0F;
    float height = 0.0F;
    std::string percentJson;
};

bool IsSupportedBackgroundImageSizeUnit(StyleDimensionUnit unit)
{
    return unit == StyleDimensionUnit::VP || unit == StyleDimensionUnit::FP || unit == StyleDimensionUnit::PERCENT;
}

bool ParseBackgroundImageSizeField(const JsonValue& value, ParsedBackgroundImageSizeField& parsed)
{
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        return false;
    }
    if (!IsSupportedBackgroundImageSizeUnit(dimension.unit)) {
        return false;
    }
    float converted = 0.0F;
    if (!ConvertDimensionToFloat(dimension, converted)) {
        return false;
    }
    if (!std::isfinite(converted) || converted < 0.0F) {
        return false;
    }
    parsed = { true, dimension.unit, converted };
    return true;
}

std::string BuildBackgroundImageSizePercentJson(
    const std::array<const char*, 2>& fieldNames, const std::array<ParsedBackgroundImageSizeField, 2>& parsed)
{
    std::ostringstream stream;
    stream << "{";
    bool first = true;
    for (size_t index = 0; index < parsed.size(); ++index) {
        if (!parsed[index].present) {
            continue;
        }
        if (!first) {
            stream << ",";
        }
        first = false;
        const char* unit = parsed[index].unit == StyleDimensionUnit::PERCENT ? "%" : "vp";
        stream << "\"" << fieldNames[index] << "\":\"" << parsed[index].value << unit << "\"";
    }
    stream << "}";
    return stream.str();
}

bool IsEmptyBackgroundImageSizeField(const JsonValue& value)
{
    if (!value.IsValid()) {
        return true;
    }
    if (!value.IsString()) {
        return false;
    }
    const std::string raw = value.GetStringValue("");
    for (char ch : raw) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return false;
        }
    }
    return true;
}

bool ParseBackgroundImageSizeWithPercent(const JsonValue& backgroundImageSize, BackgroundImageSizeParseResult& output)
{
    if (!backgroundImageSize.IsObject()) {
        return false;
    }

    output = {};
    const std::array<const char*, 2> fieldNames = { "width", "height" };
    const std::array<float*, 2> outputValues = { &output.width, &output.height };
    std::array<ParsedBackgroundImageSizeField, 2> parsed;
    bool hasPercent = false;
    bool hasPresentField = false;
    for (size_t index = 0; index < fieldNames.size(); ++index) {
        if (!backgroundImageSize.Has(fieldNames[index])) {
            continue;
        }
        hasPresentField = true;
        const JsonValue& fieldValue = backgroundImageSize.GetItem(fieldNames[index]);
        if (IsEmptyBackgroundImageSizeField(fieldValue)) {
            // An empty/whitespace dimension maps to LENGTH 0, matching NODE_BACKGROUND_IMAGE_SIZE
            // where ParseJsDimensionVp("") resolves to value=0/unit=LENGTH. The rendering layer then
            // derives this dimension from the other one by the source-image aspect ratio instead of
            // falling back to COVER/CONTAIN/AUTO. Leave *outputValues[index] at its 0 default.
            parsed[index] = { true, StyleDimensionUnit::VP, 0.0F };
            continue;
        }
        if (!ParseBackgroundImageSizeField(fieldValue, parsed[index])) {
            return false;
        }
        if (parsed[index].unit == StyleDimensionUnit::PERCENT) {
            hasPercent = true;
        } else {
            *outputValues[index] = parsed[index].value;
        }
    }

    if (!hasPresentField) {
        return false;
    }
    if (hasPercent) {
        output.percentJson = BuildBackgroundImageSizePercentJson(fieldNames, parsed);
    }
    return true;
}

bool ConvertRadiusValues(const StyleRadius& radius, std::array<float, 4>& values)
{
    const std::array<StyleDimension, 4> dimensions = { radius.topLeft, radius.topRight, radius.bottomRight,
        radius.bottomLeft };
    for (size_t index = 0; index < dimensions.size(); ++index) {
        if (!ConvertDimensionToFloat(dimensions[index], values[index])) {
            return false;
        }
    }
    return true;
}

bool HasSameRadius(const StyleRadius& radius, float& value)
{
    std::array<float, 4> values = {};
    if (!ConvertRadiusValues(radius, values)) {
        return false;
    }
    for (size_t index = 1; index < values.size(); ++index) {
        if (values[index] != values[0]) {
            return false;
        }
    }
    value = values[0];
    return true;
}

struct EdgeDimensionValues {
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
    float left = 0.0F;
    bool hasPercent = false;
    bool hasAbsolute = false;
};

bool ConvertEdgeValues(const StyleEdge& edge, EdgeDimensionValues& values)
{
    if (!ConvertDimensionToFloat(edge.top, values.top)) {
        return false;
    }
    if (!ConvertDimensionToFloat(edge.right, values.right)) {
        return false;
    }
    if (!ConvertDimensionToFloat(edge.bottom, values.bottom)) {
        return false;
    }
    if (!ConvertDimensionToFloat(edge.left, values.left)) {
        return false;
    }
    const std::array<StyleDimension, 4> dimensions = { edge.top, edge.right, edge.bottom, edge.left };
    for (const auto& dimension : dimensions) {
        if (dimension.value == 0.0F) {
            continue;
        }
        if (dimension.unit == StyleDimensionUnit::PERCENT) {
            values.hasPercent = true;
        } else {
            values.hasAbsolute = true;
        }
    }
    return true;
}

void ApplyPaddingValues(const EdgeDimensionValues& values, ArkUINodeApiAdapter& applier)
{
    if (values.hasPercent) {
        applier.SetPaddingPercent(
            values.top / 100.0F, values.right / 100.0F, values.bottom / 100.0F, values.left / 100.0F);
        return;
    }
    applier.SetPadding(values.top, values.right, values.bottom, values.left);
}

void ApplyMarginValues(const EdgeDimensionValues& values, ArkUINodeApiAdapter& applier)
{
    if (values.hasPercent) {
        applier.SetMarginPercent(
            values.top / 100.0F, values.right / 100.0F, values.bottom / 100.0F, values.left / 100.0F);
        return;
    }
    applier.SetMargin(values.top, values.right, values.bottom, values.left);
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
    if (unit == StyleDimensionUnit::VP) {
        return "vp";
    }
    if (unit == StyleDimensionUnit::FP) {
        return "fp";
    }
    if (unit == StyleDimensionUnit::PERCENT) {
        return "%";
    }
    if (unit == StyleDimensionUnit::MATCH_PARENT) {
        return "match_parent";
    }
    if (unit == StyleDimensionUnit::WRAP_CONTENT) {
        return "wrap_content";
    }
    if (unit == StyleDimensionUnit::FIX_AT_IDEAL_SIZE) {
        return "fix_at_ideal_size";
    }
    return "invalid";
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
    // Applying the image can change ArkUI's implicit size mode, so explicit size wins last.
    ApplyBackgroundImageSizeStyles(styles, applier, dispatchContext, issues);
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

    ApplyTextFontStyles(styles, nodeHandle, applier);
    ApplyTextLineStyles(styles, nodeHandle, applier);
    ApplyTextDecoration(styles.GetItem("decoration"), applier);
}

void ExtendedStyleResolver::ApplyTextFontStyles(
    const JsonValue& styles, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    JsonValue fontWeightValue = styles.GetItem("fontWeight");
    int32_t fontWeight = 0;
    if (ParseTextComponentFontWeight(fontWeightValue, fontWeight)) {
        applier.SetNodeFontWeight(nodeHandle, static_cast<A2UIFontWeight>(fontWeight));
    } else if (fontWeightValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=fontWeight ignored, invalid input=%{public}s",
            FormatStyleInput(fontWeightValue).c_str());
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
}

void ExtendedStyleResolver::ApplyTextLineStyles(
    const JsonValue& styles, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    JsonValue maxLinesValue = styles.GetItem("maxLines");
    int32_t number = 0;
    if (TryParseTextComponentMaxLines(maxLinesValue, number)) {
        applier.SetNodeTextMaxLines(nodeHandle, number);
    } else if (maxLinesValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyTextComponentStyles - style=maxLines ignored, invalid input=%{public}s",
            FormatStyleInput(maxLinesValue).c_str());
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
}

void ExtendedStyleResolver::ApplySizeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::vector<DescriptorValidationIssue>& issues, std::optional<ConstraintDispatchContext> dispatchContext)
{
    ApplyDimension(styles.GetItem("width"), applier, true, issues, dispatchContext);
    ApplyDimension(styles.GetItem("height"), applier, false, issues, dispatchContext);
    ApplyAspectRatio(styles.GetItem("aspectRatio"), applier, issues);
}

void ExtendedStyleResolver::ApplyAspectRatio(
    const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }
    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyAspectRatio - root node is null, input=%{public}s", // GCOVR_EXCL_BR_LINE
            FormatStyleInput(value).c_str());                                                // GCOVR_EXCL_BR_LINE
        return;
    }
    if (value.IsNumber()) {
        double rawRatio = value.GetNumberValue(0.0);
        if (std::isfinite(rawRatio) && rawRatio > 0.0) {
            applier.SetNodeAspectRatio(nodeHandle, static_cast<float>(rawRatio));
            LOG_A2UI(LOG_INFO, // GCOVR_EXCL_BR_LINE
                "ExtendedStyleResolver::ApplyAspectRatio - style=aspectRatio, input=%{public}s, "
                "applied=%{public}f, attribute=NODE_ASPECT_RATIO",
                FormatStyleInput(value).c_str(), static_cast<float>(rawRatio));
        } else {
            issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property aspectRatio got invalid number value, fallback to default value", "styles.aspectRatio" });
            applier.SetNodeAspectRatio(nodeHandle, DEFAULT_ASPECT_RATIO);
            LOG_A2UI(LOG_WARN, // GCOVR_EXCL_BR_LINE
                "ExtendedStyleResolver::ApplyAspectRatio - style=aspectRatio reset to default, "
                "invalid input=%{public}s",
                FormatStyleInput(value).c_str()); // GCOVR_EXCL_BR_LINE
        }
    } else {
        issues.push_back({ SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            std::string("Property aspectRatio expects number value, got type '") + value.GetTypeName() +
                "', fallback to default value",
            "styles.aspectRatio" });
        applier.SetNodeAspectRatio(nodeHandle, DEFAULT_ASPECT_RATIO);
        LOG_A2UI(LOG_WARN, // GCOVR_EXCL_BR_LINE
            "ExtendedStyleResolver::ApplyAspectRatio - style=aspectRatio reset to default, "
            "invalid input=%{public}s",
            FormatStyleInput(value).c_str()); // GCOVR_EXCL_BR_LINE
    }
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
    ApplyPaddingStyles(styles, applier, issues);
    ApplyMarginStyles(styles, applier, issues);
}

void ExtendedStyleResolver::ApplyPaddingStyles(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    StyleEdge edge;
    bool hasPaddingInput = HasAnyField(styles, { "padding", "top", "right", "bottom", "left" });
    JsonValue paddingValue = styles.GetItem("padding");
    bool paddingHasInvalidNestedField = HasInvalidKnownDimensionField(
        paddingValue, { "all", "vertical", "horizontal", "top", "right", "bottom", "left" });
    if (!ParseEdgeStyle(styles, "padding", "top", "right", "bottom", "left", edge)) {
        if (!hasPaddingInput) {
            return;
        }
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, invalid input=%{public}s",
            FormatNamedStyleInputs(styles, { "padding", "top", "right", "bottom", "left" }).c_str());
        PushStyleValidationIssue(
            issues, "padding", "Property styles.padding has invalid value and has been reset to default");
        Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
        return;
    }

    EdgeDimensionValues values;
    if (!ConvertEdgeValues(edge, values)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, dimension conversion failed, "
            "input=%{public}s",
            FormatEdge(edge).c_str());
        PushStyleValidationIssue(
            issues, "padding", "Property styles.padding has invalid value and has been reset to default");
        Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
        return;
    }
    if (values.hasPercent && values.hasAbsolute) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=padding reset to default, mixed units are not supported, "
            "input=%{public}s",
            FormatEdge(edge).c_str());
        PushStyleValidationIssue(issues, "padding",
            "Property styles.padding has mixed units which are not supported and has been reset to default");
        Reset({ .rawName = "padding", .name = StylePropertyName::PADDING }, applier);
        return;
    }
    ApplyPaddingValues(values, applier);
    if (paddingHasInvalidNestedField) {
        PushNestedDimensionValidationIssue(issues, "padding", paddingValue);
    }
}

void ExtendedStyleResolver::ApplyMarginStyles(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    StyleEdge edge;
    bool hasMarginInput = HasAnyField(styles, { "margin", "marginTop", "marginRight", "marginBottom", "marginLeft" });
    JsonValue marginValue = styles.GetItem("margin");
    bool marginHasInvalidNestedField = HasInvalidKnownDimensionField(
        marginValue, { "all", "vertical", "horizontal", "top", "right", "bottom", "left" });
    if (!ParseEdgeStyle(styles, "margin", "marginTop", "marginRight", "marginBottom", "marginLeft", edge)) {
        if (!hasMarginInput) {
            return;
        }
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, invalid input=%{public}s",
            FormatNamedStyleInputs(styles, { "margin", "marginTop", "marginRight", "marginBottom", "marginLeft" })
                .c_str());
        PushStyleValidationIssue(
            issues, "margin", "Property styles.margin has invalid value and has been reset to default");
        Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
        return;
    }

    EdgeDimensionValues values;
    if (!ConvertEdgeValues(edge, values)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, dimension conversion failed, "
            "input=%{public}s",
            FormatEdge(edge).c_str());
        PushStyleValidationIssue(
            issues, "margin", "Property styles.margin has invalid value and has been reset to default");
        Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
        return;
    }
    if (values.hasPercent && values.hasAbsolute) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyEdgeStyles - style=margin reset to default, mixed units are not supported, "
            "input=%{public}s",
            FormatEdge(edge).c_str());
        PushStyleValidationIssue(issues, "margin",
            "Property styles.margin has mixed units which are not supported and has been reset to default");
        Reset({ .rawName = "margin", .name = StylePropertyName::MARGIN }, applier);
        return;
    }
    ApplyMarginValues(values, applier);
    if (marginHasInvalidNestedField) {
        PushNestedDimensionValidationIssue(issues, "margin", marginValue);
    }
}

namespace {
void ResetInvalidBorderWidth(const JsonValue& value, const char* reason, const std::string& message,
    ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyDecorationStyles - style=borderWidth reset to default, %{public}s, "
        "input=%{public}s",
        reason, FormatStyleInput(value).c_str());
    PushStyleValidationIssue(issues, "borderWidth", message);
    ExtendedStyleResolver::Reset({ .rawName = "borderWidth", .name = StylePropertyName::BORDER_WIDTH }, applier);
}

void ApplyParsedBorderWidth(const JsonValue& value, const StyleDimension& dimension, ArkUINodeApiAdapter& applier,
    std::vector<DescriptorValidationIssue>& issues)
{
    bool isAbsolute = dimension.unit == StyleDimensionUnit::VP || dimension.unit == StyleDimensionUnit::FP;
    bool isPercent = dimension.unit == StyleDimensionUnit::PERCENT;
    if (!isAbsolute && !isPercent) {
        std::string reason = "invalid unit=" + std::string(DimensionUnitToString(dimension.unit));
        ResetInvalidBorderWidth(value, reason.c_str(),
            "Property styles.borderWidth has unsupported unit and has been reset to default", applier, issues);
        return;
    }
    float number = 0.0F;
    if (!ConvertDimensionToFloat(dimension, number)) {
        const char* reason = isPercent ? "percent conversion failed" : "conversion failed";
        std::string message =
            isPercent ? "Property styles.borderWidth percent conversion failed and has been reset to default"
                      : "Property styles.borderWidth conversion failed and has been reset to default";
        ResetInvalidBorderWidth(value, reason, message, applier, issues);
        return;
    }
    if (isPercent) {
        applier.ResetNodeBorderWidth(applier.GetRootNode());
        applier.SetBorderWidthPercent(number / 100.0F);
        return;
    }
    applier.ResetNodeBorderWidthPercent(applier.GetRootNode());
    applier.SetNodeBorderWidth(applier.GetRootNode(), number);
}
} // namespace

void ExtendedStyleResolver::ApplyDecorationStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    ApplyBorderWidthStyle(styles, applier, issues);
    ApplyOpacityStyle(styles, applier);
    ApplyVisibilityStyle(styles, applier, issues);
    ApplyRadius(styles.GetItem("borderRadius"), applier, dispatchContext, issues);
    ApplyShadow(styles.GetItem("shadow"), applier, issues,
        dispatchContext.has_value() ? dispatchContext->commonTheme : nullptr);
    ApplyLinearGradient(styles.GetItem("linearGradient"), applier, issues);
}

void ExtendedStyleResolver::ApplyBorderWidthStyle(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    JsonValue borderWidthValue = styles.GetItem("borderWidth");
    StyleDimension borderWidthDimension;
    if (StyleApplyUtils::ParseDimension(borderWidthValue, borderWidthDimension)) {
        ApplyParsedBorderWidth(borderWidthValue, borderWidthDimension, applier, issues);
        return;
    }
    if (borderWidthValue.IsValid()) {
        ResetInvalidBorderWidth(borderWidthValue, "invalid input",
            "Property styles.borderWidth has invalid value and has been reset to default", applier, issues);
    }
}

void ExtendedStyleResolver::ApplyOpacityStyle(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    float number = 0.0F;
    JsonValue opacityValue = styles.GetItem("opacity");
    if (StyleApplyUtils::ParseNumber(opacityValue, number)) {
        applier.SetNodeOpacity(applier.GetRootNode(), number);
    } else if (opacityValue.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDecorationStyles - style=opacity ignored, invalid input=%{public}s",
            FormatStyleInput(opacityValue).c_str());
    }
}

void ExtendedStyleResolver::ApplyVisibilityStyle(
    const JsonValue& styles, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
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
}

void ExtendedStyleResolver::ApplyBackgroundImageSizeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    ApplyBackgroundImageSize(
        styles.GetItem("backgroundImageSize"), "backgroundImageSize", applier, dispatchContext, issues);
    ApplyBackgroundImageSize(
        styles.GetItem("backgroundimageSize"), "backgroundimageSize", applier, dispatchContext, issues);
    ApplyBackgroundImageSize(styles.GetItem("backgroundImageSizeWithStyle"), "backgroundImageSizeWithStyle", applier,
        dispatchContext, issues);
    ApplyBackgroundImageSize(styles.GetItem("backgroundimageSizeWithStyle"), "backgroundimageSizeWithStyle", applier,
        dispatchContext, issues);
}

void ExtendedStyleResolver::ApplyCommonNodeStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    if (applier.GetRootNode() == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyCommonNodeStyles - root node is null, styleInputs=%{public}s",
            FormatNamedStyleInputs(styles,
                { "flexShrink", "backgroundImage", "backgroundimage", "clip", "layoutWeight", "constraintSize" })
                .c_str());
        return;
    }

    ApplyFlexShrinkStyle(styles.GetItem("flexShrink"), applier, dispatchContext, issues);
    ApplyBackgroundImage("backgroundImage", styles.GetItem("backgroundImage"), applier, issues);
    ApplyBackgroundImage("backgroundimage", styles.GetItem("backgroundimage"), applier, issues);
    ApplyClipStyle(styles.GetItem("clip"), applier, issues);
    ApplyLayoutWeightStyle(styles.GetItem("layoutWeight"), applier, issues);
    ApplyConstraintSizeStyle(styles.GetItem("constraintSize"), applier, dispatchContext, issues);
}

void ExtendedStyleResolver::ApplyFlexShrinkStyle(const JsonValue& value, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    float flexShrink = 0.0F;
    if (StyleApplyUtils::ParseFlexShrink(value, flexShrink)) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=flexShrink, input=%{public}s, "
            "applied=%{public}f, attribute=NODE_FLEX_SHRINK",
            FormatStyleInput(value).c_str(), flexShrink);
        applier.SetNodeFlexShrink(applier.GetRootNode(), flexShrink);
        return;
    }

    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyCommonNodeStyles - style=flexShrink reset to default, invalid input=%{public}s",
        FormatStyleInput(value).c_str());
    PushStyleValidationIssue(
        issues, "flexShrink", "Property styles.flexShrink has invalid value and has been reset to default");
    const std::string parentComponentType = dispatchContext.has_value() ? dispatchContext->parentComponentType : "";
    const int32_t apiVersion =
        dispatchContext.has_value() ? dispatchContext->apiVersion : MIN_API_VERSION_LAYOUT_POLICY;
    Reset(
        { .rawName = "flexShrink", .name = StylePropertyName::FLEX_SHRINK }, applier, apiVersion, parentComponentType);
}

void ExtendedStyleResolver::ApplyClipStyle(
    const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    bool clip = false;
    if (StyleApplyUtils::ParseClip(value, clip)) {
        LOG_A2UI(LOG_INFO,
            "ExtendedStyleResolver::ApplyCommonNodeStyles - style=clip, input=%{public}s, applied=%{public}s, "
            "attribute=NODE_CLIP",
            FormatStyleInput(value).c_str(), BoolToString(clip));
        applier.SetNodeClip(applier.GetRootNode(), clip);
        return;
    }

    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyCommonNodeStyles - style=clip reset to default, invalid input=%{public}s",
        FormatStyleInput(value).c_str());
    PushStyleValidationIssue(issues, "clip", "Property styles.clip has invalid value and has been reset to default");
    Reset({ .rawName = "clip", .name = StylePropertyName::CLIP }, applier);
}

void ExtendedStyleResolver::ApplyLayoutWeightStyle(
    const JsonValue& value, ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    float layoutWeight = 0.0F;
    if (StyleApplyUtils::ParseNumber(value, layoutWeight) && std::isfinite(layoutWeight) && layoutWeight >= 0.0F) {
        if (layoutWeight > 0.0F) {
            applier.SetNodeLayoutWeight(applier.GetRootNode(), static_cast<uint32_t>(layoutWeight));
        }
        return;
    }

    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyCommonNodeStyles - style=layoutWeight reset to default, invalid "
        "input=%{public}s",
        FormatStyleInput(value).c_str());
    PushStyleValidationIssue(
        issues, "layoutWeight", "Property styles.layoutWeight has invalid value and has been reset to default");
    Reset({ .rawName = "layoutWeight", .name = StylePropertyName::LAYOUT_WEIGHT }, applier);
}

void ExtendedStyleResolver::ApplyConstraintSizeStyle(const JsonValue& value, ArkUINodeApiAdapter& applier,
    std::optional<ConstraintDispatchContext> dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    ConstraintSizeParseResult result;
    if (ParseConstraintSizeStyle(value, result)) {
        if (result.percentJson.empty()) {
            ArkUIConstraintSizeAdapter::Clear(applier.GetRootNode());
            applier.SetNodeConstraintSize(applier.GetRootNode(), result.nativeSpec.minWidth.value,
                result.nativeSpec.maxWidth.value, result.nativeSpec.minHeight.value, result.nativeSpec.maxHeight.value);
            return;
        }
        int32_t apiVersion = dispatchContext.has_value() ? dispatchContext->apiVersion : 0;
        if (apiVersion == 0 || apiVersion >= MIN_API_VERSION_CUSTOM_MEASURE) {
            int32_t nativeResult =
                ArkUIConstraintSizeAdapter::SetPercentConstraintSize(applier.GetRootNode(), result.nativeSpec);
            if (nativeResult == A2UI_ERROR_CODE_NO_ERROR) {
                LOG_A2UI(LOG_INFO,
                    "ExtendedStyleResolver::ApplyCommonNodeStyles - constraintSize percent applied by native path, "
                    "componentId=%{public}s",
                    dispatchContext.has_value() ? dispatchContext->componentId.c_str() : "");
                return;
            }
            LOG_A2UI(LOG_WARN,
                "ExtendedStyleResolver::ApplyCommonNodeStyles - native constraintSize percent setup failed, "
                "componentId=%{public}s, result=%{public}d",
                dispatchContext.has_value() ? dispatchContext->componentId.c_str() : "", nativeResult);
        }
        ArkUIConstraintSizeAdapter::Clear(applier.GetRootNode());
        if (dispatchContext.has_value()) {
            LOG_A2UI(LOG_INFO,
                "ExtendedStyleResolver::ApplyCommonNodeStyles - constraintSize percent dispatched to ETS, "
                "componentId=%{public}s, payload=%{public}s",
                dispatchContext->componentId.c_str(), result.percentJson.c_str());
            CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
                .componentId = dispatchContext->componentId,
                .nodeUniqueId = dispatchContext->nodeUniqueId,
                .componentType = dispatchContext->componentType,
                .attributeName = "constraintSize",
                .payloadJson = result.percentJson });
        }
        return;
    }

    ReportInvalidConstraintSize(value, issues);
    Reset({ .rawName = "constraintSize", .name = StylePropertyName::CONSTRAINT_SIZE }, applier);
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

namespace {
bool ResetBasicTextProperty(StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    if (propertyName == StylePropertyName::FONT_COLOR) {
        applier.ResetNodeFontColor(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::FONT_SIZE) {
        applier.ResetNodeFontSize(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::FONT_WEIGHT) {
        applier.ResetNodeFontWeight(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::TEXT_ALIGN) {
        applier.ResetNodeTextAlign(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::MAX_LINES) {
        applier.ResetNodeTextMaxLines(nodeHandle);
        applier.ResetNodeTextInputNumberOfLines(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::TEXT_MIN_FONT_SIZE) {
        applier.ResetNodeTextMinFontSize(nodeHandle);
        return true;
    }
    return false;
}

bool ResetAdditionalTextProperty(
    StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    if (propertyName == StylePropertyName::TEXT_MAX_FONT_SIZE) {
        applier.ResetNodeTextMaxFontSize(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::TEXT_OVERFLOW) {
        applier.ResetNodeTextOverflow(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::WORD_BREAK) {
        applier.ResetNodeTextWordBreak(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::DECORATION) {
        applier.ResetNodeTextDecoration(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::PLACEHOLDER_COLOR) {
        applier.ResetNodeTextInputPlaceholderColor(nodeHandle);
        return true;
    }
    return false;
}

void ResetFlexShrink(ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier, const std::string& parentComponentType)
{
    if (parentComponentType == "Column" || parentComponentType == "Row") {
        applier.SetNodeFlexShrink(nodeHandle, 0.0F);
        return;
    }
    if (parentComponentType == "Flex") {
        applier.SetNodeFlexShrink(nodeHandle, 1.0F);
        return;
    }
    applier.ResetNodeFlexShrink(nodeHandle);
}

bool ResetVisualCommonProperty(
    StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    if (propertyName == StylePropertyName::VISIBILITY) {
        applier.ResetNodeVisibility(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::OPACITY) {
        applier.ResetNodeOpacity(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::SHADOW) {
        applier.ResetNodeShadow(nodeHandle);
        applier.ResetNodeCustomShadow(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BACKGROUND_IMAGE) {
        applier.ResetNodeBackgroundImage(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BACKGROUND_IMAGE_SIZE) {
        applier.ResetNodeBackgroundImageSize(nodeHandle);
        applier.SetNodeBackgroundImageSizeWithStyle(nodeHandle, static_cast<int32_t>(A2UIImageSize::AUTO));
        return true;
    }
    if (propertyName == StylePropertyName::LINEAR_GRADIENT) {
        applier.ResetNodeLinearGradient(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::CLIP) {
        applier.ResetNodeClip(nodeHandle);
        return true;
    }
    return false;
}

bool ResetLayoutCommonProperty(StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle,
    ArkUINodeApiAdapter& applier, const std::string& parentComponentType)
{
    if (propertyName == StylePropertyName::FLEX_SHRINK) {
        ResetFlexShrink(nodeHandle, applier, parentComponentType);
        return true;
    }
    if (propertyName == StylePropertyName::LAYOUT_WEIGHT) {
        applier.ResetNodeLayoutWeight(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::CONSTRAINT_SIZE) {
        ArkUIConstraintSizeAdapter::Clear(nodeHandle);
        applier.ResetNodeConstraintSize(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::ASPECT_RATIO) {
        applier.SetNodeAspectRatio(nodeHandle, DEFAULT_ASPECT_RATIO);
        return true;
    }
    return false;
}
} // namespace

bool ExtendedStyleResolver::ResetLayoutProperty(
    StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier, int32_t apiVersion)
{
    if (propertyName == StylePropertyName::WIDTH) {
        applier.ResetNodeWidth(nodeHandle);
        applier.ResetNodeWidthPercent(nodeHandle);
        applier.ResetNodeWidthLayoutPolicy(nodeHandle, apiVersion);
        return true;
    }
    if (propertyName == StylePropertyName::HEIGHT) {
        applier.ResetNodeHeight(nodeHandle);
        applier.ResetNodeHeightPercent(nodeHandle);
        applier.ResetNodeHeightLayoutPolicy(nodeHandle, apiVersion);
        return true;
    }
    if (propertyName == StylePropertyName::PADDING) {
        applier.ResetNodePadding(nodeHandle);
        applier.ResetNodePaddingPercent(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::MARGIN) {
        applier.ResetNodeMargin(nodeHandle);
        applier.ResetNodeMarginPercent(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BACKGROUND_COLOR) {
        applier.ResetNodeBackgroundColor(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BORDER_RADIUS) {
        applier.ResetNodeBorderRadius(nodeHandle);
        applier.ResetNodeBorderRadiusPercent(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BORDER_WIDTH) {
        applier.ResetNodeBorderWidth(nodeHandle);
        applier.ResetNodeBorderWidthPercent(nodeHandle);
        return true;
    }
    if (propertyName == StylePropertyName::BORDER_COLOR) {
        applier.ResetNodeBorderColor(nodeHandle);
        return true;
    }
    return false;
}

bool ExtendedStyleResolver::ResetTextProperty(
    StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle, ArkUINodeApiAdapter& applier)
{
    return ResetBasicTextProperty(propertyName, nodeHandle, applier) ||
           ResetAdditionalTextProperty(propertyName, nodeHandle, applier);
}

bool ExtendedStyleResolver::ResetCommonProperty(StylePropertyName propertyName, ArkUI_NodeHandle nodeHandle,
    ArkUINodeApiAdapter& applier, const std::string& parentComponentType)
{
    return ResetVisualCommonProperty(propertyName, nodeHandle, applier) ||
           ResetLayoutCommonProperty(propertyName, nodeHandle, applier, parentComponentType);
}

void ExtendedStyleResolver::Reset(const StyleResetProperty& property, ArkUINodeApiAdapter& applier, int32_t apiVersion,
    const std::string& parentComponentType)
{
    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr || property.rawName.empty()) {
        return;
    }

    if (ResetLayoutProperty(property.name, nodeHandle, applier, apiVersion)) {
        return;
    }
    if (ResetTextProperty(property.name, nodeHandle, applier)) {
        return;
    }
    ResetCommonProperty(property.name, nodeHandle, applier, parentComponentType);
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

namespace {
struct DimensionApplyContext {
    const char* propertyName;
    bool isWidth;
    ArkUI_NodeHandle nodeHandle;
    ArkUINodeApiAdapter& applier;
    std::vector<DescriptorValidationIssue>& issues;
};

StyleResetProperty GetDimensionResetProperty(const DimensionApplyContext& context)
{
    return { .rawName = context.propertyName,
        .name = context.isWidth ? StylePropertyName::WIDTH : StylePropertyName::HEIGHT };
}

void ResetDimension(const DimensionApplyContext& context, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY)
{
    ExtendedStyleResolver::Reset(GetDimensionResetProperty(context), context.applier, apiVersion);
}

void ReportDimensionFailure(const JsonValue& value, const DimensionApplyContext& context, const char* reason,
    const std::string& message, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY)
{
    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, %{public}s, input=%{public}s",
        context.propertyName, reason, FormatStyleInput(value).c_str());
    PushStyleValidationIssue(context.issues, context.propertyName, message);
    ResetDimension(context, apiVersion);
}

void HandleInvalidDimensionInput(const JsonValue& value, const DimensionApplyContext& context,
    const std::optional<ConstraintDispatchContext>& dispatchContext)
{
    if (!value.IsValid()) {
        return;
    }
    int32_t apiVersion = dispatchContext.has_value() ? static_cast<int32_t>(dispatchContext->apiVersion) : 0;
    ReportDimensionFailure(value, context, "invalid input",
        "Property styles." + std::string(context.propertyName) + " has invalid value and has been reset to default",
        apiVersion);
}

bool IsDirectResetDimensionUnit(StyleDimensionUnit unit)
{
    return unit == StyleDimensionUnit::WRAP_CONTENT || unit == StyleDimensionUnit::FIX_AT_IDEAL_SIZE;
}

bool IsPercentDimensionUnit(StyleDimensionUnit unit)
{
    return unit == StyleDimensionUnit::PERCENT || unit == StyleDimensionUnit::MATCH_PARENT;
}

bool IsAbsoluteDimensionUnit(StyleDimensionUnit unit)
{
    return unit == StyleDimensionUnit::VP || unit == StyleDimensionUnit::FP;
}

void ResetDirectDimension(const DimensionApplyContext& context)
{
    if (context.isWidth) {
        context.applier.ResetNodeWidth(context.nodeHandle);
        context.applier.ResetNodeWidthPercent(context.nodeHandle);
        return;
    }
    context.applier.ResetNodeHeight(context.nodeHandle);
    context.applier.ResetNodeHeightPercent(context.nodeHandle);
}

void ApplyPercentDimension(
    const JsonValue& value, const StyleDimension& dimension, const DimensionApplyContext& context)
{
    float resolvedValue = 0.0F;
    if (!ConvertDimensionToFloat(dimension, resolvedValue)) {
        ReportDimensionFailure(value, context, "percent conversion failed",
            "Property styles." + std::string(context.propertyName) +
                " percent conversion failed and has been reset to default");
        return;
    }
    float percentRatio = ConvertDimensionToPercentRatio(dimension, resolvedValue);
    if (context.isWidth) {
        context.applier.ResetNodeWidth(context.nodeHandle);
        context.applier.SetWidthPercent(percentRatio);
        return;
    }
    context.applier.ResetNodeHeight(context.nodeHandle);
    context.applier.SetHeightPercent(percentRatio);
}

void ApplyAbsoluteDimension(
    const JsonValue& value, const StyleDimension& dimension, const DimensionApplyContext& context)
{
    float resolvedValue = 0.0F;
    if (!ConvertDimensionToFloat(dimension, resolvedValue)) {
        ReportDimensionFailure(value, context, "absolute conversion failed",
            "Property styles." + std::string(context.propertyName) +
                " absolute conversion failed and has been reset to default");
        return;
    }
    if (context.isWidth) {
        context.applier.ResetNodeWidthPercent(context.nodeHandle);
        context.applier.SetWidth(resolvedValue);
        return;
    }
    context.applier.ResetNodeHeightPercent(context.nodeHandle);
    context.applier.SetHeight(resolvedValue);
}
} // namespace

void ExtendedStyleResolver::ApplyDimension(const JsonValue& value, ArkUINodeApiAdapter& applier, bool isWidth,
    std::vector<DescriptorValidationIssue>& issues, std::optional<ConstraintDispatchContext> dispatchContext)
{
    const char* propName = isWidth ? "width" : "height";
    StyleDimension dimension;
    if (!StyleApplyUtils::ParseDimension(value, dimension)) {
        DimensionApplyContext context = { propName, isWidth, applier.GetRootNode(), applier, issues };
        HandleInvalidDimensionInput(value, context, dispatchContext);
        return;
    }

    ArkUI_NodeHandle nodeHandle = applier.GetRootNode();
    if (nodeHandle == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyDimension - style=%{public}s skipped, root node is null, input=%{public}s",
            propName, FormatStyleInput(value).c_str());
        return;
    }

    DimensionApplyContext context = { propName, isWidth, nodeHandle, applier, issues };
    if (ApplyLayoutPolicyDimension(nodeHandle, dimension, applier, isWidth, dispatchContext)) {
        return;
    }
    if (IsDirectResetDimensionUnit(dimension.unit)) {
        ResetDirectDimension(context);
        return;
    }
    if (IsPercentDimensionUnit(dimension.unit)) {
        ApplyPercentDimension(value, dimension, context);
        return;
    }
    if (IsAbsoluteDimensionUnit(dimension.unit)) {
        ApplyAbsoluteDimension(value, dimension, context);
        return;
    }
    LOG_A2UI(LOG_WARN,
        "ExtendedStyleResolver::ApplyDimension - style=%{public}s reset to default, unsupported unit=%{public}s",
        propName, DimensionUnitToString(dimension.unit));
    PushStyleValidationIssue(issues, propName,
        "Property styles." + std::string(propName) + " has unsupported unit and has been reset to default");
    ResetDimension(context);
}

namespace {
enum class RadiusUnitMode {
    ABSOLUTE,
    PERCENT,
    MIXED,
};

RadiusUnitMode GetRadiusUnitMode(const StyleRadius& radius)
{
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
        return RadiusUnitMode::MIXED;
    }
    return hasPercent ? RadiusUnitMode::PERCENT : RadiusUnitMode::ABSOLUTE;
}

void PushNestedRadiusIssueIfNeeded(
    bool hasInvalidNestedField, const JsonValue& value, std::vector<DescriptorValidationIssue>& issues)
{
    if (hasInvalidNestedField) {
        PushNestedDimensionValidationIssue(issues, "borderRadius", value);
    }
}

bool DispatchButtonRadius(const JsonValue& value, bool hasInvalidNestedField,
    const std::optional<ConstraintDispatchContext>& dispatchContext, std::vector<DescriptorValidationIssue>& issues)
{
    if (!dispatchContext.has_value()) {
        return false;
    }
    if (dispatchContext->componentType != "Button") {
        return false;
    }
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
    PushNestedRadiusIssueIfNeeded(hasInvalidNestedField, value, issues);
    return true;
}

void ApplyPercentRadius(const JsonValue& value, const StyleRadius& radius, bool hasInvalidNestedField,
    ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    std::array<float, 4> values = {};
    if (!ConvertRadiusValues(radius, values)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyRadius - style=borderRadius reset to default, percent conversion failed, "
            "input=%{public}s",
            FormatRadius(radius).c_str());
        PushStyleValidationIssue(issues, "borderRadius",
            "Property styles.borderRadius percent conversion failed and has been reset to default");
        ExtendedStyleResolver::Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        return;
    }
    applier.ResetNodeBorderRadius(applier.GetRootNode());
    applier.SetBorderRadiusPercent(values[0], values[1], values[3], values[2]);
    PushNestedRadiusIssueIfNeeded(hasInvalidNestedField, value, issues);
}

void ApplyAbsoluteRadius(const JsonValue& value, const StyleRadius& radius, bool hasInvalidNestedField,
    ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    float sameRadius = 0.0F;
    if (HasSameRadius(radius, sameRadius)) {
        applier.ResetNodeBorderRadiusPercent(applier.GetRootNode());
        applier.SetBorderRadius(sameRadius);
        PushNestedRadiusIssueIfNeeded(hasInvalidNestedField, value, issues);
        return;
    }
    std::array<float, 4> values = {};
    if (!ConvertRadiusValues(radius, values)) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyRadius - reset to default, dimension conversion failed, input=%{public}s, "
            "parsed=%{public}s",
            FormatStyleInput(value).c_str(), FormatRadius(radius).c_str());
        PushStyleValidationIssue(issues, "borderRadius",
            "Property styles.borderRadius dimension conversion failed and has been reset to default");
        ExtendedStyleResolver::Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        return;
    }
    applier.ResetNodeBorderRadiusPercent(applier.GetRootNode());
    applier.SetNodeBorderRadius(applier.GetRootNode(), values[0], values[1], values[3], values[2]);
    PushNestedRadiusIssueIfNeeded(hasInvalidNestedField, value, issues);
}
} // namespace

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

    if (DispatchButtonRadius(value, hasInvalidNestedField, dispatchContext, issues)) {
        return;
    }

    RadiusUnitMode unitMode = GetRadiusUnitMode(radius);
    if (unitMode == RadiusUnitMode::MIXED) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyRadius - style=borderRadius reset to default, mixed units are not supported, "
            "input=%{public}s",
            FormatRadius(radius).c_str());
        PushStyleValidationIssue(issues, "borderRadius",
            "Property styles.borderRadius has mixed units which are not supported and has been reset to default");
        Reset({ .rawName = "borderRadius", .name = StylePropertyName::BORDER_RADIUS }, applier);
        return;
    }
    if (unitMode == RadiusUnitMode::PERCENT) {
        ApplyPercentRadius(value, radius, hasInvalidNestedField, applier, issues);
        return;
    }
    ApplyAbsoluteRadius(value, radius, hasInvalidNestedField, applier, issues);
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

namespace {
void ResetInvalidBackgroundImageSize(const JsonValue& value, const char* propertyName, bool isStringInput,
    ArkUINodeApiAdapter& applier, std::vector<DescriptorValidationIssue>& issues)
{
    const char* inputKind = isStringInput ? "invalid string input" : "invalid input";
    LOG_A2UI(LOG_WARN, "ExtendedStyleResolver::ApplyBackgroundImageSize - reset to default, %{public}s=%{public}s",
        inputKind, FormatStyleInput(value).c_str());
    if (propertyName == nullptr) {
        return;
    }
    issues.push_back({ SCHEMA_ERROR_CODE_INVALID_VALUE,
        "Property styles." + std::string(propertyName) + " has invalid value and has been reset to default",
        "styles." + std::string(propertyName) });
    ExtendedStyleResolver::Reset(
        { .rawName = propertyName, .name = StylePropertyName::BACKGROUND_IMAGE_SIZE }, applier);
}

void ApplyStringBackgroundImageSize(const JsonValue& value, const char* propertyName, ArkUINodeApiAdapter& applier,
    std::vector<DescriptorValidationIssue>& issues)
{
    StyleBackgroundImageSize imageSize;
    if (StyleApplyUtils::ParseBackgroundImageSize(value, imageSize) &&
        imageSize.kind == StyleBackgroundImageSizeKind::IMAGE_SIZE) {
        applier.SetNodeBackgroundImageSizeWithStyle(applier.GetRootNode(), imageSize.imageSize);
        return;
    }
    ResetInvalidBackgroundImageSize(value, propertyName, true, applier, issues);
}

void ApplyPercentBackgroundImageSize(const JsonValue& value, const BackgroundImageSizeParseResult& parsed,
    ArkUINodeApiAdapter& applier, const std::optional<ConstraintDispatchContext>& dispatchContext)
{
    if (!dispatchContext.has_value()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedStyleResolver::ApplyBackgroundImageSize - has percent but no dispatchContext, "
            "fallback to C++ path, input=%{public}s",
            FormatStyleInput(value).c_str());
        applier.SetNodeBackgroundImageSize(applier.GetRootNode(), parsed.width, parsed.height);
        return;
    }
    LOG_A2UI(LOG_INFO,
        "ExtendedStyleResolver::ApplyBackgroundImageSize - percent dispatched to ETS, "
        "componentId=%{public}s, payload=%{public}s",
        dispatchContext->componentId.c_str(), parsed.percentJson.c_str());
    CrossLanguageAttributeBridge::GetInstance().Dispatch({ .renderId = dispatchContext->renderId,
        .componentId = dispatchContext->componentId,
        .nodeUniqueId = dispatchContext->nodeUniqueId,
        .componentType = dispatchContext->componentType,
        .attributeName = "backgroundImageSize",
        .payloadJson = parsed.percentJson });
}
} // namespace

void ExtendedStyleResolver::ApplyBackgroundImageSize(const JsonValue& value, const char* propertyName,
    ArkUINodeApiAdapter& applier, std::optional<ConstraintDispatchContext> dispatchContext,
    std::vector<DescriptorValidationIssue>& issues)
{
    if (!value.IsValid()) {
        return;
    }

    if (value.IsString()) {
        ApplyStringBackgroundImageSize(value, propertyName, applier, issues);
        return;
    }

    BackgroundImageSizeParseResult parsed;
    if (!ParseBackgroundImageSizeWithPercent(value, parsed)) {
        ResetInvalidBackgroundImageSize(value, propertyName, false, applier, issues);
        return;
    }
    if (parsed.percentJson.empty()) {
        applier.SetNodeBackgroundImageSize(applier.GetRootNode(), parsed.width, parsed.height);
        return;
    }
    ApplyPercentBackgroundImageSize(value, parsed, applier, dispatchContext);
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
