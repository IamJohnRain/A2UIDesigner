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

#include "RuntimeErrorDispatchBridge.h"

#include "utils/LogA2UI.h"

#include "NapiBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

void SetStringProperty(napi_env env, napi_value object, const char* key, const std::string& value)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value propertyValue = nullptr;
    napi.CreateStringUtf8(env, value.c_str(), NAPI_AUTO_LENGTH, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

void SetInt32Property(napi_env env, napi_value object, const char* key, int32_t value)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    napi_value propertyValue = nullptr;
    napi.CreateInt32(env, value, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

} // namespace

RuntimeErrorDispatchBridge& RuntimeErrorDispatchBridge::GetInstance()
{
    static RuntimeErrorDispatchBridge instance;
    return instance;
}

void RuntimeErrorDispatchBridge::RegisterDispatchRuntimeError(napi_env env, napi_value callback)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchRuntimeError: invalid callback");
        return;
    }

    if (dispatchRuntimeErrorRef_ != nullptr && napiEnv_ != nullptr) {
        napi.DeleteReference(napiEnv_, dispatchRuntimeErrorRef_);
        dispatchRuntimeErrorRef_ = nullptr;
    }

    napiEnv_ = env;
    if (!IsNapiOk(napi.CreateReference(env, callback, 1, &dispatchRuntimeErrorRef_))) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchRuntimeError: create reference failed");
        dispatchRuntimeErrorRef_ = nullptr;
        return;
    }

    LOG_A2UI(LOG_INFO, "RegisterDispatchRuntimeError: success");
}

bool RuntimeErrorDispatchBridge::Dispatch(int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, int32_t errorCode, const std::string& errorMessage, const std::string& source) const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (napiEnv_ == nullptr || dispatchRuntimeErrorRef_ == nullptr) {
        return false;
    }

    napi_value callback = nullptr;
    if (!IsNapiOk(napi.GetReferenceValue(napiEnv_, dispatchRuntimeErrorRef_, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RuntimeErrorDispatchBridge::Dispatch: get callback failed");
        return false;
    }

    napi_value request = nullptr;
    if (!IsNapiOk(napi.CreateObject(napiEnv_, &request)) || request == nullptr) {
        LOG_A2UI(LOG_ERROR, "RuntimeErrorDispatchBridge::Dispatch: create request failed");
        return false;
    }

    SetInt32Property(napiEnv_, request, "renderId", renderId);
    SetStringProperty(napiEnv_, request, "surfaceId", surfaceId);
    SetStringProperty(napiEnv_, request, "componentId", componentId);
    SetInt32Property(napiEnv_, request, "errorCode", errorCode);
    SetStringProperty(napiEnv_, request, "errorMessage", errorMessage);
    SetStringProperty(napiEnv_, request, "source", source);

    napi_value global = nullptr;
    napi.GetGlobal(napiEnv_, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(napiEnv_, global, callback, 1, &request, &result))) {
        LOG_A2UI(LOG_ERROR, "RuntimeErrorDispatchBridge::Dispatch: call bridge failed");
        return false;
    }

    return true;
}

} // namespace NativeModule
