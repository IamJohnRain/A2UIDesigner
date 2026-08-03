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

#include "ExtendedDividerTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {
constexpr uint32_t EXTENDED_DIVIDER_LIGHT_DEFAULT_COLOR = 0x33000000u;
constexpr uint32_t EXTENDED_DIVIDER_DARK_DEFAULT_COLOR = 0x33FFFFFFu;
} // namespace

ExtendedDividerTheme::ExtendedDividerTheme(const ThemeContext& context) : ExtendedCommonTheme(context)
{
    InitializeAllProperties();
}

void ExtendedDividerTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

void ExtendedDividerTheme::InitializeAllProperties()
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedDividerTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));
}

uint32_t ExtendedDividerTheme::GetDefaultColor() const
{
    return currentContext_.colorMode == ThemeMode::DARK ? EXTENDED_DIVIDER_DARK_DEFAULT_COLOR
                                                        : EXTENDED_DIVIDER_LIGHT_DEFAULT_COLOR;
}

} // namespace NativeModule
