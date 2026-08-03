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

#ifndef A2UI_CHECKS_ENGINE_H
#define A2UI_CHECKS_ENGINE_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils/JsonAdapter.h"

namespace NativeModule {

struct ChecksResolveContext {
    int32_t renderId = -1;
    std::string surfaceId;
    std::string componentId;
};

using ChecksResolveContextProvider = std::function<ChecksResolveContext(void)>;
using ChecksDefaultTargetProvider = std::function<bool(JsonValue&)>;

class ChecksEngine {
public:
    ChecksEngine(
        ChecksResolveContextProvider contextProvider, ChecksDefaultTargetProvider defaultTargetProvider = nullptr);

    void ParseChecks(const JsonValue& descriptor);
    bool Validate(std::string* firstFailedMessage = nullptr) const;
    const std::unordered_set<std::string>& GetBindingPaths() const;

private:
    struct CheckRule {
        JsonValue conditionValue;
        std::string message;
    };

    ChecksResolveContext BuildContext() const;
    void CollectCheckBindingPaths(const JsonValue& value, std::unordered_set<std::string>& paths) const;
    bool ValidateSingleCheck(const CheckRule& checkRule, std::string* failedMessage) const;
    bool EvaluateCondition(const JsonValue& conditionValue) const;

    ChecksResolveContextProvider contextProvider_;
    ChecksDefaultTargetProvider defaultTargetProvider_;
    std::vector<CheckRule> checks_;
    std::unordered_set<std::string> bindingPaths_;
};

} // namespace NativeModule

#endif // A2UI_CHECKS_ENGINE_H
