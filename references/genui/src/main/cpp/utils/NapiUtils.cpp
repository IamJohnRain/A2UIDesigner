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

#include "NapiUtils.h"

#include <cstdint>

#include "NapiBridge.h"

namespace NativeModule {

namespace {

constexpr int32_t MAX_JSON_TO_NAPI_DEPTH = 64;

napi_value GetUndefinedValue(napi_env env)
{
    napi_value undefinedValue = nullptr;
    if (env != nullptr) {
        NapiBridge::GetInstance().Provider().GetUndefined(env, &undefinedValue);
    }
    return undefinedValue;
}

napi_value JsonValueToNapiValueInternal(napi_env env, const JsonValue& value, int32_t depth)
{
    auto& napi = NapiBridge::GetInstance().Provider();

    if (env == nullptr) {
        return nullptr;
    }
    if (depth > MAX_JSON_TO_NAPI_DEPTH) {
        return GetUndefinedValue(env);
    }
    if (!value.IsValid()) {
        return GetUndefinedValue(env);
    }

    napi_value result = nullptr;
    if (value.IsNull()) {
        if (napi.GetNull(env, &result) != napi_ok) {
            return GetUndefinedValue(env);
        }
        return result;
    }
    if (value.IsString()) {
        if (napi.CreateStringUtf8(env, value.GetStringValue("").c_str(), NAPI_AUTO_LENGTH, &result) != napi_ok) {
            return GetUndefinedValue(env);
        }
        return result;
    }
    if (value.IsNumber()) {
        if (napi.CreateDouble(env, value.GetNumberValue(0.0), &result) != napi_ok) {
            return GetUndefinedValue(env);
        }
        return result;
    }
    if (value.IsBool()) {
        if (napi.GetBoolean(env, value.GetBoolValue(false), &result) != napi_ok) {
            return GetUndefinedValue(env);
        }
        return result;
    }
    if (value.IsArray()) {
        int itemCount = value.GetArraySize();
        if (itemCount < 0) {
            return GetUndefinedValue(env);
        }
        if (napi.CreateArrayWithLength(env, static_cast<size_t>(itemCount), &result) != napi_ok || result == nullptr) {
            return GetUndefinedValue(env);
        }
        for (int index = 0; index < itemCount; ++index) {
            napi_value elementValue = JsonValueToNapiValueInternal(env, value.GetArrayItem(index), depth + 1);
            if (napi.SetElement(env, result, static_cast<uint32_t>(index), elementValue) != napi_ok) {
                return GetUndefinedValue(env);
            }
        }
        return result;
    }
    if (value.IsObject()) {
        if (napi.CreateObject(env, &result) != napi_ok || result == nullptr) {
            return GetUndefinedValue(env);
        }
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            std::string key = child.GetKey();
            if (key.empty()) {
                continue;
            }
            napi_value childValue = JsonValueToNapiValueInternal(env, child, depth + 1);
            if (napi.SetNamedProperty(env, result, key.c_str(), childValue) != napi_ok) {
                return GetUndefinedValue(env);
            }
        }
        return result;
    }

    return GetUndefinedValue(env);
}

} // namespace

bool NapiHasProperty(napi_env env, napi_value object, const char* name)
{
    bool hasProperty = false;
    NapiBridge::GetInstance().Provider().HasNamedProperty(env, object, name, &hasProperty);
    return hasProperty;
}

napi_value NapiGetProperty(napi_env env, napi_value object, const char* name)
{
    napi_value value = nullptr;
    NapiBridge::GetInstance().Provider().GetNamedProperty(env, object, name, &value);
    return value;
}

std::string NapiGetString(napi_env env, napi_value object, const char* name, const std::string& fallback)
{
    if (!NapiHasProperty(env, object, name)) {
        return fallback;
    }

    napi_value value = NapiGetProperty(env, object, name);
    size_t stringLength = 0;
    NapiBridge::GetInstance().Provider().GetValueStringUtf8(env, value, nullptr, 0, &stringLength);
    if (stringLength == 0) {
        return fallback;
    }

    std::vector<char> buffer(stringLength + 1, '\0');
    size_t copiedLength = 0;
    NapiBridge::GetInstance().Provider().GetValueStringUtf8(env, value, buffer.data(), buffer.size(), &copiedLength);
    return std::string(buffer.data(), copiedLength);
}

std::string NapiGetStringValue(napi_env env, napi_value value)
{
    size_t stringLength = 0;
    NapiBridge::GetInstance().Provider().GetValueStringUtf8(env, value, nullptr, 0, &stringLength);
    if (stringLength == 0) {
        return "";
    }

    std::vector<char> buffer(stringLength + 1, '\0');
    size_t copiedLength = 0;
    NapiBridge::GetInstance().Provider().GetValueStringUtf8(env, value, buffer.data(), buffer.size(), &copiedLength);
    return std::string(buffer.data(), copiedLength);
}

double NapiGetNumber(napi_env env, napi_value object, const char* name, double fallback)
{
    if (!NapiHasProperty(env, object, name)) {
        return fallback;
    }

    napi_value value = NapiGetProperty(env, object, name);
    double numberValue = fallback;
    NapiBridge::GetInstance().Provider().GetValueDouble(env, value, &numberValue);
    return numberValue;
}

uint32_t NapiGetUint32(napi_env env, napi_value object, const char* name, uint32_t fallback)
{
    if (!NapiHasProperty(env, object, name)) {
        return fallback;
    }

    napi_value value = NapiGetProperty(env, object, name);
    uint32_t numberValue = fallback;
    NapiBridge::GetInstance().Provider().GetValueUint32(env, value, &numberValue);
    return numberValue;
}

int32_t NapiGetInt32(napi_env env, napi_value object, const char* name, int32_t fallback)
{
    if (!NapiHasProperty(env, object, name)) {
        return fallback;
    }

    napi_value value = NapiGetProperty(env, object, name);
    int32_t numberValue = fallback;
    NapiBridge::GetInstance().Provider().GetValueInt32(env, value, &numberValue);
    return numberValue;
}

bool NapiGetBool(napi_env env, napi_value object, const char* name, bool fallback)
{
    if (!NapiHasProperty(env, object, name)) {
        return fallback;
    }

    napi_value value = NapiGetProperty(env, object, name);
    bool boolValue = fallback;
    NapiBridge::GetInstance().Provider().GetValueBool(env, value, &boolValue);
    return boolValue;
}

bool NapiIsFunctionValue(napi_env env, napi_value value)
{
    if (value == nullptr) {
        return false;
    }

    napi_valuetype valueType = napi_undefined;
    if (NapiBridge::GetInstance().Provider().Typeof(env, value, &valueType) != napi_ok) {
        return false;
    }
    return valueType == napi_function;
}

bool NapiIsArray(napi_env env, napi_value value)
{
    bool isArray = false;
    NapiBridge::GetInstance().Provider().IsArray(env, value, &isArray);
    return isArray;
}

uint32_t NapiGetArrayLength(napi_env env, napi_value array)
{
    uint32_t length = 0;
    NapiBridge::GetInstance().Provider().GetArrayLength(env, array, &length);
    return length;
}

napi_value NapiGetElement(napi_env env, napi_value array, uint32_t index)
{
    napi_value element = nullptr;
    NapiBridge::GetInstance().Provider().GetElement(env, array, index, &element);
    return element;
}

napi_value JsonValueToNapiValue(napi_env env, const JsonValue& value)
{
    return JsonValueToNapiValueInternal(env, value, 0);
}

} // namespace NativeModule
