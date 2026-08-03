#include <cmath>

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

} // namespace

void CustomComponent::NormalizeExtendedTabsProperty(const std::string& propertyName, JsonValue& value)
{
    if (propertyName == "barPosition") {
        if (IsDynamicDescriptorObject(value) || IsExpressionStringValue(value)) {
            return;
        }
        if (!value.IsString()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property barPosition expects string value, got type '" + std::string(value.GetTypeName()) +
                    "', fallback/reset has been applied",
                "barPosition");
            value = JsonValue();
            return;
        }
        std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
        if (token == "start" || token == "end" || token == "right" || token == "bottom") {
            return;
        }
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property barPosition has invalid value and has been reset to default", "barPosition");
        value = JsonValue();
        return;
    }

    if (propertyName == "vertical" || propertyName == "scrollable") {
        if (IsDynamicDescriptorObject(value) || IsExpressionStringValue(value)) {
            return;
        }
        if (ParseBooleanLikeValue(value)) {
            return;
        }
        if (value.IsString()) {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + propertyName + " has invalid value and has been reset to default", propertyName);
        } else {
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + propertyName + " expects boolean value, got type '" + std::string(value.GetTypeName()) +
                    "', fallback/reset has been applied",
                propertyName);
        }
        value = JsonValue();
        return;
    }

    if (propertyName == "tabIndex") {
        if (IsDynamicDescriptorObject(value) || IsExpressionStringValue(value)) {
            return;
        }
        float parsedNumber = 0.0F;
        if (StyleApplyUtils::ParseNumber(value, parsedNumber) && std::isfinite(parsedNumber)) {
            return;
        }
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property tabIndex expects number value, got type '" + std::string(value.GetTypeName()) +
                "', fallback/reset has been applied",
            "tabIndex");
        value = JsonValue();
    }
}

} // namespace NativeModule
