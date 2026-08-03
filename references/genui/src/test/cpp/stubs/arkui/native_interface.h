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

#ifndef ARKUI_NATIVE_INTERFACE_H
#define ARKUI_NATIVE_INTERFACE_H

#include "../js_native_api.h"
#include "native_node.h"

typedef int32_t ArkUI_ModuleType;
typedef int32_t ArkUI_ErrorCode;
typedef int32_t ArkUI_Version;

enum {
    ARKUI_NATIVE_NODE = 1,
    ARKUI_NATIVE_DIALOG = 2,
};

enum {
    ARKUI_ERROR_CODE_NO_ERROR = 0,
};

#ifdef __cplusplus
extern "C" {
#endif

int32_t OH_ArkUI_GetModuleInterfaceByType(ArkUI_ModuleType type, int32_t version, void** result);

#ifdef __cplusplus
}
#endif

#define OH_ArkUI_GetModuleInterface(type, ApiType, api)            \
    do {                                                           \
        ApiType* _tmp = nullptr;                                   \
        OH_ArkUI_GetModuleInterfaceByType(type, 1, (void**)&_tmp); \
        api = _tmp;                                                \
    } while (0)

#endif
