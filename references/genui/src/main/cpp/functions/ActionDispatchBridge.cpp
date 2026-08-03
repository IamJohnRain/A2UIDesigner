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

#include "ActionDispatchBridge.h"

#include "utils/LogA2UI.h"
#include "utils/NapiUtils.h"

#include "NapiBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

} // namespace

ActionDispatchBridge& ActionDispatchBridge::GetInstance()
{
    static ActionDispatchBridge instance;
    return instance;
}

void ActionDispatchBridge::RegisterDispatchAction(napi_env env, napi_value callback)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchAction: invalid callback");
        return;
    }

    if (dispatchActionRef_ != nullptr && napiEnv_ != nullptr) {
        napi.DeleteReference(napiEnv_, dispatchActionRef_);
        dispatchActionRef_ = nullptr;
    }

    napiEnv_ = env;
    if (!IsNapiOk(napi.CreateReference(env, callback, 1, &dispatchActionRef_))) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchAction: create reference failed");
        dispatchActionRef_ = nullptr;
        return;
    }

    LOG_A2UI(LOG_INFO, "RegisterDispatchAction: success");
}

bool ActionDispatchBridge::Dispatch(int32_t renderId, const std::string& surfaceId,
    const std::string& sourceComponentId, const std::string& eventName, const JsonValue& context) const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (napiEnv_ == nullptr || dispatchActionRef_ == nullptr) {
        LOG_A2UI(LOG_WARN, "ActionDispatchBridge::Dispatch: bridge is invalid");
        return false;
    }

    napi_value callback = nullptr;
    if (!IsNapiOk(napi.GetReferenceValue(napiEnv_, dispatchActionRef_, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "ActionDispatchBridge::Dispatch: get callback failed");
        return false;
    }

    napi_value request = nullptr;
    if (!IsNapiOk(napi.CreateObject(napiEnv_, &request)) || request == nullptr) {
        LOG_A2UI(LOG_ERROR, "ActionDispatchBridge::Dispatch: create request failed");
        return false;
    }

    napi_value value = nullptr;
    napi.CreateInt32(napiEnv_, renderId, &value);
    napi.SetNamedProperty(napiEnv_, request, "renderId", value);

    napi.CreateStringUtf8(napiEnv_, surfaceId.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, request, "surfaceId", value);

    napi.CreateStringUtf8(napiEnv_, sourceComponentId.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, request, "sourceComponentId", value);

    napi.CreateStringUtf8(napiEnv_, eventName.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, request, "name", value);

    value = JsonValueToNapiValue(napiEnv_, context);
    napi.SetNamedProperty(napiEnv_, request, "context", value);

    napi_value global = nullptr;
    napi.GetGlobal(napiEnv_, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(napiEnv_, global, callback, 1, &request, &result))) {
        LOG_A2UI(LOG_ERROR, "ActionDispatchBridge::Dispatch: call bridge failed");
        return false;
    }

    return true;
}

} // namespace NativeModule
