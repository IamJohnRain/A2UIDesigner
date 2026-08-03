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
#include "components/A2UI/column/ColumnTheme.h"
#include "components/A2UI/row/RowTheme.h"
#include "components/A2UI/text/TextTheme.h"
#include "components/A2UI/textfield/TextFieldTheme.h"
#include "components/extended/ExtendedDividerTheme.h"
#include "components/extended/ExtendedProgressTheme.h"
#include "components/extended/ExtendedTextTheme.h"

using namespace NativeModule;

/**
 * @tc.name: ThemeConstantsTest001
 * @tc.desc: Verify text variant font size constants are resolved by TextTheme.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest001)
{
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("h1"), 32.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("h2"), 28.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("h3"), 24.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("h4"), 20.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("h5"), 18.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("caption"), 12.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("body"), 16.0F);
    EXPECT_FLOAT_EQ(TextTheme::ResolveFontSize("unknown"), 16.0F);
}

/**
 * @tc.name: ThemeConstantsTest002
 * @tc.desc: Verify text field style constants are provided by TextFieldTheme.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest002)
{
    EXPECT_FLOAT_EQ(TextFieldTheme::GetLabelFontSize(), 14.0F);
    std::array<float, 4> labelMargin = TextFieldTheme::GetLabelPadding();
    EXPECT_FLOAT_EQ(labelMargin[0], 0.0F);
    EXPECT_FLOAT_EQ(labelMargin[1], 16.0F);
    EXPECT_FLOAT_EQ(labelMargin[2], 8.0F);
    EXPECT_FLOAT_EQ(labelMargin[3], 16.0F);

    EXPECT_FLOAT_EQ(TextFieldTheme::GetErrorFontSize(), 12.0F);
    std::array<float, 4> errorMargin = TextFieldTheme::GetErrorPadding();
    EXPECT_FLOAT_EQ(errorMargin[0], 8.0F);
    EXPECT_FLOAT_EQ(errorMargin[1], 16.0F);
    EXPECT_FLOAT_EQ(errorMargin[2], 0.0F);
    EXPECT_FLOAT_EQ(errorMargin[3], 16.0F);
}

/**
 * @tc.name: ThemeConstantsTest003
 * @tc.desc: Verify column alignment constants are resolved by ColumnTheme.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest003)
{
    EXPECT_EQ(ColumnTheme::ResolveAlignItems("start"), ARKUI_HORIZONTAL_ALIGNMENT_START);
    EXPECT_EQ(ColumnTheme::ResolveAlignItems("center"), ARKUI_HORIZONTAL_ALIGNMENT_CENTER);
    EXPECT_EQ(ColumnTheme::ResolveAlignItems("end"), ARKUI_HORIZONTAL_ALIGNMENT_END);
    EXPECT_EQ(ColumnTheme::ResolveAlignItems("unknown"), ARKUI_HORIZONTAL_ALIGNMENT_START);

    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("start"), ARKUI_FLEX_ALIGNMENT_START);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("center"), ARKUI_FLEX_ALIGNMENT_CENTER);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("end"), ARKUI_FLEX_ALIGNMENT_END);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("spaceAround"), ARKUI_FLEX_ALIGNMENT_SPACE_AROUND);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("spaceBetween"), ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("spaceEvenly"), ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY);
    EXPECT_EQ(ColumnTheme::ResolveJustifyContent("unknown"), ARKUI_FLEX_ALIGNMENT_START);
    EXPECT_FLOAT_EQ(ColumnTheme::GetDefaultSpace(), 8.0F);
}

/**
 * @tc.name: ThemeConstantsTest004
 * @tc.desc: Verify row alignment constants are resolved by RowTheme.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest004)
{
    EXPECT_EQ(RowTheme::ResolveAlignItems("start"), ARKUI_VERTICAL_ALIGNMENT_TOP);
    EXPECT_EQ(RowTheme::ResolveAlignItems("center"), ARKUI_VERTICAL_ALIGNMENT_CENTER);
    EXPECT_EQ(RowTheme::ResolveAlignItems("end"), ARKUI_VERTICAL_ALIGNMENT_BOTTOM);
    EXPECT_EQ(RowTheme::ResolveAlignItems("unknown"), ARKUI_VERTICAL_ALIGNMENT_TOP);

    EXPECT_EQ(RowTheme::ResolveJustifyContent("start"), ARKUI_FLEX_ALIGNMENT_START);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("center"), ARKUI_FLEX_ALIGNMENT_CENTER);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("end"), ARKUI_FLEX_ALIGNMENT_END);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("spaceAround"), ARKUI_FLEX_ALIGNMENT_SPACE_AROUND);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("spaceBetween"), ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("spaceEvenly"), ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY);
    EXPECT_EQ(RowTheme::ResolveJustifyContent("unknown"), ARKUI_FLEX_ALIGNMENT_START);
    EXPECT_FLOAT_EQ(RowTheme::GetDefaultSpace(), 16.0F);
}

/**
 * @tc.name: ThemeConstantsTest005
 * @tc.desc: Verify button text/icon style constants are provided by ButtonTheme.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest005)
{
    EXPECT_EQ(ButtonTheme::GetHeight(), 40);
    EXPECT_EQ(ButtonTheme::GetIconButtonSize(), 48);
    EXPECT_EQ(ButtonTheme::GetIconSize(), 24);
}

/**
 * @tc.name: ThemeConstantsTest006
 * @tc.desc: Verify ButtonTheme resolves variant colors under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest006)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    ButtonTheme buttonTheme(context);

    EXPECT_EQ(buttonTheme.GetFontColor("primary"), 0xFFFFFFFFU);
    EXPECT_EQ(buttonTheme.GetFontColor("borderless"), 0xFF0A59F7U);
    EXPECT_EQ(buttonTheme.GetFontColor("default"), 0xFF0A59F7U);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("primary"), 0xFF0A59F7U);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("borderless"), 0x00000000U);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("default"), 0x0C000000U);
    EXPECT_EQ(buttonTheme.GetIconColor("primary"), 0xFFFFFFFFU);
    EXPECT_EQ(buttonTheme.GetIconColor("borderless"), 0xE5000000U);
    EXPECT_EQ(buttonTheme.GetIconColor("default"), 0xE5000000U);

    buttonTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(buttonTheme.GetFontColor("primary"), 0xFFFFFFFFU);
    EXPECT_EQ(buttonTheme.GetFontColor("borderless"), 0xFF5291FFU);
    EXPECT_EQ(buttonTheme.GetFontColor("default"), 0xFF5291FFU);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("primary"), 0xFF317AF7U);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("borderless"), 0x00000000U);
    EXPECT_EQ(buttonTheme.GetBackgroundColor("default"), 0x19FFFFFFU);
    EXPECT_EQ(buttonTheme.GetIconColor("primary"), 0xFFFFFFFFU);
    EXPECT_EQ(buttonTheme.GetIconColor("borderless"), 0xE5FFFFFFU);
    EXPECT_EQ(buttonTheme.GetIconColor("default"), 0xE5FFFFFFU);
}

/**
 * @tc.name: ThemeConstantsTest007
 * @tc.desc: Verify TextFieldTheme resolves font colors under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest007)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    TextFieldTheme textFieldTheme(context);

    EXPECT_EQ(textFieldTheme.GetLabelFontColor(), 0x99000000U);
    EXPECT_EQ(textFieldTheme.GetErrorFontColor(), 0xFFE84026U);

    textFieldTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(textFieldTheme.GetLabelFontColor(), 0x99FFFFFFU);
    EXPECT_EQ(textFieldTheme.GetErrorFontColor(), 0xFFD94838U);
    EXPECT_FLOAT_EQ(TextFieldTheme::GetLabelHeight(), 56.0F);
    EXPECT_EQ(TextFieldTheme::GetLabelFontWeight(), ARKUI_FONT_WEIGHT_W500);
}

TEST(ThemeConstantsTest, ThemeConstantsTest008)
{
    ThemeContext lightContext;
    lightContext.colorMode = ThemeMode::LIGHT;
    CheckboxTheme lightTheme(lightContext);
    EXPECT_EQ(lightTheme.GetSelectedColor(), 0xFF007DFFU);
    EXPECT_EQ(lightTheme.GetUnselectedColor(), 0xFF182431U);
    EXPECT_EQ(lightTheme.GetMarkStrokeColor(), 0xFFFFFFFFU);

    lightTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(lightTheme.GetSelectedColor(), 0xFF317AF7U);
    EXPECT_EQ(lightTheme.GetUnselectedColor(), 0xFFC5C8CCU);
    EXPECT_EQ(lightTheme.GetMarkStrokeColor(), 0xFFFFFFFFU);

    ThemeContext brandContext;
    brandContext.colorMode = ThemeMode::LIGHT;
    brandContext.hasBrandColor = true;
    brandContext.brandColor = 0xFFFF6A00;
    CheckboxTheme brandTheme(brandContext);
    EXPECT_EQ(brandTheme.GetSelectedColor(), 0xFFFF6A00U);
    EXPECT_EQ(brandTheme.GetUnselectedColor(), 0xFF182431U);
}

/**
 * @tc.name: ThemeConstantsTest009
 * @tc.desc: Verify ExtendedTextTheme resolves default font color under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest009)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    ExtendedTextTheme textTheme(context);

    EXPECT_EQ(textTheme.GetDefaultFontColor(), 0xE5000000U);
    textTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(textTheme.GetDefaultFontColor(), 0x99FFFFFFU);
}

/**
 * @tc.name: ThemeConstantsTest010
 * @tc.desc: Verify ExtendedTextTheme resolves default decoration color under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest010)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    ExtendedTextTheme textTheme(context);

    EXPECT_EQ(textTheme.GetDefaultDecorationColor(), 0xFF000000U);
    textTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(textTheme.GetDefaultDecorationColor(), 0x99FFFFFFU);
}

/**
 * @tc.name: ThemeConstantsTest011
 * @tc.desc: Verify ExtendedDividerTheme resolves default colors under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest011)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    ExtendedDividerTheme dividerTheme(context);

    EXPECT_EQ(dividerTheme.GetDefaultColor(), 0x33000000U);
    dividerTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(dividerTheme.GetDefaultColor(), 0x33FFFFFFU);
}

/**
 * @tc.name: ThemeConstantsTest012
 * @tc.desc: Verify ExtendedProgressTheme resolves default colors by type under light/dark modes.
 * @tc.type: FUNC
 */
TEST(ThemeConstantsTest, ThemeConstantsTest012)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    ExtendedProgressTheme progressTheme(context);

    EXPECT_EQ(progressTheme.GetDefaultColorByType(0), 0xFF0A59F7U);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(2), 0x19000000U);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(3), 0x99000000U);

    progressTheme.UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(0), 0xFF317AF7U);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(2), 0x19FFFFFFU);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(3), 0x99FFFFFFU);
}

TEST(ThemeConstantsTest, ThemeConstantsTest013)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.hasBrandColor = true;
    context.brandColor = 0xFF123456U;

    ExtendedTextTheme textTheme(context);
    ExtendedDividerTheme dividerTheme(context);
    ExtendedProgressTheme progressTheme(context);

    EXPECT_EQ(textTheme.GetDefaultFontColor(), 0xE5000000U);
    EXPECT_EQ(dividerTheme.GetDefaultColor(), 0x33000000U);
    EXPECT_EQ(progressTheme.GetDefaultColorByType(0), 0xFF0A59F7U);
}
