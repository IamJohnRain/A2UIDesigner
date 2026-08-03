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

#include "ActionInfo.h"

namespace NativeModule {

ActionInfo::ActionInfo(const std::shared_ptr<FunctionCallInfo>& functionCall, const JsonValue& functionCallDescriptor)
    : type_(ActionType::FUNCTION_CALL), functionCall_(functionCall), functionCallDescriptor_(functionCallDescriptor)
{}

ActionInfo::ActionInfo(const std::string& eventName, const JsonValue& eventContextDescriptor)
    : type_(ActionType::EVENT), eventName_(eventName), eventContextDescriptor_(eventContextDescriptor)
{}

ActionType ActionInfo::GetType() const
{
    return type_;
}

const std::shared_ptr<FunctionCallInfo>& ActionInfo::GetFunctionCall() const
{
    return functionCall_;
}

const JsonValue& ActionInfo::GetFunctionCallDescriptor() const
{
    return functionCallDescriptor_;
}

const std::string& ActionInfo::GetEventName() const
{
    return eventName_;
}

const JsonValue& ActionInfo::GetEventContextDescriptor() const
{
    return eventContextDescriptor_;
}

bool ActionInfo::IsValid() const
{
    if (type_ == ActionType::FUNCTION_CALL) {
        return functionCall_ != nullptr && !functionCall_->GetFunctionName().empty();
    }
    if (type_ == ActionType::EVENT) {
        return !eventName_.empty();
    }
    return false;
}

} // namespace NativeModule
