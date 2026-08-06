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

#include "NativeFormatCurrencyFunction.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include "NativeFormatNumberFunction.h"

namespace NativeModule {

namespace {

constexpr double MAX_DECIMALS_VALUE = 20.0;

bool ParseDecimals(const JsonValue& decimalsArg, int& decimals)
{
    if (!decimalsArg.IsValid()) {
        return true;
    }
    if (!decimalsArg.IsNumber()) {
        return false;
    }

    double parsedDecimals = decimalsArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
    if (!std::isfinite(parsedDecimals) || std::fabs(parsedDecimals) > MAX_DECIMALS_VALUE) {
        return false;
    }
    decimals = static_cast<int>(std::fabs(parsedDecimals));
    return true;
}

} // namespace

std::string NativeFormatCurrencyFunction::GetName() const
{
    return "formatCurrency";
}

FunctionResult NativeFormatCurrencyFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    JsonValue currencyArg = resolvedArgs.GetItem("currency");
    if (!valueArg.IsNumber() || !currencyArg.IsString()) {
        return FunctionResult(std::string(""));
    }

    double value = valueArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
    std::string currency = currencyArg.GetStringValue("");
    if (currency.empty()) {
        return FunctionResult(std::string(""));
    }

    int decimals = 2;
    JsonValue decimalsArg = resolvedArgs.GetItem("decimals");
    if (!ParseDecimals(decimalsArg, decimals)) {
        return FunctionResult(std::string(""));
    }

    bool grouping = false;
    JsonValue groupingArg = resolvedArgs.GetItem("grouping");
    if (groupingArg.IsValid()) {
        if (!groupingArg.IsBool()) {
            return FunctionResult(std::string(""));
        }
        grouping = groupingArg.GetBoolValue(false);
    }

    std::string formatted;
    if (grouping) {
        formatted = NativeFormatNumberFunction::FormatWithGrouping(value, decimals);
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(decimals) << value;
        formatted = oss.str();
    }

    std::string result = currency + " " + formatted;
    return FunctionResult(std::move(result));
}

} // namespace NativeModule
