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

#include <gtest/gtest.h>

#include "components/A2UI/button/ButtonTheme.h"
#include "components/A2UI/checkbox/CheckboxTheme.h"
#include "components/A2UI/slider/SliderTheme.h"

using namespace NativeModule;

namespace {
constexpr uint32_t BRAND_COLOR = 0xFFFF6A00;
constexpr uint32_t DEFAULT_BUTTON_BACKGROUND_COLOR = 0xFF0A59F7;
constexpr uint32_t DEFAULT_CHECKBOX_SELECTED_COLOR = 0xFF317AF7;
constexpr uint32_t DEFAULT_SLIDER_SELECTED_COLOR = 0xFF007DFF;

ThemeContext CreateContext(bool hasBrandColor)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.hasBrandColor = hasBrandColor;
    context.brandColor = hasBrandColor ? BRAND_COLOR : 0;
    return context;
}
} // namespace

TEST(ThemeBrandColorTest, L0_should_apply_brand_color_to_button_background_when_context_has_brand_color)
{
    ButtonTheme theme(CreateContext(false));
    EXPECT_EQ(theme.GetBackgroundColor("primary"), DEFAULT_BUTTON_BACKGROUND_COLOR);

    theme.OnConfigChange(CreateContext(true));

    EXPECT_TRUE(theme.HasBrandColor());
    EXPECT_EQ(theme.GetBackgroundColor("primary"), BRAND_COLOR);
}

TEST(ThemeBrandColorTest, L0_should_apply_brand_color_to_slider_selected_color_when_context_has_brand_color)
{
    SliderTheme theme(CreateContext(false));
    EXPECT_EQ(theme.GetSelectedColor(), DEFAULT_SLIDER_SELECTED_COLOR);

    theme.OnConfigChange(CreateContext(true));

    EXPECT_EQ(theme.GetSelectedColor(), BRAND_COLOR);
}

TEST(ThemeBrandColorTest, L0_should_apply_brand_color_to_checkbox_selected_color_when_context_has_brand_color)
{
    CheckboxTheme theme(CreateContext(false));
    EXPECT_EQ(theme.GetSelectedColor(), DEFAULT_CHECKBOX_SELECTED_COLOR);

    theme.OnConfigChange(CreateContext(true));

    EXPECT_EQ(theme.GetSelectedColor(), BRAND_COLOR);
}
