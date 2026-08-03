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

#include "NativeLengthFunction.h"

#include <cmath>
#include <cstdint>
#include <limits>

namespace NativeModule {

std::string NativeLengthFunction::GetName() const
{
    return "length";
}

FunctionResult NativeLengthFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(false);
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsString()) {
        return FunctionResult(false);
    }

    std::string value = valueArg.GetStringValue("");
    int32_t len = static_cast<int32_t>(value.size());

    JsonValue minArg = resolvedArgs.GetItem("min");
    JsonValue maxArg = resolvedArgs.GetItem("max");
    bool hasMin = minArg.IsValid();
    bool hasMax = maxArg.IsValid();

    if (!hasMin && !hasMax) {
        return FunctionResult(false);
    }

    if (hasMin) {
        if (!minArg.IsNumber()) {
            return FunctionResult(false);
        }
        double minNumber = minArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(minNumber) || minNumber < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
            minNumber > static_cast<double>(std::numeric_limits<int32_t>::max())) {
            return FunctionResult(false);
        }
        int32_t min = static_cast<int32_t>(minNumber);
        if (len < min) {
            return FunctionResult(false);
        }
    }

    if (hasMax) {
        if (!maxArg.IsNumber()) {
            return FunctionResult(false);
        }
        double maxNumber = maxArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(maxNumber) || maxNumber < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
            maxNumber > static_cast<double>(std::numeric_limits<int32_t>::max())) {
            return FunctionResult(false);
        }
        int32_t max = static_cast<int32_t>(maxNumber);
        if (len > max) {
            return FunctionResult(false);
        }
    }

    return FunctionResult(true);
}

} // namespace NativeModule
