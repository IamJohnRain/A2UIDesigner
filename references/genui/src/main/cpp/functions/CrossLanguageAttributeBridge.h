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

#ifndef A2UI_CROSS_LANGUAGE_ATTRIBUTE_BRIDGE_H
#define A2UI_CROSS_LANGUAGE_ATTRIBUTE_BRIDGE_H

#include <cstdint>
#include <string>

#include "napi/native_api.h"

namespace NativeModule {

struct CrossLanguageAttributeRequest {
    int32_t renderId = -1;
    std::string componentId;
    int32_t nodeUniqueId = -1;
    std::string componentType;
    std::string attributeName;
    float floatValue = 0.0F;
    std::string stringValue;
    std::string payloadJson;
    bool reset = false;
};

class CrossLanguageAttributeBridge final {
public:
    static CrossLanguageAttributeBridge& GetInstance();

    void RegisterCrossLanguageCallback(napi_env env, napi_value callback);
    bool Dispatch(const CrossLanguageAttributeRequest& request) const;

private:
    CrossLanguageAttributeBridge() = default;
    ~CrossLanguageAttributeBridge() = default;
    CrossLanguageAttributeBridge(const CrossLanguageAttributeBridge&) = delete;
    CrossLanguageAttributeBridge& operator=(const CrossLanguageAttributeBridge&) = delete;

    napi_env napiEnv_ = nullptr;
    napi_ref callbackRef_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_CROSS_LANGUAGE_ATTRIBUTE_BRIDGE_H
