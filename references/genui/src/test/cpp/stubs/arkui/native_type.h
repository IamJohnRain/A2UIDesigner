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

struct ArkUI_LayoutConstraint {
    int32_t minWidth;
    int32_t maxWidth;
    int32_t minHeight;
    int32_t maxHeight;
    int32_t percentReferenceWidth;
    int32_t percentReferenceHeight;
};

typedef struct {
    int32_t width;
    int32_t height;
} ArkUI_IntSize;

typedef struct {
    int32_t x;
    int32_t y;
} ArkUI_IntOffset;

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

typedef enum {
    ARKUI_PIXELROUNDCALCPOLICY_NOFORCEROUND = 0,
    ARKUI_PIXELROUNDCALCPOLICY_FORCECEIL,
    ARKUI_PIXELROUNDCALCPOLICY_FORCEFLOOR,
} ArkUI_PixelRoundCalcPolicy;

typedef struct ArkUI_PixelRoundPolicy {
    int32_t start = ARKUI_PIXELROUNDCALCPOLICY_FORCECEIL;
    int32_t top = ARKUI_PIXELROUNDCALCPOLICY_FORCECEIL;
    int32_t end = ARKUI_PIXELROUNDCALCPOLICY_FORCECEIL;
    int32_t bottom = ARKUI_PIXELROUNDCALCPOLICY_FORCECEIL;
} ArkUI_PixelRoundPolicy;

// TDD inspection hooks: allow tests to inspect the last-created policy and
// simulate Create() failure. Dispose is a no-op so tests can read field
// values after the function under test returns.
inline ArkUI_PixelRoundPolicy*& TddLastPixelRoundPolicy()
{
    static ArkUI_PixelRoundPolicy* ptr = nullptr;
    return ptr;
}

inline bool& TddPixelRoundCreateShouldFail()
{
    static bool flag = false;
    return flag;
}

inline void TddResetPixelRoundPolicy()
{
    delete TddLastPixelRoundPolicy();
    TddLastPixelRoundPolicy() = nullptr;
    TddPixelRoundCreateShouldFail() = false;
}

inline ArkUI_PixelRoundPolicy* OH_ArkUI_PixelRoundPolicy_Create()
{
    if (TddPixelRoundCreateShouldFail()) {
        return nullptr;
    }
    delete TddLastPixelRoundPolicy();
    TddLastPixelRoundPolicy() = new ArkUI_PixelRoundPolicy();
    return TddLastPixelRoundPolicy();
}

inline void OH_ArkUI_PixelRoundPolicy_Dispose(ArkUI_PixelRoundPolicy* policy)
{
    // No-op: retain policy for post-call test inspection. TddResetPixelRoundPolicy cleans up.
}

inline void OH_ArkUI_PixelRoundPolicy_SetStart(ArkUI_PixelRoundPolicy* policy, ArkUI_PixelRoundCalcPolicy value)
{
    if (policy != nullptr) {
        policy->start = static_cast<int32_t>(value);
    }
}

inline void OH_ArkUI_PixelRoundPolicy_SetTop(ArkUI_PixelRoundPolicy* policy, ArkUI_PixelRoundCalcPolicy value)
{
    if (policy != nullptr) {
        policy->top = static_cast<int32_t>(value);
    }
}

inline void OH_ArkUI_PixelRoundPolicy_SetEnd(ArkUI_PixelRoundPolicy* policy, ArkUI_PixelRoundCalcPolicy value)
{
    if (policy != nullptr) {
        policy->end = static_cast<int32_t>(value);
    }
}

inline void OH_ArkUI_PixelRoundPolicy_SetBottom(ArkUI_PixelRoundPolicy* policy, ArkUI_PixelRoundCalcPolicy value)
{
    if (policy != nullptr) {
        policy->bottom = static_cast<int32_t>(value);
    }
}

#endif
