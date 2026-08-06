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
#include "../SurfaceErrorCodes.h"
#include "../SurfaceSlot.h"
#include "../utils/NapiUtils.h"
#include "NapiBridge.h"
#include "RuntimeErrorDispatchBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

constexpr int32_t MAX_NAPI_TO_JSON_DEPTH = 32;

struct InvokeBridgeContext {
    napi_env env = nullptr;
    napi_ref invokeLocalFunctionRef = nullptr;
};

struct InvokeRequestContext {
    int32_t renderId = -1;
    const std::string* surfaceId = nullptr;
    const std::string* componentId = nullptr;
    const FunctionCallInfo* functionCall = nullptr;
    bool normalizeOnly = false;
};

struct InvokeResponseTargets {
    JsonValue* returnValue = nullptr;
    JsonValue* normalizedArgs = nullptr;
    std::string* normalizedReturnType = nullptr;
};

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

void DispatchUnknownLocalFunctionError(
    int32_t renderId, const std::string& surfaceId, const std::string& componentId, const std::string& functionName)
{
    if (renderId < 0) {
        return;
    }
    RuntimeErrorDispatchBridge::GetInstance().Dispatch(renderId, surfaceId, componentId, SURFACE_ERROR_LOCAL_FUNCTION,
        "Local function is not registered: " + functionName, "FunctionBridge");
}

JsonValue NapiValueToJsonValue(napi_env env, napi_value value, int32_t depth);

JsonValue CreateNullJsonValue()
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNull();
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue NapiPrimitiveToJsonValue(napi_env env, napi_value value, napi_valuetype type)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (type == napi_undefined || type == napi_null) {
        return CreateNullJsonValue();
    }
    if (type == napi_string) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(NapiGetStringValue(env, value));
        return adapter != nullptr ? adapter->GetRoot() : JsonValue();
    }
    if (type == napi_number) {
        double numberValue = 0.0;
        if (napi.GetValueDouble(env, value, &numberValue) != napi_ok) {
            return JsonValue();
        }
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(numberValue);
        return adapter != nullptr ? adapter->GetRoot() : JsonValue();
    }
    if (type == napi_boolean) {
        bool boolValue = false;
        if (napi.GetValueBool(env, value, &boolValue) != napi_ok) {
            return JsonValue();
        }
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateBool(boolValue);
        return adapter != nullptr ? adapter->GetRoot() : JsonValue();
    }
    return JsonValue();
}

JsonValue NapiArrayToJsonValue(napi_env env, napi_value value, int32_t depth)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    uint32_t length = 0;
    if (napi.GetArrayLength(env, value, &length) != napi_ok) {
        return JsonValue();
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateArray();
    if (adapter == nullptr) {
        return JsonValue();
    }

    JsonValue arrayValue = adapter->GetRoot();
    for (uint32_t index = 0; index < length; ++index) {
        napi_value element = nullptr;
        if (napi.GetElement(env, value, index, &element) != napi_ok) {
            return JsonValue();
        }
        JsonValue elementValue = NapiValueToJsonValue(env, element, depth + 1);
        if (!elementValue.IsValid() || !arrayValue.Append(elementValue)) {
            return JsonValue();
        }
    }
    return arrayValue;
}

JsonValue NapiObjectToJsonValue(napi_env env, napi_value value, int32_t depth)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value propertyNames = nullptr;
    if (napi.GetPropertyNames(env, value, &propertyNames) != napi_ok || propertyNames == nullptr) {
        return JsonValue();
    }

    uint32_t propertyCount = 0;
    if (napi.GetArrayLength(env, propertyNames, &propertyCount) != napi_ok) {
        return JsonValue();
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }

    JsonValue objectValue = adapter->GetRoot();
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
        if (!jsonChildValue.IsValid() || !objectValue.Put(key.c_str(), jsonChildValue)) {
            return JsonValue();
        }
    }
    return objectValue;
}

JsonValue NapiObjectLikeToJsonValue(napi_env env, napi_value value, int32_t depth)
{
    bool isArray = false;
    auto& napi = NapiBridge::GetInstance().Provider();
    if (napi.IsArray(env, value, &isArray) != napi_ok) {
        return JsonValue();
    }
    return isArray ? NapiArrayToJsonValue(env, value, depth) : NapiObjectToJsonValue(env, value, depth);
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
    return type == napi_object ? NapiObjectLikeToJsonValue(env, value, depth)
                               : NapiPrimitiveToJsonValue(env, value, type);
}

bool ValidateInvokeTarget(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const FunctionCallInfo& functionCall)
{
    SurfaceSlot* slot = RenderManager::GetInstance().FindSurface(renderId, surfaceId);
    if (slot == nullptr || slot->GetCatalog() == nullptr) {
        LOG_A2UI(LOG_ERROR,
            "FunctionBridge::InvokeInternal: surface or catalog not found, renderId=%{public}d, surfaceId=%{public}s",
            renderId, surfaceId.c_str());
        return false;
    }
    if (slot->GetCatalog()->HasFunction(functionCall.GetFunctionName())) {
        return true;
    }

    LOG_A2UI(LOG_ERROR,
        "FunctionBridge::InvokeInternal: function is not allowed by catalog, renderId=%{public}d, "
        "surfaceId=%{public}s, functionName=%{public}s",
        renderId, surfaceId.c_str(), functionCall.GetFunctionName().c_str());
    DispatchUnknownLocalFunctionError(renderId, surfaceId, componentId, functionCall.GetFunctionName());
    return false;
}

napi_value ResolveInvokeCallback(napi_env env, napi_ref invokeLocalFunctionRef)
{
    napi_value callback = nullptr;
    auto& napi = NapiBridge::GetInstance().Provider();
    if (!IsNapiOk(napi.GetReferenceValue(env, invokeLocalFunctionRef, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: get callback failed");
        return nullptr;
    }
    return callback;
}

void SetRequestStringProperty(napi_env env, napi_value request, const char* key, const std::string& input)
{
    napi_value value = nullptr;
    auto& napi = NapiBridge::GetInstance().Provider();
    napi.CreateStringUtf8(env, input.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(env, request, key, value);
}

napi_value BuildInvokeRequest(napi_env env, const InvokeRequestContext& context)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value request = nullptr;
    if (!IsNapiOk(napi.CreateObject(env, &request)) || request == nullptr) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: create request failed");
        return nullptr;
    }

    napi_value value = nullptr;
    napi.CreateInt32(env, context.renderId, &value);
    napi.SetNamedProperty(env, request, "renderId", value);
    SetRequestStringProperty(env, request, "surfaceId", *context.surfaceId);
    SetRequestStringProperty(env, request, "componentId", *context.componentId);
    SetRequestStringProperty(env, request, "functionName", context.functionCall->GetFunctionName());
    value = JsonValueToNapiValue(env, context.functionCall->GetArgs());
    napi.SetNamedProperty(env, request, "args", value);
    SetRequestStringProperty(env, request, "returnType", context.functionCall->GetReturnType());
    if (context.normalizeOnly) {
        napi.GetBoolean(env, true, &value);
        napi.SetNamedProperty(env, request, "normalizeOnly", value);
    }
    return request;
}

napi_value CallInvokeBridge(napi_env env, napi_value callback, napi_value request)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value global = nullptr;
    napi.GetGlobal(env, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(env, global, callback, 1, &request, &result))) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: call bridge failed");
        return nullptr;
    }
    return result;
}

bool ValidateInvokeResponse(napi_env env, napi_value result, const FunctionCallInfo& functionCall)
{
    if (result == nullptr || !NapiHasProperty(env, result, "success")) {
        LOG_A2UI(LOG_ERROR, "FunctionBridge::InvokeInternal: invalid response object");
        return false;
    }

    bool success = false;
    auto& napi = NapiBridge::GetInstance().Provider();
    napi.GetValueBool(env, NapiGetProperty(env, result, "success"), &success);
    if (success) {
        return true;
    }

    std::string errorCode = NapiGetString(env, result, "errorCode", "UNKNOWN");
    std::string errorMessage = NapiGetString(env, result, "errorMessage", "invoke failed");
    LOG_A2UI(LOG_ERROR,
        "FunctionBridge::InvokeInternal: bridge failed, functionName=%{public}s, errorCode=%{public}s, "
        "errorMessage=%{public}s",
        functionCall.GetFunctionName().c_str(), errorCode.c_str(), errorMessage.c_str());
    return false;
}

bool ResolveJsonResponseProperty(
    napi_env env, napi_value result, const char* propertyName, const std::string& functionName, JsonValue& output)
{
    if (!NapiHasProperty(env, result, propertyName)) {
        output = CreateNullJsonValue();
        return true;
    }

    JsonValue resolvedValue = NapiValueToJsonValue(env, NapiGetProperty(env, result, propertyName), 0);
    if (!resolvedValue.IsValid()) {
        if (std::string(propertyName) == "normalizedArgs") {
            LOG_A2UI(LOG_ERROR,
                "FunctionBridge::InvokeInternal: normalized args conversion failed, functionName=%{public}s",
                functionName.c_str());
        } else {
            LOG_A2UI(LOG_ERROR,
                "FunctionBridge::InvokeInternal: response value conversion failed, functionName=%{public}s",
                functionName.c_str());
        }
        return false;
    }
    return CloneJsonValue(resolvedValue, output);
}

bool InvokeInternal(const InvokeBridgeContext& bridge, const InvokeRequestContext& requestContext,
    const InvokeResponseTargets& responseTargets)
{
    if (bridge.env == nullptr || bridge.invokeLocalFunctionRef == nullptr || requestContext.surfaceId == nullptr ||
        requestContext.componentId == nullptr || requestContext.functionCall == nullptr) {
        LOG_A2UI(LOG_WARN, "FunctionBridge::InvokeInternal: bridge or functionCall is invalid");
        return false;
    }
    if (!ValidateInvokeTarget(requestContext.renderId, *requestContext.surfaceId, *requestContext.componentId,
            *requestContext.functionCall)) {
        return false;
    }
    napi_value callback = ResolveInvokeCallback(bridge.env, bridge.invokeLocalFunctionRef);
    if (callback == nullptr) {
        return false;
    }
    napi_value request = BuildInvokeRequest(bridge.env, requestContext);
    if (request == nullptr) {
        return false;
    }
    napi_value result = CallInvokeBridge(bridge.env, callback, request);
    if (!ValidateInvokeResponse(bridge.env, result, *requestContext.functionCall)) {
        return false;
    }

    if (responseTargets.normalizedReturnType != nullptr) {
        *responseTargets.normalizedReturnType = NapiGetString(bridge.env, result, "normalizedReturnType",
            NapiGetString(bridge.env, result, "returnType", requestContext.functionCall->GetReturnType()));
    }

    if (responseTargets.normalizedArgs != nullptr &&
        !ResolveJsonResponseProperty(bridge.env, result, "normalizedArgs",
            requestContext.functionCall->GetFunctionName(), *responseTargets.normalizedArgs)) {
        return false;
    }
    if (responseTargets.returnValue != nullptr &&
        !ResolveJsonResponseProperty(bridge.env, result, "value", requestContext.functionCall->GetFunctionName(),
            *responseTargets.returnValue)) {
        return false;
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
    InvokeBridgeContext bridge = { .env = napiEnv_, .invokeLocalFunctionRef = invokeLocalFunctionRef_ };
    InvokeRequestContext request = {
        .renderId = renderId, .surfaceId = &surfaceId, .componentId = &componentId, .functionCall = functionCall.get()
    };
    return InvokeInternal(bridge, request, InvokeResponseTargets {});
}

bool FunctionBridge::Invoke(const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall) const
{
    return Invoke(ResolveRenderIdBySurfaceId(surfaceId), surfaceId, componentId, functionCall);
}

bool FunctionBridge::InvokeForValue(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& returnValue) const
{
    InvokeBridgeContext bridge = { .env = napiEnv_, .invokeLocalFunctionRef = invokeLocalFunctionRef_ };
    InvokeRequestContext request = {
        .renderId = renderId, .surfaceId = &surfaceId, .componentId = &componentId, .functionCall = functionCall.get()
    };
    InvokeResponseTargets responseTargets = { .returnValue = &returnValue };
    return InvokeInternal(bridge, request, responseTargets);
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
    InvokeBridgeContext bridge = { .env = napiEnv_, .invokeLocalFunctionRef = invokeLocalFunctionRef_ };
    InvokeRequestContext request = { .renderId = renderId,
        .surfaceId = &surfaceId,
        .componentId = &componentId,
        .functionCall = functionCall.get(),
        .normalizeOnly = true };
    InvokeResponseTargets responseTargets = { .normalizedArgs = &normalizedArgs,
        .normalizedReturnType = &normalizedReturnType };
    return InvokeInternal(bridge, request, responseTargets);
}

bool FunctionBridge::NormalizeFunctionCall(const std::string& surfaceId, const std::string& componentId,
    const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& normalizedArgs,
    std::string& normalizedReturnType) const
{
    return NormalizeFunctionCall(ResolveRenderIdBySurfaceId(surfaceId), surfaceId, componentId, functionCall,
        normalizedArgs, normalizedReturnType);
}

} // namespace NativeModule
