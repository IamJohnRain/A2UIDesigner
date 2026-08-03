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

#ifndef A2UI_ACTION_DISPATCHER_H
#define A2UI_ACTION_DISPATCHER_H

#include <memory>
#include <string>
#include <vector>

#include "components/actions/EventHandlerChainExecutor.h"
#include "utils/JsonAdapter.h"

namespace NativeModule {

class ActionDispatcher {
public:
    virtual ~ActionDispatcher() = default;
    virtual bool CanDispatch(const std::string& callName) const = 0;
    virtual JsonValue Dispatch(
        const std::string& callName, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context) = 0;
};

class NativeActionDispatcher final : public ActionDispatcher {
public:
    bool CanDispatch(const std::string& callName) const override;
    JsonValue Dispatch(const std::string& callName, const JsonValue& args,
        EventHandlerChainExecutor::ExecutionContext& context) override;
};

class NativeFunctionDispatcher final : public ActionDispatcher {
public:
    bool CanDispatch(const std::string& callName) const override;
    JsonValue Dispatch(const std::string& callName, const JsonValue& args,
        EventHandlerChainExecutor::ExecutionContext& context) override;
};

class BridgeFunctionDispatcher final : public ActionDispatcher {
public:
    bool CanDispatch(const std::string& callName) const override;
    JsonValue Dispatch(const std::string& callName, const JsonValue& args,
        EventHandlerChainExecutor::ExecutionContext& context) override;
};

using ActionDispatcherList = std::vector<std::unique_ptr<ActionDispatcher>>;

ActionDispatcherList CreateDefaultDispatchers();

} // namespace NativeModule

#endif // A2UI_ACTION_DISPATCHER_H
