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

#include "CheckboxTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {
constexpr uint32_t SELECTED_LIGHT_COLOR = 0xFF317AF7;
constexpr uint32_t SELECTED_DARK_COLOR = 0xFF0A59F7;
constexpr uint32_t UNSELECTED_LIGHT_COLOR = 0x66000000;
constexpr uint32_t UNSELECTED_DARK_COLOR = 0x66000000;
constexpr uint32_t MARK_STROKE_COLOR = 0xFFFFFFFF;
} // namespace

CheckboxTheme::CheckboxTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void CheckboxTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

void CheckboxTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_INFO,
        "CheckboxTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    // TODO: Initialize all theme properties based on currentContext_
    InitializeSelectedColor();
    InitializeUnselectedColor();
    InitializeMarkStrokeColor();
}

void CheckboxTheme::InitializeSelectedColor()
{
    if (currentContext_.hasBrandColor) {
        selectedColor_ = currentContext_.brandColor;
        return;
    }
    selectedColor_ = currentContext_.colorMode == ThemeMode::LIGHT ? SELECTED_LIGHT_COLOR : SELECTED_DARK_COLOR;
}

void CheckboxTheme::InitializeUnselectedColor()
{
    unselectedColor_ = currentContext_.colorMode == ThemeMode::LIGHT ? UNSELECTED_LIGHT_COLOR : UNSELECTED_DARK_COLOR;
}

void CheckboxTheme::InitializeMarkStrokeColor()
{
    markStrokeColor_ = MARK_STROKE_COLOR;
}

uint32_t CheckboxTheme::GetSelectedColor() const
{
    return selectedColor_;
}

uint32_t CheckboxTheme::GetUnselectedColor() const
{
    return unselectedColor_;
}

uint32_t CheckboxTheme::GetMarkStrokeColor() const
{
    return markStrokeColor_;
}

} // namespace NativeModule
