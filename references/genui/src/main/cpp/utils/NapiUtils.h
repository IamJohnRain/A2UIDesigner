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

#ifndef A2UI_NAPI_UTILS_H
#define A2UI_NAPI_UTILS_H

#include <js_native_api.h>
#include <string>
#include <vector>

#include "JsonAdapter.h"

namespace NativeModule {

bool NapiHasProperty(napi_env env, napi_value object, const char* name);

napi_value NapiGetProperty(napi_env env, napi_value object, const char* name);

std::string NapiGetString(napi_env env, napi_value object, const char* name, const std::string& fallback = "");

std::string NapiGetStringValue(napi_env env, napi_value value);

double NapiGetNumber(napi_env env, napi_value object, const char* name, double fallback = 0.0);

uint32_t NapiGetUint32(napi_env env, napi_value object, const char* name, uint32_t fallback = 0);

int32_t NapiGetInt32(napi_env env, napi_value object, const char* name, int32_t fallback = 0);

bool NapiIsFunctionValue(napi_env env, napi_value value);

bool NapiIsArray(napi_env env, napi_value value);

uint32_t NapiGetArrayLength(napi_env env, napi_value array);

napi_value NapiGetElement(napi_env env, napi_value array, uint32_t index);

napi_value JsonValueToNapiValue(napi_env env, const JsonValue& value);

} // namespace NativeModule

#endif // A2UI_NAPI_UTILS_H
