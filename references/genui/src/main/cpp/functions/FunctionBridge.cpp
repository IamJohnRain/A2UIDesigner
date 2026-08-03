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

#include "FunctionBridge.h"

#include <memory>

#include "utils/LogA2UI.h"

#include "../RenderManager.h"
#include "../SurfaceSlot.h"
#include "../utils/NapiUtils.h"
#include "NapiBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

constexpr int32_t MAX_NAPI_TO_JSON_DEPTH = 32;

int32_t ResolveRenderIdBySurfaceId(const std::string& surfaceId)
{
    SurfaceSlot* slot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (slot == nullptr) {
        return -1;
    }
    return slot->GetRenderId();
}

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

JsonValue NapiValueToJsonValue(napi_env env, napi_value value, int32_t depth)
{
    if (depth > MAX_NAPI_TO_JSON_DEPTH) {
        LOG_A2UI(LOG_WARN, "FunctionBridge::NapiValueToJsonValue: max depth exceeded");
        return JsonValue();
    }
    if (env == nullptr || value == nullptr) {
        return JsonValue();
    }

    auto& napi = NapiBridge::GetInstance().Provider();
    napi_valuetype type = napi_undefined;
    if (napi.Typeof(env, value, &type) != napi_ok) {
        return JsonValue();
    }
    if (type == napi_undefined || type == napi_null) {
        std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
        return nullValue != nullptr ? nullValue->GetRoot() : JsonValue();
    }

    if (type == napi_string) {
        std::unique_ptr<JsonAdapter> stringValue = JsonAdapter::CreateString(NapiGetStringValue(env, value));
        return stringValue != nullptr ? stringValue->GetRoot() : JsonValue();
    }
    if (type == napi_number) {
        double numberValue = 0.0;
        if (napi.GetValueDouble(env, value, &numberValue) != napi_ok) {
            return JsonValue();
        }
        std::unique_ptr<JsonAdapter> numberJson = JsonAdapter::CreateNumber(numberValue);
        return numberJson != nullptr ? numberJson->GetRoot() : JsonValue();
    }
    if (type == napi_boolean) {
        bool boolValue = false;
        if (napi.GetValueBool(env, value, &boolValue) != napi_ok) {
            return JsonValue();
        }
        std::unique_ptr<JsonAdapter> boolJson = JsonAdapter::CreateBool(boolValue);
        return boolJson != nullptr ? boolJson->GetRoot() : JsonValue();
    }
    if (type != napi_object) {
        return JsonValue();
    }

    bool isArray = false;
    if (napi.IsArray(env, value, &isArray) != napi_ok) {
        return JsonValue();
    }
    if (isArray) {
        uint32_t length = 0;
        if (napi.GetArrayLength(env, value, &length) != napi_ok) {
            return JsonValue();
        }
        std::unique_ptr<JsonAdapter> arrayAdapter = JsonAdapter::CreateArray();
        if (arrayAdapter == nullptr) {
            return JsonValue();
        }
        JsonValue arrayValue = arrayAdapter->GetRoot();
        for (uint32_t index = 0; index < length; ++index) {
            napi_value element = nullptr;
            if (napi.GetElement(env, value, index, &element) != napi_ok) {
                return JsonValue();
            }
            JsonValue elementValue = NapiValueToJsonValue(env, element, depth + 1);
            if (!elementValue.IsValid()) {
                return JsonValue();
            }
            if (!arrayValue.Append(elementValue)) {
                return JsonValue();
            }
        }
        return arrayValue;
    }

    napi_value propertyNames = nullptr;
    if (napi.GetPropertyNames(env, value, &propertyNames) != napi_ok || propertyNames == nullptr) {
        return JsonValue();
    }

    uint32_t propertyCount = 0;
    if (napi.GetArrayLength(env, propertyNames, &propertyCount) != napi_ok) {
        return JsonValue();
    }

    std::unique_ptr<JsonAdapter> objectAdapter = JsonAdapter::CreateObject();
    if (objectAdapter == nullptr) {
        return JsonValue();
    }
    JsonValue objectValue = objectAdapter->GetRoot();
    for (uint32_t index = 0; index < propertyCount; ++index) {
        napi_value keyValue = nullptr;
        if (napi.GetElement(env, propertyNames, index, &keyValue) != napi_ok || keyValue == nullptr) {
            return JsonValue();
        }
        std::string key = NapiGetStringValue(env, keyValue);

        napi_value childValue = nullptr;
        if (napi.GetNamedProperty(env, value, key.c_str(), &childValue) != napi_ok) {
            return JsonValue();
        }

        napi_valuetype childType = napi_undefined;
        if (napi.Typeof(env, childValue, &childType) != napi_ok) {
            return JsonValue();
        }
        if (childType == napi_function || childType == napi_symbol || childType == napi_external) {
            continue;
        }

        JsonValue jsonChildValue = NapiValueToJsonValue(env, childValue, depth + 1);
        if (!jsonChildValue.IsValid()) {
            return JsonValue();
        }
        if (!objectValue.Put(key.c_str(), jsonChildValue)) {
            return JsonValue();
        }
    }

    return objectValue;
}

bool InvokeInternal(const napi_env env, napi_ref invokeLocalFunctionRef, int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue* returnValue,
    JsonValue* normalizedArgs, std::string* normalizedReturnType, bool normalizeOnly)
{
    if (env == nullptr || invokeLocalFunctionRef == nullptr || functionCall == nullptr) {
        LOG_A2UI(LOG_WARN, "FunctionBridge::InvokeInternal: bridge or functionCall is invalid");
        return false;
    }
    auto& napi = NapiBridge::GetInstance().Provider();

    SurfaceSlot* slot = RenderManager::GetInstance().FindSurface(renderId, surfaceId);
    if (slot == nullptr || slot->GetCatalog() == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "FunctionBridge::InvokeInternal: surface or catalog not found, renderId=%{public}d, surfaceId=%{public}s",
            renderId, surfaceId.c_str());
        return false;
    }
    if (!slot->GetCatalog()->HasFunction(functionCall->GetFunctionName())) {
        LOG_A2UI(LOG_ERROR,
            "FunctionBridge::InvokeInternal: function is not allowed by catalog, renderId=%{public}d, "
            "surfaceId=%{public}s, "
            "functionName=%{public}s",
            renderId, surfaceId.c_str(), functionCall->GetFunctionName().c_str());
        return false;
    }

    napi_value callback = nullptr;
    if (!IsNapiOk(napi.GetReferenceValue(env, invokeLocalFunctionRef, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: get callback failed");
        return false;
    }

    napi_value request = nullptr;
    if (!IsNapiOk(napi.CreateObject(env, &request)) || request == nullptr) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: create request failed");
        return false;
    }

    napi_value value = nullptr;
    napi.CreateInt32(env, renderId, &value);
    napi.SetNamedProperty(env, request, "renderId", value);

    napi.CreateStringUtf8(env, surfaceId.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(env, request, "surfaceId", value);

    napi.CreateStringUtf8(env, componentId.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(env, request, "componentId", value);

    napi.CreateStringUtf8(env, functionCall->GetFunctionName().c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(env, request, "functionName", value);

    value = JsonValueToNapiValue(env, functionCall->GetArgs());
    napi.SetNamedProperty(env, request, "args", value);

    napi.CreateStringUtf8(env, functionCall->GetReturnType().c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(env, request, "returnType", value);

    if (normalizeOnly) {
        napi_get_boolean(env, true, &value);
        napi_set_named_property(env, request, "normalizeOnly", value);
    }

    napi_value global = nullptr;
    napi.GetGlobal(env, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(env, global, callback, 1, &request, &result))) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: call bridge failed");
        return false;
    }

    if (result == nullptr || !NapiHasProperty(env, result, "success")) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: invalid response object");
        return false;
    }

    napi_value successValue = NapiGetProperty(env, result, "success");
    bool success = false;
    napi.GetValueBool(env, successValue, &success);
    if (!success) {
        std::string errorCode = NapiGetString(env, result, "errorCode", "UNKNOWN");
        std::string errorMessage = NapiGetString(env, result, "errorMessage", "invoke failed");
        LOG_A2UI(LOG_ERROR,
            "FunctionBridge::InvokeInternal: bridge failed, functionName=%{public}s, errorCode=%{public}s, "
            "errorMessage=%{public}s",
            functionCall->GetFunctionName().c_str(), errorCode.c_str(), errorMessage.c_str());
        return false;
    }

    if (normalizedReturnType != nullptr) {
        *normalizedReturnType = NapiGetString(env, result, "normalizedReturnType",
            NapiGetString(env, result, "returnType", functionCall->GetReturnType()));
    }

    if (normalizedArgs != nullptr) {
        if (NapiHasProperty(env, result, "normalizedArgs")) {
            napi_value normalizedArgsValue = NapiGetProperty(env, result, "normalizedArgs");
            JsonValue resolvedNormalizedArgs = NapiValueToJsonValue(env, normalizedArgsValue, 0);
            if (!resolvedNormalizedArgs.IsValid()) {
                LOG_A2UI(LOG_ERROR,
                    "FunctionBridge::InvokeInternal: normalized args conversion failed, functionName=%{public}s",
                    functionCall->GetFunctionName().c_str());
                return false;
            }
            if (!CloneJsonValue(resolvedNormalizedArgs, *normalizedArgs)) {
                return false;
            }
        } else {
            std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
            *normalizedArgs = nullValue != nullptr ? nullValue->GetRoot() : JsonValue();
        }
    }

    if (returnValue != nullptr) {
        if (NapiHasProperty(env, result, "value")) {
            napi_value responseValue = NapiGetProperty(env, result, "value");
            JsonValue resolvedValue = NapiValueToJsonValue(env, responseValue, 0);
            if (!resolvedValue.IsValid()) {
                LOG_A2UI(LOG_ERROR,
                    "FunctionBridge::InvokeInternal: response value conversion failed, functionName=%{public}s",
                    functionCall->GetFunctionName().c_str());
                return false;
            }
            if (!CloneJsonValue(resolvedValue, *returnValue)) {
                return false;
            }
        } else {
            std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
            *returnValue = nullValue != nullptr ? nullValue->GetRoot() : JsonValue();
        }
    }

    return true;
}

} // namespace

FunctionBridge& FunctionBridge::GetInstance()
{
    static FunctionBridge instance;
    return instance;
}

void FunctionBridge::RegisterInvokeLocalFunction(napi_env env, napi_value callback)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterInvokeLocalFunction: invalid callback");
        return;
    }

    if (invokeLocalFunctionRef_ != nullptr && napiEnv_ != nullptr) {
        napi.DeleteReference(napiEnv_, invokeLocalFunctionRef_);
        invokeLocalFunctionRef_ = nullptr;
    }

    napiEnv_ = env;
    if (!IsNapiOk(napi.CreateReference(env, callback, 1, &invokeLocalFunctionRef_))) {
        LOG_A2UI(LOG_ERROR, "RegisterInvokeLocalFunction: create reference failed");
        invokeLocalFunctionRef_ = nullptr;
        return;
    }

    LOG_A2UI(LOG_INFO, "RegisterInvokeLocalFunction: success");
}

bool FunctionBridge::Invoke(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall) const
{
    return InvokeInternal(napiEnv_, invokeLocalFunctionRef_, renderId, surfaceId, componentId, functionCall, nullptr,
        nullptr, nullptr, false);
}

bool FunctionBridge::Invoke(const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall) const
{
    return Invoke(ResolveRenderIdBySurfaceId(surfaceId), surfaceId, componentId, functionCall);
}

bool FunctionBridge::InvokeForValue(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& returnValue) const
{
    return InvokeInternal(napiEnv_, invokeLocalFunctionRef_, renderId, surfaceId, componentId, functionCall,
        &returnValue, nullptr, nullptr, false);
}

bool FunctionBridge::InvokeForValue(const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& returnValue) const
{
    return InvokeForValue(ResolveRenderIdBySurfaceId(surfaceId), surfaceId, componentId, functionCall, returnValue);
}

bool FunctionBridge::NormalizeFunctionCall(int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& normalizedArgs,
    std::string& normalizedReturnType) const
{
    return InvokeInternal(napiEnv_, invokeLocalFunctionRef_, renderId, surfaceId, componentId, functionCall, nullptr,
        &normalizedArgs, &normalizedReturnType, true);
}

bool FunctionBridge::NormalizeFunctionCall(const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& normalizedArgs,
    std::string& normalizedReturnType) const
{
    return NormalizeFunctionCall(ResolveRenderIdBySurfaceId(surfaceId), surfaceId, componentId, functionCall,
        normalizedArgs, normalizedReturnType);
}

} // namespace NativeModule
