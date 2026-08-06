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

#include <cmath>

#include "components/TypeValidation.h"
#include "components/custom/CustomComponent.h"

namespace NativeModule {

void CustomComponent::NormalizeExtendedTabsProperty(const std::string& propertyName, JsonValue& value)
{
    auto report = [this](const std::string& code, const std::string& message, const std::string& path) {
        ReportCustomSchemaWarning(code, message, path);
    };

    if (propertyName == "barPosition") {
        if (IsDynamicValue(value)) {
            return;
        }
        if (!value.IsString()) {
            ReportTypeMismatchAndReset(report, value, "string", "barPosition");
            return;
        }
        std::string token = StyleApplyUtils::TrimToken(value.GetStringValue(""));
        if (token == "start" || token == "end") {
            return;
        }
        ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property barPosition has invalid value and has been reset to default", "barPosition");
        value = JsonValue();
        return;
    }

    if (propertyName == "vertical" || propertyName == "scrollable") {
        if (IsDynamicValue(value)) {
            return;
        }
        if (value.IsBool()) {
            return;
        }
        ReportTypeMismatchAndReset(report, value, "boolean", propertyName);
        return;
    }

    if (propertyName == "tabIndex") {
        if (IsDynamicValue(value)) {
            return;
        }
        if (IsLiteralNumber(value)) {
            double tabIndex = value.GetNumberValue(-1.0);
            if (tabIndex >= 0.0 && std::floor(tabIndex) == tabIndex) {
                return;
            }
            ReportCustomSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property tabIndex must be a non-negative integer and has been reset to default", "tabIndex");
            value = JsonValue();
            return;
        }
        ReportTypeMismatchAndReset(report, value, "number", "tabIndex");
    }
}

} // namespace NativeModule
