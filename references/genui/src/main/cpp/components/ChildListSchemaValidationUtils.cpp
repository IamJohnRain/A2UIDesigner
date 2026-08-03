#include "components/ChildListSchemaValidationUtils.h"

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
        if (emptyArrayPolicy == ChildListEmptyArrayPolicy::WARN_INVALID_VALUE && childrenValue.GetArraySize() == 0) {
            AppendIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + propertyName + " cannot be an empty array", propertyName);
            return issues;
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
                AppendIssue(
                    issues, SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property " + childPath + " is required", childPath);
            }
        }
        return issues;
    }

    const RequiredStringPropertyState componentIdState = GetRequiredStringPropertyState(childrenValue, "componentId");
    const std::string componentIdPath = BuildPropertyPath(propertyName, "componentId");
    if (componentIdState == RequiredStringPropertyState::TYPE_MISMATCH) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + componentIdPath + " expects string value for template object", componentIdPath);
    } else if (componentIdState != RequiredStringPropertyState::VALID) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property " + componentIdPath + " is required for template object", componentIdPath);
    }

    const RequiredStringPropertyState pathState = GetRequiredStringPropertyState(childrenValue, "path");
    const std::string pathPropertyPath = BuildPropertyPath(propertyName, "path");
    if (pathState == RequiredStringPropertyState::TYPE_MISMATCH) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + pathPropertyPath + " expects string value for template object", pathPropertyPath);
    } else if (pathState != RequiredStringPropertyState::VALID) {
        AppendIssue(issues, SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property " + pathPropertyPath + " is required for template object", pathPropertyPath);
    }
    return issues;
}

} // namespace NativeModule
