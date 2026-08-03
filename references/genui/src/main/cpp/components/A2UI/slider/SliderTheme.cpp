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

#include "SliderTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {
constexpr uint32_t DEFAULT_SLIDER_SELECTED_COLOR = 0xFF007DFF;
} // namespace

SliderTheme::SliderTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void SliderTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

void SliderTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_INFO,
        "SliderTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    // TODO: Initialize all theme properties based on currentContext_
    InitializeSelectedColor();
}

void SliderTheme::InitializeSelectedColor()
{
    if (currentContext_.hasBrandColor) {
        selectedColor_ = currentContext_.brandColor;
        return;
    }
    selectedColor_ = DEFAULT_SLIDER_SELECTED_COLOR;
}

} // namespace NativeModule
