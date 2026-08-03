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

#include "NativeRegexFunction.h"

#include <regex>

#include "utils/LogA2UI.h"

namespace NativeModule {

std::string NativeRegexFunction::GetName() const
{
    return "regex";
}

FunctionResult NativeRegexFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(false);
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    JsonValue patternArg = resolvedArgs.GetItem("pattern");
    if (!valueArg.IsString() || !patternArg.IsString()) {
        return FunctionResult(false);
    }

    std::string value = valueArg.GetStringValue("");
    std::string pattern = patternArg.GetStringValue("");
    if (pattern.empty()) {
        return FunctionResult(false);
    }

    try {
        std::regex re(pattern);
        bool matched = std::regex_match(value, re);
        return FunctionResult(matched);
    } catch (const std::regex_error& e) {
        LOG_A2UI(LOG_ERROR, "NativeRegexFunction::Execute: invalid regex pattern: %{public}s", e.what());
        return FunctionResult(false);
    }
}

} // namespace NativeModule
