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

#include "ExtendedProgressTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {
constexpr uint32_t EXTENDED_PROGRESS_LINEAR_LIGHT_DEFAULT_COLOR = 0xFF0A59F7u;
constexpr uint32_t EXTENDED_PROGRESS_LINEAR_DARK_DEFAULT_COLOR = 0xFF317AF7u;
constexpr uint32_t EXTENDED_PROGRESS_ECLIPSE_LIGHT_DEFAULT_COLOR = 0x19000000u;
constexpr uint32_t EXTENDED_PROGRESS_ECLIPSE_DARK_DEFAULT_COLOR = 0x19FFFFFFu;
constexpr uint32_t EXTENDED_PROGRESS_SCALE_RING_LIGHT_DEFAULT_COLOR = 0x99000000u;
constexpr uint32_t EXTENDED_PROGRESS_SCALE_RING_DARK_DEFAULT_COLOR = 0x99FFFFFFu;
constexpr uint32_t EXTENDED_PROGRESS_FALLBACK_DEFAULT_COLOR = 0xFFFF7DFFu;

constexpr int32_t DEFAULT_PROGRESS_TYPE = 0;
constexpr int32_t ECLIPSE_PROGRESS_TYPE = 2;
constexpr int32_t SCALE_RING_PROGRESS_TYPE = 3;
} // namespace

ExtendedProgressTheme::ExtendedProgressTheme(const ThemeContext& context) : ExtendedCommonTheme(context)
{
    InitializeAllProperties();
}

void ExtendedProgressTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

void ExtendedProgressTheme::InitializeAllProperties()
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedProgressTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));
}

uint32_t ExtendedProgressTheme::GetDefaultColorByType(int32_t progressType) const
{
    ThemeMode colorMode = currentContext_.colorMode;
    if (progressType == DEFAULT_PROGRESS_TYPE) {
        return colorMode == ThemeMode::DARK ? EXTENDED_PROGRESS_LINEAR_DARK_DEFAULT_COLOR
                                            : EXTENDED_PROGRESS_LINEAR_LIGHT_DEFAULT_COLOR;
    }
    if (progressType == ECLIPSE_PROGRESS_TYPE) {
        return colorMode == ThemeMode::DARK ? EXTENDED_PROGRESS_ECLIPSE_DARK_DEFAULT_COLOR
                                            : EXTENDED_PROGRESS_ECLIPSE_LIGHT_DEFAULT_COLOR;
    }
    if (progressType == SCALE_RING_PROGRESS_TYPE) {
        return colorMode == ThemeMode::DARK ? EXTENDED_PROGRESS_SCALE_RING_DARK_DEFAULT_COLOR
                                            : EXTENDED_PROGRESS_SCALE_RING_LIGHT_DEFAULT_COLOR;
    }
    return EXTENDED_PROGRESS_FALLBACK_DEFAULT_COLOR;
}

} // namespace NativeModule
