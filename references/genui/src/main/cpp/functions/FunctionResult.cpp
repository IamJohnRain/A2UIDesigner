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

#include "FunctionResult.h"

#include <cmath>
#include <sstream>

namespace NativeModule {

const std::string FunctionResult::EMPTY_STRING = "";

FunctionResult::FunctionResult()
    : type_(FunctionResultType::NULL_VALUE), boolValue_(false), intValue_(0), doubleValue_(0.0)
{}

FunctionResult::FunctionResult(bool value)
    : type_(FunctionResultType::BOOL), boolValue_(value), intValue_(0), doubleValue_(0.0)
{}

FunctionResult::FunctionResult(int32_t value)
    : type_(FunctionResultType::INT), boolValue_(false), intValue_(value), doubleValue_(0.0)
{}

FunctionResult::FunctionResult(double value)
    : type_(FunctionResultType::DOUBLE), boolValue_(false), intValue_(0), doubleValue_(value)
{}

FunctionResult::FunctionResult(const std::string& value)
    : type_(FunctionResultType::STRING), boolValue_(false), intValue_(0), doubleValue_(0.0), stringValue_(value)
{}

FunctionResult::FunctionResult(std::string&& value)
    : type_(FunctionResultType::STRING), boolValue_(false), intValue_(0), doubleValue_(0.0),
      stringValue_(std::move(value))
{}

FunctionResult::FunctionResult(const JsonValue& value)
    : type_(FunctionResultType::JSON_VALUE), boolValue_(false), intValue_(0), doubleValue_(0.0)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(value);
    if (adapter == nullptr) {
        jsonValue_ = JsonValue();
        return;
    }
    jsonOwner_ = std::shared_ptr<JsonAdapter>(std::move(adapter));
    jsonValue_ = jsonOwner_->GetRoot();
}

FunctionResultType FunctionResult::GetType() const
{
    return type_;
}

bool FunctionResult::IsNull() const
{
    return type_ == FunctionResultType::NULL_VALUE;
}

bool FunctionResult::IsBool() const
{
    return type_ == FunctionResultType::BOOL;
}

bool FunctionResult::IsInt() const
{
    return type_ == FunctionResultType::INT;
}

bool FunctionResult::IsDouble() const
{
    return type_ == FunctionResultType::DOUBLE;
}

bool FunctionResult::IsString() const
{
    return type_ == FunctionResultType::STRING;
}

bool FunctionResult::IsJsonValue() const
{
    return type_ == FunctionResultType::JSON_VALUE;
}

bool FunctionResult::GetBoolValue(bool fallback) const
{
    if (type_ == FunctionResultType::BOOL) {
        return boolValue_;
    }
    return fallback;
}

int32_t FunctionResult::GetIntValue(int32_t fallback) const
{
    if (type_ == FunctionResultType::INT) {
        return intValue_;
    }
    return fallback;
}

double FunctionResult::GetDoubleValue(double fallback) const
{
    if (type_ == FunctionResultType::DOUBLE) {
        return doubleValue_;
    }
    return fallback;
}

const std::string& FunctionResult::GetStringValue(const std::string& fallback) const
{
    if (type_ == FunctionResultType::STRING) {
        return stringValue_;
    }
    return fallback;
}

JsonValue FunctionResult::GetJsonValue() const
{
    if (type_ == FunctionResultType::JSON_VALUE) {
        return jsonValue_;
    }
    return JsonValue();
}

JsonValue FunctionResult::ToJsonValue() const
{
    switch (type_) {
        case FunctionResultType::NULL_VALUE: {
            std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNull();
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case FunctionResultType::BOOL: {
            std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateBool(boolValue_);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case FunctionResultType::INT: {
            std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(static_cast<double>(intValue_));
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case FunctionResultType::DOUBLE: {
            std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(doubleValue_);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case FunctionResultType::STRING: {
            std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(stringValue_);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case FunctionResultType::JSON_VALUE:
            return jsonValue_;
        default:
            return JsonValue();
    }
}

std::string FunctionResult::ToJsonLiteral() const
{
    switch (type_) {
        case FunctionResultType::NULL_VALUE:
            return "null";
        case FunctionResultType::BOOL:
            return boolValue_ ? "true" : "false";
        case FunctionResultType::INT:
            return std::to_string(intValue_);
        case FunctionResultType::DOUBLE: {
            std::ostringstream oss;
            double intPart = 0;
            if (std::modf(doubleValue_, &intPart) == 0.0 && !std::isinf(doubleValue_)) {
                oss << static_cast<int64_t>(doubleValue_);
            } else {
                oss << doubleValue_;
            }
            return oss.str();
        }
        case FunctionResultType::STRING:
            return JsonEscapeString(stringValue_);
        case FunctionResultType::JSON_VALUE:
            return jsonValue_.IsValid() ? jsonValue_.ToJsonLiteral() : "null";
        default:
            return "null";
    }
}

std::string FunctionResult::ToString() const
{
    switch (type_) {
        case FunctionResultType::NULL_VALUE:
            return "null";
        case FunctionResultType::BOOL:
            return boolValue_ ? "true" : "false";
        case FunctionResultType::INT:
            return std::to_string(intValue_);
        case FunctionResultType::DOUBLE: {
            std::ostringstream oss;
            oss << doubleValue_;
            return oss.str();
        }
        case FunctionResultType::STRING:
            return stringValue_;
        case FunctionResultType::JSON_VALUE:
            return jsonValue_.IsValid() ? jsonValue_.ToJsonLiteral() : "null";
        default:
            return "null";
    }
}

bool FunctionResult::Equals(const FunctionResult& other) const
{
    if (type_ != other.type_) {
        return false;
    }
    switch (type_) {
        case FunctionResultType::NULL_VALUE:
            return true;
        case FunctionResultType::BOOL:
            return boolValue_ == other.boolValue_;
        case FunctionResultType::INT:
            return intValue_ == other.intValue_;
        case FunctionResultType::DOUBLE:
            return doubleValue_ == other.doubleValue_;
        case FunctionResultType::STRING:
            return stringValue_ == other.stringValue_;
        case FunctionResultType::JSON_VALUE:
            return jsonValue_.ToJsonLiteral() == other.jsonValue_.ToJsonLiteral();
        default:
            return false;
    }
}

} // namespace NativeModule
