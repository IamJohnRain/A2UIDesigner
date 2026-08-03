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

#include "WarningDispatchBridge.h"

#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "NapiBridge.h"

namespace NativeModule {

namespace {

bool IsNapiOk(napi_status status)
{
    return status == napi_ok;
}

void SetStringProperty(INapiProvider& napi, napi_env env, napi_value object, const char* key, const std::string& value)
{
    napi_value propertyValue = nullptr;
    napi.CreateStringUtf8(env, value.c_str(), NAPI_AUTO_LENGTH, &propertyValue);
    napi.SetNamedProperty(env, object, key, propertyValue);
}

} // namespace

WarningDispatchBridge& WarningDispatchBridge::GetInstance()
{
    static WarningDispatchBridge instance;
    return instance;
}

void WarningDispatchBridge::RegisterDispatchWarning(napi_env env, napi_value callback)
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (env == nullptr || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchWarning: invalid callback");
        return;
    }

    if (dispatchWarningRef_ != nullptr && napiEnv_ != nullptr) {
        napi.DeleteReference(napiEnv_, dispatchWarningRef_);
        dispatchWarningRef_ = nullptr;
    }

    napiEnv_ = env;
    if (!IsNapiOk(napi.CreateReference(env, callback, 1, &dispatchWarningRef_))) {
        LOG_A2UI(LOG_ERROR, "RegisterDispatchWarning: create reference failed");
        dispatchWarningRef_ = nullptr;
        return;
    }

    LOG_A2UI(LOG_INFO, "RegisterDispatchWarning: success");
}

bool WarningDispatchBridge::Dispatch(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const std::string& code, const std::string& message, const std::string& path, const std::string& itemType,
    const std::string& itemName) const
{
    auto& napi = NapiBridge::GetInstance().Provider();
    if (napiEnv_ == nullptr || dispatchWarningRef_ == nullptr) {
        return false;
    }

    napi_value callback = nullptr;
    if (!IsNapiOk(napi.GetReferenceValue(napiEnv_, dispatchWarningRef_, &callback)) || callback == nullptr) {
        LOG_A2UI(LOG_ERROR, "WarningDispatchBridge::Dispatch: get callback failed");
        return false;
    }

    napi_value request = nullptr;
    if (!IsNapiOk(napi.CreateObject(napiEnv_, &request)) || request == nullptr) {
        LOG_A2UI(LOG_ERROR, "WarningDispatchBridge::Dispatch: create request failed");
        return false;
    }

    napi_value renderIdValue = nullptr;
    napi.CreateInt32(napiEnv_, renderId, &renderIdValue);
    napi.SetNamedProperty(napiEnv_, request, "renderId", renderIdValue);

    SetStringProperty(napi, napiEnv_, request, "surfaceId", surfaceId);
    SetStringProperty(napi, napiEnv_, request, "componentId", componentId);
    SetStringProperty(napi, napiEnv_, request, "code", code);
    SetStringProperty(napi, napiEnv_, request, "message", message);
    SetStringProperty(napi, napiEnv_, request, "path", path);
    SetStringProperty(napi, napiEnv_, request, "itemType", itemType);
    SetStringProperty(napi, napiEnv_, request, "itemName", itemName);

    napi_value global = nullptr;
    napi.GetGlobal(napiEnv_, &global);

    napi_value result = nullptr;
    if (!IsNapiOk(napi.CallFunction(napiEnv_, global, callback, 1, &request, &result))) {
        LOG_A2UI(LOG_ERROR, "WarningDispatchBridge::Dispatch: call bridge failed");
        return false;
    }

    return true;
}

} // namespace NativeModule
