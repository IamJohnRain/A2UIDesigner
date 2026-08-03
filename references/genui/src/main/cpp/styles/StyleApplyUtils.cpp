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

#include "StyleApplyUtilsInternal.h"

namespace NativeModule {

std::string StyleApplyUtils::TrimToken(const std::string& value)
{
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    auto end =
        std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

bool StyleApplyUtils::ParseColor(const JsonValue& value, uint32_t& color)
{
    if (value.IsNumber()) {
        color = value.GetUint32Value(0);
        return true;
    }
    if (!value.IsString()) {
        return false;
    }

    return ParseHexColorString(value.GetStringValue(""), color);
}

bool StyleApplyUtils::ParseHexColorString(const std::string& value, uint32_t& color)
{
    std::string hex = StyleApplyUtilsInternal::NormalizeHexColor(TrimToken(value));
    if (hex.empty()) {
        return false;
    }

    for (char character : hex) {
        if (std::isxdigit(static_cast<unsigned char>(character)) == 0) {
            return false;
        }
    }

    char* end = nullptr;
    unsigned long parsed = std::strtoul(hex.c_str(), &end, 16);
    if (end == nullptr || *end != '\0') {
        return false;
    }
    color = static_cast<uint32_t>(parsed);
    return true;
}

bool StyleApplyUtils::ParseNumber(const JsonValue& value, float& number)
{
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsNumber()) {
        number = static_cast<float>(value.GetNumberValue(0.0));
        return true;
    }
    if (!value.IsString()) {
        return false;
    }
    return StyleApplyUtilsInternal::ParseFloatToken(TrimToken(value.GetStringValue("")), number);
}

bool StyleApplyUtils::IsExpressionString(const std::string& value)
{
    std::string token = TrimToken(value);
    return token.size() >= 4 && token.rfind("{{", 0) == 0 && token.compare(token.size() - 2, 2, "}}") == 0;
}

} // namespace NativeModule
