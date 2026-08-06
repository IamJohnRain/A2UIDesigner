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

#include "NativeEmailFunction.h"

#include <regex>

namespace NativeModule {

namespace {

constexpr char EMAIL_PATTERN[] = R"(^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@)"
                                 R"([a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)"
                                 R"((?:\.[a-zA-Z](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$)";

} // namespace

std::string NativeEmailFunction::GetName() const
{
    return "email";
}

FunctionResult NativeEmailFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(false);
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsString()) {
        return FunctionResult(false);
    }

    std::string value = valueArg.GetStringValue("");
    if (value.empty()) {
        return FunctionResult(false);
    }

    static const std::regex emailPattern(EMAIL_PATTERN);

    bool matched = std::regex_match(value, emailPattern);
    return FunctionResult(matched);
}

} // namespace NativeModule
