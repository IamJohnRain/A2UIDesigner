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

#include <cctype>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <utility>

#include "components/TypeValidation.h"
#include "components/custom/CustomComponent.h"
#include "styles/StyleApplyUtils.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

using ReportFunction = std::function<void(const std::string&, const std::string&, const std::string&)>;

struct NumberFieldPolicy {
    bool allowNegative = false;
    bool unitInterval = false;
    bool emptyStringIsTypeMismatch = false;
};

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

class CommonStyleValidator {
public:
    explicit CommonStyleValidator(ReportFunction report) : report_(std::move(report)) {}

    void Normalize(JsonValue& value)
    {
        NormalizeDimensionStyle(value, "width", "styles.width");
        NormalizeDimensionStyle(value, "height", "styles.height");
        NormalizeConstraintSize(value);
        NormalizeBackgroundImage(value);
        NormalizeBackgroundImageSize(value, "backgroundImageSizeWithStyle");
        NormalizeBackgroundImageSize(value, "backgroundImageSize");
        NormalizeBackgroundImageSize(value, "backgroundimageSize");
        NormalizeEdgeStyle(value, "margin", "styles.margin");
        NormalizeEdgeStyle(value, "padding", "styles.padding");
        NormalizeBorderRadius(value);
        NormalizeBorderWidth(value);
        NormalizeClip(value);
        NormalizeColorField(value, "backgroundColor", "styles.backgroundColor");
        NormalizeBorderColor(value);
        NormalizeLinearGradient(value);
        NormalizeBoundedNumber(value, "layoutWeight", "styles.layoutWeight", std::numeric_limits<float>::max());
        NormalizeBoundedNumber(value, "flexShrink", "styles.flexShrink", std::numeric_limits<float>::max());
        NormalizeShadow(value);
        NormalizeVisibility(value);
    }

private:
    void NormalizeDimensionField(JsonValue& parentValue, const char* key, const std::string& path)
    {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }
        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicValue(fieldValue)) {
            return;
        }

        StyleDimension dimension;
        if (StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
            return;
        }
        if (fieldValue.IsNull() || IsEmptyStringValue(fieldValue) || fieldValue.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects dimension value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    }

    void NormalizeDimensionStyle(JsonValue& parentValue, const char* key, const std::string& path)
    {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }
        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicValue(fieldValue)) {
            return;
        }

        StyleDimension dimension;
        if (StyleApplyUtils::ParseDimension(fieldValue, dimension)) {
            return;
        }
        if (fieldValue.IsNull() || fieldValue.IsString() || fieldValue.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects dimension value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    }

    void NormalizeNumberField(
        JsonValue& parentValue, const char* key, const std::string& path, const NumberFieldPolicy& policy)
    {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }
        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicValue(fieldValue)) {
            return;
        }

        float parsedNumber = 0.0F;
        bool parsed = StyleApplyUtils::ParseNumber(fieldValue, parsedNumber) && std::isfinite(parsedNumber);
        bool valid = parsed && (policy.allowNegative || parsedNumber >= 0.0F) &&
                     (!policy.unitInterval || (parsedNumber >= 0.0F && parsedNumber <= 1.0F));
        if (valid) {
            return;
        }
        bool typeMismatch = (policy.emptyStringIsTypeMismatch && IsEmptyStringValue(fieldValue)) ||
                            (!fieldValue.IsNumber() && !fieldValue.IsString()) || (!parsed && fieldValue.IsString());
        if (typeMismatch && !fieldValue.IsNull()) {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects number value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        } else {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        }
        parentValue.Remove(key);
    }

    void NormalizeColorField(JsonValue& parentValue, const char* key, const std::string& path)
    {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }
        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicValue(fieldValue) || ParseColorLikeValue(fieldValue)) {
            return;
        }
        if (fieldValue.IsNull() || fieldValue.IsString() || fieldValue.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects color value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    }

    void NormalizeBooleanField(JsonValue& parentValue, const char* key, const std::string& path)
    {
        if (key == nullptr || !parentValue.Has(key)) {
            return;
        }
        JsonValue fieldValue = parentValue.GetItem(key);
        if (IsDynamicValue(fieldValue) || ParseBooleanLikeValue(fieldValue)) {
            return;
        }
        if (fieldValue.IsNull() || fieldValue.IsString()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects boolean value, got type '" + std::string(fieldValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        parentValue.Remove(key);
    }

    void NormalizeConstraintSize(JsonValue& value)
    {
        if (!value.Has("constraintSize") || IsDynamicValue(value.GetItem("constraintSize"))) {
            return;
        }
        JsonValue constraintSize = value.GetItem("constraintSize");
        if (!constraintSize.IsObject()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.constraintSize has invalid value and has been reset to default",
                "styles.constraintSize");
            value.Remove("constraintSize");
            return;
        }
        NormalizeDimensionField(constraintSize, "minWidth", "styles.constraintSize.minWidth");
        NormalizeDimensionField(constraintSize, "maxWidth", "styles.constraintSize.maxWidth");
        NormalizeDimensionField(constraintSize, "minHeight", "styles.constraintSize.minHeight");
        NormalizeDimensionField(constraintSize, "maxHeight", "styles.constraintSize.maxHeight");
        if (!HasAnyChildField(constraintSize)) {
            value.Remove("constraintSize");
        }
    }

    void NormalizeBackgroundImage(JsonValue& value)
    {
        if (!value.Has("backgroundImage") || IsDynamicValue(value.GetItem("backgroundImage"))) {
            return;
        }
        JsonValue backgroundImageValue = value.GetItem("backgroundImage");
        std::string backgroundImage;
        if (StyleApplyUtils::ParseBackgroundImage(backgroundImageValue, backgroundImage)) {
            return;
        }
        if (backgroundImageValue.IsNull() || backgroundImageValue.IsString()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.backgroundImage has invalid value and has been reset to default",
                "styles.backgroundImage");
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.backgroundImage expects string value, got type '" +
                    std::string(backgroundImageValue.GetTypeName()) + "', fallback/reset has been applied",
                "styles.backgroundImage");
        }
        value.Remove("backgroundImage");
    }

    void NormalizeBackgroundImageSize(JsonValue& value, const char* styleName)
    {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }
        JsonValue imageSizeValue = value.GetItem(styleName);
        if (IsDynamicValue(imageSizeValue)) {
            return;
        }

        const std::string path = std::string("styles.") + styleName;
        if (imageSizeValue.IsObject()) {
            NormalizeDimensionField(imageSizeValue, "width", path + ".width");
            NormalizeDimensionField(imageSizeValue, "height", path + ".height");
            if (!HasAnyChildField(imageSizeValue)) {
                value.ReplaceString(styleName, "auto");
            }
            return;
        }
        StyleBackgroundImageSize imageSize;
        if (!StyleApplyUtils::ParseBackgroundImageSize(imageSizeValue, imageSize)) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
            value.ReplaceString(styleName, "auto");
        }
    }

    void NormalizeEdgeStyle(JsonValue& value, const char* styleName, const std::string& path)
    {
        if (!value.Has(styleName) || IsDynamicValue(value.GetItem(styleName))) {
            return;
        }
        JsonValue edgeValue = value.GetItem(styleName);
        if (edgeValue.IsObject()) {
            NormalizeDimensionField(edgeValue, "all", path + ".all");
            NormalizeDimensionField(edgeValue, "vertical", path + ".vertical");
            NormalizeDimensionField(edgeValue, "horizontal", path + ".horizontal");
            NormalizeDimensionField(edgeValue, "top", path + ".top");
            NormalizeDimensionField(edgeValue, "right", path + ".right");
            NormalizeDimensionField(edgeValue, "bottom", path + ".bottom");
            NormalizeDimensionField(edgeValue, "left", path + ".left");
            return;
        }
        StyleEdge edge;
        if (!StyleApplyUtils::ParseEdge(edgeValue, edge)) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
            value.Remove(styleName);
        }
    }

    void NormalizeBorderRadius(JsonValue& value)
    {
        if (!value.Has("borderRadius") || IsDynamicValue(value.GetItem("borderRadius"))) {
            return;
        }
        JsonValue radiusValue = value.GetItem("borderRadius");
        if (radiusValue.IsObject()) {
            NormalizeDimensionField(radiusValue, "all", "styles.borderRadius.all");
            NormalizeDimensionField(radiusValue, "topLeft", "styles.borderRadius.topLeft");
            NormalizeDimensionField(radiusValue, "topRight", "styles.borderRadius.topRight");
            NormalizeDimensionField(radiusValue, "bottomLeft", "styles.borderRadius.bottomLeft");
            NormalizeDimensionField(radiusValue, "bottomRight", "styles.borderRadius.bottomRight");
            return;
        }
        StyleRadius radius;
        if (!StyleApplyUtils::ParseRadius(radiusValue, radius)) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.borderRadius has invalid value and has been reset to default", "styles.borderRadius");
            value.Remove("borderRadius");
        }
    }

    void NormalizeBorderWidth(JsonValue& value)
    {
        if (!value.Has("borderWidth") || IsDynamicValue(value.GetItem("borderWidth"))) {
            return;
        }
        JsonValue borderWidthValue = value.GetItem("borderWidth");
        if (borderWidthValue.IsObject()) {
            NormalizeDimensionField(borderWidthValue, "all", "styles.borderWidth.all");
            NormalizeDimensionField(borderWidthValue, "vertical", "styles.borderWidth.vertical");
            NormalizeDimensionField(borderWidthValue, "horizontal", "styles.borderWidth.horizontal");
            NormalizeDimensionField(borderWidthValue, "top", "styles.borderWidth.top");
            NormalizeDimensionField(borderWidthValue, "right", "styles.borderWidth.right");
            NormalizeDimensionField(borderWidthValue, "bottom", "styles.borderWidth.bottom");
            NormalizeDimensionField(borderWidthValue, "left", "styles.borderWidth.left");
            if (!HasAnyChildField(borderWidthValue)) {
                value.Remove("borderWidth");
            }
            return;
        }
        StyleEdge borderWidth;
        if (StyleApplyUtils::ParseEdge(borderWidthValue, borderWidth)) {
            return;
        }
        ReportInvalidBorderWidth(borderWidthValue);
        value.Remove("borderWidth");
    }

    void ReportInvalidBorderWidth(const JsonValue& borderWidthValue)
    {
        if (borderWidthValue.IsNull() || borderWidthValue.IsString() || borderWidthValue.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.borderWidth has invalid value and has been reset to default", "styles.borderWidth");
            return;
        }
        report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.borderWidth expects dimension value, got type '" +
                std::string(borderWidthValue.GetTypeName()) + "', fallback/reset has been applied",
            "styles.borderWidth");
    }

    void NormalizeClip(JsonValue& value)
    {
        if (!value.Has("clip") || IsDynamicValue(value.GetItem("clip"))) {
            return;
        }
        JsonValue clipValue = value.GetItem("clip");
        bool clip = false;
        if (StyleApplyUtils::ParseClip(clipValue, clip) || ParseBooleanLikeValue(clipValue)) {
            return;
        }
        if (clipValue.IsNull() || clipValue.IsString()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.clip has invalid value and has been reset to default", "styles.clip");
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property styles.clip expects boolean value, got type '" + std::string(clipValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                "styles.clip");
        }
        value.Remove("clip");
    }

    void NormalizeBorderColor(JsonValue& value)
    {
        if (!value.Has("borderColor") || IsDynamicValue(value.GetItem("borderColor"))) {
            return;
        }
        JsonValue borderColorValue = value.GetItem("borderColor");
        if (borderColorValue.IsObject()) {
            NormalizeColorField(borderColorValue, "top", "styles.borderColor.top");
            NormalizeColorField(borderColorValue, "right", "styles.borderColor.right");
            NormalizeColorField(borderColorValue, "bottom", "styles.borderColor.bottom");
            NormalizeColorField(borderColorValue, "left", "styles.borderColor.left");
            if (!HasAnyChildField(borderColorValue)) {
                value.Remove("borderColor");
            }
            return;
        }
        if (ParseColorLikeValue(borderColorValue)) {
            return;
        }
        ReportInvalidBorderColor(borderColorValue);
        value.Remove("borderColor");
    }

    void ReportInvalidBorderColor(const JsonValue& value)
    {
        if (value.IsNull() || value.IsString() || value.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.borderColor has invalid value and has been reset to default", "styles.borderColor");
            return;
        }
        report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles.borderColor expects color value, got type '" + std::string(value.GetTypeName()) +
                "', fallback/reset has been applied",
            "styles.borderColor");
    }

    void NormalizeLinearGradient(JsonValue& value)
    {
        if (!value.Has("linearGradient") || IsDynamicValue(value.GetItem("linearGradient"))) {
            return;
        }
        JsonValue gradient = value.GetItem("linearGradient");
        if (!gradient.IsObject()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.linearGradient has invalid value and has been reset to default",
                "styles.linearGradient");
            value.Remove("linearGradient");
            return;
        }

        bool shouldRemove = NormalizeGradientColors(gradient);
        NormalizeGradientAngle(gradient);
        NormalizeGradientDirection(gradient);
        NormalizeBooleanField(gradient, "repeating", "styles.linearGradient.repeating");
        NormalizeGradientStops(gradient);
        if (shouldRemove || !HasAnyChildField(gradient)) {
            value.Remove("linearGradient");
        }
    }

    bool NormalizeGradientColors(JsonValue& gradient)
    {
        if (!gradient.Has("colors")) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.linearGradient has invalid value and has been reset to default",
                "styles.linearGradient");
            return true;
        }
        JsonValue colors = gradient.GetItem("colors");
        if (IsDynamicValue(colors) || (colors.IsArray() && colors.GetArraySize() > 0)) {
            return false;
        }
        report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.linearGradient.colors has invalid value and has been reset to default",
            "styles.linearGradient.colors");
        gradient.Remove("colors");
        return true;
    }

    void NormalizeGradientAngle(JsonValue& gradient)
    {
        if (!gradient.Has("angle")) {
            return;
        }
        JsonValue angle = gradient.GetItem("angle");
        float parsedAngle = 0.0F;
        bool valid = IsDynamicValue(angle) || StyleApplyUtils::ParseNumber(angle, parsedAngle) ||
                     (angle.IsString() && !IsEmptyStringValue(angle));
        if (valid) {
            return;
        }
        bool invalidValue = angle.IsNull() || angle.IsString();
        report_(invalidValue ? SCHEMA_ERROR_CODE_INVALID_VALUE : SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            invalidValue ? "Property styles.linearGradient.angle has invalid value and has been reset to default"
                         : "Property styles.linearGradient.angle expects string or number value, got type '" +
                               std::string(angle.GetTypeName()) + "', fallback/reset has been applied",
            "styles.linearGradient.angle");
        gradient.Remove("angle");
    }

    void NormalizeGradientDirection(JsonValue& gradient)
    {
        if (!gradient.Has("direction")) {
            return;
        }
        JsonValue direction = gradient.GetItem("direction");
        if (IsDynamicValue(direction) || ParseGradientDirectionValue(direction)) {
            return;
        }
        report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.linearGradient.direction has invalid value and has been reset to default",
            "styles.linearGradient.direction");
        gradient.Remove("direction");
    }

    void NormalizeGradientStops(JsonValue& gradient)
    {
        if (!gradient.Has("stops")) {
            return;
        }
        JsonValue stops = gradient.GetItem("stops");
        if (IsDynamicValue(stops) || stops.IsArray()) {
            return;
        }
        report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property styles.linearGradient.stops has invalid value and has been reset to default",
            "styles.linearGradient.stops");
        gradient.Remove("stops");
    }

    void NormalizeBoundedNumber(JsonValue& value, const char* key, const std::string& path, float maximum)
    {
        if (!value.Has(key) || IsDynamicValue(value.GetItem(key))) {
            return;
        }
        JsonValue numberValue = value.GetItem(key);
        float parsedNumber = 0.0F;
        bool valid = StyleApplyUtils::ParseNumber(numberValue, parsedNumber) && std::isfinite(parsedNumber) &&
                     parsedNumber >= 0.0F && parsedNumber <= maximum;
        if (valid) {
            return;
        }
        if (numberValue.IsNull() || numberValue.IsString() || numberValue.IsNumber()) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + path + " has invalid value and has been reset to default", path);
        } else {
            report_(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + path + " expects number value, got type '" + std::string(numberValue.GetTypeName()) +
                    "', fallback/reset has been applied",
                path);
        }
        value.Remove(key);
    }

    void NormalizeShadow(JsonValue& value)
    {
        if (!value.Has("shadow") || IsDynamicValue(value.GetItem("shadow"))) {
            return;
        }
        JsonValue shadowValue = value.GetItem("shadow");
        if (!shadowValue.IsObject()) {
            StyleShadow shadow;
            if (!StyleApplyUtils::ParseShadow(shadowValue, shadow) || !shadow.valid) {
                report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property styles.shadow has invalid value and has been reset to default", "styles.shadow");
                value.Remove("shadow");
            }
            return;
        }
        NormalizeShadowObject(shadowValue);
        if (!HasAnyChildField(shadowValue)) {
            value.Remove("shadow");
        }
    }

    void NormalizeShadowObject(JsonValue& shadow)
    {
        if (shadow.Has("style") && !IsDynamicValue(shadow.GetItem("style")) &&
            !ParseShadowStyleValue(shadow.GetItem("style"))) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.shadow.style has invalid value and has been reset to default", "styles.shadow.style");
            shadow.Remove("style");
        }
        NormalizeNumberField(shadow, "radius", "styles.shadow.radius", NumberFieldPolicy {});
        NormalizeNumberField(shadow, "offsetX", "styles.shadow.offsetX",
            NumberFieldPolicy { .allowNegative = true, .emptyStringIsTypeMismatch = true });
        NormalizeNumberField(shadow, "offsetY", "styles.shadow.offsetY",
            NumberFieldPolicy { .allowNegative = true, .emptyStringIsTypeMismatch = true });
        NormalizeColorField(shadow, "color", "styles.shadow.color");
        if (shadow.Has("type") && !IsDynamicValue(shadow.GetItem("type")) &&
            !ParseShadowTypeValue(shadow.GetItem("type"))) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.shadow.type has invalid value and has been reset to default", "styles.shadow.type");
            shadow.Remove("type");
        }
        NormalizeBooleanField(shadow, "fill", "styles.shadow.fill");
    }

    void NormalizeVisibility(JsonValue& value)
    {
        if (!value.Has("visibility") || IsDynamicValue(value.GetItem("visibility"))) {
            return;
        }
        JsonValue visibilityValue = value.GetItem("visibility");
        A2UIVisibility visibility = A2UIVisibility::VISIBLE;
        if (!StyleApplyUtils::ParseVisibility(visibilityValue, visibility)) {
            report_(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.visibility has invalid value and has been reset to default", "styles.visibility");
            value.Remove("visibility");
        }
    }

    ReportFunction report_;
};

} // namespace

void CustomComponent::NormalizeExtendedCommonStyles(JsonValue& value)
{
    UpdateFlexShrinkStyleState(value);
    if (!value.IsObject()) {
        const bool isNull = value.IsNull();
        ReportCustomSchemaWarning(isNull ? SCHEMA_ERROR_CODE_INVALID_VALUE : SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            isNull ? "Property styles has invalid value and has been reset to default"
                   : "Property styles expects object value, field has been ignored",
            "styles");
        value = JsonValue();
        return;
    }

    CommonStyleValidator validator([this](const std::string& code, const std::string& message,
                                       const std::string& path) { ReportCustomSchemaWarning(code, message, path); });
    validator.Normalize(value);
}

void CustomComponent::UpdateFlexShrinkStyleState(const JsonValue& styles)
{
    if (!styles.IsObject()) {
        if (flexShrinkStyleState_ != FlexShrinkStyleState::UNSPECIFIED) {
            flexShrinkStyleState_ = FlexShrinkStyleState::PARENT_DEFAULT;
        }
        SyncFlexShrinkParentDefaultProperties();
        return;
    }
    if (!styles.Has("flexShrink")) {
        if (flexShrinkStyleState_ != FlexShrinkStyleState::UNSPECIFIED) {
            flexShrinkStyleState_ = FlexShrinkStyleState::PARENT_DEFAULT;
        }
        SyncFlexShrinkParentDefaultProperties();
        return;
    }

    JsonValue value = styles.GetItem("flexShrink");
    if (IsDynamicValue(value)) {
        flexShrinkStyleState_ = FlexShrinkStyleState::DYNAMIC_VALUE;
    } else {
        float parsedValue = 0.0F;
        flexShrinkStyleState_ = StyleApplyUtils::ParseFlexShrink(value, parsedValue)
                                    ? FlexShrinkStyleState::EXPLICIT_VALUE
                                    : FlexShrinkStyleState::PARENT_DEFAULT;
    }
    SyncFlexShrinkParentDefaultProperties();
}

} // namespace NativeModule
