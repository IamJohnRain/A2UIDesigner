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

#ifndef A2UI_NATIVE_PLURALIZE_FUNCTION_H
#define A2UI_NATIVE_PLURALIZE_FUNCTION_H

#include <chrono>
#include <string>

#include "NativeFunctionBase.h"
#include "napi/native_api.h"

namespace NativeModule {

class PluralLocaleManager {
public:
    static PluralLocaleManager& GetInstance();
    void RegisterLocaleProvider(napi_env env, napi_value callback);
    void SetLocale(const std::string& locale);
    std::string GetLocale();

private:
    PluralLocaleManager() = default;
    std::string CallProvider();
    napi_env env_ = nullptr;
    napi_ref providerRef_ = nullptr;
    std::string cachedLocale_ = "en";
    std::chrono::steady_clock::time_point lastCallTime_;
    static constexpr int64_t CACHE_TTL_MS = 1000;
};

class NativePluralizeFunction : public NativeFunctionBase {
public:
    std::string GetName() const override;
    FunctionResult Execute(const JsonValue& resolvedArgs) override;
};

} // namespace NativeModule

#endif // A2UI_NATIVE_PLURALIZE_FUNCTION_H
