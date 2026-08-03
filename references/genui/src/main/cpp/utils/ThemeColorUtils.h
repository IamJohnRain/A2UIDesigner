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

#ifndef A2UI_THEME_COLOR_UTILS_H
#define A2UI_THEME_COLOR_UTILS_H

#include <cstdint>
#include <string>

namespace NativeModule {

class ThemeColorUtils {
public:
    ThemeColorUtils() = delete;
    ~ThemeColorUtils() = delete;

    /**
     * @brief Parse a theme color string to ARGB.
     * Supports #RRGGBB and #AARRGGBB.
     * @param colorString The source color string.
     * @param argb Parsed ARGB output.
     * @return true when parsing succeeds.
     */
    static bool TryParseArgb(const std::string& colorString, uint32_t& argb);

    /**
     * @brief Invert RGB channels and keep alpha unchanged.
     * @param color Input ARGB color.
     * @return Inverted ARGB color.
     */
    static uint32_t InvertRgbKeepAlpha(uint32_t color);
};

} // namespace NativeModule

#endif // A2UI_THEME_COLOR_UTILS_H
