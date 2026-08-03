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

namespace NativeModule {

ArkUI_NodeType A2UIArkUITypeConverter::ToArkUINodeType(A2UINodeType type)
{
    switch (type) {
        case A2UINodeType::TEXT:
            return ARKUI_NODE_TEXT;
        case A2UINodeType::COLUMN:
            return ARKUI_NODE_COLUMN;
        case A2UINodeType::ROW:
            return ARKUI_NODE_ROW;
        case A2UINodeType::BUTTON:
            return ARKUI_NODE_BUTTON;
        case A2UINodeType::IMAGE:
            return ARKUI_NODE_IMAGE;
        case A2UINodeType::LIST:
            return ARKUI_NODE_LIST;
        case A2UINodeType::LIST_ITEM:
            return ARKUI_NODE_LIST_ITEM;
        case A2UINodeType::TEXT_INPUT:
            return ARKUI_NODE_TEXT_INPUT;
        case A2UINodeType::CHECKBOX:
            return ARKUI_NODE_CHECKBOX;
        case A2UINodeType::SLIDER:
            return ARKUI_NODE_SLIDER;
        case A2UINodeType::SCROLL:
            return ARKUI_NODE_SCROLL;
        case A2UINodeType::STACK:
            return ARKUI_NODE_STACK;
        case A2UINodeType::TEXT_AREA:
            return ARKUI_NODE_TEXT_AREA;
        case A2UINodeType::RADIO:
            return ARKUI_NODE_RADIO;
        case A2UINodeType::PROGRESS:
            return ARKUI_NODE_PROGRESS;
        case A2UINodeType::GRID:
            return ARKUI_NODE_GRID;
        case A2UINodeType::GRID_ITEM:
            return ARKUI_NODE_GRID_ITEM;
        case A2UINodeType::TOGGLE:
            return ARKUI_NODE_TOGGLE;
        case A2UINodeType::FLEX:
            return ARKUI_NODE_FLEX;
        case A2UINodeType::CHECKBOX_GROUP:
            return ARKUI_NODE_CHECKBOX_GROUP;
        case A2UINodeType::DIVIDER:
#ifdef ARKUI_NODE_DIVIDER
            return ARKUI_NODE_DIVIDER;
#else
            return ARKUI_NODE_ROW;
#endif
    }

    return ARKUI_NODE_TEXT;
}

ArkUI_ButtonType A2UIArkUITypeConverter::ToArkUIButtonType(A2UIButtonType type)
{
    switch (type) {
        case A2UIButtonType::NORMAL:
            return ARKUI_BUTTON_TYPE_NORMAL;
        case A2UIButtonType::CAPSULE:
            return ARKUI_BUTTON_TYPE_CAPSULE;
        case A2UIButtonType::CIRCLE:
            return ARKUI_BUTTON_TYPE_CIRCLE;
    }

    return ARKUI_BUTTON_TYPE_NORMAL;
}

ArkUI_FlexAlignment A2UIArkUITypeConverter::ToArkUIFlexAlignment(A2UIFlexAlignment value)
{
    switch (value) {
        case A2UIFlexAlignment::START:
            return ARKUI_FLEX_ALIGNMENT_START;
        case A2UIFlexAlignment::CENTER:
            return ARKUI_FLEX_ALIGNMENT_CENTER;
        case A2UIFlexAlignment::END:
            return ARKUI_FLEX_ALIGNMENT_END;
        case A2UIFlexAlignment::SPACE_BETWEEN:
            return ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN;
        case A2UIFlexAlignment::SPACE_AROUND:
            return ARKUI_FLEX_ALIGNMENT_SPACE_AROUND;
        case A2UIFlexAlignment::SPACE_EVENLY:
            return ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY;
    }

    return ARKUI_FLEX_ALIGNMENT_START;
}

ArkUI_FlexDirection A2UIArkUITypeConverter::ToArkUIFlexDirection(A2UIFlexDirection value)
{
    switch (value) {
        case A2UIFlexDirection::ROW:
            return ARKUI_FLEX_DIRECTION_ROW;
        case A2UIFlexDirection::COLUMN:
            return ARKUI_FLEX_DIRECTION_COLUMN;
        case A2UIFlexDirection::ROW_REVERSE:
            return ARKUI_FLEX_DIRECTION_ROW_REVERSE;
        case A2UIFlexDirection::COLUMN_REVERSE:
            return ARKUI_FLEX_DIRECTION_COLUMN_REVERSE;
    }

    return ARKUI_FLEX_DIRECTION_ROW;
}

ArkUI_FlexWrap A2UIArkUITypeConverter::ToArkUIFlexWrap(A2UIFlexWrap value)
{
    switch (value) {
        case A2UIFlexWrap::NO_WRAP:
            return ARKUI_FLEX_WRAP_NO_WRAP;
        case A2UIFlexWrap::WRAP:
            return ARKUI_FLEX_WRAP_WRAP;
        case A2UIFlexWrap::WRAP_REVERSE:
            return ARKUI_FLEX_WRAP_WRAP_REVERSE;
    }

    return ARKUI_FLEX_WRAP_NO_WRAP;
}

ArkUI_HorizontalAlignment A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(A2UIHorizontalAlignment value)
{
    switch (value) {
        case A2UIHorizontalAlignment::START:
            return ARKUI_HORIZONTAL_ALIGNMENT_START;
        case A2UIHorizontalAlignment::CENTER:
            return ARKUI_HORIZONTAL_ALIGNMENT_CENTER;
        case A2UIHorizontalAlignment::END:
            return ARKUI_HORIZONTAL_ALIGNMENT_END;
    }

    return ARKUI_HORIZONTAL_ALIGNMENT_START;
}

ArkUI_ItemAlignment A2UIArkUITypeConverter::ToArkUIItemAlignment(A2UIItemAlignment value)
{
    switch (value) {
        case A2UIItemAlignment::AUTO:
            return ARKUI_ITEM_ALIGNMENT_AUTO;
        case A2UIItemAlignment::START:
            return ARKUI_ITEM_ALIGNMENT_START;
        case A2UIItemAlignment::CENTER:
            return ARKUI_ITEM_ALIGNMENT_CENTER;
        case A2UIItemAlignment::END:
            return ARKUI_ITEM_ALIGNMENT_END;
        case A2UIItemAlignment::STRETCH:
            return ARKUI_ITEM_ALIGNMENT_STRETCH;
        case A2UIItemAlignment::BASELINE:
            return ARKUI_ITEM_ALIGNMENT_BASELINE;
    }

    return ARKUI_ITEM_ALIGNMENT_AUTO;
}

ArkUI_VerticalAlignment A2UIArkUITypeConverter::ToArkUIVerticalAlignment(A2UIVerticalAlignment value)
{
    switch (value) {
        case A2UIVerticalAlignment::TOP:
            return ARKUI_VERTICAL_ALIGNMENT_TOP;
        case A2UIVerticalAlignment::CENTER:
            return ARKUI_VERTICAL_ALIGNMENT_CENTER;
        case A2UIVerticalAlignment::BOTTOM:
            return ARKUI_VERTICAL_ALIGNMENT_BOTTOM;
    }

    return ARKUI_VERTICAL_ALIGNMENT_TOP;
}

ArkUI_ObjectFit A2UIArkUITypeConverter::ToArkUIObjectFit(A2UIObjectFit value)
{
    switch (value) {
        case A2UIObjectFit::CONTAIN:
            return ARKUI_OBJECT_FIT_CONTAIN;
        case A2UIObjectFit::COVER:
            return ARKUI_OBJECT_FIT_COVER;
        case A2UIObjectFit::AUTO:
            return ARKUI_OBJECT_FIT_AUTO;
        case A2UIObjectFit::FILL:
            return ARKUI_OBJECT_FIT_FILL;
        case A2UIObjectFit::SCALE_DOWN:
            return ARKUI_OBJECT_FIT_SCALE_DOWN;
        case A2UIObjectFit::NONE:
            return ARKUI_OBJECT_FIT_NONE;
        case A2UIObjectFit::NONE_AND_ALIGN_TOP_START:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START;
        case A2UIObjectFit::NONE_AND_ALIGN_TOP:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP;
        case A2UIObjectFit::NONE_AND_ALIGN_TOP_END:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_END;
        case A2UIObjectFit::NONE_AND_ALIGN_START:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_START;
        case A2UIObjectFit::NONE_AND_ALIGN_CENTER:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_CENTER;
        case A2UIObjectFit::NONE_AND_ALIGN_END:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_END;
        case A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_START;
        case A2UIObjectFit::NONE_AND_ALIGN_BOTTOM:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM;
        case A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END:
            return ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_END;
        case A2UIObjectFit::NONE_MATRIX:
            return ARKUI_OBJECT_FIT_NONE_MATRIX;
    }

    return ARKUI_OBJECT_FIT_COVER;
}

A2UIObjectFit A2UIArkUITypeConverter::FromArkUIObjectFit(ArkUI_ObjectFit value)
{
    switch (value) {
        case ARKUI_OBJECT_FIT_CONTAIN:
            return A2UIObjectFit::CONTAIN;
        case ARKUI_OBJECT_FIT_COVER:
            return A2UIObjectFit::COVER;
        case ARKUI_OBJECT_FIT_AUTO:
            return A2UIObjectFit::AUTO;
        case ARKUI_OBJECT_FIT_FILL:
            return A2UIObjectFit::FILL;
        case ARKUI_OBJECT_FIT_SCALE_DOWN:
            return A2UIObjectFit::SCALE_DOWN;
        case ARKUI_OBJECT_FIT_NONE:
            return A2UIObjectFit::NONE;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START:
            return A2UIObjectFit::NONE_AND_ALIGN_TOP_START;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP:
            return A2UIObjectFit::NONE_AND_ALIGN_TOP;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_END:
            return A2UIObjectFit::NONE_AND_ALIGN_TOP_END;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_START:
            return A2UIObjectFit::NONE_AND_ALIGN_START;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_CENTER:
            return A2UIObjectFit::NONE_AND_ALIGN_CENTER;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_END:
            return A2UIObjectFit::NONE_AND_ALIGN_END;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_START:
            return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM:
            return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM;
        case ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_END:
            return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END;
        case ARKUI_OBJECT_FIT_NONE_MATRIX:
            return A2UIObjectFit::NONE_MATRIX;
    }

    return A2UIObjectFit::COVER;
}

ArkUI_SliderStyle A2UIArkUITypeConverter::ToArkUISliderStyle(A2UISliderStyle value)
{
    switch (value) {
        case A2UISliderStyle::OUT_SET:
            return ARKUI_SLIDER_STYLE_OUT_SET;
    }

    return ARKUI_SLIDER_STYLE_OUT_SET;
}

ArkUI_TextInputType A2UIArkUITypeConverter::ToArkUITextInputType(A2UITextInputType value)
{
    switch (value) {
        case A2UITextInputType::NORMAL:
            return ARKUI_TEXTINPUT_TYPE_NORMAL;
        case A2UITextInputType::NUMBER:
            return ARKUI_TEXTINPUT_TYPE_NUMBER;
        case A2UITextInputType::PHONE_NUMBER:
            return ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER;
        case A2UITextInputType::EMAIL:
            return ARKUI_TEXTINPUT_TYPE_EMAIL;
        case A2UITextInputType::PASSWORD:
            return ARKUI_TEXTINPUT_TYPE_PASSWORD;
        case A2UITextInputType::NUMBER_PASSWORD:
            return ARKUI_TEXTINPUT_TYPE_NUMBER_PASSWORD;
        case A2UITextInputType::SCREEN_LOCK_PASSWORD:
            return ARKUI_TEXTINPUT_TYPE_SCREEN_LOCK_PASSWORD;
        case A2UITextInputType::USER_NAME:
            return ARKUI_TEXTINPUT_TYPE_USER_NAME;
        case A2UITextInputType::NEW_PASSWORD:
            return ARKUI_TEXTINPUT_TYPE_NEW_PASSWORD;
        case A2UITextInputType::NUMBER_DECIMAL:
            return ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL;
        case A2UITextInputType::ONE_TIME_CODE:
            return ARKUI_TEXTINPUT_TYPE_ONE_TIME_CODE;
    }

    return ARKUI_TEXTINPUT_TYPE_NORMAL;
}

ArkUI_Visibility A2UIArkUITypeConverter::ToArkUIVisibility(A2UIVisibility value)
{
    switch (value) {
        case A2UIVisibility::VISIBLE:
            return ARKUI_VISIBILITY_VISIBLE;
        case A2UIVisibility::HIDDEN:
            return ARKUI_VISIBILITY_HIDDEN;
        case A2UIVisibility::NONE:
            return ARKUI_VISIBILITY_NONE;
    }

    return ARKUI_VISIBILITY_VISIBLE;
}

ArkUI_ScrollBarDisplayMode A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(A2UIScrollBarDisplayMode value)
{
    switch (value) {
        case A2UIScrollBarDisplayMode::OFF:
            return ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF;
        case A2UIScrollBarDisplayMode::AUTO:
            return ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO;
        case A2UIScrollBarDisplayMode::ON:
            return ARKUI_SCROLL_BAR_DISPLAY_MODE_ON;
    }

    return ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO;
}

ArkUI_ScrollNestedMode A2UIArkUITypeConverter::ToArkUIScrollNestedMode(A2UIScrollNestedMode value)
{
    switch (value) {
        case A2UIScrollNestedMode::SELF_ONLY:
            return ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
        case A2UIScrollNestedMode::SELF_FIRST:
            return ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
        case A2UIScrollNestedMode::PARENT_FIRST:
            return ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST;
        case A2UIScrollNestedMode::PARALLEL:
            return ARKUI_SCROLL_NESTED_MODE_PARALLEL;
    }

    return ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
}

ArkUI_NodeEventType A2UIArkUITypeConverter::ToArkUINodeEventType(A2UINodeEventType value)
{
    switch (value) {
        case A2UINodeEventType::ON_CLICK:
            return NODE_ON_CLICK;
        case A2UINodeEventType::TEXT_INPUT_ON_CHANGE:
            return NODE_TEXT_INPUT_ON_CHANGE;
        case A2UINodeEventType::TEXT_AREA_ON_CHANGE:
            return NODE_TEXT_AREA_ON_CHANGE;
        case A2UINodeEventType::ON_APPEAR:
            return NODE_EVENT_ON_APPEAR;
        case A2UINodeEventType::ON_CLICK_EVENT:
            return NODE_ON_CLICK_EVENT;
        case A2UINodeEventType::TOGGLE_ON_CHANGE:
            return NODE_TOGGLE_ON_CHANGE;
        case A2UINodeEventType::RADIO_ON_CHANGE:
            return NODE_RADIO_EVENT_ON_CHANGE;
        case A2UINodeEventType::CHECKBOX_ON_CHANGE:
            return NODE_CHECKBOX_EVENT_ON_CHANGE;
        case A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE:
            return NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE;
        case A2UINodeEventType::SCROLL_ON_REACH_START:
            return NODE_SCROLL_EVENT_ON_REACH_START;
        case A2UINodeEventType::SCROLL_ON_REACH_END:
            return NODE_SCROLL_EVENT_ON_REACH_END;
    }

    return NODE_ON_CLICK;
}

A2UINodeEventType A2UIArkUITypeConverter::FromArkUINodeEventType(ArkUI_NodeEventType value)
{
    switch (value) {
        case NODE_ON_CLICK:
            return A2UINodeEventType::ON_CLICK;
        case NODE_TEXT_INPUT_ON_CHANGE:
            return A2UINodeEventType::TEXT_INPUT_ON_CHANGE;
        case NODE_TEXT_AREA_ON_CHANGE:
            return A2UINodeEventType::TEXT_AREA_ON_CHANGE;
        case NODE_EVENT_ON_APPEAR:
            return A2UINodeEventType::ON_APPEAR;
        case NODE_ON_CLICK_EVENT:
            return A2UINodeEventType::ON_CLICK_EVENT;
        case NODE_TOGGLE_ON_CHANGE:
            return A2UINodeEventType::TOGGLE_ON_CHANGE;
        case NODE_RADIO_EVENT_ON_CHANGE:
            return A2UINodeEventType::RADIO_ON_CHANGE;
        case NODE_CHECKBOX_EVENT_ON_CHANGE:
            return A2UINodeEventType::CHECKBOX_ON_CHANGE;
        case NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE:
            return A2UINodeEventType::CHECKBOX_GROUP_ON_CHANGE;
        case NODE_SCROLL_EVENT_ON_REACH_START:
            return A2UINodeEventType::SCROLL_ON_REACH_START;
        case NODE_SCROLL_EVENT_ON_REACH_END:
            return A2UINodeEventType::SCROLL_ON_REACH_END;
    }

    return A2UINodeEventType::ON_CLICK;
}

ArkUI_CancelButtonStyle A2UIArkUITypeConverter::ToArkUICancelButtonStyle(A2UICancelButtonStyle value)
{
    switch (value) {
        case A2UICancelButtonStyle::CONSTANT:
            return ARKUI_CANCELBUTTON_STYLE_CONSTANT;
        case A2UICancelButtonStyle::INVISIBLE:
            return ARKUI_CANCELBUTTON_STYLE_INVISIBLE;
        case A2UICancelButtonStyle::INPUT:
            return ARKUI_CANCELBUTTON_STYLE_INPUT;
    }

    return ARKUI_CANCELBUTTON_STYLE_INPUT;
}

ArkUI_Axis A2UIArkUITypeConverter::ToArkUIAxis(A2UIAxis value)
{
    switch (value) {
        case A2UIAxis::VERTICAL:
            return ARKUI_AXIS_VERTICAL;
        case A2UIAxis::HORIZONTAL:
            return ARKUI_AXIS_HORIZONTAL;
    }

    return ARKUI_AXIS_VERTICAL;
}

ArkUI_ListItemAlignment A2UIArkUITypeConverter::ToArkUIListItemAlignment(A2UIListItemAlignment value)
{
    switch (value) {
        case A2UIListItemAlignment::START:
            return ARKUI_LIST_ITEM_ALIGNMENT_START;
        case A2UIListItemAlignment::CENTER:
            return ARKUI_LIST_ITEM_ALIGNMENT_CENTER;
        case A2UIListItemAlignment::END:
            return ARKUI_LIST_ITEM_ALIGNMENT_END;
    }

    return ARKUI_LIST_ITEM_ALIGNMENT_START;
}

ArkUI_Alignment A2UIArkUITypeConverter::ToArkUIAlignment(A2UIAlignment value)
{
    switch (value) {
        case A2UIAlignment::TOP_START:
            return ARKUI_ALIGNMENT_TOP_START;
        case A2UIAlignment::TOP:
            return ARKUI_ALIGNMENT_TOP;
        case A2UIAlignment::TOP_END:
            return ARKUI_ALIGNMENT_TOP_END;
        case A2UIAlignment::START:
            return ARKUI_ALIGNMENT_START;
        case A2UIAlignment::CENTER:
            return ARKUI_ALIGNMENT_CENTER;
        case A2UIAlignment::END:
            return ARKUI_ALIGNMENT_END;
        case A2UIAlignment::BOTTOM_START:
            return ARKUI_ALIGNMENT_BOTTOM_START;
        case A2UIAlignment::BOTTOM:
            return ARKUI_ALIGNMENT_BOTTOM;
        case A2UIAlignment::BOTTOM_END:
            return ARKUI_ALIGNMENT_BOTTOM_END;
    }

    return ARKUI_ALIGNMENT_TOP_START;
}

ArkUI_FontWeight A2UIArkUITypeConverter::ToArkUIFontWeight(A2UIFontWeight value)
{
    switch (value) {
        case A2UIFontWeight::W100:
            return ARKUI_FONT_WEIGHT_W100;
        case A2UIFontWeight::W200:
            return ARKUI_FONT_WEIGHT_W200;
        case A2UIFontWeight::W300:
            return ARKUI_FONT_WEIGHT_W300;
        case A2UIFontWeight::W400:
            return ARKUI_FONT_WEIGHT_W400;
        case A2UIFontWeight::W500:
            return ARKUI_FONT_WEIGHT_W500;
        case A2UIFontWeight::W600:
            return ARKUI_FONT_WEIGHT_W600;
        case A2UIFontWeight::W700:
            return ARKUI_FONT_WEIGHT_W700;
        case A2UIFontWeight::W800:
            return ARKUI_FONT_WEIGHT_W800;
        case A2UIFontWeight::W900:
            return ARKUI_FONT_WEIGHT_W900;
        case A2UIFontWeight::BOLD:
            return ARKUI_FONT_WEIGHT_BOLD;
        case A2UIFontWeight::NORMAL:
            return ARKUI_FONT_WEIGHT_NORMAL;
        case A2UIFontWeight::BOLDER:
            return ARKUI_FONT_WEIGHT_BOLDER;
        case A2UIFontWeight::LIGHTER:
            return ARKUI_FONT_WEIGHT_LIGHTER;
        case A2UIFontWeight::MEDIUM:
            return ARKUI_FONT_WEIGHT_MEDIUM;
        case A2UIFontWeight::REGULAR:
            return ARKUI_FONT_WEIGHT_REGULAR;
    }

    return ARKUI_FONT_WEIGHT_NORMAL;
}

ArkUI_ImageSize A2UIArkUITypeConverter::ToArkUIImageSize(A2UIImageSize value)
{
    switch (value) {
        case A2UIImageSize::AUTO:
            return ARKUI_IMAGE_SIZE_AUTO;
        case A2UIImageSize::COVER:
            return ARKUI_IMAGE_SIZE_COVER;
        case A2UIImageSize::CONTAIN:
            return ARKUI_IMAGE_SIZE_CONTAIN;
        case A2UIImageSize::FILL:
            return A2UI_ARKUI_IMAGE_SIZE_FILL;
    }

    return ARKUI_IMAGE_SIZE_AUTO;
}

// Resolves the project layout-policy enum to the ArkUI NODE_*_LAYOUTPOLICY integer by SDK enumerator name, so the
// mapping tracks any SDK renumbering automatically.
int32_t A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy value)
{
    switch (value) {
        case A2UILayoutPolicy::MATCH_PARENT:
            return static_cast<int32_t>(ARKUI_LAYOUTPOLICY_MATCHPARENT);
        case A2UILayoutPolicy::WRAP_CONTENT:
            return static_cast<int32_t>(ARKUI_LAYOUTPOLICY_WRAPCONTENT);
        case A2UILayoutPolicy::FIX_AT_IDEAL_SIZE:
            return static_cast<int32_t>(ARKUI_LAYOUTPOLICY_FIXATIDEALSIZE);
    }

    return static_cast<int32_t>(ARKUI_LAYOUTPOLICY_WRAPCONTENT);
}

ArkUI_LinearGradientDirection A2UIArkUITypeConverter::ToArkUILinearGradientDirection(A2UILinearGradientDirection value)
{
    switch (value) {
        case A2UILinearGradientDirection::LEFT:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT;
        case A2UILinearGradientDirection::TOP:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_TOP;
        case A2UILinearGradientDirection::RIGHT:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT;
        case A2UILinearGradientDirection::BOTTOM:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_BOTTOM;
        case A2UILinearGradientDirection::LEFT_TOP:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP;
        case A2UILinearGradientDirection::LEFT_BOTTOM:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM;
        case A2UILinearGradientDirection::RIGHT_TOP:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP;
        case A2UILinearGradientDirection::RIGHT_BOTTOM:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM;
        case A2UILinearGradientDirection::NONE:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_NONE;
        case A2UILinearGradientDirection::CUSTOM:
            return ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM;
    }

    return ARKUI_LINEAR_GRADIENT_DIRECTION_NONE;
}

ArkUI_ShadowType A2UIArkUITypeConverter::ToArkUIShadowType(A2UIShadowType value)
{
    switch (value) {
        case A2UIShadowType::COLOR:
            return ARKUI_SHADOW_TYPE_COLOR;
        case A2UIShadowType::BLUR:
            return ARKUI_SHADOW_TYPE_BLUR;
    }

    return ARKUI_SHADOW_TYPE_COLOR;
}

ArkUI_ShadowStyle A2UIArkUITypeConverter::ToArkUIShadowStyle(A2UIShadowStyle value)
{
    switch (value) {
        case A2UIShadowStyle::OUTER_DEFAULT_XS:
            return ARKUI_SHADOW_STYLE_OUTER_DEFAULT_XS;
        case A2UIShadowStyle::OUTER_DEFAULT_SM:
            return ARKUI_SHADOW_STYLE_OUTER_DEFAULT_SM;
        case A2UIShadowStyle::OUTER_DEFAULT_MD:
            return ARKUI_SHADOW_STYLE_OUTER_DEFAULT_MD;
        case A2UIShadowStyle::OUTER_DEFAULT_LG:
            return ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG;
        case A2UIShadowStyle::OUTER_FLOATING_SM:
            return ARKUI_SHADOW_STYLE_OUTER_FLOATING_SM;
        case A2UIShadowStyle::OUTER_FLOATING_MD:
            return ARKUI_SHADOW_STYLE_OUTER_FLOATING_MD;
    }

    return ARKUI_SHADOW_STYLE_OUTER_DEFAULT_XS;
}

int32_t A2UIArkUITypeConverter::ToArkUIWordBreak(A2UIWordBreak value)
{
    switch (value) {
        case A2UIWordBreak::NORMAL:
            return ARKUI_WORD_BREAK_NORMAL;
        case A2UIWordBreak::BREAK_ALL:
            return ARKUI_WORD_BREAK_BREAK_ALL;
        case A2UIWordBreak::BREAK_WORD:
            return ARKUI_WORD_BREAK_BREAK_WORD;
        case A2UIWordBreak::HYPHENATION:
            return ARKUI_WORD_BREAK_HYPHENATION;
    }

    return ARKUI_WORD_BREAK_NORMAL;
}

A2UINodeAdapterEventType A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(uint32_t value)
{
    switch (value) {
        case NODE_ADAPTER_EVENT_WILL_ATTACH_TO_NODE:
            return A2UINodeAdapterEventType::WILL_ATTACH_TO_NODE;
        case NODE_ADAPTER_EVENT_WILL_DETACH_FROM_NODE:
            return A2UINodeAdapterEventType::WILL_DETACH_FROM_NODE;
        case NODE_ADAPTER_EVENT_ON_GET_NODE_ID:
            return A2UINodeAdapterEventType::ON_GET_NODE_ID;
        case NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER:
            return A2UINodeAdapterEventType::ON_ADD_NODE_TO_ADAPTER;
        case NODE_ADAPTER_EVENT_ON_REMOVE_NODE_FROM_ADAPTER:
            return A2UINodeAdapterEventType::ON_REMOVE_NODE_FROM_ADAPTER;
    }

    return A2UINodeAdapterEventType::ON_GET_NODE_ID;
}

ArkUI_NodeAttributeType A2UIArkUITypeConverter::ToArkUINodeAttributeType(A2UINodeAttributeType value)
{
    switch (value) {
        case A2UINodeAttributeType::WIDTH:
            return NODE_WIDTH;
        case A2UINodeAttributeType::HEIGHT:
            return NODE_HEIGHT;
        case A2UINodeAttributeType::WIDTH_PERCENT:
            return NODE_WIDTH_PERCENT;
        case A2UINodeAttributeType::HEIGHT_PERCENT:
            return NODE_HEIGHT_PERCENT;
        case A2UINodeAttributeType::GRID_NODE_ADAPTER:
            return NODE_GRID_NODE_ADAPTER;
        case A2UINodeAttributeType::LIST_NODE_ADAPTER:
            return NODE_LIST_NODE_ADAPTER;
    }

    return NODE_WIDTH;
}

int32_t A2UIArkUITypeConverter::ToArkUIDialogAlignment(A2UIAlignment value)
{
    switch (value) {
        case A2UIAlignment::TOP_START:
            return ARKUI_ALIGNMENT_TOP_START;
        case A2UIAlignment::TOP:
            return ARKUI_ALIGNMENT_TOP;
        case A2UIAlignment::TOP_END:
            return ARKUI_ALIGNMENT_TOP_END;
        case A2UIAlignment::START:
            return ARKUI_ALIGNMENT_START;
        case A2UIAlignment::CENTER:
            return ARKUI_ALIGNMENT_CENTER;
        case A2UIAlignment::END:
            return ARKUI_ALIGNMENT_END;
        case A2UIAlignment::BOTTOM_START:
            return ARKUI_ALIGNMENT_BOTTOM_START;
        case A2UIAlignment::BOTTOM:
            return ARKUI_ALIGNMENT_BOTTOM;
        case A2UIAlignment::BOTTOM_END:
            return ARKUI_ALIGNMENT_BOTTOM_END;
    }

    return ARKUI_ALIGNMENT_TOP_START;
}

int32_t A2UIArkUITypeConverter::ToArkUICheckboxShape(A2UICheckboxShape value)
{
    switch (value) {
        case A2UICheckboxShape::CIRCLE:
            return ArkUI_CHECKBOX_SHAPE_CIRCLE;
        case A2UICheckboxShape::ROUNDED_SQUARE:
            return ArkUI_CHECKBOX_SHAPE_ROUNDED_SQUARE;
    }

    return ArkUI_CHECKBOX_SHAPE_CIRCLE;
}

} // namespace NativeModule
