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

#include "TextFieldTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {
namespace {
constexpr float LABEL_FONT_SIZE = 14.0F;
constexpr float LABEL_PADDING_BOTTOM = 8.0F;
constexpr float LABEL_PADDING_START = 16.0F;
constexpr float LABEL_PADDING_END = 16.0F;
constexpr uint32_t LABEL_FONT_LIGHT_COLOR = 0x99000000;
constexpr uint32_t LABEL_FONT_DARK_COLOR = 0x99FFFFFF;
constexpr float LABEL_HEIGHT = 56.0F;

constexpr float ERROR_FONT_SIZE = 12.0F;
constexpr uint32_t ERROR_FONT_LIGHT_COLOR = 0xFFE84026;
constexpr uint32_t ERROR_FONT_DARK_COLOR = 0xFFD94838;

constexpr float ERROR_PADDING_END = 16.0F;
constexpr float ERROR_PADDING_START = 16.0F;
constexpr float ERROR_PADDING_TOP = 8.0F;
} // namespace

TextFieldTheme::TextFieldTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void TextFieldTheme::OnConfigChange(const ThemeContext& context)
{
    InitializeAllProperties();
}

void TextFieldTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_DEBUG,
        "TextFieldTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));
}

uint32_t TextFieldTheme::GetLabelFontColor()
{
    return currentContext_.colorMode == ThemeMode::LIGHT ? LABEL_FONT_LIGHT_COLOR : LABEL_FONT_DARK_COLOR;
}

float TextFieldTheme::GetLabelFontSize()
{
    return LABEL_FONT_SIZE;
}

float TextFieldTheme::GetLabelHeight()
{
    return LABEL_HEIGHT;
}

std::array<float, 4> TextFieldTheme::GetLabelPadding()
{
    return { 0.0F, LABEL_PADDING_END, LABEL_PADDING_BOTTOM, LABEL_PADDING_START };
}

float TextFieldTheme::GetErrorFontSize()
{
    return ERROR_FONT_SIZE;
}

uint32_t TextFieldTheme::GetErrorFontColor()
{
    return currentContext_.colorMode == ThemeMode::LIGHT ? ERROR_FONT_LIGHT_COLOR : ERROR_FONT_DARK_COLOR;
}

std::array<float, 4> TextFieldTheme::GetErrorPadding()
{
    return { ERROR_PADDING_TOP, ERROR_PADDING_END, 0.0F, ERROR_PADDING_START };
}

A2UIFontWeight TextFieldTheme::GetLabelFontWeight()
{
    return A2UIFontWeight::W500;
}

} // namespace NativeModule
