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

#include "ExtendedComponentStyleValidation.h"

#include <cmath>
#include <map>
#include <set>
#include <string>

#include "styles/StyleApplyUtils.h"

#include "ExtendedComponent.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

bool IsDynamicOrExpressionValue(const JsonValue& value)
{
    return (value.IsObject() && (value.Has("path") || value.Has("call"))) ||
           (value.IsString() && StyleApplyUtils::IsExpressionString(value.GetStringValue("")));
}

} // namespace

bool IsExtendedComponentSpecificStyleKey(const std::string& componentType, const std::string& styleName)
{
    static const std::map<std::string, std::set<std::string>> styleKeys = {
        { "Button", { "fontScaleMode", "minFontScale", "maxFontScale" } },
        { "Column", { "justifyContent", "alignItems" } },
        { "Grid", { "columnsTemplate", "rowsTemplate", "columnsGap", "rowsGap" } },
        { "List", { "listDirection", "scrollBar", "nestedScroll" } },
        { "Row", { "justifyContent", "alignItems", "wrap" } },
        { "Stack", { "alignContent" } },
        { "Text", { "fontColor", "fontSize", "fontWeight", "maxLines", "minFontSize", "maxFontSize", "textOverflow",
                      "textAlign", "wordBreak", "decoration", "fontScaleMode", "minFontScale", "maxFontScale" } },
        { "TextInput", { "selectedBackgroundColor", "fontScaleMode", "minFontScale", "maxFontScale", "cancelButton",
                           "underlineColor" } },
        { "Image", { "objectFit", "fillColor" } },
        { "Divider", { "strokeWidth", "vertical", "color" } },
        { "Progress", { "color", "type", "strokeWidth" } },
        { "Toggle", { "selectedColor", "unSelectedColor", "switchPointColor" } },
        { "Radio", { "checkedBackgroundColor", "unCheckedBorderColor", "indicatorColor" } },
        { "Checkbox", { "selectedColor", "unselectedColor", "shape", "mark" } },
        { "CheckboxGroup", { "selectedColor", "unSelectedColor", "checkboxShape", "mark" } },
    };

    auto iter = styleKeys.find(componentType);
    if (iter == styleKeys.end()) {
        return false;
    }
    return iter->second.find(styleName) != iter->second.end();
}

void ExtendedComponent::ReportStyleTypeMismatch(const std::string& propertyPath, const std::string& expectedType) const
{
    if (propertyPath.empty()) {
        return;
    }
    ReportExtendedSchemaWarning(
        SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Property " + propertyPath + " expects " + expectedType, propertyPath);
}

void ExtendedComponent::ReportStyleInvalidValue(const std::string& propertyPath) const
{
    if (propertyPath.empty()) {
        return;
    }
    ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
        "Property " + propertyPath + " has invalid value and has been reset to default", propertyPath);
}

void ExtendedComponent::ValidateStyleEnumProperty(
    const JsonValue& styles, const std::string& styleName, std::initializer_list<const char*> allowedValues) const
{
    if (!styles.IsObject() || styleName.empty() || !styles.Has(styleName.c_str())) {
        return;
    }

    JsonValue value = styles.GetItem(styleName.c_str());
    if (IsDynamicOrExpressionValue(value)) {
        return;
    }

    const std::string propertyPath = "styles." + styleName;
    if (!value.IsString()) {
        ReportStyleTypeMismatch(propertyPath, "string");
        return;
    }

    const std::string rawValue = value.GetStringValue("");
    if (rawValue.empty()) {
        ReportStyleInvalidValue(propertyPath);
        return;
    }

    for (const char* allowedValue : allowedValues) {
        if (allowedValue != nullptr && rawValue == allowedValue) {
            return;
        }
    }
    ReportStyleInvalidValue(propertyPath);
}

void ExtendedComponent::ValidateStyleStringProperty(const JsonValue& styles, const std::string& styleName) const
{
    if (!styles.IsObject() || styleName.empty() || !styles.Has(styleName.c_str())) {
        return;
    }

    JsonValue value = styles.GetItem(styleName.c_str());
    if (IsDynamicOrExpressionValue(value)) {
        return;
    }

    const std::string propertyPath = "styles." + styleName;
    if (!value.IsString()) {
        ReportStyleTypeMismatch(propertyPath, "string");
        return;
    }

    if (value.GetStringValue("").empty()) {
        ReportStyleInvalidValue(propertyPath);
    }
}

void ExtendedComponent::ValidateStyleNumberProperty(
    const JsonValue& styles, const std::string& styleName, double minimumValue) const
{
    if (!styles.IsObject() || styleName.empty() || !styles.Has(styleName.c_str())) {
        return;
    }

    JsonValue value = styles.GetItem(styleName.c_str());
    if (IsDynamicOrExpressionValue(value)) {
        return;
    }

    const std::string propertyPath = "styles." + styleName;
    if (!value.IsNumber()) {
        ReportStyleTypeMismatch(propertyPath, "number");
        return;
    }

    double numericValue = value.GetNumberValue(minimumValue - 1.0);
    if (!std::isfinite(numericValue) || numericValue < minimumValue) {
        ReportStyleInvalidValue(propertyPath);
    }
}

bool ExtendedComponent::IsDynamicValueDescriptor(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool ExtendedComponent::HasDynamicStyleValue(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys, const std::string& styleName) const
{
    return styles.IsObject() && !styleName.empty() && dynamicStyleKeys.find(styleName) != dynamicStyleKeys.end() &&
           styles.Has(styleName.c_str());
}

void ExtendedComponent::ReportDynamicStyleTypeMismatch(
    const std::string& propertyPath, const std::string& expectedType) const
{
    if (propertyPath.empty()) {
        return;
    }
    ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
        "Property " + propertyPath + " expects " + expectedType + ", fallback/reset has been applied", propertyPath);
}

void ExtendedComponent::ReportDynamicStyleInvalidValue(const std::string& propertyPath, const std::string& reason) const
{
    if (propertyPath.empty()) {
        return;
    }
    ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
        "Property " + propertyPath + " " + reason + ", fallback/reset has been applied", propertyPath);
}

void ExtendedComponent::ValidateDynamicStyleEnumProperty(const JsonValue& styles,
    const std::set<std::string>& dynamicStyleKeys, const std::string& styleName,
    std::initializer_list<const char*> allowedValues) const
{
    if (!HasDynamicStyleValue(styles, dynamicStyleKeys, styleName)) {
        return;
    }

    JsonValue value = styles.GetItem(styleName.c_str());
    const std::string propertyPath = "styles." + styleName;
    if (!value.IsString()) {
        ReportDynamicStyleTypeMismatch(propertyPath, "string enum");
        return;
    }

    const std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
    for (const char* allowedValue : allowedValues) {
        if (allowedValue != nullptr && token == allowedValue) {
            return;
        }
    }
    ReportDynamicStyleInvalidValue(propertyPath, "is out of enum range");
}

void ExtendedComponent::ValidateDynamicStyleNumberProperty(const JsonValue& styles,
    const std::set<std::string>& dynamicStyleKeys, const std::string& styleName, bool requirePositive) const
{
    if (!HasDynamicStyleValue(styles, dynamicStyleKeys, styleName)) {
        return;
    }

    JsonValue value = styles.GetItem(styleName.c_str());
    float parsed = 0.0F;
    bool parsedNumber = StyleApplyUtils::ParseNumber(value, parsed);
    const std::string propertyPath = "styles." + styleName;
    if (!value.IsNumber()) {
        ReportDynamicStyleTypeMismatch(propertyPath, "number");
        return;
    }
    if (!parsedNumber || !std::isfinite(parsed) || (requirePositive && parsed <= 0.0F) ||
        (!requirePositive && parsed < 0.0F)) {
        ReportDynamicStyleInvalidValue(propertyPath, "is out of range");
    }
}

} // namespace NativeModule
