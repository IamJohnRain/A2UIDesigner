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

#include "CheckboxGroupTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {
constexpr uint32_t SELECTED_LIGHT_COLOR = 0xFF007DFF;
constexpr uint32_t SELECTED_DARK_COLOR = 0xFF3F97E9;
constexpr uint32_t UNSELECTED_LIGHT_COLOR = 0x66182431;
constexpr uint32_t UNSELECTED_DARK_COLOR = 0x66FFFFFF;
constexpr uint32_t MARK_STROKE_COLOR = 0xFFFFFFFF;
} // namespace

CheckboxGroupTheme::CheckboxGroupTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void CheckboxGroupTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

void CheckboxGroupTheme::InitializeAllProperties()
{
    LOG_A2UI(LOG_INFO,
        "CheckboxGroupTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    InitializeSelectedColor();
    InitializeUnselectedColor();
    InitializeMarkStrokeColor();
}

void CheckboxGroupTheme::InitializeSelectedColor()
{
    if (currentContext_.hasBrandColor) {
        selectedColor_ = currentContext_.brandColor;
        return;
    }
    selectedColor_ = currentContext_.colorMode == ThemeMode::LIGHT ? SELECTED_LIGHT_COLOR : SELECTED_DARK_COLOR;
}

void CheckboxGroupTheme::InitializeUnselectedColor()
{
    unselectedColor_ = currentContext_.colorMode == ThemeMode::LIGHT ? UNSELECTED_LIGHT_COLOR : UNSELECTED_DARK_COLOR;
}

void CheckboxGroupTheme::InitializeMarkStrokeColor()
{
    markStrokeColor_ = MARK_STROKE_COLOR;
}

uint32_t CheckboxGroupTheme::GetSelectedColor() const
{
    return selectedColor_;
}

uint32_t CheckboxGroupTheme::GetUnselectedColor() const
{
    return unselectedColor_;
}

uint32_t CheckboxGroupTheme::GetMarkStrokeColor() const
{
    return markStrokeColor_;
}

} // namespace NativeModule
