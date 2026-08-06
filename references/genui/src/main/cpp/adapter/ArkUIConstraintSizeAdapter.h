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

#ifndef A2UI_ARKUI_CONSTRAINT_SIZE_ADAPTER_H
#define A2UI_ARKUI_CONSTRAINT_SIZE_ADAPTER_H

#include <cfloat>

#include "A2UIArkUITypes.h"

namespace NativeModule {

struct A2UIConstraintDimension {
    float value = 0.0F;
    bool isPercent = false;
};

struct A2UIConstraintSizeSpec {
    A2UIConstraintDimension minWidth { 0.0F, false };
    A2UIConstraintDimension maxWidth { FLT_MAX, false };
    A2UIConstraintDimension minHeight { 0.0F, false };
    A2UIConstraintDimension maxHeight { FLT_MAX, false };
};

class ArkUIConstraintSizeAdapter final {
public:
    static int32_t SetPercentConstraintSize(ArkUI_NodeHandle node, const A2UIConstraintSizeSpec& spec);
    static void Clear(ArkUI_NodeHandle node);
    static void Dispose(ArkUI_NodeHandle node);
    static ArkUI_NodeHandle GetMountNode(ArkUI_NodeHandle node);
    static ArkUI_NodeHandle GetContentNode(ArkUI_NodeHandle node);
};

} // namespace NativeModule

#endif // A2UI_ARKUI_CONSTRAINT_SIZE_ADAPTER_H
