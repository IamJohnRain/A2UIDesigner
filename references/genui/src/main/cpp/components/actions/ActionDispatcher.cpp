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

#include "components/actions/ActionDispatcher.h"

#include "components/actions/NativeActionRegistry.h"
#include "functions/FunctionBridge.h"
#include "functions/FunctionCallInfo.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

bool NativeActionDispatcher::CanDispatch(const std::string& callName) const
{
    return NativeActionRegistry::GetInstance().HasAction(callName);
}

JsonValue NativeActionDispatcher::Dispatch(
    const std::string& callName, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context)
{
    return NativeActionRegistry::GetInstance().Execute(callName, args, context);
}

bool NativeFunctionDispatcher::CanDispatch(const std::string& callName) const
{
    return NativeFunctionRegistry::GetInstance().HasFunction(callName);
}

JsonValue NativeFunctionDispatcher::Dispatch(
    const std::string& callName, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context)
{
    DynamicResolveContext resolveContext = {
        .renderId = context.renderId, .surfaceId = context.surfaceId, .componentId = context.componentId
    };
    auto fnResult = NativeFunctionRegistry::GetInstance().Execute(callName, args, resolveContext);
    if (fnResult.success) {
        std::unique_ptr<JsonAdapter> ownedValue = JsonAdapter::Clone(fnResult.value);
        if (ownedValue == nullptr) {
            return JsonValue();
        }
        context.ownedValues.push_back(std::move(ownedValue));
        return context.ownedValues.back()->GetRoot();
    }
    return JsonValue();
}

bool BridgeFunctionDispatcher::CanDispatch(const std::string& callName) const
{
    static_cast<void>(callName);
    return false;
}

JsonValue BridgeFunctionDispatcher::Dispatch(
    const std::string& callName, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context)
{
    auto functionCall = std::make_shared<FunctionCallInfo>(callName, args, "void");
    bool invoked =
        FunctionBridge::GetInstance().Invoke(context.renderId, context.surfaceId, context.componentId, functionCall);
    if (!invoked) {
        LOG_A2UI(LOG_WARN, "BridgeFunctionDispatcher: invoke failed, call='%{public}s', componentId=%{public}s",
            callName.c_str(), context.componentId.c_str());
    }
    return JsonValue();
}

ActionDispatcherList CreateDefaultDispatchers()
{
    ActionDispatcherList dispatchers;
    dispatchers.push_back(std::make_unique<NativeActionDispatcher>());
    dispatchers.push_back(std::make_unique<NativeFunctionDispatcher>());
    dispatchers.push_back(std::make_unique<BridgeFunctionDispatcher>());
    return dispatchers;
}

} // namespace NativeModule
