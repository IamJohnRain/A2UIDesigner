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

#ifndef A2UI_WARNING_DISPATCH_BRIDGE_H
#define A2UI_WARNING_DISPATCH_BRIDGE_H

#include <cstdint>
#include <string>

#include "napi/native_api.h"

namespace NativeModule {

class WarningDispatchBridge final {
public:
    static WarningDispatchBridge& GetInstance();

    void RegisterDispatchWarning(napi_env env, napi_value callback);
    bool Dispatch(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
        const std::string& code, const std::string& message, const std::string& path, const std::string& itemType,
        const std::string& itemName) const;

private:
    WarningDispatchBridge() = default;
    ~WarningDispatchBridge() = default;
    WarningDispatchBridge(const WarningDispatchBridge&) = delete;
    WarningDispatchBridge& operator=(const WarningDispatchBridge&) = delete;

    napi_env napiEnv_ = nullptr;
    napi_ref dispatchWarningRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_WARNING_DISPATCH_BRIDGE_H
