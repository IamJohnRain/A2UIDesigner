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

#ifndef A2UI_I_ARKUI_NATIVE_PROVIDER_H
#define A2UI_I_ARKUI_NATIVE_PROVIDER_H

#include <arkui/native_dialog.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <cstdint>
#include <js_native_api.h>

namespace NativeModule {

class IArkUINativeProvider {
public:
    virtual ~IArkUINativeProvider() = default;

    virtual ArkUI_NativeNodeAPI_1* GetNativeNodeAPI() = 0;

    virtual int32_t GetNodeContentFromNapiValue(
        napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle) = 0;
    virtual int32_t GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle) = 0;

    virtual int32_t NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle) = 0;
    virtual int32_t NodeContent_InsertNode(
        ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position) = 0;
    virtual int32_t NodeContent_RemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle) = 0;

    virtual ArkUI_NodeAdapterHandle NodeAdapter_Create() = 0;
    virtual void NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle) = 0;
    virtual int32_t NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count) = 0;
    virtual int32_t NodeAdapter_RegisterEventReceiver(
        ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*)) = 0;
    virtual int32_t NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle) = 0;

    virtual void* NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event) = 0;
    virtual uint32_t NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event) = 0;
    virtual uint32_t NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event) = 0;
    virtual ArkUI_NodeHandle NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event) = 0;
    virtual int32_t NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId) = 0;
    virtual int32_t NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item) = 0;

    virtual ArkUI_NodeHandle NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event) = 0;
    virtual ArkUI_NodeEventType NodeEvent_GetEventType(const ArkUI_NodeEvent* event) = 0;
    virtual ArkUI_NodeComponentEvent* NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event) = 0;
    virtual ArkUI_StringAsyncEvent* NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event) = 0;

    virtual ArkUI_NativeDialogAPI_1* GetNativeDialogAPI() = 0;
    virtual void* DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event) = 0;
    virtual void DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock) = 0;
    virtual int32_t NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle) = 0;
    virtual int32_t SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled) = 0;
    virtual int32_t GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId) = 0;
};

} // namespace NativeModule

#endif // A2UI_I_ARKUI_NATIVE_PROVIDER_H
