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

#include "checks/ChecksEngine.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include "data/DynamicValueResolver.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr char CHECK_DEFAULT_MESSAGE[] = "Invalid value";

bool IsLegacyCheckFunction(const std::string& normalizedFunctionName)
{
    return normalizedFunctionName == "required" || normalizedFunctionName == "regex" ||
           normalizedFunctionName == "length" || normalizedFunctionName == "numeric" ||
           normalizedFunctionName == "email";
}

bool JsonValueToConditionBool(const JsonValue& value, bool fallback = false)
{
    if (!value.IsValid()) {
        return fallback;
    }
    if (value.IsNull()) {
        return false;
    }
    if (value.IsBool()) {
        return value.GetBoolValue(fallback);
    }
    return true;
}

bool InjectDefaultTargetValue(JsonValue& conditionValue, const ChecksDefaultTargetProvider& defaultTargetProvider)
{
    if (!conditionValue.IsObject() || !conditionValue.Has("call") || defaultTargetProvider == nullptr) {
        return true;
    }

    JsonValue argsValue = conditionValue.GetItem("args");
    if (argsValue.IsValid() && (!argsValue.IsObject() || argsValue.Has("value"))) {
        return true;
    }

    JsonValue targetValue;
    if (!defaultTargetProvider(targetValue) || !targetValue.IsValid()) {
        return true;
    }

    if (!argsValue.IsValid()) {
        argsValue = conditionValue.PutObject("args");
    }
    if (!argsValue.IsValid() || !argsValue.IsObject()) {
        return false;
    }
    return argsValue.Put("value", targetValue);
}
} // namespace

ChecksEngine::ChecksEngine(
    ChecksResolveContextProvider contextProvider, ChecksDefaultTargetProvider defaultTargetProvider)
    : contextProvider_(std::move(contextProvider)), defaultTargetProvider_(std::move(defaultTargetProvider))
{}

ChecksResolveContext ChecksEngine::BuildContext() const
{
    if (contextProvider_ == nullptr) {
        return {};
    }
    return contextProvider_();
}

void ChecksEngine::CollectCheckBindingPaths(const JsonValue& value, std::unordered_set<std::string>& paths) const
{
    if (!value.IsValid()) {
        return;
    }
    if (value.IsObject()) {
        if (value.Has("path")) {
            JsonValue pathValue = value.GetItem("path");
            if (pathValue.IsString()) {
                std::string path = pathValue.GetStringValue("");
                if (!path.empty()) {
                    paths.insert(path);
                }
            }
        }
        JsonValue child = value.GetChild();
        while (child.IsValid()) {
            CollectCheckBindingPaths(child, paths);
            child = child.GetNext();
        }
        return;
    }
    if (value.IsArray()) {
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            CollectCheckBindingPaths(value.GetArrayItem(index), paths);
        }
    }
}

void ChecksEngine::ParseChecks(const JsonValue& descriptor)
{
    checks_.clear();
    bindingPaths_.clear();

    JsonValue checksValue = descriptor;
    if (descriptor.IsObject() && descriptor.Has("checks")) {
        checksValue = descriptor.GetItem("checks");
    }
    if (!checksValue.IsArray()) {
        return;
    }

    int checkCount = checksValue.GetArraySize();
    for (int index = 0; index < checkCount; ++index) {
        JsonValue checkValue = checksValue.GetArrayItem(index);
        if (!checkValue.IsValid()) {
            continue;
        }
        if (!checkValue.IsObject()) {
            LOG_A2UI(LOG_WARN, "ChecksEngine::ParseChecks: check item must be object");
            continue;
        }

        JsonValue conditionValue = checkValue.GetItem("condition");
        if (!conditionValue.IsValid() || !IsLegacyCheckFunction(conditionValue.GetString("call", ""))) {
            LOG_A2UI(LOG_WARN, "ChecksEngine::ParseChecks: check item missing condition");
            continue;
        } else {
            CollectCheckBindingPaths(conditionValue, bindingPaths_);
        }

        CheckRule rule = { .conditionValue = conditionValue,
            .message = checkValue.GetString("message", CHECK_DEFAULT_MESSAGE) };
        checks_.push_back(std::move(rule));
    }
}

bool ChecksEngine::Validate(std::string* firstFailedMessage) const
{
    if (firstFailedMessage != nullptr) {
        firstFailedMessage->clear();
    }
    for (const auto& checkRule : checks_) {
        if (!ValidateSingleCheck(checkRule, firstFailedMessage)) {
            return false;
        }
    }
    return true;
}

const std::unordered_set<std::string>& ChecksEngine::GetBindingPaths() const
{
    return bindingPaths_;
}

bool ChecksEngine::ValidateSingleCheck(const CheckRule& checkRule, std::string* failedMessage) const
{
    if (!checkRule.conditionValue.IsValid()) {
        if (failedMessage != nullptr) {
            *failedMessage = checkRule.message.empty() ? CHECK_DEFAULT_MESSAGE : checkRule.message;
        }
        LOG_A2UI(LOG_WARN, "ChecksEngine::ValidateSingleCheck: invalid check condition");
        return false;
    }

    bool pass = EvaluateCondition(checkRule.conditionValue);
    if (!pass && failedMessage != nullptr) {
        *failedMessage = checkRule.message.empty() ? CHECK_DEFAULT_MESSAGE : checkRule.message;
    }
    return pass;
}

bool ChecksEngine::EvaluateCondition(const JsonValue& conditionValue) const
{
    if (!conditionValue.IsValid()) {
        return false;
    }

    JsonValue evaluatingCondition = conditionValue;
    std::unique_ptr<JsonAdapter> normalizedAdapter;
    if (conditionValue.IsObject() && conditionValue.Has("call")) {
        normalizedAdapter = JsonAdapter::Clone(conditionValue);
        if (normalizedAdapter == nullptr) {
            return false;
        }
        evaluatingCondition = normalizedAdapter->GetRoot();
        if (!InjectDefaultTargetValue(evaluatingCondition, defaultTargetProvider_)) {
            return false;
        }
    }

    ChecksResolveContext contextInfo = BuildContext();
    DynamicResolveContext context = {
        .renderId = contextInfo.renderId, .surfaceId = contextInfo.surfaceId, .componentId = contextInfo.componentId
    };
    ResolvedValue resolved = DynamicValueResolver::Resolve(evaluatingCondition, context);
    if (!resolved.success) {
        return false;
    }
    return JsonValueToConditionBool(resolved.value, false);
}
} // namespace NativeModule
