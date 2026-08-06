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

#ifndef A2UI_ACTION_INFO_H
#define A2UI_ACTION_INFO_H

#include <memory>
#include <string>

#include "../utils/JsonAdapter.h"
#include "FunctionCallInfo.h"

namespace NativeModule {

enum class ActionType { UNKNOWN = 0, FUNCTION_CALL = 1, EVENT = 2 };

class ActionInfo {
public:
    ActionInfo() = default;
    explicit ActionInfo(
        const std::shared_ptr<FunctionCallInfo>& functionCall, const JsonValue& functionCallDescriptor = JsonValue());
    ActionInfo(const std::string& eventName, const JsonValue& eventContextDescriptor);
    ~ActionInfo() = default;

    ActionType GetType() const;
    const std::shared_ptr<FunctionCallInfo>& GetFunctionCall() const;
    const JsonValue& GetFunctionCallDescriptor() const;
    const std::string& GetEventName() const;
    const JsonValue& GetEventContextDescriptor() const;
    bool IsValid() const;

private:
    ActionType type_ = ActionType::UNKNOWN;
    std::shared_ptr<FunctionCallInfo> functionCall_;
    JsonValue functionCallDescriptor_;
    std::string eventName_;
    JsonValue eventContextDescriptor_;
};

} // namespace NativeModule

#endif // A2UI_ACTION_INFO_H
