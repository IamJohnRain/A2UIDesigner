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

#ifndef ARKUI_NATIVE_NODE_H
#define ARKUI_NATIVE_NODE_H

#include <cstddef>
#include <cstdint>

#include "native_type.h"

typedef uint32_t ArkUI_NodeType;
typedef int32_t ArkUI_ButtonType;
typedef int32_t ArkUI_FlexAlignment;
typedef int32_t ArkUI_FlexDirection;
typedef int32_t ArkUI_FlexWrap;
typedef int32_t ArkUI_HorizontalAlignment;
typedef int32_t ArkUI_ItemAlignment;
typedef int32_t ArkUI_VerticalAlignment;
typedef int32_t ArkUI_ObjectFit;
typedef int32_t ArkUI_SliderStyle;
typedef int32_t ArkUI_TextInputType;
typedef int32_t ArkUI_TextAreaType;
typedef int32_t ArkUI_Visibility;
typedef int32_t ArkUI_NodeAttributeType;
typedef int32_t ArkUI_ScrollBarDisplayMode;
typedef int32_t ArkUI_ScrollNestedMode;
typedef int32_t ArkUI_NodeEventType;
typedef int32_t ArkUI_CancelButtonStyle;

struct ArkUI_NodeAdapter {
    int dummy;
};
typedef struct ArkUI_NodeAdapter* ArkUI_NodeAdapterHandle;
struct ArkUI_NodeAdapterEvent {
    int dummy;
};
struct ArkUI_NodeEvent {
    int dummy;
};
struct ArkUI_StringAsyncEvent {
    char* pStr;
};

typedef union {
    float f32;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    const char* string;
    void* ptr;
} ArkUI_NumberValue;

#ifndef MAX_COMPONENT_EVENT_ARG_NUM
#define MAX_COMPONENT_EVENT_ARG_NUM 12
#endif

struct ArkUI_NodeComponentEvent {
    ArkUI_NumberValue data[MAX_COMPONENT_EVENT_ARG_NUM];
};

typedef struct {
    ArkUI_NumberValue* value;
    int32_t size;
    const char* string;
    void* object;
} ArkUI_AttributeItem;

typedef void (*ArkUI_NodeEventCallback)(ArkUI_NodeEvent* event);
typedef void (*ArkUI_NodeAdapterEventCallback)(ArkUI_NodeAdapterEvent* event);

typedef struct {
    ArkUI_NodeHandle (*createNode)(ArkUI_NodeType type);
    void (*disposeNode)(ArkUI_NodeHandle node);
    int32_t (*addChild)(ArkUI_NodeHandle parent, ArkUI_NodeHandle child);
    int32_t (*removeChild)(ArkUI_NodeHandle parent, ArkUI_NodeHandle child);
    int32_t (*insertChildAt)(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index);
    int32_t (*setAttribute)(ArkUI_NodeHandle node, int32_t attribute, ArkUI_AttributeItem* item);
    int32_t (*resetAttribute)(ArkUI_NodeHandle node, int32_t attribute);
    int32_t (*setUserData)(ArkUI_NodeHandle node, void* userData);
    void* (*getUserData)(ArkUI_NodeHandle node);
    int32_t (*addNodeEventReceiver)(ArkUI_NodeHandle node, ArkUI_NodeEventCallback callback);
    int32_t (*removeNodeEventReceiver)(ArkUI_NodeHandle node, ArkUI_NodeEventCallback callback);
    int32_t (*registerNodeEvent)(ArkUI_NodeHandle node, int32_t eventType, int32_t eventId, void* userData);
    int32_t (*unregisterNodeEvent)(ArkUI_NodeHandle node, int32_t eventType);
} ArkUI_NativeNodeAPI_1;

enum {
    ARKUI_NODE_TEXT = 0,
    ARKUI_NODE_COLUMN = 1,
    ARKUI_NODE_ROW = 2,
    ARKUI_NODE_BUTTON = 3,
    ARKUI_NODE_IMAGE = 4,
    ARKUI_NODE_LIST = 5,
    ARKUI_NODE_LIST_ITEM = 6,
    ARKUI_NODE_TEXT_INPUT = 7,
    ARKUI_NODE_CHECKBOX = 8,
    ARKUI_NODE_SLIDER = 9,
    ARKUI_NODE_SCROLL = 10,
    ARKUI_NODE_STACK = 11,
    ARKUI_NODE_TEXT_AREA = 12,
    ARKUI_NODE_RADIO = 13,
    ARKUI_NODE_PROGRESS = 14,
    ARKUI_NODE_GRID = 15,
    ARKUI_NODE_GRID_ITEM = 16,
    ARKUI_NODE_TOGGLE = 17,
    ARKUI_NODE_FLEX = 18,
    ARKUI_NODE_CHECKBOX_GROUP = 21,
};

enum {
    NODE_WIDTH = 0,
    NODE_HEIGHT = 1,
    NODE_WIDTH_PERCENT = 2,
    NODE_HEIGHT_PERCENT = 3,
    NODE_BACKGROUND_COLOR = 4,
    NODE_PADDING = 5,
    NODE_MARGIN = 6,
    NODE_MARGIN_PERCENT = 77,
    NODE_BORDER_RADIUS = 7,
    NODE_BORDER_WIDTH = 8,
    NODE_BORDER_WIDTH_PERCENT = 78,
    NODE_BORDER_COLOR = 9,
    NODE_SHADOW = 10,
    NODE_CUSTOM_SHADOW = 11,
    NODE_ENABLED = 12,
    NODE_LAYOUT_WEIGHT = 13,
    NODE_ACCESSIBILITY_GROUP = 14,
    NODE_ACCESSIBILITY_TEXT = 15,
    NODE_ACCESSIBILITY_DESCRIPTION = 16,
    NODE_TEXT_CONTENT = 17,
    NODE_FONT_SIZE = 18,
    NODE_FONT_COLOR = 19,
    NODE_BUTTON_TYPE = 20,
    NODE_CHECKBOX_SELECT = 21,
    NODE_CHECKBOX_SELECT_COLOR = 22,
    NODE_CHECKBOX_SHAPE = 23,
    NODE_IMAGE_SRC = 24,
    NODE_IMAGE_OBJECT_FIT = 25,
    NODE_IMAGE_ALT = 26,
    NODE_TEXT_INPUT_TEXT = 27,
    NODE_TEXT_INPUT_TYPE = 28,
    NODE_TEXT_INPUT_NUMBER_OF_LINES = 29,
    NODE_TEXT_INPUT_SHOW_PASSWORD_ICON = 30,
    NODE_COLUMN_ALIGN_ITEMS = 31,
    NODE_COLUMN_JUSTIFY_CONTENT = 32,
    NODE_ROW_ALIGN_ITEMS = 33,
    NODE_ROW_JUSTIFY_CONTENT = 34,
    NODE_LIST_DIRECTION = 35,
    NODE_LIST_ALIGN_LIST_ITEM = 36,
    NODE_LIST_NODE_ADAPTER = 37,
    NODE_SLIDER_MIN_VALUE = 38,
    NODE_SLIDER_MAX_VALUE = 39,
    NODE_SLIDER_VALUE = 40,
    NODE_SLIDER_STEP = 41,
    NODE_SLIDER_STYLE = 42,
    NODE_CONSTRAINT_SIZE = 43,
    NODE_VISIBILITY = 44,
    NODE_TEXT_AREA_TEXT = 45,
    NODE_TEXT_AREA_TYPE = 46,
    NODE_TEXT_AREA_NUMBER_OF_LINES = 47,
    NODE_ID = 48,
    NODE_OPACITY = 49,
    NODE_TEXT_INPUT_PLACEHOLDER = 50,
    NODE_TEXT_INPUT_PLACEHOLDER_COLOR = 51,
    NODE_FONT_WEIGHT = 52,
    NODE_TEXT_MAX_LINES = 53,
    NODE_TEXT_OVERFLOW = 54,
    NODE_FLEX_SHRINK = 55,
    NODE_BACKGROUND_IMAGE = 56,
    NODE_CLIP = 57,
    NODE_TEXT_DECORATION = 58,
    NODE_TEXT_ALIGN = 59,
    NODE_LINEAR_GRADIENT = 60,
    NODE_BACKGROUND_IMAGE_SIZE = 61,
    NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE = 62,
    NODE_BORDER_RADIUS_PERCENT = 63,
    NODE_LINEAR_LAYOUT_SPACE = 64,
    NODE_RADIO_STYLE = 65,
    NODE_PADDING_PERCENT = 66,
    NODE_PROGRESS_VALUE = 71,
    NODE_PROGRESS_TOTAL = 72,
    NODE_PROGRESS_COLOR = 73,
    NODE_PROGRESS_TYPE = 74,
    NODE_LIST_SPACE = 77,
    NODE_SCROLL_BAR_DISPLAY_MODE = 78,
    NODE_SCROLL_NESTED_SCROLL = 79,
    NODE_LIST_LANES = 85,
    NODE_STACK_ALIGN_CONTENT = 80,
    NODE_GRID_COLUMN_TEMPLATE = 81,
    NODE_GRID_ROW_TEMPLATE = 82,
    NODE_GRID_COLUMN_GAP = 83,
    NODE_GRID_ROW_GAP = 84,
    NODE_GRID_ALIGN_ITEMS = 1013008,
    NODE_GRID_NODE_ADAPTER = 88,
    NODE_WIDTH_LAYOUTPOLICY = 105,
    NODE_HEIGHT_LAYOUTPOLICY = 106,
    NODE_FLEX_OPTION = 86,
    NODE_FLEX_SPACE = 87,
    NODE_TEXT_MAX_FONT_SIZE = 67,
    NODE_TEXT_MIN_FONT_SIZE = 68,
    NODE_TEXT_WORD_BREAK = 69,
    NODE_RADIO_CHECKED = 74,
    NODE_RADIO_VALUE = 75,
    NODE_RADIO_GROUP = 76,
    NODE_CHECKBOX_UNSELECT_COLOR = 8001,
    NODE_CHECKBOX_MARK = 8002,
    NODE_CHECKBOX_NAME = 8003,
    NODE_CHECKBOX_GROUP = 8004,
    NODE_CHECKBOX_GROUP_NAME = 21000,
    NODE_CHECKBOX_GROUP_SELECT_ALL = 21001,
    NODE_CHECKBOX_GROUP_SELECTED_COLOR = 21002,
    NODE_CHECKBOX_GROUP_UNSELECTED_COLOR = 21003,
    NODE_CHECKBOX_GROUP_MARK = 21004,
    NODE_CHECKBOX_GROUP_SHAPE = 21005,
    NODE_TEXT_INPUT_CARET_COLOR = 7002,
    NODE_TEXT_INPUT_SHOW_UNDERLINE = 7004,
    NODE_TEXT_INPUT_MAX_LENGTH = 7005,
    NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR = 7011,
    NODE_TEXT_INPUT_CANCEL_BUTTON = 7014,
    NODE_TEXT_INPUT_UNDERLINE_COLOR = 7016,
    NODE_TEXT_INPUT_WORD_BREAK = 7029,
    NODE_TOGGLE_SELECTED_COLOR = 5000,
    NODE_TOGGLE_SWITCH_POINT_COLOR = 5001,
    NODE_TOGGLE_VALUE = 5002,
    NODE_TOGGLE_UNSELECTED_COLOR = 5003,
    NODE_ASPECT_RATIO = 87,
    NODE_BUTTON_LABEL = 9000,
    NODE_BUTTON_MIN_FONT_SCALE = 9002,
    NODE_BUTTON_MAX_FONT_SCALE = 9003,
    NODE_ON_CLICK = 100,
    NODE_ON_CLICK_EVENT = 110,
    NODE_TOGGLE_ON_CHANGE = 5004,
    NODE_TEXT_INPUT_ON_CHANGE = 101,
    NODE_TEXT_AREA_ON_CHANGE = 102,
    NODE_EVENT_ON_APPEAR = 103,
    NODE_RADIO_EVENT_ON_CHANGE = 18000,
    NODE_CHECKBOX_EVENT_ON_CHANGE = 19000,
    NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE = 20000,
    NODE_SCROLL_EVENT_ON_REACH_START = 21000,
    NODE_SCROLL_EVENT_ON_REACH_END = 21001,
    NODE_SLIDER_SELECTED_COLOR = 48,
};

enum {
    ARKUI_BUTTON_TYPE_NORMAL = 0,
    ARKUI_BUTTON_TYPE_CAPSULE = 1,
    ARKUI_BUTTON_TYPE_CIRCLE = 2,
};

enum {
    ARKUI_FLEX_ALIGNMENT_START = 1,
    ARKUI_FLEX_ALIGNMENT_CENTER = 2,
    ARKUI_FLEX_ALIGNMENT_END = 3,
    ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN = 6,
    ARKUI_FLEX_ALIGNMENT_SPACE_AROUND = 7,
    ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY = 8,
};

enum {
    ARKUI_FLEX_DIRECTION_ROW = 0,
    ARKUI_FLEX_DIRECTION_COLUMN = 1,
    ARKUI_FLEX_DIRECTION_ROW_REVERSE = 2,
    ARKUI_FLEX_DIRECTION_COLUMN_REVERSE = 3,
};

enum {
    ARKUI_FLEX_WRAP_NO_WRAP = 0,
    ARKUI_FLEX_WRAP_WRAP = 1,
    ARKUI_FLEX_WRAP_WRAP_REVERSE = 2,
};

enum {
    ARKUI_ITEM_ALIGNMENT_AUTO = 0,
    ARKUI_ITEM_ALIGNMENT_START = 1,
    ARKUI_ITEM_ALIGNMENT_CENTER = 2,
    ARKUI_ITEM_ALIGNMENT_END = 3,
    ARKUI_ITEM_ALIGNMENT_STRETCH = 4,
    ARKUI_ITEM_ALIGNMENT_BASELINE = 5,
};

enum {
    ARKUI_HORIZONTAL_ALIGNMENT_START = 0,
    ARKUI_HORIZONTAL_ALIGNMENT_CENTER = 1,
    ARKUI_HORIZONTAL_ALIGNMENT_END = 2,
};

enum {
    ARKUI_VERTICAL_ALIGNMENT_TOP = 0,
    ARKUI_VERTICAL_ALIGNMENT_CENTER = 1,
    ARKUI_VERTICAL_ALIGNMENT_BOTTOM = 2,
};

enum {
    ARKUI_OBJECT_FIT_CONTAIN = 0,
    ARKUI_OBJECT_FIT_COVER = 1,
    ARKUI_OBJECT_FIT_AUTO = 2,
    ARKUI_OBJECT_FIT_FILL = 3,
    ARKUI_OBJECT_FIT_SCALE_DOWN = 4,
    ARKUI_OBJECT_FIT_NONE = 5,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_START = 6,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP = 7,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_TOP_END = 8,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_START = 9,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_CENTER = 10,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_END = 11,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_START = 12,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM = 13,
    ARKUI_OBJECT_FIT_NONE_AND_ALIGN_BOTTOM_END = 14,
    ARKUI_OBJECT_FIT_NONE_MATRIX = 15,
};

enum {
    ARKUI_WORD_BREAK_NORMAL = 0,
    ARKUI_WORD_BREAK_BREAK_ALL = 1,
    ARKUI_WORD_BREAK_BREAK_WORD = 2,
    ARKUI_WORD_BREAK_HYPHENATION = 3,
};

enum {
    ARKUI_TEXTINPUT_TYPE_NORMAL = 0,
    ARKUI_TEXTINPUT_TYPE_NUMBER = 2,
    ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER = 3,
    ARKUI_TEXTINPUT_TYPE_EMAIL = 5,
    ARKUI_TEXTINPUT_TYPE_PASSWORD = 7,
    ARKUI_TEXTINPUT_TYPE_NUMBER_PASSWORD = 8,
    ARKUI_TEXTINPUT_TYPE_SCREEN_LOCK_PASSWORD = 9,
    ARKUI_TEXTINPUT_TYPE_USER_NAME = 10,
    ARKUI_TEXTINPUT_TYPE_NEW_PASSWORD = 11,
    ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL = 12,
    ARKUI_TEXTINPUT_TYPE_ONE_TIME_CODE = 14,
};

enum {
    ARKUI_CANCELBUTTON_STYLE_CONSTANT = 0,
    ARKUI_CANCELBUTTON_STYLE_INVISIBLE = 1,
    ARKUI_CANCELBUTTON_STYLE_INPUT = 2,
};

enum {
    ARKUI_TEXTAREA_TYPE_NORMAL = 0,
    ARKUI_TEXTAREA_TYPE_NUMBER = 2,
};

enum {
    ARKUI_SLIDER_STYLE_OUT_SET = 0,
};

enum ArkUI_CheckboxShape {
    ArkUI_CHECKBOX_SHAPE_CIRCLE = 0,
    ArkUI_CHECKBOX_SHAPE_ROUNDED_SQUARE = 1,
};

typedef int32_t ArkUI_Axis;
typedef int32_t ArkUI_ListItemAlignment;

enum {
    ARKUI_AXIS_VERTICAL = 0,
    ARKUI_AXIS_HORIZONTAL = 1,
};

enum {
    ARKUI_LIST_ITEM_ALIGNMENT_START = 0,
    ARKUI_LIST_ITEM_ALIGNMENT_CENTER = 1,
    ARKUI_LIST_ITEM_ALIGNMENT_END = 2,
};

enum {
    ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF = 0,
    ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO = 1,
    ARKUI_SCROLL_BAR_DISPLAY_MODE_ON = 2,
};

enum {
    ARKUI_SCROLL_NESTED_MODE_SELF_ONLY = 0,
    ARKUI_SCROLL_NESTED_MODE_SELF_FIRST = 1,
    ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST = 2,
    ARKUI_SCROLL_NESTED_MODE_PARALLEL = 3,
};

enum {
    NODE_ADAPTER_EVENT_WILL_ATTACH_TO_NODE = 1,
    NODE_ADAPTER_EVENT_WILL_DETACH_FROM_NODE = 2,
    NODE_ADAPTER_EVENT_ON_GET_NODE_ID = 3,
    NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER = 4,
    NODE_ADAPTER_EVENT_ON_REMOVE_NODE_FROM_ADAPTER = 5,
};

enum {
    ARKUI_VISIBILITY_VISIBLE = 0,
    ARKUI_VISIBILITY_HIDDEN = 1,
    ARKUI_VISIBILITY_NONE = 2,
};

#ifdef __cplusplus
extern "C" {
#endif

ArkUI_NodeHandle OH_ArkUI_NodeEvent_GetNodeHandle(ArkUI_NodeEvent* event);
ArkUI_NodeEventType OH_ArkUI_NodeEvent_GetEventType(ArkUI_NodeEvent* event);
ArkUI_NodeComponentEvent* OH_ArkUI_NodeEvent_GetNodeComponentEvent(ArkUI_NodeEvent* event);
ArkUI_StringAsyncEvent* OH_ArkUI_NodeEvent_GetStringAsyncEvent(ArkUI_NodeEvent* event);

ArkUI_NodeAdapterHandle OH_ArkUI_NodeAdapter_Create(void);
void OH_ArkUI_NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle);
void OH_ArkUI_NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, int32_t size);
void OH_ArkUI_NodeAdapter_RegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, ArkUI_NodeAdapterEventCallback callback);
void OH_ArkUI_NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle);

void* OH_ArkUI_NodeAdapterEvent_GetUserData(ArkUI_NodeAdapterEvent* event);
int32_t OH_ArkUI_NodeAdapterEvent_GetType(ArkUI_NodeAdapterEvent* event);
int32_t OH_ArkUI_NodeAdapterEvent_GetItemIndex(ArkUI_NodeAdapterEvent* event);
int32_t OH_ArkUI_NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t id);
int32_t OH_ArkUI_NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle node);
ArkUI_NodeHandle OH_ArkUI_NodeAdapterEvent_GetRemovedNode(ArkUI_NodeAdapterEvent* event);

int32_t OH_ArkUI_NodeContent_AddNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node);
int32_t OH_ArkUI_NodeContent_RemoveNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node);
int32_t OH_ArkUI_NodeContent_InsertNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node, int32_t position);

#ifdef __cplusplus
}
#endif

#endif
