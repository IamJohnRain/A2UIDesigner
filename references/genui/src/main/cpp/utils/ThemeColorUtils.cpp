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

#include "utils/ThemeColorUtils.h"

#include <cctype>

namespace NativeModule {

namespace {

bool IsHexDigit(char ch)
{
    return std::isxdigit(static_cast<unsigned char>(ch)) != 0;
}

bool TryParseHexByte(char high, char low, uint8_t& value)
{
    if (!IsHexDigit(high) || !IsHexDigit(low)) {
        return false;
    }

    auto toNibble = [](char ch) -> uint8_t {
        if (ch >= '0' && ch <= '9') {
            return static_cast<uint8_t>(ch - '0');
        }
        char normalized = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return static_cast<uint8_t>(normalized - 'a' + 10);
    };

    value = static_cast<uint8_t>((toNibble(high) << 4) | toNibble(low));
    return true;
}

} // namespace

bool ThemeColorUtils::TryParseArgb(const std::string& colorString, uint32_t& argb)
{
    if (colorString.size() != 7 && colorString.size() != 9) {
        return false;
    }
    if (colorString[0] != '#') {
        return false;
    }

    uint8_t alpha = 0xFF;
    size_t offset = 1;
    if (colorString.size() == 9) {
        if (!TryParseHexByte(colorString[1], colorString[2], alpha)) {
            return false;
        }
        offset = 3;
    }

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    if (!TryParseHexByte(colorString[offset], colorString[offset + 1], red) ||
        !TryParseHexByte(colorString[offset + 2], colorString[offset + 3], green) ||
        !TryParseHexByte(colorString[offset + 4], colorString[offset + 5], blue)) {
        return false;
    }

    argb = (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(red) << 16) |
           (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
    return true;
}

uint32_t ThemeColorUtils::InvertRgbKeepAlpha(uint32_t color)
{
    constexpr uint32_t ALPHA_MASK = 0xFF000000;
    constexpr uint32_t RGB_MASK = 0x00FFFFFF;
    return (color & ALPHA_MASK) | ((~color) & RGB_MASK);
}

} // namespace NativeModule
