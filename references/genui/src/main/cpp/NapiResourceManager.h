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

#ifndef A2UI_NAPI_RESOURCE_MANAGER_H
#define A2UI_NAPI_RESOURCE_MANAGER_H

#include <arkui/native_node_napi.h>

#include <js_native_api.h>

namespace NativeModule {

/**
 * NapiResourceManager - Manages process-level NAPI resources
 * This class manages NAPI environment and custom component callbacks
 * that are shared across all RenderSlots and SurfaceManagers
 */
class NapiResourceManager {
public:
    NapiResourceManager() = default;
    ~NapiResourceManager();

    // Disable copy and move
    NapiResourceManager(const NapiResourceManager&) = delete;
    NapiResourceManager& operator=(const NapiResourceManager&) = delete;
    NapiResourceManager(NapiResourceManager&&) = delete;
    NapiResourceManager& operator=(NapiResourceManager&&) = delete;

    /**
     * Register the create custom component callback
     * @param env The NAPI environment
     * @param info The NAPI callback info
     * @return NAPI value
     */
    napi_value RegisterCreateCustomComponent(napi_env env, napi_callback_info info);

    /**
     * Register the update custom component callback
     * @param env The NAPI environment
     * @param info The NAPI callback info
     * @return NAPI value
     */
    napi_value RegisterUpdateCustomComponent(napi_env env, napi_callback_info info);

    /**
     * Get the NAPI environment
     * @return The NAPI environment
     */
    napi_env GetNapiEnv() const
    {
        return napiEnv_;
    }

    /**
     * Get the create custom component reference
     * @return The create custom component reference
     */
    napi_ref GetCreateCustomComponentRef() const
    {
        return createCustomComponentRef_;
    }

    /**
     * Get the update custom component reference
     * @return The update custom component reference
     */
    napi_ref GetUpdateCustomComponentRef() const
    {
        return updateCustomComponentRef_;
    }

    /**
     * Check if custom component callbacks are registered
     * @return true if both callbacks are registered
     */
    bool IsCustomComponentRegistered() const
    {
        return napiEnv_ != nullptr && createCustomComponentRef_ != nullptr && updateCustomComponentRef_ != nullptr;
    }

private:
    napi_env napiEnv_ = nullptr;
    napi_ref createCustomComponentRef_ = nullptr;
    napi_ref updateCustomComponentRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_NAPI_RESOURCE_MANAGER_H
