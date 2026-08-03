#include <cctype>
#include <cmath>
#include <set>

#include "components/custom/CustomComponent.h"
#include "components/custom/CustomComponentExpressionBinding.h"
#include "styles/StyleApplyUtils.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

bool IsDynamicDescriptorObject(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool IsEmptyStringValue(const JsonValue& value)
{
    return value.IsString() && StyleApplyUtils::TrimToken(value.GetStringValue("")).empty();
}

bool IsAllowedExtendedTabType(const std::string& value)
{
    return value == "underline" || value == "capsule";
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

bool IsValidFontWeightValue(const JsonValue& value)
{
    if (value.IsNumber()) {
        return IsValidFontWeightNumber(value.GetNumberValue(400.0));
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (char& ch : token) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    static const std::set<std::string> allowedKeywords = { "bold", "normal", "bolder", "lighter", "medium", "regular" };
    if (allowedKeywords.count(token) > 0) {
        return true;
    }

    float parsedNumber = 0.0F;
    return StyleApplyUtils::ParseNumber(value, parsedNumber) && IsValidFontWeightNumber(parsedNumber);
}

} // namespace

void CustomComponent::NormalizeExtendedTabContentProperty(const std::string& propertyName, JsonValue& value)
{
    if (propertyName == "title" || propertyName == "icon" || propertyName == "selectedSrc") {
        if (value.IsString() || value.IsObject()) {
            return;
        }

        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyName + " expects string value or dynamic descriptor, got type '" +
                std::string(value.GetTypeName()) + "', fallback/reset has been applied",
            propertyName);
        value = JsonValue();
        return;
    }

    if (propertyName == "tabType") {
        if (IsExpressionStringValue(value) || IsDynamicDescriptorObject(value)) {
            return;
        }

        if (!value.IsString()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property tabType expects string value, got type '" + std::string(value.GetTypeName()) +
                    "', fallback/reset has been applied",
                "tabType");
            value = JsonValue();
            return;
        }

        std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
        if (!token.empty() && !IsAllowedExtendedTabType(token)) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property tabType has invalid value and has been reset to default", "tabType");
            value = JsonValue();
        }
        return;
    }

    if (propertyName == "child") {
        if (!value.IsString()) {
            value = JsonValue();
            return;
        }

        if (StyleApplyUtils::TrimToken(value.GetStringValue("")).empty()) {
            value = JsonValue();
        }
        return;
    }

    if (propertyName == "styles") {
        NormalizeExtendedTabContentStyles(value);
    }
}

void CustomComponent::NormalizeExtendedTabContentStyles(JsonValue& value)
{
    if (!value.IsObject()) {
        return;
    }

    auto dropInvalidNumberStyle = [this, &value](const char* styleName) {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }

        JsonValue styleValue = value.GetItem(styleName);
        if (IsDynamicDescriptorObject(styleValue) || IsExpressionStringValue(styleValue)) {
            return;
        }
        float parsedNumber = 0.0F;
        if (StyleApplyUtils::ParseNumber(styleValue, parsedNumber)) {
            return;
        }

        std::string propertyPath = std::string("styles.") + styleName;
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyPath + " expects number value, got type '" + styleValue.GetTypeName() +
                "', fallback/reset has been applied",
            propertyPath);
        value.Remove(styleName);
    };

    dropInvalidNumberStyle("fontSize");
    dropInvalidNumberStyle("iconSize");
    dropInvalidNumberStyle("space");

    auto dropInvalidNonEmptyStringStyle = [this, &value](const char* styleName) {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }

        JsonValue styleValue = value.GetItem(styleName);
        if (styleValue.IsString() && !IsEmptyStringValue(styleValue)) {
            return;
        }
        if (styleValue.IsObject()) {
            return;
        }

        std::string propertyPath = std::string("styles.") + styleName;
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyPath + " expects non-empty string value, got type '" +
                std::string(styleValue.GetTypeName()) + "', fallback/reset has been applied",
            propertyPath);
        value.Remove(styleName);
    };

    dropInvalidNonEmptyStringStyle("selectedColor");
    dropInvalidNonEmptyStringStyle("unSelectedColor");
    dropInvalidNonEmptyStringStyle("defaultBackgroundColor");
    dropInvalidNonEmptyStringStyle("selectedBackgroundColor");
    dropInvalidNonEmptyStringStyle("defaultBorderColor");
    dropInvalidNonEmptyStringStyle("selectedBorderColor");

    if (!value.Has("fontWeight")) {
        return;
    }

    JsonValue fontWeightValue = value.GetItem("fontWeight");
    if (IsDynamicDescriptorObject(fontWeightValue) || IsExpressionStringValue(fontWeightValue)) {
        return;
    }
    if (IsValidFontWeightValue(fontWeightValue)) {
        return;
    }

    std::string propertyPath = "styles.fontWeight";
    if (fontWeightValue.IsString() || fontWeightValue.IsNumber()) {
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + propertyPath + " expects valid font weight, fallback/reset has been applied", propertyPath);
    } else {
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyPath + " expects string or number value, got type '" + fontWeightValue.GetTypeName() +
                "', fallback/reset has been applied",
            propertyPath);
    }
    value.Remove("fontWeight");
}

} // namespace NativeModule
