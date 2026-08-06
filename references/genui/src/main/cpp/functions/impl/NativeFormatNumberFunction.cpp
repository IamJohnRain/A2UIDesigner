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

#include "NativeFormatNumberFunction.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

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

std::string NativeFormatNumberFunction::GetName() const
{
    return "formatNumber";
}

FunctionResult NativeFormatNumberFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    if (!valueArg.IsNumber()) {
        return FunctionResult(std::string(""));
    }

    double value = valueArg.GetNumberValue(std::numeric_limits<double>::quiet_NaN());
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
        formatted = FormatWithGrouping(value, decimals);
    } else {
        formatted = FormatDecimal(value, decimals);
    }

    return FunctionResult(std::move(formatted));
}

std::string NativeFormatNumberFunction::FormatDecimal(double value, int decimals)
{
    double factor = std::pow(10.0, decimals);
    double rounded = std::round(value * factor) / factor;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << rounded;
    return oss.str();
}

std::string NativeFormatNumberFunction::FormatWithGrouping(double value, int decimals)
{
    std::string formatted = FormatDecimal(std::fabs(value), decimals);

    std::string intPart;
    std::string decPart;
    size_t dotPos = formatted.find('.');
    if (dotPos != std::string::npos) {
        intPart = formatted.substr(0, dotPos);
        decPart = formatted.substr(dotPos);
    } else {
        intPart = formatted;
    }

    std::string grouped;
    int count = 0;
    for (int i = static_cast<int>(intPart.size()) - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) {
            grouped = ',' + grouped;
        }
        grouped = intPart[static_cast<size_t>(i)] + grouped;
        ++count;
    }

    std::string result = grouped + decPart;
    if (value < 0) {
        result = "-" + result;
    }
    return result;
}

} // namespace NativeModule
