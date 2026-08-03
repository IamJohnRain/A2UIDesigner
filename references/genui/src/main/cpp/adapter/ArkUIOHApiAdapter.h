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

#ifndef A2UI_ARKUI_OH_API_ADAPTER_H
#define A2UI_ARKUI_OH_API_ADAPTER_H

#include <cstdint>
#include <js_native_api.h>
#include <window_manager/oh_display_manager.h>

#include "A2UIArkUITypes.h"

namespace NativeModule {

class ArkUIOHApiAdapter final {
public:
    static int32_t GetModuleInterfaceByType(int32_t type, int32_t version, void** result);
    static int32_t GetNodeContentFromNapiValue(napi_env env, napi_value value, A2UINodeContentHandle* contentHandle);
    static int32_t GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle);
    static int32_t NodeContentAddNode(A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle);
    static int32_t NodeContentInsertNode(
        A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position);
    static int32_t NodeContentRemoveNode(A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle);
    static A2UINodeAdapterHandle NodeAdapterCreate();
    static void NodeAdapterDispose(A2UINodeAdapterHandle handle);
    static int32_t NodeAdapterSetTotalNodeCount(A2UINodeAdapterHandle handle, uint32_t count);
    static int32_t NodeAdapterRegisterEventReceiver(
        A2UINodeAdapterHandle handle, void* userData, void (*callback)(A2UINodeAdapterEvent*));
    static int32_t NodeAdapterUnregisterEventReceiver(A2UINodeAdapterHandle handle);
    static void* NodeAdapterEventGetUserData(const A2UINodeAdapterEvent* event);
    static A2UINodeAdapterEventType NodeAdapterEventGetType(const A2UINodeAdapterEvent* event);
    static uint32_t NodeAdapterEventGetItemIndex(const A2UINodeAdapterEvent* event);
    static ArkUI_NodeHandle NodeAdapterEventGetRemovedNode(const A2UINodeAdapterEvent* event);
    static int32_t NodeAdapterEventSetNodeId(A2UINodeAdapterEvent* event, int32_t nodeId);
    static int32_t NodeAdapterEventSetItem(A2UINodeAdapterEvent* event, ArkUI_NodeHandle item);
    static ArkUI_NodeHandle NodeEventGetNodeHandle(const A2UINodeEvent* event);
    static A2UINodeEventType NodeEventGetEventType(const A2UINodeEvent* event);
    static A2UINodeComponentEvent* NodeEventGetNodeComponentEvent(const A2UINodeEvent* event);
    static A2UIStringAsyncEvent* NodeEventGetStringAsyncEvent(const A2UINodeEvent* event);
    static void* DialogDismissEventGetUserData(A2UIDialogDismissEvent* event);
    static void DialogDismissEventSetShouldBlockDismiss(A2UIDialogDismissEvent* event, bool shouldBlock);
    static int32_t NodeAdapterReloadAllItems(A2UINodeAdapterHandle handle);
    static int32_t SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled);
    static int32_t GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId);
    static NativeDisplayManager_ErrorCode GetDefaultDisplayDensityPixels(float* densityPixels);
    static NativeDisplayManager_ErrorCode GetDefaultDisplayScaledDensity(float* scaledDensity);
};

} // namespace NativeModule

#endif // A2UI_ARKUI_OH_API_ADAPTER_H
