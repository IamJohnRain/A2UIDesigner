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

#ifndef A2UI_COMPONENTS_TYPE_VALIDATION_H
#define A2UI_COMPONENTS_TYPE_VALIDATION_H

#include <cmath>
#include <string>

#include "components/custom/CustomComponentExpressionBinding.h"
#include "styles/StyleApplyUtils.h"
#include "utils/JsonAdapter.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

// ---------------------------------------------------------------------------
// Dynamic / deferred value detection
// ---------------------------------------------------------------------------

/**
 * Returns true when @p value is a dynamic descriptor object carrying a
 * "path" (data-binding) or "call" (function-call) key.
 */
inline bool IsDynamicDescriptorObject(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

/**
 * Returns true when @p value should be resolved at runtime rather than
 * validated statically.  Covers both dynamic descriptor objects and
 * expression strings (the {{ ... }} syntax).
 */
inline bool IsDynamicValue(const JsonValue& value)
{
    return IsDynamicDescriptorObject(value) || IsExpressionStringValue(value);
}

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------

/**
 * Returns true when @p value is a string that is empty or contains only
 * whitespace after trimming.
 */
inline bool IsEmptyStringValue(const JsonValue& value)
{
    return value.IsString() && StyleApplyUtils::TrimToken(value.GetStringValue("")).empty();
}

/**
 * Returns true when @p value is a non-empty string (trimmed) and is NOT
 * a dynamic expression or descriptor — i.e. a plain literal string that
 * carries meaningful content.
 */
inline bool IsNonEmptyLiteralString(const JsonValue& value)
{
    return value.IsString() && !IsEmptyStringValue(value);
}

// ---------------------------------------------------------------------------
// Strict literal type checks
// ---------------------------------------------------------------------------

/**
 * Returns true when @p value is a boolean literal.
 */
inline bool IsLiteralBool(const JsonValue& value)
{
    return value.IsBool();
}

/**
 * Returns true when @p value is a finite number literal.
 */
inline bool IsLiteralNumber(const JsonValue& value)
{
    return value.IsNumber() && std::isfinite(value.GetNumberValue(0.0));
}

// ---------------------------------------------------------------------------
// Color validation
// ---------------------------------------------------------------------------

/**
 * Returns true when @p value is a valid color string.
 * Accepts hex colours (#RRGGBB, #AARRGGBB) and the transparent keyword.
 */
inline bool IsValidColorValue(const JsonValue& value)
{
    if (!value.IsString()) {
        return false;
    }
    if (StyleApplyUtils::TrimToken(value.GetStringValue("")) == "transparent") {
        return true;
    }
    uint32_t color = 0;
    return StyleApplyUtils::ParseColor(value, color);
}

// ---------------------------------------------------------------------------
// Type-mismatch warning helpers
// ---------------------------------------------------------------------------

/**
 * Builds a standardised type-mismatch message.
 *
 * Example output:
 *   "Property scrollable expects boolean value, got type 'string',
 *    fallback/reset has been applied"
 */
inline std::string BuildTypeMismatchMessage(
    const std::string& propertyName, const std::string& expectedType, const JsonValue& value)
{
    return "Property " + propertyName + " expects " + expectedType + " value, got type '" +
           std::string(value.GetTypeName()) + "', fallback/reset has been applied";
}

/**
 * Reports a TYPE_MISMATCH schema warning for @p value and resets it.
 *
 * @p report  A callable with the same signature as
 *            CustomComponent::ReportCustomSchemaWarning(code, message, path).
 *
 * After the call @p value is set to a default-constructed (empty) JsonValue.
 */
template<typename ReportFn>
inline void ReportTypeMismatchAndReset(
    ReportFn&& report, JsonValue& value, const std::string& expectedType, const std::string& propertyName)
{
    report(SCHEMA_ERROR_CODE_TYPE_MISMATCH, BuildTypeMismatchMessage(propertyName, expectedType, value), propertyName);
    value = JsonValue();
}

/**
 * Same as above, but the reset action is customised via @p reset
 * (e.g. removing a key from a parent object instead of replacing the value).
 */
template<typename ReportFn, typename ResetFn>
inline void ReportTypeMismatchAndReset(ReportFn&& report, ResetFn&& reset, const JsonValue& value,
    const std::string& expectedType, const std::string& propertyName)
{
    report(SCHEMA_ERROR_CODE_TYPE_MISMATCH, BuildTypeMismatchMessage(propertyName, expectedType, value), propertyName);
    reset();
}

} // namespace NativeModule

#endif // A2UI_COMPONENTS_TYPE_VALIDATION_H
