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

#include "NativeFormatDateFunction.h"

#include <optional>
#include <sstream>
#include <vector>

#include "utils/LogA2UI.h"

namespace NativeModule {

static const char* MONTH_NAMES[] = { "January", "February", "March", "April", "May", "June", "July", "August",
    "September", "October", "November", "December" };

static const char* MONTH_ABBREVS[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov",
    "Dec" };

static const char* DAY_NAMES[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

static const char* DAY_ABBREVS[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static int DayOfWeek(int year, int month, int day)
{
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (h + 6) % 7;
}

static std::string PadInt(int value, int width)
{
    std::string s = std::to_string(value < 0 ? -value : value);
    while (static_cast<int>(s.size()) < width) {
        s = "0" + s;
    }
    return s;
}

using DateTimeParts = NativeFormatDateFunction::DateTimeParts;
using PatternTokenFormatter = void (*)(std::ostringstream&, const DateTimeParts&, size_t);

struct PatternTokenHandler {
    char token;
    PatternTokenFormatter formatter;
};

static void AppendYear(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    result << (runLength >= 3 ? PadInt(parts.year, 4) : PadInt(parts.year % 100, 2));
}

static void AppendMonth(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    bool isValidMonth = parts.month >= 1 && parts.month <= 12;
    if (runLength >= 4) {
        result << (isValidMonth ? MONTH_NAMES[parts.month - 1] : "");
    } else if (runLength == 3) {
        result << (isValidMonth ? MONTH_ABBREVS[parts.month - 1] : "");
    } else if (runLength == 2) {
        result << PadInt(parts.month, 2);
    } else {
        result << parts.month;
    }
}

static void AppendDay(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    if (runLength >= 2) {
        result << PadInt(parts.day, 2);
    } else {
        result << parts.day;
    }
}

static void AppendWeekday(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    int dayOfWeek = DayOfWeek(parts.year, parts.month, parts.day);
    result << (runLength >= 4 ? DAY_NAMES[dayOfWeek] : DAY_ABBREVS[dayOfWeek]);
}

static void AppendHour24(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    if (runLength >= 2) {
        result << PadInt(parts.hour, 2);
    } else {
        result << parts.hour;
    }
}

static void AppendHour12(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    int hour = parts.hour % 12;
    if (hour == 0) {
        hour = 12;
    }
    if (runLength >= 2) {
        result << PadInt(hour, 2);
    } else {
        result << hour;
    }
}

static void AppendMinute(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    if (runLength >= 2) {
        result << PadInt(parts.minute, 2);
    } else {
        result << parts.minute;
    }
}

static void AppendSecond(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    if (runLength >= 2) {
        result << PadInt(parts.second, 2);
    } else {
        result << parts.second;
    }
}

static void AppendMeridiem(std::ostringstream& result, const DateTimeParts& parts, size_t runLength)
{
    static_cast<void>(runLength);
    result << (parts.hour < 12 ? "AM" : "PM");
}

static const PatternTokenHandler PATTERN_TOKEN_HANDLERS[] = { { 'y', AppendYear }, { 'M', AppendMonth },
    { 'd', AppendDay }, { 'E', AppendWeekday }, { 'H', AppendHour24 }, { 'h', AppendHour12 }, { 'm', AppendMinute },
    { 's', AppendSecond }, { 'a', AppendMeridiem } };

static std::optional<PatternTokenFormatter> FindPatternTokenFormatter(char token)
{
    for (const auto& handler : PATTERN_TOKEN_HANDLERS) {
        if (handler.token == token) {
            return handler.formatter;
        }
    }
    return std::nullopt;
}

static void AppendLiteral(std::ostringstream& result, char token, size_t runLength)
{
    for (size_t index = 0; index < runLength; ++index) {
        result << token;
    }
}

std::string NativeFormatDateFunction::GetName() const
{
    return "formatDate";
}

FunctionResult NativeFormatDateFunction::Execute(const JsonValue& resolvedArgs)
{
    if (!resolvedArgs.IsObject()) {
        return FunctionResult(std::string(""));
    }

    JsonValue valueArg = resolvedArgs.GetItem("value");
    JsonValue formatArg = resolvedArgs.GetItem("format");
    if (!valueArg.IsString() || !formatArg.IsString()) {
        return FunctionResult(std::string(""));
    }

    std::string value = valueArg.GetStringValue("");
    std::string format = formatArg.GetStringValue("");
    if (value.empty() || format.empty()) {
        return FunctionResult(std::string(""));
    }

    DateTimeParts parts;
    if (!ParseISO8601(value, parts)) {
        return FunctionResult(std::string(""));
    }

    std::string result = ApplyPattern(parts, format);
    return FunctionResult(std::move(result));
}

bool NativeFormatDateFunction::ParseISO8601(const std::string& iso, DateTimeParts& parts)
{
    if (iso.size() < 10) {
        return false;
    }

    if (iso[4] != '-' || iso[7] != '-') {
        return false;
    }

    try {
        parts.year = std::stoi(iso.substr(0, 4));
        parts.month = std::stoi(iso.substr(5, 2));
        parts.day = std::stoi(iso.substr(8, 2));
    } catch (...) {
        LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid date format: %{public}s", iso.c_str());
        return false;
    }

    if (iso.size() > 10 && iso[10] == 'T') {
        if (iso.size() >= 19) {
            try {
                parts.hour = std::stoi(iso.substr(11, 2));
                parts.minute = std::stoi(iso.substr(14, 2));
                parts.second = std::stoi(iso.substr(17, 2));
            } catch (...) {
                LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid time format: %{public}s", iso.c_str());
                return false;
            }
        } else if (iso.size() >= 16) {
            try {
                parts.hour = std::stoi(iso.substr(11, 2));
                parts.minute = std::stoi(iso.substr(14, 2));
            } catch (...) {
                LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid hour/minute: %{public}s", iso.c_str());
                return false;
            }
        }
    }

    return true;
}

std::string NativeFormatDateFunction::ApplyPattern(const DateTimeParts& parts, const std::string& pattern)
{
    std::ostringstream result;
    size_t index = 0;
    while (index < pattern.size()) {
        char token = pattern[index];
        size_t runLength = 1;
        while (index + runLength < pattern.size() && pattern[index + runLength] == token) {
            ++runLength;
        }
        std::optional<PatternTokenFormatter> formatter = FindPatternTokenFormatter(token);
        if (formatter.has_value()) {
            formatter.value()(result, parts, runLength);
        } else {
            AppendLiteral(result, token, runLength);
        }
        index += runLength;
    }

    return result.str();
}

} // namespace NativeModule
