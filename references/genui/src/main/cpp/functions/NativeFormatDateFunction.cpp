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

    int year = 0, month = 1, day = 1, hour = 0, minute = 0, second = 0;
    if (!ParseISO8601(value, year, month, day, hour, minute, second)) {
        return FunctionResult(std::string(""));
    }

    std::string result = ApplyPattern(year, month, day, hour, minute, second, format);
    return FunctionResult(std::move(result));
}

bool NativeFormatDateFunction::ParseISO8601(
    const std::string& iso, int& year, int& month, int& day, int& hour, int& minute, int& second)
{
    if (iso.size() < 10) {
        return false;
    }

    if (iso[4] != '-' || iso[7] != '-') {
        return false;
    }

    try {
        year = std::stoi(iso.substr(0, 4));
        month = std::stoi(iso.substr(5, 2));
        day = std::stoi(iso.substr(8, 2));
    } catch (...) {
        LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid date format: %{public}s", iso.c_str());
        return false;
    }

    if (iso.size() > 10 && iso[10] == 'T') {
        if (iso.size() >= 19) {
            try {
                hour = std::stoi(iso.substr(11, 2));
                minute = std::stoi(iso.substr(14, 2));
                second = std::stoi(iso.substr(17, 2));
            } catch (...) {
                LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid time format: %{public}s", iso.c_str());
                return false;
            }
        } else if (iso.size() >= 16) {
            try {
                hour = std::stoi(iso.substr(11, 2));
                minute = std::stoi(iso.substr(14, 2));
            } catch (...) {
                LOG_A2UI(LOG_ERROR, "ParseISO8601: invalid hour/minute: %{public}s", iso.c_str());
                return false;
            }
        }
    }

    return true;
}

std::string NativeFormatDateFunction::ApplyPattern(
    int year, int month, int day, int hour, int minute, int second, const std::string& pattern)
{
    std::ostringstream result;
    int dow = DayOfWeek(year, month, day);
    size_t i = 0;

    while (i < pattern.size()) {
        char c = pattern[i];
        int runLen = 1;
        while (i + runLen < pattern.size() && pattern[i + runLen] == c) {
            ++runLen;
        }

        switch (c) {
            case 'y':
                if (runLen >= 3) {
                    result << PadInt(year, 4);
                } else {
                    result << PadInt(year % 100, 2);
                }
                break;
            case 'M':
                if (runLen >= 4) {
                    result << (month >= 1 && month <= 12 ? MONTH_NAMES[month - 1] : "");
                } else if (runLen == 3) {
                    result << (month >= 1 && month <= 12 ? MONTH_ABBREVS[month - 1] : "");
                } else if (runLen == 2) {
                    result << PadInt(month, 2);
                } else {
                    result << month;
                }
                break;
            case 'd':
                if (runLen >= 2) {
                    result << PadInt(day, 2);
                } else {
                    result << day;
                }
                break;
            case 'E':
                if (runLen >= 4) {
                    result << DAY_NAMES[dow];
                } else {
                    result << DAY_ABBREVS[dow];
                }
                break;
            case 'H':
                if (runLen >= 2) {
                    result << PadInt(hour, 2);
                } else {
                    result << hour;
                }
                break;
            case 'h': {
                int h12 = hour % 12;
                if (h12 == 0) {
                    h12 = 12;
                }
                if (runLen >= 2) {
                    result << PadInt(h12, 2);
                } else {
                    result << h12;
                }
                break;
            }
            case 'm':
                if (runLen >= 2) {
                    result << PadInt(minute, 2);
                } else {
                    result << minute;
                }
                break;
            case 's':
                if (runLen >= 2) {
                    result << PadInt(second, 2);
                } else {
                    result << second;
                }
                break;
            case 'a':
                result << (hour < 12 ? "AM" : "PM");
                break;
            default:
                for (int r = 0; r < runLen; ++r) {
                    result << c;
                }
                break;
        }

        i += runLen;
    }

    return result.str();
}

} // namespace NativeModule
