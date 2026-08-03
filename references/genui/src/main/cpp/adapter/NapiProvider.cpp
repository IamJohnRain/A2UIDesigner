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

#include "NapiProvider.h"

#include <js_native_api.h>

namespace NativeModule {

napi_status NapiProvider::GetCbInfo(
    napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* thisArg, void** data)
{
    return napi_get_cb_info(env, cbinfo, argc, argv, thisArg, data);
}

napi_status NapiProvider::GetValueInt32(napi_env env, napi_value value, int32_t* result)
{
    return napi_get_value_int32(env, value, result);
}

napi_status NapiProvider::GetValueBool(napi_env env, napi_value value, bool* result)
{
    return napi_get_value_bool(env, value, result);
}

napi_status NapiProvider::GetValueDouble(napi_env env, napi_value value, double* result)
{
    return napi_get_value_double(env, value, result);
}

napi_status NapiProvider::GetValueUint32(napi_env env, napi_value value, uint32_t* result)
{
    return napi_get_value_uint32(env, value, result);
}

napi_status NapiProvider::GetValueStringUtf8(napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result)
{
    return napi_get_value_string_utf8(env, value, buf, bufsize, result);
}

napi_status NapiProvider::Typeof(napi_env env, napi_value value, napi_valuetype* result)
{
    return napi_typeof(env, value, result);
}

napi_status NapiProvider::GetGlobal(napi_env env, napi_value* result)
{
    return napi_get_global(env, result);
}

napi_status NapiProvider::GetUndefined(napi_env env, napi_value* result)
{
    return napi_get_undefined(env, result);
}

napi_status NapiProvider::GetNull(napi_env env, napi_value* result)
{
    return napi_get_null(env, result);
}

napi_status NapiProvider::CreateInt32(napi_env env, int32_t value, napi_value* result)
{
    return napi_create_int32(env, value, result);
}

napi_status NapiProvider::CreateUint32(napi_env env, uint32_t value, napi_value* result)
{
    return napi_create_uint32(env, value, result);
}

napi_status NapiProvider::CreateDouble(napi_env env, double value, napi_value* result)
{
    return napi_create_double(env, value, result);
}

napi_status NapiProvider::CreateBoolean(napi_env env, bool value, napi_value* result)
{
    return napi_get_boolean(env, value, result);
}

napi_status NapiProvider::CreateStringUtf8(napi_env env, const char* str, size_t length, napi_value* result)
{
    return napi_create_string_utf8(env, str, length, result);
}

napi_status NapiProvider::CreateObject(napi_env env, napi_value* result)
{
    return napi_create_object(env, result);
}

napi_status NapiProvider::CreateArrayWithLength(napi_env env, size_t length, napi_value* result)
{
    return napi_create_array_with_length(env, length, result);
}

napi_status NapiProvider::CreateFunction(
    napi_env env, const char* utf8name, size_t length, napi_callback cb, void* data, napi_value* result)
{
    return napi_create_function(env, utf8name, length, cb, data, result);
}

napi_status NapiProvider::CreateReference(napi_env env, napi_value value, uint32_t initialRefcount, napi_ref* result)
{
    return napi_create_reference(env, value, initialRefcount, result);
}

napi_status NapiProvider::DeleteReference(napi_env env, napi_ref ref)
{
    return napi_delete_reference(env, ref);
}

napi_status NapiProvider::GetReferenceValue(napi_env env, napi_ref ref, napi_value* result)
{
    return napi_get_reference_value(env, ref, result);
}

napi_status NapiProvider::SetNamedProperty(napi_env env, napi_value object, const char* key, napi_value value)
{
    return napi_set_named_property(env, object, key, value);
}

napi_status NapiProvider::GetNamedProperty(napi_env env, napi_value object, const char* key, napi_value* result)
{
    return napi_get_named_property(env, object, key, result);
}

napi_status NapiProvider::HasNamedProperty(napi_env env, napi_value object, const char* key, bool* result)
{
    return napi_has_named_property(env, object, key, result);
}

napi_status NapiProvider::GetPropertyNames(napi_env env, napi_value object, napi_value* result)
{
    return napi_get_property_names(env, object, result);
}

napi_status NapiProvider::SetElement(napi_env env, napi_value object, uint32_t index, napi_value value)
{
    return napi_set_element(env, object, index, value);
}

napi_status NapiProvider::GetElement(napi_env env, napi_value object, uint32_t index, napi_value* result)
{
    return napi_get_element(env, object, index, result);
}

napi_status NapiProvider::IsArray(napi_env env, napi_value value, bool* result)
{
    return napi_is_array(env, value, result);
}

napi_status NapiProvider::GetArrayLength(napi_env env, napi_value value, uint32_t* result)
{
    return napi_get_array_length(env, value, result);
}

napi_status NapiProvider::CallFunction(
    napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv, napi_value* result)
{
    return napi_call_function(env, recv, func, argc, argv, result);
}

napi_status NapiProvider::OpenHandleScope(napi_env env, napi_handle_scope* result)
{
    return napi_open_handle_scope(env, result);
}

napi_status NapiProvider::CloseHandleScope(napi_env env, napi_handle_scope scope)
{
    return napi_close_handle_scope(env, scope);
}

napi_status NapiProvider::OpenEscapableHandleScope(napi_env env, napi_escapable_handle_scope* result)
{
    return napi_open_escapable_handle_scope(env, result);
}

napi_status NapiProvider::CloseEscapableHandleScope(napi_env env, napi_escapable_handle_scope scope)
{
    return napi_close_escapable_handle_scope(env, scope);
}

napi_status NapiProvider::EscapeHandle(
    napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value* result)
{
    return napi_escape_handle(env, scope, escapee, result);
}

napi_status NapiProvider::CreateError(napi_env env, napi_value code, napi_value msg, napi_value* result)
{
    return napi_create_error(env, code, msg, result);
}

napi_status NapiProvider::Throw(napi_env env, napi_value error)
{
    return napi_throw(env, error);
}

napi_status NapiProvider::GetBoolean(napi_env env, bool value, napi_value* result)
{
    return napi_get_boolean(env, value, result);
}

} // namespace NativeModule
