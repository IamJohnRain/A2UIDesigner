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

#ifndef A2UI_SCHEMA_WARNING_TEST_HELPER_H
#define A2UI_SCHEMA_WARNING_TEST_HELPER_H

#include <string>

#include "functions/WarningDispatchBridge.h"

#include "include/mock_napi_provider.h"

namespace NativeModule::TestHelpers {

struct DispatchCallbacks {
    napi_env env = nullptr;
    napi_value warningCallback = nullptr;
};

inline DispatchCallbacks RegisterWarningDispatchCallback(MockNapiProvider* mockNapi)
{
    DispatchCallbacks callbacks;
    if (mockNapi == nullptr) {
        return callbacks;
    }

    callbacks.env = reinterpret_cast<napi_env>(0x1400);
    mockNapi->CreateFunction(
        callbacks.env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &callbacks.warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(callbacks.env, callbacks.warningCallback);
    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionArgs_.clear();
    mockNapi->callFunctionArgsHistory_.clear();
    return callbacks;
}

inline napi_value GetRequestProperty(const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    if (mockNapi == nullptr || request == nullptr) {
        return nullptr;
    }

    auto objectIt = mockNapi->objectProperties_.find(request);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return nullptr;
    }

    auto propertyIt = objectIt->second.find(key);
    return propertyIt == objectIt->second.end() ? nullptr : propertyIt->second;
}

inline std::string GetRequestStringProperty(
    const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    napi_value value = GetRequestProperty(mockNapi, request, key);
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }

    auto stringIt = mockNapi->stringValues_.find(value);
    return stringIt == mockNapi->stringValues_.end() ? "" : stringIt->second;
}

inline size_t CountWarningRequests(
    const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    if (mockNapi == nullptr) {
        return 0;
    }

    size_t count = 0;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }

        napi_value request = args[0];
        std::string warningCode = GetRequestStringProperty(mockNapi, request, "code");
        std::string warningPath = GetRequestStringProperty(mockNapi, request, "path");
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

} // namespace NativeModule::TestHelpers

#endif // A2UI_SCHEMA_WARNING_TEST_HELPER_H
