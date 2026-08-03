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

#ifndef A2UI_EVAL_RESULT_H
#define A2UI_EVAL_RESULT_H

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

enum class EvalValueType { STRING, NUMBER, BOOLEAN, NULL_VALUE, JSON_VALUE, UNDEFINED };

struct EvalResult {
    EvalValueType type = EvalValueType::UNDEFINED;
    std::string stringValue;
    double numberValue = 0.0;
    bool boolValue = false;
    JsonValue jsonValue;
    bool hasEvaluationError = false;

    static EvalResult Undefined()
    {
        return {};
    }

    static EvalResult FromNumber(double value)
    {
        EvalResult result;
        result.type = EvalValueType::NUMBER;
        result.numberValue = value;
        return result;
    }

    static EvalResult FromString(const std::string& value)
    {
        EvalResult result;
        result.type = EvalValueType::STRING;
        result.stringValue = value;
        return result;
    }

    static EvalResult FromString(std::string&& value)
    {
        EvalResult result;
        result.type = EvalValueType::STRING;
        result.stringValue = std::move(value);
        return result;
    }

    static EvalResult FromBool(bool value)
    {
        EvalResult result;
        result.type = EvalValueType::BOOLEAN;
        result.boolValue = value;
        return result;
    }

    static EvalResult Null()
    {
        EvalResult result;
        result.type = EvalValueType::NULL_VALUE;
        return result;
    }

    static EvalResult FromJson(const JsonValue& value)
    {
        if (!value.IsValid()) {
            return Undefined();
        }
        if (value.IsNull()) {
            return Null();
        }
        if (value.IsBool()) {
            return FromBool(value.GetBoolValue(false));
        }
        if (value.IsNumber()) {
            return FromNumber(value.GetNumberValue(0.0));
        }
        if (value.IsString()) {
            return FromString(value.GetStringValue(""));
        }

        EvalResult result;
        result.type = EvalValueType::JSON_VALUE;
        result.jsonValue = value;
        return result;
    }

    bool IsDefined() const
    {
        return type != EvalValueType::UNDEFINED;
    }

    bool IsUndefined() const
    {
        return type == EvalValueType::UNDEFINED;
    }

    bool IsString() const
    {
        return type == EvalValueType::STRING;
    }

    bool IsNumber() const
    {
        return type == EvalValueType::NUMBER;
    }

    bool IsBoolean() const
    {
        return type == EvalValueType::BOOLEAN;
    }

    bool IsNull() const
    {
        return type == EvalValueType::NULL_VALUE;
    }

    bool IsJson() const
    {
        return type == EvalValueType::JSON_VALUE;
    }

    bool IsArray() const
    {
        return IsJson() && jsonValue.IsArray();
    }

    bool IsObject() const
    {
        return IsJson() && jsonValue.IsObject();
    }

    const JsonValue& AsJson() const
    {
        return jsonValue;
    }

    std::string AsString() const
    {
        switch (type) {
            case EvalValueType::STRING:
                return stringValue;
            case EvalValueType::NUMBER:
                return NumberToString(numberValue);
            case EvalValueType::BOOLEAN:
                return boolValue ? "true" : "false";
            case EvalValueType::NULL_VALUE:
                return "null";
            case EvalValueType::JSON_VALUE:
                return jsonValue.ToString("");
            case EvalValueType::UNDEFINED:
            default:
                return "";
        }
    }

    double AsNumber() const
    {
        switch (type) {
            case EvalValueType::NUMBER:
                return numberValue;
            case EvalValueType::BOOLEAN:
                return boolValue ? 1.0 : 0.0;
            case EvalValueType::STRING:
                return StringToNumber(stringValue);
            case EvalValueType::JSON_VALUE:
                return jsonValue.ToNumber(0.0);
            case EvalValueType::NULL_VALUE:
            case EvalValueType::UNDEFINED:
            default:
                return 0.0;
        }
    }

    bool AsBool() const
    {
        switch (type) {
            case EvalValueType::BOOLEAN:
                return boolValue;
            case EvalValueType::NUMBER:
                return numberValue != 0.0 && !std::isnan(numberValue);
            case EvalValueType::STRING:
                return !stringValue.empty();
            case EvalValueType::JSON_VALUE:
                return jsonValue.ToBool(false);
            case EvalValueType::NULL_VALUE:
            case EvalValueType::UNDEFINED:
            default:
                return false;
        }
    }

private:
    static std::string NumberToString(double value)
    {
        if (!std::isfinite(value)) {
            return "";
        }
        double intPart = 0.0;
        if (std::modf(value, &intPart) == 0.0 && std::abs(value) < 1e15) {
            std::ostringstream oss;
            oss.precision(0);
            oss << static_cast<long long>(value);
            return oss.str();
        }
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    static double StringToNumber(const std::string& value)
    {
        if (value.empty()) {
            return 0.0;
        }
        char* end = nullptr;
        errno = 0;
        double number = std::strtod(value.c_str(), &end);
        if (end == value.c_str()) {
            return 0.0;
        }
        while (*end != '\0') {
            if (std::isspace(static_cast<unsigned char>(*end)) == 0) {
                return 0.0;
            }
            ++end;
        }
        if (errno == ERANGE || !std::isfinite(number)) {
            return 0.0;
        }
        return number;
    }
};

} // namespace NativeModule

#endif // A2UI_EVAL_RESULT_H
