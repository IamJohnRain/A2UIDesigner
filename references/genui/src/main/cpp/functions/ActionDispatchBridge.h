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

#ifndef A2UI_ACTION_DISPATCH_BRIDGE_H
#define A2UI_ACTION_DISPATCH_BRIDGE_H

#include <cstdint>
#include <string>

#include "utils/JsonAdapter.h"

#include "napi/native_api.h"

namespace NativeModule {

class ActionDispatchBridge final {
public:
    static ActionDispatchBridge& GetInstance();

    void RegisterDispatchAction(napi_env env, napi_value callback);
    bool Dispatch(int32_t renderId, const std::string& surfaceId, const std::string& sourceComponentId,
        const std::string& eventName, const JsonValue& context) const;

private:
    ActionDispatchBridge() = default;
    ~ActionDispatchBridge() = default;
    ActionDispatchBridge(const ActionDispatchBridge&) = delete;
    ActionDispatchBridge& operator=(const ActionDispatchBridge&) = delete;

    napi_env napiEnv_ = nullptr;
    napi_ref dispatchActionRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_ACTION_DISPATCH_BRIDGE_H
