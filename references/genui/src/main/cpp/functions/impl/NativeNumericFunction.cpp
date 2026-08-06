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

#include "NativeNumericFunction.h"

#include <cmath>
#include <limits>

namespace NativeModule {

std::string NativeNumericFunction::GetName() const
{
    return "numeric";
}

FunctionResult NativeNumericFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(false);
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsNumber()) {
        return FunctionResult(false);
    }

    double value = valueArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(value)) {
        return FunctionResult(false);
    }

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
        double min = minArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(min)) {
            return FunctionResult(false);
        }
        if (value < min) {
            return FunctionResult(false);
        }
    }

    if (hasMax) {
        if (!maxArg.IsNumber()) {
            return FunctionResult(false);
        }
        double max = maxArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(max)) {
            return FunctionResult(false);
        }
        if (value > max) {
            return FunctionResult(false);
        }
    }

    return FunctionResult(true);
}

} // namespace NativeModule
