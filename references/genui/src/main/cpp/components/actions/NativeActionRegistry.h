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

#ifndef A2UI_NATIVE_ACTION_REGISTRY_H
#define A2UI_NATIVE_ACTION_REGISTRY_H

#include <functional>
#include <map>
#include <string>

#include "components/actions/EventHandlerChainExecutor.h"
#include "utils/JsonAdapter.h"

namespace NativeModule {

using NativeActionHandler =
    std::function<JsonValue(const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context)>;

class NativeActionRegistry final {
public:
    static NativeActionRegistry& GetInstance();

    void Register(const std::string& name, NativeActionHandler handler);
    bool HasAction(const std::string& name) const;
    JsonValue Execute(
        const std::string& name, const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& context);

    void Clear();

private:
    NativeActionRegistry() = default;
    std::map<std::string, NativeActionHandler> actions_;
};

} // namespace NativeModule

#endif // A2UI_NATIVE_ACTION_REGISTRY_H
