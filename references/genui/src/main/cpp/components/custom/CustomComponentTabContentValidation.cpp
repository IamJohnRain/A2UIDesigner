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
#include <set>

#include "components/TypeValidation.h"
#include "components/custom/CustomComponent.h"

namespace NativeModule {

namespace {

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
    return allowedKeywords.count(token) > 0;
}

} // namespace

void CustomComponent::NormalizeExtendedTabContentProperty(const std::string& propertyName, JsonValue& value)
{
    auto report = [this](const std::string& code, const std::string& message, const std::string& path) {
        ReportCustomSchemaWarning(code, message, path);
    };

    if (propertyName == "title" || propertyName == "icon" || propertyName == "selectedSrc") {
        if (value.IsString() || value.IsObject()) {
            return;
        }

        ReportTypeMismatchAndReset(report, value, "string", propertyName);
        return;
    }

    if (propertyName == "child") {
        value = JsonValue();
        return;
    }

    if (propertyName == "tabType") {
        if (IsDynamicValue(value)) {
            return;
        }

        if (!value.IsString()) {
            ReportTypeMismatchAndReset(report, value, "string", "tabType");
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

    if (propertyName == "styles") {
        NormalizeExtendedTabContentStyles(value);
    }
}

void CustomComponent::NormalizeExtendedTabContentStyles(JsonValue& value)
{
    if (!value.IsObject()) {
        return;
    }

    NormalizeExtendedTabContentNumberStyles(value);
    NormalizeExtendedTabContentStringStyles(value);
    NormalizeExtendedTabContentFontWeightStyle(value);
}

void CustomComponent::NormalizeExtendedTabContentNumberStyles(JsonValue& value)
{
    auto report = [this](const std::string& code, const std::string& message, const std::string& path) {
        ReportCustomSchemaWarning(code, message, path);
    };

    auto dropInvalidNumberStyle = [this, &value, &report](const char* styleName) {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }

        JsonValue styleValue = value.GetItem(styleName);
        if (IsDynamicValue(styleValue)) {
            return;
        }
        if (IsLiteralNumber(styleValue)) {
            if (styleValue.GetNumberValue(-1.0) >= 0.0) {
                return;
            }
            std::string propertyPath = std::string("styles.") + styleName;
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + propertyPath + " must be greater than or equal to 0, fallback/reset has been applied",
                propertyPath);
            value.Remove(styleName);
            return;
        }

        std::string propertyPath = std::string("styles.") + styleName;
        ReportTypeMismatchAndReset(
            report, [&value, styleName]() { value.Remove(styleName); }, styleValue, "number", propertyPath);
    };

    dropInvalidNumberStyle("fontSize");
    dropInvalidNumberStyle("iconSize");
    dropInvalidNumberStyle("space");
}

void CustomComponent::NormalizeExtendedTabContentStringStyles(JsonValue& value)
{
    auto report = [this](const std::string& code, const std::string& message, const std::string& path) {
        ReportCustomSchemaWarning(code, message, path);
    };

    auto dropInvalidNonEmptyStringStyle = [this, &value, &report](const char* styleName) {
        if (styleName == nullptr || !value.Has(styleName)) {
            return;
        }

        JsonValue styleValue = value.GetItem(styleName);
        if (IsDynamicValue(styleValue)) {
            return;
        }

        std::string propertyPath = std::string("styles.") + styleName;

        if (IsNonEmptyLiteralString(styleValue)) {
            return;
        }

        ReportTypeMismatchAndReset(
            report, [&value, styleName]() { value.Remove(styleName); }, styleValue, "non-empty string", propertyPath);
    };

    dropInvalidNonEmptyStringStyle("selectedColor");
    dropInvalidNonEmptyStringStyle("unSelectedColor");
    dropInvalidNonEmptyStringStyle("defaultBackgroundColor");
    dropInvalidNonEmptyStringStyle("selectedBackgroundColor");
    dropInvalidNonEmptyStringStyle("defaultBorderColor");
    dropInvalidNonEmptyStringStyle("selectedBorderColor");
}

void CustomComponent::NormalizeExtendedTabContentFontWeightStyle(JsonValue& value)
{
    auto report = [this](const std::string& code, const std::string& message, const std::string& path) {
        ReportCustomSchemaWarning(code, message, path);
    };

    if (!value.Has("fontWeight")) {
        return;
    }

    JsonValue fontWeightValue = value.GetItem("fontWeight");
    if (IsDynamicValue(fontWeightValue)) {
        return;
    }
    if (IsValidFontWeightValue(fontWeightValue)) {
        return;
    }

    std::string propertyPath = "styles.fontWeight";
    if (fontWeightValue.IsString() || fontWeightValue.IsNumber()) {
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + propertyPath + " expects valid font weight, fallback/reset has been applied", propertyPath);
        value.Remove("fontWeight");
    } else {
        ReportTypeMismatchAndReset(
            report, [&value]() { value.Remove("fontWeight"); }, fontWeightValue, "string or number", propertyPath);
    }
}

} // namespace NativeModule
