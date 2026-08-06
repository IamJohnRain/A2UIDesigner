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

#include "ArkUINativeAPI.h"
#ifndef TDD_BUILD
#include "ArkUINativeProvider.h"
#endif

namespace NativeModule {

ArkUINativeAPI::ArkUINativeAPI()
{
#ifndef TDD_BUILD
    provider_ = std::make_unique<ArkUINativeProvider>();
#endif
}

ArkUINativeAPI& ArkUINativeAPI::GetInstance()
{
    static ArkUINativeAPI instance;
    return instance;
}

void ArkUINativeAPI::SetProvider(std::unique_ptr<IArkUINativeProvider> provider)
{
    GetInstance().provider_ = std::move(provider);
}

bool ArkUINativeAPI::IsNativeAPIAvailable() const
{
    return GetNativeNodeAPI() != nullptr;
}

ArkUI_NativeNodeAPI_1* ArkUINativeAPI::GetNativeNodeAPI() const
{
    return provider_ == nullptr ? nullptr : provider_->GetNativeNodeAPI();
}

int32_t ArkUINativeAPI::GetNodeContentFromNapiValue(
    napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle)
{
    return provider_->GetNodeContentFromNapiValue(env, value, contentHandle);
}

int32_t ArkUINativeAPI::GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle)
{
    return provider_->GetNodeHandleFromNapiValue(env, value, handle);
}

int32_t ArkUINativeAPI::NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return provider_->NodeContent_AddNode(contentHandle, nodeHandle);
}

int32_t ArkUINativeAPI::NodeContent_InsertNode(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position)
{
    return provider_->NodeContent_InsertNode(contentHandle, nodeHandle, position);
}

int32_t ArkUINativeAPI::NodeContent_RemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return provider_->NodeContent_RemoveNode(contentHandle, nodeHandle);
}

ArkUI_NodeAdapterHandle ArkUINativeAPI::NodeAdapter_Create()
{
    return provider_->NodeAdapter_Create();
}

void ArkUINativeAPI::NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle)
{
    provider_->NodeAdapter_Dispose(handle);
}

int32_t ArkUINativeAPI::NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count)
{
    return provider_->NodeAdapter_SetTotalNodeCount(handle, count);
}

int32_t ArkUINativeAPI::NodeAdapter_RegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*))
{
    return provider_->NodeAdapter_RegisterEventReceiver(handle, userData, callback);
}

int32_t ArkUINativeAPI::NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle)
{
    return provider_->NodeAdapter_UnregisterEventReceiver(handle);
}

void* ArkUINativeAPI::NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event)
{
    return provider_->NodeAdapterEvent_GetUserData(event);
}

uint32_t ArkUINativeAPI::NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event)
{
    return provider_->NodeAdapterEvent_GetType(event);
}

uint32_t ArkUINativeAPI::NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event)
{
    return provider_->NodeAdapterEvent_GetItemIndex(event);
}

ArkUI_NodeHandle ArkUINativeAPI::NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event)
{
    return provider_->NodeAdapterEvent_GetRemovedNode(event);
}

int32_t ArkUINativeAPI::NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId)
{
    return provider_->NodeAdapterEvent_SetNodeId(event, nodeId);
}

int32_t ArkUINativeAPI::NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item)
{
    return provider_->NodeAdapterEvent_SetItem(event, item);
}

int32_t ArkUINativeAPI::NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle)
{
    return provider_->NodeAdapter_ReloadAllItems(handle);
}

ArkUI_NodeHandle ArkUINativeAPI::NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event)
{
    return provider_->NodeEvent_GetNodeHandle(event);
}

ArkUI_NodeEventType ArkUINativeAPI::NodeEvent_GetEventType(const ArkUI_NodeEvent* event)
{
    return provider_->NodeEvent_GetEventType(event);
}

ArkUI_NodeComponentEvent* ArkUINativeAPI::NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event)
{
    return provider_->NodeEvent_GetNodeComponentEvent(event);
}

ArkUI_StringAsyncEvent* ArkUINativeAPI::NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event)
{
    return provider_->NodeEvent_GetStringAsyncEvent(event);
}

ArkUI_NativeDialogAPI_1* ArkUINativeAPI::GetNativeDialogAPI()
{
    return provider_ == nullptr ? nullptr : provider_->GetNativeDialogAPI();
}

void* ArkUINativeAPI::DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event)
{
    return provider_->DialogDismissEvent_GetUserData(event);
}

void ArkUINativeAPI::DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock)
{
    provider_->DialogDismissEvent_SetShouldBlockDismiss(event, shouldBlock);
}

int32_t ArkUINativeAPI::SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled)
{
    return provider_->SetCrossLanguageOption(node, enabled);
}

int32_t ArkUINativeAPI::GetNodeUniqueId(ArkUI_NodeHandle node) const
{
    int32_t uniqueId = -1;
    if (provider_ == nullptr || provider_->GetNodeUniqueId(node, &uniqueId) != 0) {
        return -1;
    }
    return uniqueId;
}

} // namespace NativeModule
