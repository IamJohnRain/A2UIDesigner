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

#ifndef A2UI_RUNTIME_ERROR_DISPATCH_BRIDGE_H
#define A2UI_RUNTIME_ERROR_DISPATCH_BRIDGE_H

#include <cstdint>
#include <string>

#include "napi/native_api.h"

namespace NativeModule {

class RuntimeErrorDispatchBridge final {
public:
    static RuntimeErrorDispatchBridge& GetInstance();

    void RegisterDispatchRuntimeError(napi_env env, napi_value callback);
    bool Dispatch(int32_t renderId, const std::string& surfaceId, const std::string& componentId, int32_t errorCode,
        const std::string& errorMessage, const std::string& source) const;

private:
    RuntimeErrorDispatchBridge() = default;
    ~RuntimeErrorDispatchBridge() = default;
    RuntimeErrorDispatchBridge(const RuntimeErrorDispatchBridge&) = delete;
    RuntimeErrorDispatchBridge& operator=(const RuntimeErrorDispatchBridge&) = delete;

    napi_env napiEnv_ = nullptr;
    napi_ref dispatchRuntimeErrorRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_RUNTIME_ERROR_DISPATCH_BRIDGE_H
