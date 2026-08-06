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

#include "components/ChildListSchemaValidationUtils.h"

#include "utils/LocalVariableNameUtils.h"
#include "utils/RequiredStringPropertyUtils.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

std::string BuildPropertyPath(const std::string& propertyName, const std::string& suffix)
{
    if (propertyName.empty()) {
        return suffix;
    }
    if (suffix.empty()) {
        return propertyName;
    }
    return propertyName + "." + suffix;
}

void AppendIssue(std::vector<SchemaValidationIssue>& issues, const std::string& code, const std::string& message,
    const std::string& propertyPath)
{
    issues.push_back(SchemaValidationIssue { .code = code, .message = message, .propertyPath = propertyPath });
}

void ValidateTemplateLocalVariable(const JsonValue& childrenValue, const std::string& propertyName,
    const std::string& variableName, const std::string& fallbackName, std::vector<SchemaValidationIssue>& issues)
{
    if (!childrenValue.Has(variableName.c_str())) {
        return;
    }

    const std::string propertyPath = BuildPropertyPath(propertyName, variableName);
    JsonValue value = childrenValue.GetItem(variableName.c_str());
    if (!value.IsString()) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyPath + " expects string local variable name, fallback to $" + fallbackName,
            propertyPath);
        return;
    }

    if (!IsValidLocalVariableName(value.GetStringValue(""))) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + propertyPath + " has invalid local variable name, fallback to $" + fallbackName,
            propertyPath);
    }
}

void ValidateStaticChildList(const JsonValue& childrenValue, const std::string& propertyName,
    ChildListEmptyArrayPolicy emptyArrayPolicy, std::vector<SchemaValidationIssue>& issues)
{
    if (emptyArrayPolicy == ChildListEmptyArrayPolicy::WARN_INVALID_VALUE && childrenValue.GetArraySize() == 0) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "Property " + propertyName + " cannot be an empty array",
            propertyName);
        return;
    }

    const int childCount = childrenValue.GetArraySize();
    for (int index = 0; index < childCount; ++index) {
        JsonValue childItemValue = childrenValue.GetArrayItem(index);
        const std::string childPath = propertyName + "[" + std::to_string(index) + "]";
        if (!childItemValue.IsString()) {
            AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + childPath + " expects string child id, got type '" +
                    std::string(childItemValue.GetTypeName()) + "'",
                childPath);
            continue;
        }
        if (childItemValue.GetStringValue("").empty()) {
            AppendIssue(issues, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property " + childPath + " is required", childPath);
        }
    }
}

void ValidateRequiredTemplateStringProperty(const JsonValue& childrenValue, const std::string& propertyName,
    const std::string& requiredPropertyName, std::vector<SchemaValidationIssue>& issues)
{
    const RequiredStringPropertyState state =
        GetRequiredStringPropertyState(childrenValue, requiredPropertyName.c_str());
    const std::string propertyPath = BuildPropertyPath(propertyName, requiredPropertyName);
    if (state == RequiredStringPropertyState::TYPE_MISMATCH) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyPath + " expects string value for template object", propertyPath);
    } else if (state != RequiredStringPropertyState::VALID) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property " + propertyPath + " is required for template object", propertyPath);
    }
}

void ValidateTemplateLocalVariableConflict(
    const JsonValue& childrenValue, const std::string& propertyName, std::vector<SchemaValidationIssue>& issues)
{
    JsonValue indexValue = childrenValue.GetItem("indexVar");
    JsonValue itemValue = childrenValue.GetItem("itemVar");
    if (!indexValue.IsString() || !itemValue.IsString()) {
        return;
    }

    const std::string indexVar = indexValue.GetStringValue("");
    const std::string itemVar = itemValue.GetStringValue("");
    if (!IsValidLocalVariableName(indexVar) || !IsValidLocalVariableName(itemVar) || indexVar != itemVar) {
        return;
    }

    AppendIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE,
        "Properties " + BuildPropertyPath(propertyName, "indexVar") + " and " +
            BuildPropertyPath(propertyName, "itemVar") +
            " must use different local variable names, "
            "fallback to $index and $item",
        propertyName);
}

} // namespace

std::vector<SchemaValidationIssue> ValidateChildListSchema(
    const JsonValue& descriptor, const std::string& propertyName, ChildListEmptyArrayPolicy emptyArrayPolicy)
{
    std::vector<SchemaValidationIssue> issues;
    if (!descriptor.IsObject() || propertyName.empty() || !descriptor.Has(propertyName.c_str())) {
        return issues;
    }

    JsonValue childrenValue = descriptor.GetItem(propertyName.c_str());
    if (!childrenValue.IsArray() && !childrenValue.IsObject()) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyName + " expects array or template object, got type '" +
                std::string(childrenValue.GetTypeName()) + "', field has been ignored",
            propertyName);
        return issues;
    }

    if (childrenValue.IsArray()) {
        ValidateStaticChildList(childrenValue, propertyName, emptyArrayPolicy, issues);
        return issues;
    }

    ValidateRequiredTemplateStringProperty(childrenValue, propertyName, "componentId", issues);
    ValidateRequiredTemplateStringProperty(childrenValue, propertyName, "path", issues);
    ValidateTemplateLocalVariable(childrenValue, propertyName, "indexVar", "index", issues);
    ValidateTemplateLocalVariable(childrenValue, propertyName, "itemVar", "item", issues);
    ValidateTemplateLocalVariableConflict(childrenValue, propertyName, issues);
    return issues;
}

} // namespace NativeModule
