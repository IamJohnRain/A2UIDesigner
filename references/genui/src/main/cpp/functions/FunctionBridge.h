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

#ifndef A2UI_FUNCTION_BRIDGE_H
#define A2UI_FUNCTION_BRIDGE_H

#include <cstdint>
#include <memory>
#include <string>

#include "FunctionCallInfo.h"
#include "napi/native_api.h"

namespace NativeModule {

class FunctionBridge final {
public:
    static FunctionBridge& GetInstance();

    void RegisterInvokeLocalFunction(napi_env env, napi_value callback);
    bool Invoke(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall) const;
    bool Invoke(const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall) const;
    bool InvokeForValue(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& returnValue) const;
    bool InvokeForValue(const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& returnValue) const;
    bool NormalizeFunctionCall(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& normalizedArgs,
        std::string& normalizedReturnType) const;
    bool NormalizeFunctionCall(const std::string& surfaceId, const std::string& componentId,
        const std::shared_ptr<FunctionCallInfo>& functionCall, JsonValue& normalizedArgs,
        std::string& normalizedReturnType) const;

private:
    FunctionBridge() = default;
    ~FunctionBridge() = default;
    FunctionBridge(const FunctionBridge&) = delete;
    FunctionBridge& operator=(const FunctionBridge&) = delete;

    napi_env napiEnv_ = nullptr;
    napi_ref invokeLocalFunctionRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_FUNCTION_BRIDGE_H
