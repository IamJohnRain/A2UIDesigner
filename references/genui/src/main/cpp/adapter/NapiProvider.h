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

#ifndef A2UI_NAPI_PROVIDER_H
#define A2UI_NAPI_PROVIDER_H

#include "include/INapiProvider.h"

namespace NativeModule {

class NapiProvider : public INapiProvider {
public:
    NapiProvider() = default;
    ~NapiProvider() override = default;

    napi_status GetCbInfo(napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* thisArg,
        void** data) override;
    napi_status GetValueInt32(napi_env env, napi_value value, int32_t* result) override;
    napi_status GetValueBool(napi_env env, napi_value value, bool* result) override;
    napi_status GetValueDouble(napi_env env, napi_value value, double* result) override;
    napi_status GetValueUint32(napi_env env, napi_value value, uint32_t* result) override;
    napi_status GetValueStringUtf8(napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result) override;
    napi_status Typeof(napi_env env, napi_value value, napi_valuetype* result) override;
    napi_status GetGlobal(napi_env env, napi_value* result) override;
    napi_status GetUndefined(napi_env env, napi_value* result) override;
    napi_status GetNull(napi_env env, napi_value* result) override;

    napi_status CreateInt32(napi_env env, int32_t value, napi_value* result) override;
    napi_status CreateUint32(napi_env env, uint32_t value, napi_value* result) override;
    napi_status CreateDouble(napi_env env, double value, napi_value* result) override;
    napi_status CreateBoolean(napi_env env, bool value, napi_value* result) override;
    napi_status CreateStringUtf8(napi_env env, const char* str, size_t length, napi_value* result) override;
    napi_status CreateObject(napi_env env, napi_value* result) override;
    napi_status CreateArrayWithLength(napi_env env, size_t length, napi_value* result) override;
    napi_status CreateFunction(
        napi_env env, const char* utf8name, size_t length, napi_callback cb, void* data, napi_value* result) override;
    napi_status CreateReference(napi_env env, napi_value value, uint32_t initialRefcount, napi_ref* result) override;
    napi_status DeleteReference(napi_env env, napi_ref ref) override;
    napi_status GetReferenceValue(napi_env env, napi_ref ref, napi_value* result) override;

    napi_status SetNamedProperty(napi_env env, napi_value object, const char* key, napi_value value) override;
    napi_status GetNamedProperty(napi_env env, napi_value object, const char* key, napi_value* result) override;
    napi_status HasNamedProperty(napi_env env, napi_value object, const char* key, bool* result) override;
    napi_status GetPropertyNames(napi_env env, napi_value object, napi_value* result) override;
    napi_status SetElement(napi_env env, napi_value object, uint32_t index, napi_value value) override;
    napi_status GetElement(napi_env env, napi_value object, uint32_t index, napi_value* result) override;
    napi_status IsArray(napi_env env, napi_value value, bool* result) override;
    napi_status GetArrayLength(napi_env env, napi_value value, uint32_t* result) override;

    napi_status CallFunction(napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv,
        napi_value* result) override;

    napi_status OpenHandleScope(napi_env env, napi_handle_scope* result) override;
    napi_status CloseHandleScope(napi_env env, napi_handle_scope scope) override;
    napi_status OpenEscapableHandleScope(napi_env env, napi_escapable_handle_scope* result) override;
    napi_status CloseEscapableHandleScope(napi_env env, napi_escapable_handle_scope scope) override;
    napi_status EscapeHandle(
        napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value* result) override;

    napi_status CreateError(napi_env env, napi_value code, napi_value msg, napi_value* result) override;
    napi_status Throw(napi_env env, napi_value error) override;
    napi_status GetBoolean(napi_env env, bool value, napi_value* result) override;
};

} // namespace NativeModule

#endif // A2UI_NAPI_PROVIDER_H
