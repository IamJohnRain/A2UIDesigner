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

#include "A2UIArkUITypeConverter.h"
#include "ArkUIOHApiAdapter.h"
#include "mock_arkui_native_provider.h"

namespace NativeModule {
namespace {

MockArkUINativeProvider& GetMockProvider()
{
    MockArkUINativeProvider* activeProvider = MockArkUINativeProvider::GetActiveInstance();
    return activeProvider != nullptr ? *activeProvider : MockArkUINativeProvider::GetInstance();
}

} // namespace

int32_t ArkUIOHApiAdapter::GetModuleInterfaceByType(int32_t type, int32_t version, void** result)
{
    if (result == nullptr) {
        return -1;
    }
    *result = nullptr;
    if (type == ARKUI_NATIVE_NODE && version == 1) {
        *result = GetMockProvider().GetNativeNodeAPI();
        return *result != nullptr ? 0 : -1;
    }
    if (type == ARKUI_NATIVE_DIALOG && version == 1) {
        *result = GetMockProvider().GetNativeDialogAPI();
        return *result != nullptr ? 0 : -1;
    }
    return -1;
}

int32_t ArkUIOHApiAdapter::GetNodeContentFromNapiValue(
    napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle)
{
    return GetMockProvider().GetNodeContentFromNapiValue(env, value, contentHandle);
}

int32_t ArkUIOHApiAdapter::GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle)
{
    return GetMockProvider().GetNodeHandleFromNapiValue(env, value, handle);
}

int32_t ArkUIOHApiAdapter::NodeContentAddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return GetMockProvider().NodeContent_AddNode(contentHandle, nodeHandle);
}

int32_t ArkUIOHApiAdapter::NodeContentInsertNode(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position)
{
    return GetMockProvider().NodeContent_InsertNode(contentHandle, nodeHandle, position);
}

int32_t ArkUIOHApiAdapter::NodeContentRemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    return GetMockProvider().NodeContent_RemoveNode(contentHandle, nodeHandle);
}

ArkUI_NodeAdapterHandle ArkUIOHApiAdapter::NodeAdapterCreate()
{
    return GetMockProvider().NodeAdapter_Create();
}

void ArkUIOHApiAdapter::NodeAdapterDispose(ArkUI_NodeAdapterHandle handle)
{
    GetMockProvider().NodeAdapter_Dispose(handle);
}

int32_t ArkUIOHApiAdapter::NodeAdapterSetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count)
{
    return GetMockProvider().NodeAdapter_SetTotalNodeCount(handle, count);
}

int32_t ArkUIOHApiAdapter::NodeAdapterRegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*))
{
    return GetMockProvider().NodeAdapter_RegisterEventReceiver(handle, userData, callback);
}

int32_t ArkUIOHApiAdapter::NodeAdapterUnregisterEventReceiver(ArkUI_NodeAdapterHandle handle)
{
    return GetMockProvider().NodeAdapter_UnregisterEventReceiver(handle);
}

void* ArkUIOHApiAdapter::NodeAdapterEventGetUserData(const ArkUI_NodeAdapterEvent* event)
{
    return GetMockProvider().NodeAdapterEvent_GetUserData(event);
}

A2UINodeAdapterEventType ArkUIOHApiAdapter::NodeAdapterEventGetType(const A2UINodeAdapterEvent* event)
{
    return A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(GetMockProvider().NodeAdapterEvent_GetType(event));
}

uint32_t ArkUIOHApiAdapter::NodeAdapterEventGetItemIndex(const ArkUI_NodeAdapterEvent* event)
{
    return GetMockProvider().NodeAdapterEvent_GetItemIndex(event);
}

ArkUI_NodeHandle ArkUIOHApiAdapter::NodeAdapterEventGetRemovedNode(const ArkUI_NodeAdapterEvent* event)
{
    return GetMockProvider().NodeAdapterEvent_GetRemovedNode(event);
}

int32_t ArkUIOHApiAdapter::NodeAdapterEventSetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId)
{
    return GetMockProvider().NodeAdapterEvent_SetNodeId(event, nodeId);
}

int32_t ArkUIOHApiAdapter::NodeAdapterEventSetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item)
{
    return GetMockProvider().NodeAdapterEvent_SetItem(event, item);
}

ArkUI_NodeHandle ArkUIOHApiAdapter::NodeEventGetNodeHandle(const ArkUI_NodeEvent* event)
{
    return GetMockProvider().NodeEvent_GetNodeHandle(event);
}

A2UINodeEventType ArkUIOHApiAdapter::NodeEventGetEventType(const A2UINodeEvent* event)
{
    return A2UIArkUITypeConverter::FromArkUINodeEventType(GetMockProvider().NodeEvent_GetEventType(event));
}

ArkUI_NodeComponentEvent* ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(const ArkUI_NodeEvent* event)
{
    return GetMockProvider().NodeEvent_GetNodeComponentEvent(event);
}

ArkUI_StringAsyncEvent* ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(const ArkUI_NodeEvent* event)
{
    return GetMockProvider().NodeEvent_GetStringAsyncEvent(event);
}

void* ArkUIOHApiAdapter::DialogDismissEventGetUserData(ArkUI_DialogDismissEvent* event)
{
    return GetMockProvider().DialogDismissEvent_GetUserData(event);
}

void ArkUIOHApiAdapter::DialogDismissEventSetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock)
{
    GetMockProvider().DialogDismissEvent_SetShouldBlockDismiss(event, shouldBlock);
}

int32_t ArkUIOHApiAdapter::NodeAdapterReloadAllItems(ArkUI_NodeAdapterHandle handle)
{
    return GetMockProvider().NodeAdapter_ReloadAllItems(handle);
}

int32_t ArkUIOHApiAdapter::SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled)
{
    return GetMockProvider().SetCrossLanguageOption(node, enabled);
}

int32_t ArkUIOHApiAdapter::GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId)
{
    return GetMockProvider().GetNodeUniqueId(node, uniqueId);
}

NativeDisplayManager_ErrorCode ArkUIOHApiAdapter::GetDefaultDisplayDensityPixels(float* densityPixels)
{
    return OH_NativeDisplayManager_GetDefaultDisplayDensityPixels(densityPixels);
}

NativeDisplayManager_ErrorCode ArkUIOHApiAdapter::GetDefaultDisplayScaledDensity(float* scaledDensity)
{
    return OH_NativeDisplayManager_GetDefaultDisplayScaledDensity(scaledDensity);
}

} // namespace NativeModule
