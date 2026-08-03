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

#include "A2UIArkUITypeConverter.h"

#include <gtest/gtest.h>

namespace NativeModule {
namespace {

TEST(A2UIArkUITypeConverterTest, ShouldMapFlexAlignmentToRealArkUIValues)
{
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::START)), 1);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::CENTER)), 2);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::END)), 3);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::SPACE_BETWEEN)), 6);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::SPACE_AROUND)), 7);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment::SPACE_EVENLY)), 8);
}

TEST(A2UIArkUITypeConverterTest, ShouldMapImageObjectFitToRealArkUIValues)
{
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::CONTAIN)), 0);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::COVER)), 1);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::AUTO)), 2);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::FILL)), 3);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::SCALE_DOWN)), 4);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE)), 5);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(static_cast<ArkUI_ObjectFit>(2)), A2UIObjectFit::AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(static_cast<ArkUI_ObjectFit>(3)), A2UIObjectFit::FILL);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(static_cast<ArkUI_ObjectFit>(4)), A2UIObjectFit::SCALE_DOWN);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(static_cast<ArkUI_ObjectFit>(5)), A2UIObjectFit::NONE);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_TOP_START)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_TOP)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_TOP_END)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_END);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_START)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_START);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_CENTER)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_CENTER);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_END)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_END);
    EXPECT_EQ(
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_START);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END)),
        ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_END);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit::NONE_MATRIX)),
        ARKUI_OBJECT_FIT_NONE_MATRIX);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START),
        A2UIObjectFit::NONE_AND_ALIGN_TOP_START);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP),
        A2UIObjectFit::NONE_AND_ALIGN_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_END),
        A2UIObjectFit::NONE_AND_ALIGN_TOP_END);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_START),
        A2UIObjectFit::NONE_AND_ALIGN_START);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_CENTER),
        A2UIObjectFit::NONE_AND_ALIGN_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_END),
        A2UIObjectFit::NONE_AND_ALIGN_END);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_START),
        A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM),
        A2UIObjectFit::NONE_AND_ALIGN_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_END),
        A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(ARKUI_OBJECT_FIT_NONE_MATRIX), A2UIObjectFit::NONE_MATRIX);
}

TEST(A2UIArkUITypeConverterTest, ShouldMapButtonTypeAndAxisToRealArkUIValues)
{
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIButtonType(A2UIButtonType::NORMAL)), 0);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIButtonType(A2UIButtonType::CAPSULE)), 1);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIButtonType(A2UIButtonType::CIRCLE)), 2);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIAxis(A2UIAxis::VERTICAL)), 0);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIAxis(A2UIAxis::HORIZONTAL)), 1);
}

TEST(A2UIArkUITypeConverterTest, ShouldMapLayoutAndVisualEnumsExplicitly)
{
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexDirection(A2UIFlexDirection::ROW), ARKUI_FLEX_DIRECTION_ROW);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexDirection(A2UIFlexDirection::COLUMN), ARKUI_FLEX_DIRECTION_COLUMN);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIFlexDirection(A2UIFlexDirection::ROW_REVERSE), ARKUI_FLEX_DIRECTION_ROW_REVERSE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexDirection(A2UIFlexDirection::COLUMN_REVERSE),
        ARKUI_FLEX_DIRECTION_COLUMN_REVERSE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexWrap(A2UIFlexWrap::NO_WRAP), ARKUI_FLEX_WRAP_NO_WRAP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexWrap(A2UIFlexWrap::WRAP), ARKUI_FLEX_WRAP_WRAP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexWrap(A2UIFlexWrap::WRAP_REVERSE), ARKUI_FLEX_WRAP_WRAP_REVERSE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(A2UIHorizontalAlignment::START),
        ARKUI_HORIZONTAL_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(A2UIHorizontalAlignment::CENTER),
        ARKUI_HORIZONTAL_ALIGNMENT_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(A2UIHorizontalAlignment::END),
        ARKUI_HORIZONTAL_ALIGNMENT_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::AUTO), ARKUI_ITEM_ALIGNMENT_AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::START), ARKUI_ITEM_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::CENTER), ARKUI_ITEM_ALIGNMENT_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::END), ARKUI_ITEM_ALIGNMENT_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::STRETCH), ARKUI_ITEM_ALIGNMENT_STRETCH);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment::BASELINE), ARKUI_ITEM_ALIGNMENT_BASELINE);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIVerticalAlignment(A2UIVerticalAlignment::TOP), ARKUI_VERTICAL_ALIGNMENT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVerticalAlignment(A2UIVerticalAlignment::CENTER),
        ARKUI_VERTICAL_ALIGNMENT_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVerticalAlignment(A2UIVerticalAlignment::BOTTOM),
        ARKUI_VERTICAL_ALIGNMENT_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUISliderStyle(A2UISliderStyle::OUT_SET), ARKUI_SLIDER_STYLE_OUT_SET);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVisibility(A2UIVisibility::VISIBLE), ARKUI_VISIBILITY_VISIBLE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVisibility(A2UIVisibility::HIDDEN), ARKUI_VISIBILITY_HIDDEN);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVisibility(A2UIVisibility::NONE), ARKUI_VISIBILITY_NONE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(A2UIScrollBarDisplayMode::OFF),
        ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(A2UIScrollBarDisplayMode::AUTO),
        ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(A2UIScrollBarDisplayMode::ON),
        ARKUI_SCROLL_BAR_DISPLAY_MODE_ON);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(A2UIScrollNestedMode::SELF_ONLY),
        ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(A2UIScrollNestedMode::SELF_FIRST),
        ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(A2UIScrollNestedMode::PARENT_FIRST),
        ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(A2UIScrollNestedMode::PARALLEL),
        ARKUI_SCROLL_NESTED_MODE_PARALLEL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIListItemAlignment(A2UIListItemAlignment::START),
        ARKUI_LIST_ITEM_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIListItemAlignment(A2UIListItemAlignment::CENTER),
        ARKUI_LIST_ITEM_ALIGNMENT_CENTER);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIListItemAlignment(A2UIListItemAlignment::END), ARKUI_LIST_ITEM_ALIGNMENT_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::TOP_START), ARKUI_ALIGNMENT_TOP_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::TOP), ARKUI_ALIGNMENT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::TOP_END), ARKUI_ALIGNMENT_TOP_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::START), ARKUI_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::CENTER), ARKUI_ALIGNMENT_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::END), ARKUI_ALIGNMENT_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::BOTTOM_START), ARKUI_ALIGNMENT_BOTTOM_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::BOTTOM), ARKUI_ALIGNMENT_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment::BOTTOM_END), ARKUI_ALIGNMENT_BOTTOM_END);
}

TEST(A2UIArkUITypeConverterTest, ShouldMapTextAndStyleEnumsExplicitly)
{
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::NORMAL), ARKUI_TEXTINPUT_TYPE_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::NUMBER), ARKUI_TEXTINPUT_TYPE_NUMBER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::PHONE_NUMBER),
        ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::EMAIL), ARKUI_TEXTINPUT_TYPE_EMAIL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::PASSWORD), ARKUI_TEXTINPUT_TYPE_PASSWORD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::NUMBER_PASSWORD),
        ARKUI_TEXTINPUT_TYPE_NUMBER_PASSWORD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::SCREEN_LOCK_PASSWORD),
        ARKUI_TEXTINPUT_TYPE_SCREEN_LOCK_PASSWORD);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::USER_NAME), ARKUI_TEXTINPUT_TYPE_USER_NAME);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::NEW_PASSWORD),
        ARKUI_TEXTINPUT_TYPE_NEW_PASSWORD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::NUMBER_DECIMAL),
        ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType::ONE_TIME_CODE),
        ARKUI_TEXTINPUT_TYPE_ONE_TIME_CODE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUICancelButtonStyle(A2UICancelButtonStyle::CONSTANT),
        ARKUI_CANCELBUTTON_STYLE_CONSTANT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUICancelButtonStyle(A2UICancelButtonStyle::INVISIBLE),
        ARKUI_CANCELBUTTON_STYLE_INVISIBLE);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUICancelButtonStyle(A2UICancelButtonStyle::INPUT), ARKUI_CANCELBUTTON_STYLE_INPUT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W100), ARKUI_FONT_WEIGHT_W100);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W200), ARKUI_FONT_WEIGHT_W200);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W300), ARKUI_FONT_WEIGHT_W300);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W400), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W500), ARKUI_FONT_WEIGHT_W500);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W600), ARKUI_FONT_WEIGHT_W600);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W700), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W800), ARKUI_FONT_WEIGHT_W800);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::W900), ARKUI_FONT_WEIGHT_W900);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::BOLD), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::NORMAL), ARKUI_FONT_WEIGHT_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::BOLDER), ARKUI_FONT_WEIGHT_BOLDER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::LIGHTER), ARKUI_FONT_WEIGHT_LIGHTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::MEDIUM), ARKUI_FONT_WEIGHT_MEDIUM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight::REGULAR), ARKUI_FONT_WEIGHT_REGULAR);
    EXPECT_EQ(
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIImageSize(A2UIImageSize::AUTO)), ARKUI_IMAGE_SIZE_AUTO);
    EXPECT_EQ(
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIImageSize(A2UIImageSize::COVER)), ARKUI_IMAGE_SIZE_COVER);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIImageSize(A2UIImageSize::CONTAIN)),
        ARKUI_IMAGE_SIZE_CONTAIN);
    EXPECT_EQ(static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIImageSize(A2UIImageSize::FILL)), 3);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::LEFT),
        ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::TOP),
        ARKUI_LINEAR_GRADIENT_DIRECTION_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::RIGHT),
        ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::BOTTOM),
        ARKUI_LINEAR_GRADIENT_DIRECTION_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::LEFT_TOP),
        ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::LEFT_BOTTOM),
        ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::RIGHT_TOP),
        ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::RIGHT_BOTTOM),
        ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::NONE),
        ARKUI_LINEAR_GRADIENT_DIRECTION_NONE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection::CUSTOM),
        ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowType(A2UIShadowType::COLOR), ARKUI_SHADOW_TYPE_COLOR);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowType(A2UIShadowType::BLUR), ARKUI_SHADOW_TYPE_BLUR);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_DEFAULT_XS),
        ARKUI_SHADOW_STYLE_OUTER_DEFAULT_XS);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_DEFAULT_SM),
        ARKUI_SHADOW_STYLE_OUTER_DEFAULT_SM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_DEFAULT_MD),
        ARKUI_SHADOW_STYLE_OUTER_DEFAULT_MD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_DEFAULT_LG),
        ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_FLOATING_SM),
        ARKUI_SHADOW_STYLE_OUTER_FLOATING_SM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle::OUTER_FLOATING_MD),
        ARKUI_SHADOW_STYLE_OUTER_FLOATING_MD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIWordBreak(A2UIWordBreak::NORMAL), ARKUI_WORD_BREAK_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIWordBreak(A2UIWordBreak::BREAK_ALL), ARKUI_WORD_BREAK_BREAK_ALL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIWordBreak(A2UIWordBreak::BREAK_WORD), ARKUI_WORD_BREAK_BREAK_WORD);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIWordBreak(A2UIWordBreak::HYPHENATION), ARKUI_WORD_BREAK_HYPHENATION);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUICheckboxShape(A2UICheckboxShape::CIRCLE), ArkUI_CHECKBOX_SHAPE_CIRCLE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUICheckboxShape(A2UICheckboxShape::ROUNDED_SQUARE),
        ArkUI_CHECKBOX_SHAPE_ROUNDED_SQUARE);
}

TEST(A2UIArkUITypeConverterTest, ShouldMapNodeEventAndAttributeEnumsExplicitly)
{
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::ON_CLICK), NODE_ON_CLICK);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::TEXT_INPUT_ON_CHANGE),
        NODE_TEXT_INPUT_ON_CHANGE);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::TEXT_AREA_ON_CHANGE), NODE_TEXT_AREA_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::ON_APPEAR), NODE_EVENT_ON_APPEAR);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::ON_CLICK_EVENT), NODE_ON_CLICK_EVENT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::TOGGLE_ON_CHANGE), NODE_TOGGLE_ON_CHANGE);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::RADIO_ON_CHANGE), NODE_RADIO_EVENT_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::CHECKBOX_ON_CHANGE),
        NODE_CHECKBOX_EVENT_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE),
        NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::SCROLL_ON_REACH_START),
        NODE_SCROLL_EVENT_ON_REACH_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType::SCROLL_ON_REACH_END),
        NODE_SCROLL_EVENT_ON_REACH_END);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_ON_CLICK), A2UINodeEventType::ON_CLICK);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_TEXT_INPUT_ON_CHANGE),
        A2UINodeEventType::TEXT_INPUT_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_TEXT_AREA_ON_CHANGE),
        A2UINodeEventType::TEXT_AREA_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_EVENT_ON_APPEAR), A2UINodeEventType::ON_APPEAR);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_ON_CLICK_EVENT), A2UINodeEventType::ON_CLICK_EVENT);
    EXPECT_EQ(
        A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_TOGGLE_ON_CHANGE), A2UINodeEventType::TOGGLE_ON_CHANGE);
    EXPECT_EQ(
        A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_RADIO_EVENT_ON_CHANGE), A2UINodeEventType::RADIO_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_CHECKBOX_EVENT_ON_CHANGE),
        A2UINodeEventType::CHECKBOX_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE),
        A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_SCROLL_EVENT_ON_REACH_START),
        A2UINodeEventType::SCROLL_ON_REACH_START);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(NODE_SCROLL_EVENT_ON_REACH_END),
        A2UINodeEventType::SCROLL_ON_REACH_END);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(NODE_ADAPTER_EVENT_WILL_ATTACH_TO_NODE),
        A2UINodeAdapterEventType::WILL_ATTACH_TO_NODE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(NODE_ADAPTER_EVENT_WILL_DETACH_FROM_NODE),
        A2UINodeAdapterEventType::WILL_DETACH_FROM_NODE);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(NODE_ADAPTER_EVENT_ON_GET_NODE_ID),
        A2UINodeAdapterEventType::ON_GET_NODE_ID);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER),
        A2UINodeAdapterEventType::ON_ADD_NODE_TO_ADAPTER);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(NODE_ADAPTER_EVENT_ON_REMOVE_NODE_FROM_ADAPTER),
        A2UINodeAdapterEventType::ON_REMOVE_NODE_FROM_ADAPTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::WIDTH), NODE_WIDTH);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::HEIGHT), NODE_HEIGHT);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::WIDTH_PERCENT), NODE_WIDTH_PERCENT);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::HEIGHT_PERCENT), NODE_HEIGHT_PERCENT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::LIST_NODE_ADAPTER),
        NODE_LIST_NODE_ADAPTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType::GRID_NODE_ADAPTER),
        NODE_GRID_NODE_ADAPTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::TOP_START), ARKUI_ALIGNMENT_TOP_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::TOP), ARKUI_ALIGNMENT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::TOP_END), ARKUI_ALIGNMENT_TOP_END);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::START), ARKUI_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::CENTER), ARKUI_ALIGNMENT_CENTER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::END), ARKUI_ALIGNMENT_END);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::BOTTOM_START), ARKUI_ALIGNMENT_BOTTOM_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::BOTTOM), ARKUI_ALIGNMENT_BOTTOM);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment::BOTTOM_END), ARKUI_ALIGNMENT_BOTTOM_END);
}

TEST(A2UIArkUITypeConverterTest, ShouldFallbackForUnknownEnumValues)
{
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeType(static_cast<A2UINodeType>(-1)), ARKUI_NODE_TEXT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIButtonType(static_cast<A2UIButtonType>(-1)), ARKUI_BUTTON_TYPE_NORMAL);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIFlexAlignment(static_cast<A2UIFlexAlignment>(-1)), ARKUI_FLEX_ALIGNMENT_START);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIFlexDirection(static_cast<A2UIFlexDirection>(-1)), ARKUI_FLEX_DIRECTION_ROW);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFlexWrap(static_cast<A2UIFlexWrap>(-1)), ARKUI_FLEX_WRAP_NO_WRAP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(static_cast<A2UIHorizontalAlignment>(-1)),
        ARKUI_HORIZONTAL_ALIGNMENT_START);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIItemAlignment(static_cast<A2UIItemAlignment>(-1)), ARKUI_ITEM_ALIGNMENT_AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVerticalAlignment(static_cast<A2UIVerticalAlignment>(-1)),
        ARKUI_VERTICAL_ALIGNMENT_TOP);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIObjectFit(static_cast<A2UIObjectFit>(-1)), ARKUI_OBJECT_FIT_COVER);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUIObjectFit(static_cast<ArkUI_ObjectFit>(-1)), A2UIObjectFit::COVER);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUISliderStyle(static_cast<A2UISliderStyle>(-1)), ARKUI_SLIDER_STYLE_OUT_SET);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUITextInputType(static_cast<A2UITextInputType>(-1)), ARKUI_TEXTINPUT_TYPE_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIVisibility(static_cast<A2UIVisibility>(-1)), ARKUI_VISIBILITY_VISIBLE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(static_cast<A2UIScrollBarDisplayMode>(-1)),
        ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(static_cast<A2UIScrollNestedMode>(-1)),
        ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeEventType(static_cast<A2UINodeEventType>(-1)), NODE_ON_CLICK);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeEventType(static_cast<ArkUI_NodeEventType>(-1)),
        A2UINodeEventType::ON_CLICK);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUICancelButtonStyle(static_cast<A2UICancelButtonStyle>(-1)),
        ARKUI_CANCELBUTTON_STYLE_INPUT);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAxis(static_cast<A2UIAxis>(-1)), ARKUI_AXIS_VERTICAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIListItemAlignment(static_cast<A2UIListItemAlignment>(-1)),
        ARKUI_LIST_ITEM_ALIGNMENT_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIAlignment(static_cast<A2UIAlignment>(-1)), ARKUI_ALIGNMENT_TOP_START);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIFontWeight(static_cast<A2UIFontWeight>(-1)), ARKUI_FONT_WEIGHT_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIImageSize(static_cast<A2UIImageSize>(-1)), ARKUI_IMAGE_SIZE_AUTO);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUILinearGradientDirection(static_cast<A2UILinearGradientDirection>(-1)),
        ARKUI_LINEAR_GRADIENT_DIRECTION_NONE);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowType(static_cast<A2UIShadowType>(-1)), ARKUI_SHADOW_TYPE_COLOR);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIShadowStyle(static_cast<A2UIShadowStyle>(-1)),
        ARKUI_SHADOW_STYLE_OUTER_DEFAULT_XS);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUIWordBreak(static_cast<A2UIWordBreak>(-1)), ARKUI_WORD_BREAK_NORMAL);
    EXPECT_EQ(A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(99), A2UINodeAdapterEventType::ON_GET_NODE_ID);
    EXPECT_EQ(A2UIArkUITypeConverter::ToArkUINodeAttributeType(static_cast<A2UINodeAttributeType>(-1)), NODE_WIDTH);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUIDialogAlignment(static_cast<A2UIAlignment>(-1)), ARKUI_ALIGNMENT_TOP_START);
    EXPECT_EQ(
        A2UIArkUITypeConverter::ToArkUICheckboxShape(static_cast<A2UICheckboxShape>(-1)), ArkUI_CHECKBOX_SHAPE_CIRCLE);
}

} // namespace
} // namespace NativeModule
