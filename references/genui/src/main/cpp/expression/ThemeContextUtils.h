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

#ifndef A2UI_THEME_CONTEXT_UTILS_H
#define A2UI_THEME_CONTEXT_UTILS_H

#include <string>

#include "theme/ThemeBase.h"

namespace NativeModule {

inline std::string BreakpointToString(Breakpoint bp)
{
    switch (bp) {
        case Breakpoint::XS:
            return "xs";
        case Breakpoint::SM:
            return "sm";
        case Breakpoint::MD:
            return "md";
        case Breakpoint::LG:
            return "lg";
        case Breakpoint::XL:
            return "xl";
        default:
            return "sm";
    }
}

inline std::string ColorModeToString(ThemeMode mode)
{
    switch (mode) {
        case ThemeMode::LIGHT:
            return "light";
        case ThemeMode::DARK:
            return "dark";
        default:
            return "light";
    }
}

} // namespace NativeModule

#endif // A2UI_THEME_CONTEXT_UTILS_H
