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

#ifndef ARKUI_NATIVE_DIALOG_H
#define ARKUI_NATIVE_DIALOG_H

#include <stddef.h>
#include <stdint.h>

#include "native_type.h"

typedef struct ArkUI_DialogDismissEvent ArkUI_DialogDismissEvent;

typedef struct ArkUI_NativeDialog {
    int placeholder;
} ArkUI_NativeDialog;

typedef ArkUI_NativeDialog* ArkUI_NativeDialogHandle;

typedef int32_t ArkUI_DialogAlignment;

typedef struct {
    ArkUI_NativeDialogHandle (*create)(void);
    void (*dispose)(ArkUI_NativeDialogHandle handle);
    int32_t (*setContent)(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content);
    int32_t (*setContentAlignment)(
        ArkUI_NativeDialogHandle handle, ArkUI_DialogAlignment alignment, float offsetX, float offsetY);
    int32_t (*setModalMode)(ArkUI_NativeDialogHandle handle, bool modal);
    int32_t (*setAutoCancel)(ArkUI_NativeDialogHandle handle, bool autoCancel);
    int32_t (*registerOnWillDismissWithUserData)(
        ArkUI_NativeDialogHandle handle, void* userData, void (*callback)(ArkUI_DialogDismissEvent* event));
    int32_t (*show)(ArkUI_NativeDialogHandle handle, bool showInSubWindow);
    int32_t (*close)(ArkUI_NativeDialogHandle handle);
    int32_t (*enableCustomStyle)(ArkUI_NativeDialogHandle handle, bool enable);
} ArkUI_NativeDialogAPI_1;

#ifdef __cplusplus
extern "C" {
#endif

void* OH_ArkUI_DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event);
void OH_ArkUI_DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock);
ArkUI_NativeDialogAPI_1* GetNativeDialogApi(void);

#ifdef __cplusplus
}
#endif

#endif
