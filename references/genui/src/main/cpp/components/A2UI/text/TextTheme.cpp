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

#include "TextTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {
namespace {
constexpr float H1_FONT_SIZE = 32.0F;
constexpr float H2_FONT_SIZE = 28.0F;
constexpr float H3_FONT_SIZE = 24.0F;      // sys.float.Title_M
constexpr float H4_FONT_SIZE = 20.0F;      // sys.float.Title_S
constexpr float H5_FONT_SIZE = 18.0F;      // sys.float.Subtitle_L
constexpr float CAPTION_FONT_SIZE = 12.0F; // sys.float.Caption_L
constexpr float BODY_FONT_SIZE = 16.0F;    // sys.float.Body_L
} // namespace

TextTheme::TextTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void TextTheme::OnConfigChange(const ThemeContext& context)
{
    // Note: context is already updated by ThemeBase::UpdateContext
    InitializeAllProperties();
}

void TextTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_DEBUG,
        "TextFieldTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));
}

float TextTheme::ResolveFontSize(const std::string& variant)
{
    if (variant == "h1") {
        return H1_FONT_SIZE;
    }
    if (variant == "h2") {
        return H2_FONT_SIZE;
    }
    if (variant == "h3") {
        return H3_FONT_SIZE;
    }
    if (variant == "h4") {
        return H4_FONT_SIZE;
    }
    if (variant == "h5") {
        return H5_FONT_SIZE;
    }
    if (variant == "caption") {
        return CAPTION_FONT_SIZE;
    }
    return BODY_FONT_SIZE;
}

} // namespace NativeModule
