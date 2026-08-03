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

#include "NapiResourceManager.h"

#include "utils/LogA2UI.h"
#include "utils/NapiUtils.h"

#include "NapiBridge.h"

namespace NativeModule {

NapiResourceManager::~NapiResourceManager()
{
    // Clean up NAPI references
    if (createCustomComponentRef_ != nullptr && napiEnv_ != nullptr) {
        NapiBridge::GetInstance().Provider().DeleteReference(napiEnv_, createCustomComponentRef_);
        createCustomComponentRef_ = nullptr;
    }
    if (updateCustomComponentRef_ != nullptr && napiEnv_ != nullptr) {
        NapiBridge::GetInstance().Provider().DeleteReference(napiEnv_, updateCustomComponentRef_);
        updateCustomComponentRef_ = nullptr;
    }
    napiEnv_ = nullptr;
}

napi_value NapiResourceManager::RegisterCreateCustomComponent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterCreateCustomComponent: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }
    if (createCustomComponentRef_ != nullptr) {
        LOG_A2UI(LOG_WARN, "RegisterCreateCustomComponent: already registered");
        return nullptr;
    }
    napiEnv_ = env;
    NapiBridge::GetInstance().Provider().CreateReference(env, args[0], 1, &createCustomComponentRef_);
    LOG_A2UI(LOG_INFO, "RegisterCreateCustomComponent success");
    return nullptr;
}

napi_value NapiResourceManager::RegisterUpdateCustomComponent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NapiBridge::GetInstance().Provider().GetCbInfo(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || !NapiIsFunctionValue(env, args[0])) {
        LOG_A2UI(LOG_ERROR, "RegisterUpdateCustomComponent: invalid args, argc=%{public}zu", argc);
        return nullptr;
    }
    if (updateCustomComponentRef_ != nullptr) {
        LOG_A2UI(LOG_WARN, "RegisterUpdateCustomComponent: already registered");
        return nullptr;
    }
    napiEnv_ = env;
    NapiBridge::GetInstance().Provider().CreateReference(env, args[0], 1, &updateCustomComponentRef_);
    LOG_A2UI(LOG_INFO, "RegisterUpdateCustomComponent success");
    return nullptr;
}

} // namespace NativeModule
