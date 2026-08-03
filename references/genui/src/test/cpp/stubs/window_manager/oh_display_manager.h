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

#ifndef WINDOW_MANAGER_OH_DISPLAY_MANAGER_H
#define WINDOW_MANAGER_OH_DISPLAY_MANAGER_H

#include <cstdint>

typedef int32_t NativeDisplayManager_ErrorCode;

enum {
    DISPLAY_MANAGER_OK = 0,
    DISPLAY_MANAGER_ERROR_INVALID_PARAM = 401,
    DISPLAY_MANAGER_ERROR_SYSTEM_ABNORMAL = 1300002,
};

#ifdef __cplusplus
extern "C" {
#endif

NativeDisplayManager_ErrorCode OH_NativeDisplayManager_GetDefaultDisplayDensityPixels(float* densityPixels);
NativeDisplayManager_ErrorCode OH_NativeDisplayManager_GetDefaultDisplayScaledDensity(float* scaledDensity);

#ifdef __cplusplus
}
#endif

#endif // WINDOW_MANAGER_OH_DISPLAY_MANAGER_H
