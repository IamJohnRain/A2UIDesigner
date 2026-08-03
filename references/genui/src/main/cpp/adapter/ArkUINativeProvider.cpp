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

#include "ArkUINativeProvider.h"

#include "ArkUINodeApiAdapter.h"
#include "ArkUIOHApiAdapter.h"

namespace NativeModule {

ArkUI_NativeNodeAPI_1* ArkUINativeProvider::GetNativeNodeAPI()
{
    return reinterpret_cast<ArkUI_NativeNodeAPI_1*>(ArkUINodeApiAdapter::GetNativeNodeAPI());
}

int32_t ArkUINativeProvider::GetNodeContentFromNapiValue(
    napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle)
{
    return ArkUIOHApiAdapter::GetNodeContentFromNapiValue(env, value, contentHandle);
}

int32_t ArkUINativeProvider::GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle)
{
    return ArkUIOHApiAdapter::GetNodeHandleFromNapiValue(env, value, handle);
}

int32_t ArkUINativeProvider::NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return ArkUIOHApiAdapter::NodeContentAddNode(contentHandle, nodeHandle);
}

int32_t ArkUINativeProvider::NodeContent_InsertNode(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position)
{
    return ArkUIOHApiAdapter::NodeContentInsertNode(contentHandle, nodeHandle, position);
}

int32_t ArkUINativeProvider::NodeContent_RemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return ArkUIOHApiAdapter::NodeContentRemoveNode(contentHandle, nodeHandle);
}

ArkUI_NodeAdapterHandle ArkUINativeProvider::NodeAdapter_Create()
{
    return ArkUIOHApiAdapter::NodeAdapterCreate();
}

void ArkUINativeProvider::NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle)
{
    ArkUIOHApiAdapter::NodeAdapterDispose(handle);
}

int32_t ArkUINativeProvider::NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count)
{
    return ArkUIOHApiAdapter::NodeAdapterSetTotalNodeCount(handle, count);
}

int32_t ArkUINativeProvider::NodeAdapter_RegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*))
{
    return ArkUIOHApiAdapter::NodeAdapterRegisterEventReceiver(handle, userData, callback);
}

int32_t ArkUINativeProvider::NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle)
{
    return ArkUIOHApiAdapter::NodeAdapterUnregisterEventReceiver(handle);
}

void* ArkUINativeProvider::NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event)
{
    return ArkUIOHApiAdapter::NodeAdapterEventGetUserData(event);
}

uint32_t ArkUINativeProvider::NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event)
{
    return static_cast<uint32_t>(ArkUIOHApiAdapter::NodeAdapterEventGetType(event));
}

uint32_t ArkUINativeProvider::NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event)
{
    return ArkUIOHApiAdapter::NodeAdapterEventGetItemIndex(event);
}

ArkUI_NodeHandle ArkUINativeProvider::NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event)
{
    return ArkUIOHApiAdapter::NodeAdapterEventGetRemovedNode(event);
}

int32_t ArkUINativeProvider::NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId)
{
    return ArkUIOHApiAdapter::NodeAdapterEventSetNodeId(event, nodeId);
}

int32_t ArkUINativeProvider::NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item)
{
    return ArkUIOHApiAdapter::NodeAdapterEventSetItem(event, item);
}

ArkUI_NodeHandle ArkUINativeProvider::NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event)
{
    return ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
}

ArkUI_NodeEventType ArkUINativeProvider::NodeEvent_GetEventType(const ArkUI_NodeEvent* event)
{
    return static_cast<ArkUI_NodeEventType>(ArkUIOHApiAdapter::NodeEventGetEventType(event));
}

ArkUI_NodeComponentEvent* ArkUINativeProvider::NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event)
{
    return ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
}

ArkUI_StringAsyncEvent* ArkUINativeProvider::NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event)
{
    return ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(event);
}

ArkUI_NativeDialogAPI_1* ArkUINativeProvider::GetNativeDialogAPI()
{
    return reinterpret_cast<ArkUI_NativeDialogAPI_1*>(ArkUINodeApiAdapter::GetNativeDialogAPI());
}

void* ArkUINativeProvider::DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event)
{
    return ArkUIOHApiAdapter::DialogDismissEventGetUserData(event);
}

void ArkUINativeProvider::DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock)
{
    ArkUIOHApiAdapter::DialogDismissEventSetShouldBlockDismiss(event, shouldBlock);
}

int32_t ArkUINativeProvider::NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle)
{
    return ArkUIOHApiAdapter::NodeAdapterReloadAllItems(handle);
}

int32_t ArkUINativeProvider::SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled)
{
    return ArkUIOHApiAdapter::SetCrossLanguageOption(node, enabled);
}

int32_t ArkUINativeProvider::GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId)
{
    return ArkUIOHApiAdapter::GetNodeUniqueId(node, uniqueId);
}

} // namespace NativeModule
