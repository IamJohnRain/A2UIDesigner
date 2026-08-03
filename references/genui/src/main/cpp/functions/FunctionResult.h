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

#ifndef A2UI_FUNCTION_RESULT_H
#define A2UI_FUNCTION_RESULT_H

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "../utils/JsonAdapter.h"

namespace NativeModule {

enum class FunctionResultType { NULL_VALUE = 0, BOOL, INT, DOUBLE, STRING, JSON_VALUE };

class FunctionResult {
public:
    FunctionResult();

    explicit FunctionResult(bool value);

    explicit FunctionResult(int32_t value);

    explicit FunctionResult(double value);

    explicit FunctionResult(const std::string& value);

    explicit FunctionResult(std::string&& value);

    explicit FunctionResult(const JsonValue& value);

    ~FunctionResult() = default;
    FunctionResult(const FunctionResult&) = default;
    FunctionResult(FunctionResult&&) = default;
    FunctionResult& operator=(const FunctionResult&) = default;
    FunctionResult& operator=(FunctionResult&&) = default;

    FunctionResultType GetType() const;

    bool IsNull() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsString() const;
    bool IsJsonValue() const;
    bool GetBoolValue(bool fallback = false) const;
    int32_t GetIntValue(int32_t fallback = 0) const;
    double GetDoubleValue(double fallback = 0.0) const;
    const std::string& GetStringValue(const std::string& fallback = EMPTY_STRING) const;
    JsonValue GetJsonValue() const;

    JsonValue ToJsonValue() const;

    std::string ToJsonLiteral() const;

    std::string ToString() const;

    bool Equals(const FunctionResult& other) const;

private:
    static const std::string EMPTY_STRING;

    static std::string JsonEscapeString(const std::string& s)
    {
        std::ostringstream oss;
        oss << '"';
        for (char c : s) {
            if (c == '"') {
                oss << '\\';
            }
            oss << c;
        }
        oss << '"';
        return oss.str();
    }

    FunctionResultType type_;
    bool boolValue_ = false;
    int32_t intValue_ = 0;
    double doubleValue_ = 0.0;
    std::string stringValue_;
    JsonValue jsonValue_;
    std::shared_ptr<JsonAdapter> jsonOwner_;
};

} // namespace NativeModule

#endif // A2UI_FUNCTION_RESULT_H
