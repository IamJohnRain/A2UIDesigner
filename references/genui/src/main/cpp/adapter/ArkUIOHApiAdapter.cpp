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

#include "ArkUIOHApiAdapter.h"

#include "A2UIArkUITypeConverter.h"

#ifndef TDD_BUILD
#include "SharedLibrarySymbolLoader.h"
#endif

namespace NativeModule {

#ifndef TDD_BUILD
namespace {
using LibraryId = SharedLibraryId;
using SymbolSpec = SharedLibrarySymbolSpec;

constexpr int32_t MIN_SUPPORTED_ROM_API_VERSION = 0;

SymbolSpec MakeArkUISymbolSpec(const char* symbol)
{
    return { LibraryId::ARKUI, symbol, MIN_SUPPORTED_ROM_API_VERSION };
}

SymbolSpec MakeDisplaySymbolSpec(const char* symbol)
{
    return { LibraryId::DISPLAY, symbol, MIN_SUPPORTED_ROM_API_VERSION };
}

template<typename Function>
Function ResolveArkUISymbol(const char* symbol)
{
    if (symbol == nullptr) {
        return nullptr;
    }
    return SharedLibrarySymbolLoader::GetInstance().Resolve<Function>(MakeArkUISymbolSpec(symbol));
}

template<typename Function>
Function ResolveDisplaySymbol(const char* symbol)
{
    if (symbol == nullptr) {
        return nullptr;
    }
    return SharedLibrarySymbolLoader::GetInstance().Resolve<Function>(MakeDisplaySymbolSpec(symbol));
}
} // namespace
#endif

namespace {

ArkUI_NodeContentHandle ToArkUINodeContentHandle(A2UINodeContentHandle handle)
{
    return reinterpret_cast<ArkUI_NodeContentHandle>(handle);
}

A2UINodeContentHandle FromArkUINodeContentHandle(ArkUI_NodeContentHandle handle)
{
    return reinterpret_cast<A2UINodeContentHandle>(handle);
}

ArkUI_NodeAdapterHandle ToArkUINodeAdapterHandle(A2UINodeAdapterHandle handle)
{
    return reinterpret_cast<ArkUI_NodeAdapterHandle>(handle);
}

A2UINodeAdapterHandle FromArkUINodeAdapterHandle(ArkUI_NodeAdapterHandle handle)
{
    return reinterpret_cast<A2UINodeAdapterHandle>(handle);
}

ArkUI_NodeAdapterEvent* ToArkUINodeAdapterEvent(A2UINodeAdapterEvent* event)
{
    return reinterpret_cast<ArkUI_NodeAdapterEvent*>(event);
}

const ArkUI_NodeAdapterEvent* ToArkUINodeAdapterEvent(const A2UINodeAdapterEvent* event)
{
    return reinterpret_cast<const ArkUI_NodeAdapterEvent*>(event);
}

ArkUI_NodeEvent* ToArkUINodeEvent(A2UINodeEvent* event)
{
    return reinterpret_cast<ArkUI_NodeEvent*>(event);
}

const ArkUI_NodeEvent* ToArkUINodeEvent(const A2UINodeEvent* event)
{
    return reinterpret_cast<const ArkUI_NodeEvent*>(event);
}

ArkUI_DialogDismissEvent* ToArkUIDialogDismissEvent(A2UIDialogDismissEvent* event)
{
    return reinterpret_cast<ArkUI_DialogDismissEvent*>(event);
}

} // namespace

int32_t ArkUIOHApiAdapter::GetModuleInterfaceByType(int32_t type, int32_t version, void** result)
{
    if (result == nullptr) {
        return -1;
    }
    *result = nullptr;
#ifdef TDD_BUILD
    return OH_ArkUI_GetModuleInterfaceByType(type, version, result);
#else
    using Function = void* (*)(ArkUI_NativeAPIVariantKind, const char*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_QueryModuleInterfaceByName");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    const char* structName = nullptr;
    if (type == ARKUI_NATIVE_NODE && version == 1) {
        structName = "ArkUI_NativeNodeAPI_1";
    } else if (type == ARKUI_NATIVE_DIALOG && version == 1) {
        structName = "ArkUI_NativeDialogAPI_1";
    }
    if (structName == nullptr) {
        return -1;
    }
    *result = function(static_cast<ArkUI_NativeAPIVariantKind>(type), structName);
    return *result != nullptr ? 0 : -1;
#endif
}

int32_t ArkUIOHApiAdapter::GetNodeContentFromNapiValue(
    napi_env env, napi_value value, A2UINodeContentHandle* contentHandle)
{
    ArkUI_NodeContentHandle rawHandle = nullptr;
#ifdef TDD_BUILD
    int32_t result = OH_ArkUI_GetNodeContentFromNapiValue(env, value, &rawHandle);
#else
    using Function = int32_t (*)(napi_env, napi_value, ArkUI_NodeContentHandle*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_GetNodeContentFromNapiValue");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    int32_t result = function(env, value, &rawHandle);
#endif
    if (contentHandle != nullptr) {
        *contentHandle = FromArkUINodeContentHandle(rawHandle);
    }
    return result;
}

int32_t ArkUIOHApiAdapter::GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle)
{
#ifdef TDD_BUILD
    return OH_ArkUI_GetNodeHandleFromNapiValue(env, value, handle);
#else
    using Function = int32_t (*)(napi_env, napi_value, ArkUI_NodeHandle*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_GetNodeHandleFromNapiValue");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(env, value, handle);
#endif
}

int32_t ArkUIOHApiAdapter::NodeContentAddNode(A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeContent_AddNode(ToArkUINodeContentHandle(contentHandle), nodeHandle);
#else
    using Function = int32_t (*)(ArkUI_NodeContentHandle, ArkUI_NodeHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeContent_AddNode");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeContentHandle(contentHandle), nodeHandle);
#endif
}

int32_t ArkUIOHApiAdapter::NodeContentInsertNode(
    A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeContent_InsertNode(ToArkUINodeContentHandle(contentHandle), nodeHandle, position);
#else
    using Function = int32_t (*)(ArkUI_NodeContentHandle, ArkUI_NodeHandle, int32_t);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeContent_InsertNode");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeContentHandle(contentHandle), nodeHandle, position);
#endif
}

int32_t ArkUIOHApiAdapter::NodeContentRemoveNode(A2UINodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeContent_RemoveNode(ToArkUINodeContentHandle(contentHandle), nodeHandle);
#else
    using Function = int32_t (*)(ArkUI_NodeContentHandle, ArkUI_NodeHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeContent_RemoveNode");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeContentHandle(contentHandle), nodeHandle);
#endif
}

A2UINodeAdapterHandle ArkUIOHApiAdapter::NodeAdapterCreate()
{
#ifdef TDD_BUILD
    return FromArkUINodeAdapterHandle(OH_ArkUI_NodeAdapter_Create());
#else
    using Function = ArkUI_NodeAdapterHandle (*)();
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_Create");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return FromArkUINodeAdapterHandle(function());
#endif
}

void ArkUIOHApiAdapter::NodeAdapterDispose(A2UINodeAdapterHandle handle)
{
#ifdef TDD_BUILD
    OH_ArkUI_NodeAdapter_Dispose(ToArkUINodeAdapterHandle(handle));
#else
    using Function = void (*)(ArkUI_NodeAdapterHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_Dispose");
        probed = true;
    }
    if (function != nullptr) {
        function(ToArkUINodeAdapterHandle(handle));
    }
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterSetTotalNodeCount(A2UINodeAdapterHandle handle, uint32_t count)
{
#ifdef TDD_BUILD
    OH_ArkUI_NodeAdapter_SetTotalNodeCount(ToArkUINodeAdapterHandle(handle), static_cast<int32_t>(count));
    return 0;
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterHandle, uint32_t);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_SetTotalNodeCount");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeAdapterHandle(handle), count);
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterRegisterEventReceiver(
    A2UINodeAdapterHandle handle, void* userData, void (*callback)(A2UINodeAdapterEvent*))
{
#ifdef TDD_BUILD
    OH_ArkUI_NodeAdapter_RegisterEventReceiver(
        ToArkUINodeAdapterHandle(handle), userData, reinterpret_cast<ArkUI_NodeAdapterEventCallback>(callback));
    return 0;
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterHandle, void*, void (*)(ArkUI_NodeAdapterEvent*));
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_RegisterEventReceiver");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(
        ToArkUINodeAdapterHandle(handle), userData, reinterpret_cast<void (*)(ArkUI_NodeAdapterEvent*)>(callback));
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterUnregisterEventReceiver(A2UINodeAdapterHandle handle)
{
#ifdef TDD_BUILD
    OH_ArkUI_NodeAdapter_UnregisterEventReceiver(ToArkUINodeAdapterHandle(handle));
    return 0;
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_UnregisterEventReceiver");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeAdapterHandle(handle));
#endif
}

void* ArkUIOHApiAdapter::NodeAdapterEventGetUserData(const A2UINodeAdapterEvent* event)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeAdapterEvent_GetUserData(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)));
#else
    using Function = void* (*)(ArkUI_NodeAdapterEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_GetUserData");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return function(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)));
#endif
}

A2UINodeAdapterEventType ArkUIOHApiAdapter::NodeAdapterEventGetType(const A2UINodeAdapterEvent* event)
{
#ifdef TDD_BUILD
    return A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(static_cast<uint32_t>(
        OH_ArkUI_NodeAdapterEvent_GetType(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)))));
#else
    using Function = uint32_t (*)(ArkUI_NodeAdapterEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_GetType");
        probed = true;
    }
    if (function == nullptr) {
        return A2UINodeAdapterEventType::ON_GET_NODE_ID;
    }
    return A2UIArkUITypeConverter::FromArkUINodeAdapterEventType(
        function(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event))));
#endif
}

uint32_t ArkUIOHApiAdapter::NodeAdapterEventGetItemIndex(const A2UINodeAdapterEvent* event)
{
#ifdef TDD_BUILD
    return static_cast<uint32_t>(
        OH_ArkUI_NodeAdapterEvent_GetItemIndex(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event))));
#else
    using Function = uint32_t (*)(ArkUI_NodeAdapterEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_GetItemIndex");
        probed = true;
    }
    if (function == nullptr) {
        return 0;
    }
    return function(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)));
#endif
}

ArkUI_NodeHandle ArkUIOHApiAdapter::NodeAdapterEventGetRemovedNode(const A2UINodeAdapterEvent* event)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeAdapterEvent_GetRemovedNode(
        const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)));
#else
    using Function = ArkUI_NodeHandle (*)(ArkUI_NodeAdapterEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_GetRemovedNode");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return function(const_cast<ArkUI_NodeAdapterEvent*>(ToArkUINodeAdapterEvent(event)));
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterEventSetNodeId(A2UINodeAdapterEvent* event, int32_t nodeId)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeAdapterEvent_SetNodeId(ToArkUINodeAdapterEvent(event), nodeId);
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterEvent*, int32_t);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_SetNodeId");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeAdapterEvent(event), nodeId);
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterEventSetItem(A2UINodeAdapterEvent* event, ArkUI_NodeHandle item)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeAdapterEvent_SetItem(ToArkUINodeAdapterEvent(event), item);
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterEvent*, ArkUI_NodeHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapterEvent_SetItem");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeAdapterEvent(event), item);
#endif
}

ArkUI_NodeHandle ArkUIOHApiAdapter::NodeEventGetNodeHandle(const A2UINodeEvent* event)
{
#ifdef TDD_BUILD
    return OH_ArkUI_NodeEvent_GetNodeHandle(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event)));
#else
    using Function = ArkUI_NodeHandle (*)(ArkUI_NodeEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeEvent_GetNodeHandle");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return function(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event)));
#endif
}

A2UINodeEventType ArkUIOHApiAdapter::NodeEventGetEventType(const A2UINodeEvent* event)
{
#ifdef TDD_BUILD
    return A2UIArkUITypeConverter::FromArkUINodeEventType(
        OH_ArkUI_NodeEvent_GetEventType(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#else
    using Function = ArkUI_NodeEventType (*)(ArkUI_NodeEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeEvent_GetEventType");
        probed = true;
    }
    if (function == nullptr) {
        return A2UINodeEventType::ON_CLICK;
    }
    return A2UIArkUITypeConverter::FromArkUINodeEventType(
        function(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#endif
}

A2UINodeComponentEvent* ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(const A2UINodeEvent* event)
{
#ifdef TDD_BUILD
    return reinterpret_cast<A2UINodeComponentEvent*>(
        OH_ArkUI_NodeEvent_GetNodeComponentEvent(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#else
    using Function = ArkUI_NodeComponentEvent* (*)(ArkUI_NodeEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeEvent_GetNodeComponentEvent");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<A2UINodeComponentEvent*>(function(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#endif
}

A2UIStringAsyncEvent* ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(const A2UINodeEvent* event)
{
#ifdef TDD_BUILD
    return reinterpret_cast<A2UIStringAsyncEvent*>(
        OH_ArkUI_NodeEvent_GetStringAsyncEvent(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#else
    using Function = ArkUI_StringAsyncEvent* (*)(ArkUI_NodeEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeEvent_GetStringAsyncEvent");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<A2UIStringAsyncEvent*>(function(const_cast<ArkUI_NodeEvent*>(ToArkUINodeEvent(event))));
#endif
}

void* ArkUIOHApiAdapter::DialogDismissEventGetUserData(A2UIDialogDismissEvent* event)
{
#ifdef TDD_BUILD
    return OH_ArkUI_DialogDismissEvent_GetUserData(ToArkUIDialogDismissEvent(event));
#else
    using Function = void* (*)(ArkUI_DialogDismissEvent*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_DialogDismissEvent_GetUserData");
        probed = true;
    }
    if (function == nullptr) {
        return nullptr;
    }
    return function(ToArkUIDialogDismissEvent(event));
#endif
}

void ArkUIOHApiAdapter::DialogDismissEventSetShouldBlockDismiss(A2UIDialogDismissEvent* event, bool shouldBlock)
{
#ifdef TDD_BUILD
    OH_ArkUI_DialogDismissEvent_SetShouldBlockDismiss(ToArkUIDialogDismissEvent(event), shouldBlock);
#else
    using Function = void (*)(ArkUI_DialogDismissEvent*, bool);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_DialogDismissEvent_SetShouldBlockDismiss");
        probed = true;
    }
    if (function != nullptr) {
        function(ToArkUIDialogDismissEvent(event), shouldBlock);
    }
#endif
}

int32_t ArkUIOHApiAdapter::NodeAdapterReloadAllItems(A2UINodeAdapterHandle handle)
{
#ifdef TDD_BUILD
    (void)handle;
    return 0;
#else
    using Function = int32_t (*)(ArkUI_NodeAdapterHandle);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeAdapter_ReloadAllItems");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(ToArkUINodeAdapterHandle(handle));
#endif
}

int32_t ArkUIOHApiAdapter::SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled)
{
#ifdef TDD_BUILD
    (void)node;
    (void)enabled;
    return 0;
#else
    using CreateOptionFunction = void* (*)();
    using DestroyOptionFunction = void (*)(void*);
    using SetAttributeSettingStatusFunction = void (*)(void*, bool);
    using SetCrossLanguageOptionFunction = int32_t (*)(ArkUI_NodeHandle, void*);
    static CreateOptionFunction createOption = nullptr;
    static DestroyOptionFunction destroyOption = nullptr;
    static SetAttributeSettingStatusFunction setAttributeSettingStatus = nullptr;
    static SetCrossLanguageOptionFunction setCrossLanguageOption = nullptr;
    static bool probed = false;
    if (node == nullptr) {
        return -1;
    }
    if (!probed) {
        createOption = ResolveArkUISymbol<CreateOptionFunction>("OH_ArkUI_CrossLanguageOption_Create");
        destroyOption = ResolveArkUISymbol<DestroyOptionFunction>("OH_ArkUI_CrossLanguageOption_Destroy");
        setAttributeSettingStatus = ResolveArkUISymbol<SetAttributeSettingStatusFunction>(
            "OH_ArkUI_CrossLanguageOption_SetAttributeSettingStatus");
        setCrossLanguageOption =
            ResolveArkUISymbol<SetCrossLanguageOptionFunction>("OH_ArkUI_NodeUtils_SetCrossLanguageOption");
        probed = true;
    }
    if (createOption == nullptr || destroyOption == nullptr || setAttributeSettingStatus == nullptr ||
        setCrossLanguageOption == nullptr) {
        return -1;
    }
    void* option = createOption();
    if (option == nullptr) {
        return -1;
    }
    setAttributeSettingStatus(option, enabled);
    int32_t result = setCrossLanguageOption(node, option);
    destroyOption(option);
    return result;
#endif
}

int32_t ArkUIOHApiAdapter::GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId)
{
#ifdef TDD_BUILD
    (void)node;
    if (uniqueId != nullptr) {
        *uniqueId = -1;
    }
    return 0;
#else
    using Function = int32_t (*)(ArkUI_NodeHandle, int32_t*);
    static Function function = nullptr;
    static bool probed = false;
    if (node == nullptr || uniqueId == nullptr) {
        return -1;
    }
    *uniqueId = -1;
    if (!probed) {
        function = ResolveArkUISymbol<Function>("OH_ArkUI_NodeUtils_GetNodeUniqueId");
        probed = true;
    }
    if (function == nullptr) {
        return -1;
    }
    return function(node, uniqueId);
#endif
}

NativeDisplayManager_ErrorCode ArkUIOHApiAdapter::GetDefaultDisplayDensityPixels(float* densityPixels)
{
#ifdef TDD_BUILD
    return OH_NativeDisplayManager_GetDefaultDisplayDensityPixels(densityPixels);
#else
    using Function = NativeDisplayManager_ErrorCode (*)(float*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveDisplaySymbol<Function>("OH_NativeDisplayManager_GetDefaultDisplayDensityPixels");
        probed = true;
    }
    if (function == nullptr) {
        return DISPLAY_MANAGER_ERROR_SYSTEM_ABNORMAL;
    }
    return function(densityPixels);
#endif
}

NativeDisplayManager_ErrorCode ArkUIOHApiAdapter::GetDefaultDisplayScaledDensity(float* scaledDensity)
{
#ifdef TDD_BUILD
    return OH_NativeDisplayManager_GetDefaultDisplayScaledDensity(scaledDensity);
#else
    using Function = NativeDisplayManager_ErrorCode (*)(float*);
    static Function function = nullptr;
    static bool probed = false;
    if (!probed) {
        function = ResolveDisplaySymbol<Function>("OH_NativeDisplayManager_GetDefaultDisplayScaledDensity");
        probed = true;
    }
    if (function == nullptr) {
        return DISPLAY_MANAGER_ERROR_SYSTEM_ABNORMAL;
    }
    return function(scaledDensity);
#endif
}

} // namespace NativeModule
