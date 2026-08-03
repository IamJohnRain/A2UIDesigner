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

#ifndef A2UI_INAPI_PROVIDER_H
#define A2UI_INAPI_PROVIDER_H

#include <cstddef>
#include <cstdint>
#include <js_native_api.h>
#include <memory>

namespace NativeModule {

class INapiProvider {
public:
    virtual ~INapiProvider() = default;

    virtual napi_status GetCbInfo(
        napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* thisArg, void** data) = 0;
    virtual napi_status GetValueInt32(napi_env env, napi_value value, int32_t* result) = 0;
    virtual napi_status GetValueBool(napi_env env, napi_value value, bool* result) = 0;
    virtual napi_status GetValueDouble(napi_env env, napi_value value, double* result) = 0;
    virtual napi_status GetValueUint32(napi_env env, napi_value value, uint32_t* result) = 0;
    virtual napi_status GetValueStringUtf8(
        napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result) = 0;
    virtual napi_status Typeof(napi_env env, napi_value value, napi_valuetype* result) = 0;
    virtual napi_status GetGlobal(napi_env env, napi_value* result) = 0;
    virtual napi_status GetUndefined(napi_env env, napi_value* result) = 0;
    virtual napi_status GetNull(napi_env env, napi_value* result) = 0;

    virtual napi_status CreateInt32(napi_env env, int32_t value, napi_value* result) = 0;
    virtual napi_status CreateUint32(napi_env env, uint32_t value, napi_value* result) = 0;
    virtual napi_status CreateDouble(napi_env env, double value, napi_value* result) = 0;
    virtual napi_status CreateBoolean(napi_env env, bool value, napi_value* result) = 0;
    virtual napi_status CreateStringUtf8(napi_env env, const char* str, size_t length, napi_value* result) = 0;
    virtual napi_status CreateObject(napi_env env, napi_value* result) = 0;
    virtual napi_status CreateArrayWithLength(napi_env env, size_t length, napi_value* result) = 0;
    virtual napi_status CreateFunction(
        napi_env env, const char* utf8name, size_t length, napi_callback cb, void* data, napi_value* result) = 0;
    virtual napi_status CreateReference(napi_env env, napi_value value, uint32_t initialRefcount, napi_ref* result) = 0;
    virtual napi_status DeleteReference(napi_env env, napi_ref ref) = 0;
    virtual napi_status GetReferenceValue(napi_env env, napi_ref ref, napi_value* result) = 0;

    virtual napi_status SetNamedProperty(napi_env env, napi_value object, const char* key, napi_value value) = 0;
    virtual napi_status GetNamedProperty(napi_env env, napi_value object, const char* key, napi_value* result) = 0;
    virtual napi_status HasNamedProperty(napi_env env, napi_value object, const char* key, bool* result) = 0;
    virtual napi_status GetPropertyNames(napi_env env, napi_value object, napi_value* result) = 0;
    virtual napi_status SetElement(napi_env env, napi_value object, uint32_t index, napi_value value) = 0;
    virtual napi_status GetElement(napi_env env, napi_value object, uint32_t index, napi_value* result) = 0;
    virtual napi_status IsArray(napi_env env, napi_value value, bool* result) = 0;
    virtual napi_status GetArrayLength(napi_env env, napi_value value, uint32_t* result) = 0;

    virtual napi_status CallFunction(
        napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv, napi_value* result) = 0;

    virtual napi_status OpenHandleScope(napi_env env, napi_handle_scope* result) = 0;
    virtual napi_status CloseHandleScope(napi_env env, napi_handle_scope scope) = 0;
    virtual napi_status OpenEscapableHandleScope(napi_env env, napi_escapable_handle_scope* result) = 0;
    virtual napi_status CloseEscapableHandleScope(napi_env env, napi_escapable_handle_scope scope) = 0;
    virtual napi_status EscapeHandle(
        napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value* result) = 0;

    virtual napi_status CreateError(napi_env env, napi_value code, napi_value msg, napi_value* result) = 0;
    virtual napi_status Throw(napi_env env, napi_value error) = 0;
    virtual napi_status GetBoolean(napi_env env, bool value, napi_value* result) = 0;
};

} // namespace NativeModule

#endif // A2UI_INAPI_PROVIDER_H
