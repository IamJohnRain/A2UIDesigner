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

#include "components/actions/NativeActionRegistry.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

NativeActionRegistry& NativeActionRegistry::GetInstance()
{
    static NativeActionRegistry instance;
    return instance;
}

void NativeActionRegistry::Register(const std::string& name, NativeActionHandler handler)
{
    actions_[name] = std::move(handler);
}

bool NativeActionRegistry::HasAction(const std::string& name) const
{
    return actions_.find(name) != actions_.end();
}

JsonValue NativeActionRegistry::Execute(
    const std::string& name, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context)
{
    auto it = actions_.find(name);
    if (it == actions_.end()) {
        LOG_A2UI(LOG_WARN, "NativeActionRegistry::Execute - action not found, name=%{public}s, componentId=%{public}s",
            name.c_str(), context.componentId.c_str());
        return JsonValue();
    }
    return it->second(args, context);
}

void NativeActionRegistry::Clear()
{
    actions_.clear();
}

} // namespace NativeModule
