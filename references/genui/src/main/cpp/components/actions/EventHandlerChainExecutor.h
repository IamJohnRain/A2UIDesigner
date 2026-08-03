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

#ifndef A2UI_EVENT_HANDLER_CHAIN_EXECUTOR_H
#define A2UI_EVENT_HANDLER_CHAIN_EXECUTOR_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/actions/EventHandlerParser.h"
#include "utils/JsonAdapter.h"

namespace NativeModule {

class DataModel;

class EventHandlerChainExecutor final {
public:
    struct ExecutionContext {
        int32_t renderId = 0;
        std::string surfaceId;
        std::string componentId;
        std::shared_ptr<DataModel> dataModel;
        JsonValue eventContext;
        JsonValue externalEventContext;
        bool hasExternalEventContext = false;
        std::map<std::string, JsonValue> localVariables;
        std::vector<std::unique_ptr<JsonAdapter>> ownedValues;
    };

    static void ExecuteChain(const std::vector<EventHandlerStep>& handlers, ExecutionContext& context);
};

struct DispatchParams {
    const EventHandlerMap& handlers;
    const std::string& eventName;
    const std::string& surfaceId;
    const std::string& componentId;
    int32_t renderId;
    const JsonValue& extraContext;
    const std::map<std::string, JsonValue>* localVariables = nullptr;
};

bool DispatchEventToHandlers(const DispatchParams& params);

} // namespace NativeModule

#endif // A2UI_EVENT_HANDLER_CHAIN_EXECUTOR_H
