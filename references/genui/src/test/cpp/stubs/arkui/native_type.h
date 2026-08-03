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

#ifndef ARKUI_NATIVE_TYPE_H
#define ARKUI_NATIVE_TYPE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

struct ArkUI_Node {
    int dummy;
};
typedef struct ArkUI_Node* ArkUI_NodeHandle;
struct ArkUI_NodeContent {
    int dummy;
};
typedef struct ArkUI_NodeContent* ArkUI_NodeContentHandle;

typedef struct {
    const uint32_t* colors;
    float* stops;
    int size;
} ArkUI_ColorStop;

typedef enum {
    ARKUI_ALIGNMENT_TOP_START = 0,
    ARKUI_ALIGNMENT_TOP,
    ARKUI_ALIGNMENT_TOP_END,
    ARKUI_ALIGNMENT_START,
    ARKUI_ALIGNMENT_CENTER,
    ARKUI_ALIGNMENT_END,
    ARKUI_ALIGNMENT_BOTTOM_START,
    ARKUI_ALIGNMENT_BOTTOM,
    ARKUI_ALIGNMENT_BOTTOM_END,
} ArkUI_Alignment;

typedef enum {
    ARKUI_IMAGE_SIZE_AUTO = 0,
    ARKUI_IMAGE_SIZE_COVER = 1,
    ARKUI_IMAGE_SIZE_CONTAIN = 2,
} ArkUI_ImageSize;

typedef enum {
    ARKUI_FONT_WEIGHT_W100 = 0,
    ARKUI_FONT_WEIGHT_W200,
    ARKUI_FONT_WEIGHT_W300,
    ARKUI_FONT_WEIGHT_W400,
    ARKUI_FONT_WEIGHT_W500,
    ARKUI_FONT_WEIGHT_W600,
    ARKUI_FONT_WEIGHT_W700,
    ARKUI_FONT_WEIGHT_W800,
    ARKUI_FONT_WEIGHT_W900,
    ARKUI_FONT_WEIGHT_BOLD,
    ARKUI_FONT_WEIGHT_NORMAL,
    ARKUI_FONT_WEIGHT_BOLDER,
    ARKUI_FONT_WEIGHT_LIGHTER,
    ARKUI_FONT_WEIGHT_MEDIUM,
    ARKUI_FONT_WEIGHT_REGULAR,
} ArkUI_FontWeight;

typedef enum {
    ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT = 0,
    ARKUI_LINEAR_GRADIENT_DIRECTION_TOP = 1,
    ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT = 2,
    ARKUI_LINEAR_GRADIENT_DIRECTION_BOTTOM = 3,
    ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP = 4,
    ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM = 5,
    ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP = 6,
    ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM = 7,
    ARKUI_LINEAR_GRADIENT_DIRECTION_NONE = 8,
    ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM = 9,
} ArkUI_LinearGradientDirection;

typedef enum {
    ARKUI_SHADOW_TYPE_COLOR = 0,
    ARKUI_SHADOW_TYPE_BLUR = 1,
} ArkUI_ShadowType;

typedef enum {
    ARKUI_SHADOW_STYLE_OUTER_DEFAULT_XS = 0,
    ARKUI_SHADOW_STYLE_OUTER_DEFAULT_SM,
    ARKUI_SHADOW_STYLE_OUTER_DEFAULT_MD,
    ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG,
    ARKUI_SHADOW_STYLE_OUTER_FLOATING_SM,
    ARKUI_SHADOW_STYLE_OUTER_FLOATING_MD,
} ArkUI_ShadowStyle;

typedef enum {
    ARKUI_LAYOUTPOLICY_MATCHPARENT = 0,
    ARKUI_LAYOUTPOLICY_WRAPCONTENT,
    ARKUI_LAYOUTPOLICY_FIXATIDEALSIZE,
} ArkUI_LayoutPolicy;

#endif
