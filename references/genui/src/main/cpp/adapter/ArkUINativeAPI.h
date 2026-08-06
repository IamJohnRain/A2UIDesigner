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

#ifndef A2UI_ARKUI_NATIVE_API_H
#define A2UI_ARKUI_NATIVE_API_H

#include <memory>

#include "IArkUINativeProvider.h"

namespace NativeModule {

class ArkUINativeAPI {
public:
    static ArkUINativeAPI& GetInstance();
    // ArkUI_NativeNodeAPI_1* GetNativeNodeAPI() const { return nativeNodeApi_; }

    IArkUINativeProvider* GetProvider() const
    {
        return provider_.get();
    }

    bool IsNativeAPIAvailable() const;
    ArkUI_NativeNodeAPI_1* GetNativeNodeAPI() const;

    int32_t GetNodeContentFromNapiValue(napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle);
    int32_t GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle);

    int32_t NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle);
    int32_t NodeContent_InsertNode(
        ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position);
    int32_t NodeContent_RemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle);

    ArkUI_NodeAdapterHandle NodeAdapter_Create();
    void NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle);
    int32_t NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count);
    int32_t NodeAdapter_RegisterEventReceiver(
        ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*));
    int32_t NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle);

    void* NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event);
    uint32_t NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event);
    uint32_t NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event);
    ArkUI_NodeHandle NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event);
    int32_t NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId);
    int32_t NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item);
    int32_t NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle);

    ArkUI_NodeHandle NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event);
    ArkUI_NodeEventType NodeEvent_GetEventType(const ArkUI_NodeEvent* event);
    ArkUI_NodeComponentEvent* NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event);
    ArkUI_StringAsyncEvent* NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event);

    ArkUI_NativeDialogAPI_1* GetNativeDialogAPI();
    void* DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event);
    void DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock);

    int32_t SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled);
    int32_t GetNodeUniqueId(ArkUI_NodeHandle node) const;

    static void SetProvider(std::unique_ptr<IArkUINativeProvider> provider);

private:
    ArkUINativeAPI();
    ~ArkUINativeAPI() = default;
    ArkUINativeAPI(const ArkUINativeAPI&) = delete;
    ArkUINativeAPI& operator=(const ArkUINativeAPI&) = delete;

    std::unique_ptr<IArkUINativeProvider> provider_;
};

} // namespace NativeModule

#endif // A2UI_ARKUI_NATIVE_API_H
