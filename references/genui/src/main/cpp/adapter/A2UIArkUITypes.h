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

#ifndef A2UI_ARKUI_TYPES_H
#define A2UI_ARKUI_TYPES_H

#include <arkui/native_dialog.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>

#include <cstddef>
#include <cstdint>

namespace NativeModule {

using ArkUI_NodeHandle = ::ArkUI_NodeHandle;
using A2UINodeContentHandle = ::ArkUI_NodeContentHandle;
using A2UINodeAdapterHandle = ::ArkUI_NodeAdapterHandle;
using A2UINativeDialogHandle = ::ArkUI_NativeDialogHandle;
using A2UINodeEvent = ::ArkUI_NodeEvent;
using A2UINodeAdapterEvent = ::ArkUI_NodeAdapterEvent;
using A2UIDialogDismissEvent = ::ArkUI_DialogDismissEvent;
using A2UIStringAsyncEvent = ::ArkUI_StringAsyncEvent;
using A2UINumberValue = ::ArkUI_NumberValue;
using A2UINodeComponentEvent = ::ArkUI_NodeComponentEvent;

constexpr std::size_t A2UI_MAX_COMPONENT_EVENT_ARG_NUM = MAX_COMPONENT_EVENT_ARG_NUM;
constexpr int32_t A2UI_ERROR_CODE_NO_ERROR = 0;

#ifdef ARKUI_IMAGE_SIZE_FILL
constexpr ArkUI_ImageSize A2UI_ARKUI_IMAGE_SIZE_FILL = ARKUI_IMAGE_SIZE_FILL;
#else
constexpr ArkUI_ImageSize A2UI_ARKUI_IMAGE_SIZE_FILL = static_cast<ArkUI_ImageSize>(3);
#endif

enum class A2UINodeType : int32_t {
    TEXT = 0,
    COLUMN = 1,
    ROW = 2,
    BUTTON = 3,
    IMAGE = 4,
    LIST = 5,
    LIST_ITEM = 6,
    TEXT_INPUT = 7,
    CHECKBOX = 8,
    SLIDER = 9,
    SCROLL = 10,
    STACK = 11,
    TEXT_AREA = 12,
    RADIO = 13,
    PROGRESS = 14,
    GRID = 15,
    GRID_ITEM = 16,
    TOGGLE = 17,
    FLEX = 18,
    CHECKBOX_GROUP = 21,
    DIVIDER = 1000,
};

enum class A2UIButtonType : int32_t {
    CAPSULE = 0,
    NORMAL = 1,
    CIRCLE = 2,
};

enum class A2UIFlexAlignment : int32_t {
    CENTER = 0,
    END = 1,
    SPACE_AROUND = 2,
    SPACE_BETWEEN = 3,
    SPACE_EVENLY = 4,
    START = 5,
};

enum class A2UIFlexDirection : int32_t {
    ROW = 0,
    COLUMN = 1,
    ROW_REVERSE = 2,
    COLUMN_REVERSE = 3,
};

enum class A2UIFlexWrap : int32_t {
    NO_WRAP = 0,
    WRAP = 1,
    WRAP_REVERSE = 2,
};

enum class A2UIHorizontalAlignment : int32_t {
    START = 0,
    CENTER = 1,
    END = 2,
};

enum class A2UIItemAlignment : int32_t {
    AUTO = 0,
    START = 1,
    CENTER = 2,
    END = 3,
    STRETCH = 4,
    BASELINE = 5,
};

enum class A2UIVerticalAlignment : int32_t {
    TOP = 0,
    CENTER = 1,
    BOTTOM = 2,
};

enum class A2UIObjectFit : int32_t {
    CONTAIN = 0,
    COVER = 1,
    FILL = 2,
    SCALE_DOWN = 3,
    NONE = 4,
    AUTO = 5,
    NONE_AND_ALIGN_TOP_START = 6,
    NONE_AND_ALIGN_TOP = 7,
    NONE_AND_ALIGN_TOP_END = 8,
    NONE_AND_ALIGN_START = 9,
    NONE_AND_ALIGN_CENTER = 10,
    NONE_AND_ALIGN_END = 11,
    NONE_AND_ALIGN_BOTTOM_START = 12,
    NONE_AND_ALIGN_BOTTOM = 13,
    NONE_AND_ALIGN_BOTTOM_END = 14,
    NONE_MATRIX = 15,
};

enum class A2UISliderStyle : int32_t {
    OUT_SET = 0,
};

enum class A2UITextInputType : int32_t {
    NORMAL = 0,
    NUMBER = 2,
    PHONE_NUMBER = 3,
    EMAIL = 5,
    PASSWORD = 7,
    NUMBER_PASSWORD = 8,
    SCREEN_LOCK_PASSWORD = 9,
    USER_NAME = 10,
    NEW_PASSWORD = 11,
    NUMBER_DECIMAL = 12,
    ONE_TIME_CODE = 14,
};

enum class A2UIVisibility : int32_t {
    VISIBLE = 0,
    HIDDEN = 1,
    NONE = 2,
};

enum class A2UIScrollBarDisplayMode : int32_t {
    OFF = 0,
    AUTO = 1,
    ON = 2,
};

enum class A2UIScrollNestedMode : int32_t {
    SELF_ONLY = 0,
    SELF_FIRST = 1,
    PARENT_FIRST = 2,
    PARALLEL = 3,
};

enum class A2UINodeEventType : int32_t {
    ON_CLICK = 100,
    TEXT_INPUT_ON_CHANGE = 101,
    TEXT_AREA_ON_CHANGE = 102,
    ON_APPEAR = 103,
    ON_AREA_CHANGE = 3,
    ON_SIZE_CHANGE = 30,
    ON_CLICK_EVENT = 110,
    TOGGLE_ON_CHANGE = 5004,
    RADIO_ON_CHANGE = 18000,
    CHECKBOX_ON_CHANGE = 19000,
    CHECKBOX_GROUP_ON_CHANGE = 20000,
    SCROLL_ON_REACH_START = 21000,
    SCROLL_ON_REACH_END = 21001,
};

enum class A2UICancelButtonStyle : int32_t {
    CONSTANT = 0,
    INVISIBLE = 1,
    INPUT = 2,
};

enum class A2UICheckboxShape : int32_t {
    CIRCLE = 0,
    ROUNDED_SQUARE = 1,
};

enum class A2UIAxis : int32_t {
    HORIZONTAL = 0,
    VERTICAL = 1,
};

enum class A2UIListItemAlignment : int32_t {
    START = 0,
    CENTER = 1,
    END = 2,
};

enum class A2UIAlignment : int32_t {
    TOP_START = 0,
    TOP = 1,
    TOP_END = 2,
    START = 3,
    CENTER = 4,
    END = 5,
    BOTTOM_START = 6,
    BOTTOM = 7,
    BOTTOM_END = 8,
};

enum class A2UIFontWeight : int32_t {
    W100 = 0,
    W200 = 1,
    W300 = 2,
    W400 = 3,
    W500 = 4,
    W600 = 5,
    W700 = 6,
    W800 = 7,
    W900 = 8,
    BOLD = 9,
    NORMAL = 10,
    BOLDER = 11,
    LIGHTER = 12,
    MEDIUM = 13,
    REGULAR = 14,
};

enum class A2UIImageSize : int32_t {
    AUTO = 0,
    COVER = 1,
    CONTAIN = 2,
    FILL = 3,
};

// Layout policy passed to the ArkUI NODE_WIDTH/HEIGHT_LAYOUTPOLICY attributes. Project-local mirror of the SDK
// enum ArkUI_LayoutPolicy; the actual ArkUI integer is resolved by name through ToArkUILayoutPolicy, so this enum's
// ordinals stay stable and decoupled from SDK renumbering.
enum class A2UILayoutPolicy : int32_t {
    MATCH_PARENT = 0,
    WRAP_CONTENT = 1,
    FIX_AT_IDEAL_SIZE = 2,
};

enum class A2UILinearGradientDirection : int32_t {
    LEFT = 0,
    TOP = 1,
    RIGHT = 2,
    BOTTOM = 3,
    LEFT_TOP = 4,
    LEFT_BOTTOM = 5,
    RIGHT_TOP = 6,
    RIGHT_BOTTOM = 7,
    NONE = 8,
    CUSTOM = 9,
};

enum class A2UIShadowType : int32_t {
    COLOR = 0,
    BLUR = 1,
};

enum class A2UIShadowStyle : int32_t {
    OUTER_DEFAULT_XS = 0,
    OUTER_DEFAULT_SM = 1,
    OUTER_DEFAULT_MD = 2,
    OUTER_DEFAULT_LG = 3,
    OUTER_FLOATING_SM = 4,
    OUTER_FLOATING_MD = 5,
};

enum class A2UIWordBreak : int32_t {
    NORMAL = 0,
    BREAK_ALL = 1,
    BREAK_WORD = 2,
    HYPHENATION = 3,
};

enum class A2UINodeAdapterEventType : int32_t {
    WILL_ATTACH_TO_NODE = 1,
    WILL_DETACH_FROM_NODE = 2,
    ON_GET_NODE_ID = 3,
    ON_ADD_NODE_TO_ADAPTER = 4,
    ON_REMOVE_NODE_FROM_ADAPTER = 5,
};

enum class A2UINodeAttributeType : int32_t {
    WIDTH = 0,
    HEIGHT = 1,
    WIDTH_PERCENT = 2,
    HEIGHT_PERCENT = 3,
    GRID_NODE_ADAPTER = 88,
    LIST_NODE_ADAPTER = 37,
};

} // namespace NativeModule

#endif // A2UI_ARKUI_TYPES_H
