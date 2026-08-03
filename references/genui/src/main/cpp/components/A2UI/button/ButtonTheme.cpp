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

#include "ButtonTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {
namespace {
constexpr uint32_t PRIMARY_BUTTON_LIGHT_BACKGROUND_COLOR = 0xFF0A59F7;
constexpr uint32_t PRIMARY_BUTTON_LIGHT_TEXT_COLOR = 0xFFFFFFFF;
constexpr uint32_t PRIMARY_BUTTON_DARK_BACKGROUND_COLOR = 0xFF317AF7;
constexpr uint32_t PRIMARY_BUTTON_DARK_TEXT_COLOR = 0xFFFFFFFF;

constexpr uint32_t DEFAULT_BUTTON_LIGHT_BACKGROUND_COLOR = 0x0C000000;
constexpr uint32_t DEFAULT_BUTTON_LIGHT_TEXT_COLOR = 0xFF0A59F7;
constexpr uint32_t DEFAULT_BUTTON_DARK_BACKGROUND_COLOR = 0x19FFFFFF;
constexpr uint32_t DEFAULT_BUTTON_DARK_TEXT_COLOR = 0xFF5291FF;

constexpr uint32_t BORDERLESS_BUTTON_LIGHT_BACKGROUND_COLOR = 0x00000000;
constexpr uint32_t BORDERLESS_BUTTON_LIGHT_TEXT_COLOR = 0xFF0A59F7;
constexpr uint32_t BORDERLESS_BUTTON_DARK_BACKGROUND_COLOR = 0x00000000;
constexpr uint32_t BORDERLESS_BUTTON_DARK_TEXT_COLOR = 0xFF5291FF;

constexpr float DEFAULT_BUTTON_PADDING_VERTICAL = 8.0F;
constexpr float DEFAULT_BUTTON_PADDING_HORIZONTAL = 16.0F;
constexpr int32_t DEFAULT_BUTTON_HEIGHT = 40;
constexpr int32_t DEFAULT_BUTTON_FONT_WEIGHT = 500;
constexpr int32_t ICON_BUTTON_SIZE = 48;
constexpr int32_t ICON_SIZE = 24;
constexpr float ICON_BUTTON_PADDING = 12.0F;
constexpr uint32_t DEFAULT_BUTTON_LIGHT_ICON_COLOR = 0xE5000000;
constexpr uint32_t DEFAULT_BUTTON_DARK_ICON_COLOR = 0xE5FFFFFF;
constexpr uint32_t PRIMARY_BUTTON_ICON_COLOR = 0xFFFFFFFF;
constexpr uint32_t BORDERLESS_BUTTON_LIGHT_ICON_COLOR = 0xE5000000;
constexpr uint32_t BORDERLESS_BUTTON_DARK_ICON_COLOR = 0xE5FFFFFF;
} // namespace

ButtonTheme::ButtonTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void ButtonTheme::OnConfigChange(const ThemeContext& context)
{
    // Note: context is already updated by ThemeBase::UpdateContext
    currentContext_ = context;
    // Initialize all properties with new context
    InitializeAllProperties();
}

void ButtonTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_DEBUG,
        "ButtonTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));
}

uint32_t ButtonTheme::GetFontColor(const std::string& variant) const
{
    if (variant == "primary") {
        return currentContext_.colorMode == ThemeMode::LIGHT ? PRIMARY_BUTTON_LIGHT_TEXT_COLOR
                                                             : PRIMARY_BUTTON_DARK_TEXT_COLOR;
    }
    if (variant == "borderless") {
        return HasBrandColor()                                 ? currentContext_.brandColor
               : currentContext_.colorMode == ThemeMode::LIGHT ? BORDERLESS_BUTTON_LIGHT_TEXT_COLOR
                                                               : BORDERLESS_BUTTON_DARK_TEXT_COLOR;
    }
    return currentContext_.colorMode == ThemeMode::LIGHT ? DEFAULT_BUTTON_LIGHT_TEXT_COLOR
                                                         : DEFAULT_BUTTON_DARK_TEXT_COLOR;
}

uint32_t ButtonTheme::GetBackgroundColor(const std::string& variant) const
{
    if (variant == "primary") {
        return HasBrandColor()                                 ? currentContext_.brandColor
               : currentContext_.colorMode == ThemeMode::LIGHT ? PRIMARY_BUTTON_LIGHT_BACKGROUND_COLOR
                                                               : PRIMARY_BUTTON_DARK_BACKGROUND_COLOR;
    }
    if (variant == "borderless") {
        return currentContext_.colorMode == ThemeMode::LIGHT ? BORDERLESS_BUTTON_LIGHT_BACKGROUND_COLOR
                                                             : BORDERLESS_BUTTON_DARK_BACKGROUND_COLOR;
    }
    return HasBrandColor()                                 ? currentContext_.brandColor
           : currentContext_.colorMode == ThemeMode::LIGHT ? DEFAULT_BUTTON_LIGHT_BACKGROUND_COLOR
                                                           : DEFAULT_BUTTON_DARK_BACKGROUND_COLOR;
}

A2UIFontWeight ButtonTheme::GetFontWeight()
{
    return A2UIFontWeight::W500;
}

int32_t ButtonTheme::GetHeight()
{
    return DEFAULT_BUTTON_HEIGHT;
}

std::array<float, 4> ButtonTheme::GetPadding()
{
    return { DEFAULT_BUTTON_PADDING_VERTICAL, DEFAULT_BUTTON_PADDING_HORIZONTAL, DEFAULT_BUTTON_PADDING_VERTICAL,
        DEFAULT_BUTTON_PADDING_HORIZONTAL };
}

std::array<float, 4> ButtonTheme::GetIconPadding()
{
    return { ICON_BUTTON_PADDING, ICON_BUTTON_PADDING, ICON_BUTTON_PADDING, ICON_BUTTON_PADDING };
}

int32_t ButtonTheme::GetIconButtonSize()
{
    return ICON_BUTTON_SIZE;
}

int32_t ButtonTheme::GetIconSize()
{
    return ICON_SIZE;
}

uint32_t ButtonTheme::GetIconColor(const std::string& variant)
{
    if (variant == "primary") {
        return PRIMARY_BUTTON_ICON_COLOR;
    }
    if (variant == "borderless") {
        return currentContext_.colorMode == ThemeMode::LIGHT ? BORDERLESS_BUTTON_LIGHT_ICON_COLOR
                                                             : DEFAULT_BUTTON_DARK_ICON_COLOR;
    }
    return currentContext_.colorMode == ThemeMode::LIGHT ? BORDERLESS_BUTTON_LIGHT_ICON_COLOR
                                                         : BORDERLESS_BUTTON_DARK_ICON_COLOR;
}
} // namespace NativeModule
