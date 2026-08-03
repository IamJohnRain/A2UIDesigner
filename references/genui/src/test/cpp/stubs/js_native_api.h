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

#ifndef JS_NATIVE_API_H
#define JS_NATIVE_API_H

#include <cstddef>
#include <cstdint>

typedef int napi_status;

typedef struct napi_env__ {
    int dummy;
}* napi_env;

typedef struct napi_value__ {
    int dummy;
}* napi_value;

typedef struct napi_ref__ {
    int dummy;
}* napi_ref;

typedef struct napi_callback_info__ {
    int dummy;
}* napi_callback_info;

typedef struct napi_handle_scope__ {
    int dummy;
}* napi_handle_scope;

typedef struct napi_escapable_handle_scope__ {
    int dummy;
}* napi_escapable_handle_scope;

typedef napi_value (*napi_callback)(napi_env env, napi_callback_info info);

typedef enum {
    napi_ok = 0,
    napi_generic_failure = 1,
    napi_invalid_arg = 2,
    napi_object_expected = 3,
    napi_string_expected = 4,
    napi_name_expected = 5,
    napi_function_expected = 6,
    napi_number_expected = 7,
    napi_boolean_expected = 8,
    napi_array_expected = 9,
    napi_generic_failure2 = 10,
} napi_status_enum;

typedef enum {
    napi_undefined = 0,
    napi_null = 1,
    napi_boolean = 2,
    napi_number = 3,
    napi_string = 4,
    napi_symbol = 5,
    napi_object = 6,
    napi_function = 7,
    napi_external = 8,
    napi_bigint = 9
} napi_valuetype;

#define NAPI_AUTO_LENGTH (~(size_t)0)

#ifdef __cplusplus
extern "C" {
#endif

napi_status napi_get_cb_info(
    napi_env env, napi_callback_info cbinfo, size_t* argc, napi_value* argv, napi_value* this_arg, void** data);
napi_status napi_has_named_property(napi_env env, napi_value object, const char* utf8name, bool* result);
napi_status napi_get_named_property(napi_env env, napi_value object, const char* utf8name, napi_value* result);
napi_status napi_set_named_property(napi_env env, napi_value object, const char* utf8name, napi_value value);
napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char* buf, size_t bufsize, size_t* result);
napi_status napi_get_value_double(napi_env env, napi_value value, double* result);
napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t* result);
napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t* result);
napi_status napi_get_value_bool(napi_env env, napi_value value, bool* result);
napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype* result);
napi_status napi_is_array(napi_env env, napi_value value, bool* result);
napi_status napi_get_array_length(napi_env env, napi_value value, uint32_t* result);
napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value* result);
napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value);
napi_status napi_create_reference(napi_env env, napi_value value, uint32_t initial_refcount, napi_ref* result);
napi_status napi_delete_reference(napi_env env, napi_ref ref);
napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value* result);
napi_status napi_create_object(napi_env env, napi_value* result);
napi_status napi_create_int32(napi_env env, int32_t value, napi_value* result);
napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value* result);
napi_status napi_create_double(napi_env env, double value, napi_value* result);
napi_status napi_create_string_utf8(napi_env env, const char* str, size_t length, napi_value* result);
napi_status napi_get_boolean(napi_env env, bool value, napi_value* result);
napi_status napi_get_undefined(napi_env env, napi_value* result);
napi_status napi_get_global(napi_env env, napi_value* result);
napi_status napi_call_function(
    napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value* argv, napi_value* result);
napi_status napi_open_handle_scope(napi_env env, napi_handle_scope* result);
napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope);
napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value* result);
napi_status napi_get_property_names(napi_env env, napi_value object, napi_value* result);

#ifdef __cplusplus
}
#endif

#endif
