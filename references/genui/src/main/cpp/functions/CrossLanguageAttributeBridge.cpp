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

#include "CrossLanguageAttributeBridge.h"

#include "utils/LogA2UI.h"

#include "NapiBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

} // namespace

CrossLanguageAttributeBridge& CrossLanguageAttributeBridge::GetInstance()
{
    static CrossLanguageAttributeBridge instance;
    return instance;
}

void CrossLanguageAttributeBridge::RegisterCrossLanguageCallback(napi_env env, napi_value callback)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterCrossLanguageCallback: invalid callback");
        return;
    }

    if (callbackRef_ != nullptr && napiEnv_ != nullptr) {
        napi.DeleteReference(napiEnv_, callbackRef_);
        callbackRef_ = nullptr;
    }

    napiEnv_ = env;
    if (!IsNapiOk(napi.CreateReference(env, callback, 1, &callbackRef_))) {
        LOG_A2UI(LOG_ERROR, "RegisterCrossLanguageCallback: create reference failed");
        callbackRef_ = nullptr;
        return;
    }

    LOG_A2UI(LOG_INFO, "RegisterCrossLanguageCallback: success");
}

bool CrossLanguageAttributeBridge::Dispatch(const CrossLanguageAttributeRequest& request) const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    LOG_A2UI(LOG_DEBUG,
        "CrossLanguageAttributeBridge::Dispatch: sending, renderId=%{public}d, componentId=%{public}s, "
        "uniqueId=%{public}d, componentType=%{public}s, attribute=%{public}s",
        request.renderId, request.componentId.c_str(), request.nodeUniqueId, request.componentType.c_str(),
        request.attributeName.c_str());
    if (napiEnv_ == nullptr || callbackRef_ == nullptr) {
        LOG_A2UI(LOG_WARN, "CrossLanguageAttributeBridge::Dispatch: bridge is invalid");
        return false;
    }

    napi_value callback = nullptr;
    if (!IsNapiOk(napi.GetReferenceValue(napiEnv_, callbackRef_, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "CrossLanguageAttributeBridge::Dispatch: get callback failed");
        return false;
    }

    napi_value obj = nullptr;
    if (!IsNapiOk(napi.CreateObject(napiEnv_, &obj)) || obj == nullptr) {
        LOG_A2UI(LOG_ERROR, "CrossLanguageAttributeBridge::Dispatch: create object failed");
        return false;
    }

    napi_value value = nullptr;
    napi.CreateInt32(napiEnv_, request.renderId, &value);
    napi.SetNamedProperty(napiEnv_, obj, "renderId", value);

    value = nullptr;
    napi.CreateStringUtf8(napiEnv_, request.componentId.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, obj, "componentId", value);

    value = nullptr;
    napi.CreateInt32(napiEnv_, request.nodeUniqueId, &value);
    napi.SetNamedProperty(napiEnv_, obj, "nodeUniqueId", value);

    value = nullptr;
    napi.CreateStringUtf8(napiEnv_, request.componentType.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, obj, "componentType", value);

    napi.CreateStringUtf8(napiEnv_, request.attributeName.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, obj, "attributeName", value);

    napi.CreateDouble(napiEnv_, static_cast<double>(request.floatValue), &value);
    napi.SetNamedProperty(napiEnv_, obj, "floatValue", value);

    value = nullptr;
    napi.CreateStringUtf8(napiEnv_, request.stringValue.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, obj, "stringValue", value);

    value = nullptr;
    napi.CreateStringUtf8(napiEnv_, request.payloadJson.c_str(), NAPI_AUTO_LENGTH, &value);
    napi.SetNamedProperty(napiEnv_, obj, "payloadJson", value);

    value = nullptr;
    napi.GetBoolean(napiEnv_, request.reset, &value);
    napi.SetNamedProperty(napiEnv_, obj, "reset", value);

    napi_value global = nullptr;
    napi.GetGlobal(napiEnv_, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(napiEnv_, global, callback, 1, &obj, &result))) {
        LOG_A2UI(LOG_ERROR, "CrossLanguageAttributeBridge::Dispatch: call bridge failed");
        return false;
    }

    LOG_A2UI(LOG_DEBUG,
        "CrossLanguageAttributeBridge::Dispatch: success, renderId=%{public}d, componentId=%{public}s, "
        "attribute=%{public}s",
        request.renderId, request.componentId.c_str(), request.attributeName.c_str());
    return true;
}

} // namespace NativeModule
