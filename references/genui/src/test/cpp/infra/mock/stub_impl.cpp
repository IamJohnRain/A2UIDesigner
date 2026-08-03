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

#include <arkui/native_dialog.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <arkui/native_node_napi.h>

#include <js_native_api.h>
#include <window_manager/oh_display_manager.h>

#include "ArkUINativeAPI.h"
#include "NapiBridge.h"

namespace {

float g_displayDensityPixels = 1.0F;
float g_displayScaledDensity = 1.0F;

} // namespace

#ifdef __cplusplus
extern "C" {
#endif

ArkUI_NodeHandle OH_ArkUI_NodeEvent_GetNodeHandle(ArkUI_NodeEvent* event)
{
    return nullptr;
}
ArkUI_NodeEventType OH_ArkUI_NodeEvent_GetEventType(ArkUI_NodeEvent* event)
{
    return static_cast<ArkUI_NodeEventType>(-1);
}
ArkUI_StringAsyncEvent* OH_ArkUI_NodeEvent_GetStringAsyncEvent(ArkUI_NodeEvent* event)
{
    return nullptr;
}

ArkUI_NodeAdapterHandle OH_ArkUI_NodeAdapter_Create(void)
{
    return nullptr;
}
void OH_ArkUI_NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle) {}
void OH_ArkUI_NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, int32_t size) {}
void OH_ArkUI_NodeAdapter_RegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, ArkUI_NodeAdapterEventCallback callback)
{}
void OH_ArkUI_NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle) {}

void* OH_ArkUI_NodeAdapterEvent_GetUserData(ArkUI_NodeAdapterEvent* event)
{
    return nullptr;
}
int32_t OH_ArkUI_NodeAdapterEvent_GetType(ArkUI_NodeAdapterEvent* event)
{
    return -1;
}
int32_t OH_ArkUI_NodeAdapterEvent_GetItemIndex(ArkUI_NodeAdapterEvent* event)
{
    return -1;
}
int32_t OH_ArkUI_NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t id)
{
    static_cast<void>(event);
    static_cast<void>(id);
    return 0;
}
int32_t OH_ArkUI_NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle node)
{
    static_cast<void>(event);
    static_cast<void>(node);
    return 0;
}
ArkUI_NodeHandle OH_ArkUI_NodeAdapterEvent_GetRemovedNode(ArkUI_NodeAdapterEvent* event)
{
    return nullptr;
}

int32_t OH_ArkUI_NodeContent_AddNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node)
{
    return -1;
}
int32_t OH_ArkUI_NodeContent_RemoveNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node)
{
    return -1;
}
int32_t OH_ArkUI_NodeContent_InsertNode(ArkUI_NodeContentHandle content, ArkUI_NodeHandle node, int32_t position)
{
    return -1;
}

int32_t OH_ArkUI_GetNodeContentFromNapiValue(napi_env env, napi_value value, ArkUI_NodeContentHandle* result)
{
    return NativeModule::ArkUINativeAPI::GetInstance().GetNodeContentFromNapiValue(env, value, result);
}

int32_t OH_ArkUI_GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* result)
{
    return NativeModule::ArkUINativeAPI::GetInstance().GetNodeHandleFromNapiValue(env, value, result);
}

napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetReferenceValue(env, ref, result);
}

napi_status napi_create_object(napi_env env, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateObject(env, result);
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateInt32(env, value, result);
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_generic_failure;
}

napi_status napi_create_double(napi_env env, double value, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateDouble(env, value, result);
}

napi_status napi_create_string_utf8(napi_env env, const char* str, size_t length, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateStringUtf8(env, str, length, result);
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetBoolean(env, value, result);
}

napi_status napi_get_undefined(napi_env env, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetUndefined(env, result);
}

napi_status napi_get_global(napi_env env, napi_value* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_generic_failure;
}

napi_status napi_call_function(
    napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CallFunction(env, recv, func, argc, argv, result);
}

napi_status napi_open_handle_scope(napi_env env, napi_handle_scope* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().OpenHandleScope(env, result);
}

napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CloseHandleScope(env, scope);
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateArrayWithLength(env, length, result);
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetPropertyNames(env, object, result);
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char* utf8name, napi_value value)
{
    return NativeModule::NapiBridge::GetInstance().Provider().SetNamedProperty(env, object, utf8name, value);
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value)
{
    return NativeModule::NapiBridge::GetInstance().Provider().SetElement(env, object, index, value);
}

napi_status napi_get_cb_info(
    napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* this_arg, void** data)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetCbInfo(env, cbinfo, argc, argv, this_arg, data);
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char* utf8name, bool* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().HasNamedProperty(env, object, utf8name, result);
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char* utf8name, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetNamedProperty(env, object, utf8name, result);
}

napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetValueStringUtf8(env, value, buf, bufsize, result);
}

napi_status napi_get_value_double(napi_env env, napi_value value, double* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetValueDouble(env, value, result);
}

napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetValueUint32(env, value, result);
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetValueInt32(env, value, result);
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetValueBool(env, value, result);
}

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().Typeof(env, value, result);
}

napi_status napi_is_array(napi_env env, napi_value value, bool* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().IsArray(env, value, result);
}

napi_status napi_get_array_length(napi_env env, napi_value value, uint32_t* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetArrayLength(env, value, result);
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().GetElement(env, object, index, result);
}

napi_status napi_create_reference(napi_env env, napi_value value, uint32_t initial_refcount, napi_ref* result)
{
    return NativeModule::NapiBridge::GetInstance().Provider().CreateReference(env, value, initial_refcount, result);
}

napi_status napi_delete_reference(napi_env env, napi_ref ref)
{
    return NativeModule::NapiBridge::GetInstance().Provider().DeleteReference(env, ref);
}

void* OH_ArkUI_DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event)
{
    return nullptr;
}
void OH_ArkUI_DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock) {}

static ArkUI_NativeDialogAPI_1 g_dialogApi = {};
ArkUI_NativeDialogAPI_1* GetNativeDialogApi(void)
{
    return nullptr;
}

int32_t OH_ArkUI_GetModuleInterfaceByType(ArkUI_ModuleType type, int32_t version, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return -1;
}

NativeDisplayManager_ErrorCode OH_NativeDisplayManager_GetDefaultDisplayScaledDensity(float* scaledDensity)
{
    if (scaledDensity != nullptr) {
        *scaledDensity = g_displayScaledDensity;
    }
    return DISPLAY_MANAGER_OK;
}

NativeDisplayManager_ErrorCode OH_NativeDisplayManager_GetDefaultDisplayDensityPixels(float* densityPixels)
{
    if (densityPixels != nullptr) {
        *densityPixels = g_displayDensityPixels;
    }
    return DISPLAY_MANAGER_OK;
}

#ifdef __cplusplus
}
#endif
