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

#include "components/actions/EventHandlerParser.h"

#include "utils/LocalVariableNameUtils.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

const std::set<std::string> EventHandlerParser::KNOWN_EVENT_NAMES = { "onClick", "onAppear", "onChange", "onSelect",
    "onReachStart", "onReachEnd" };

EventHandlerMap EventHandlerParser::Parse(const JsonValue& descriptor)
{
    EventHandlerMap result;

    if (!descriptor.IsObject()) {
        return result;
    }

    JsonValue child = descriptor.GetChild();
    while (child.IsValid()) {
        std::string key = child.GetKey();
        if (KNOWN_EVENT_NAMES.count(key) > 0 && child.IsArray()) {
            auto handlers = ParseHandlerArray(child);
            if (!handlers.empty()) {
                EventListenerInfo info;
                info.eventName = key;
                info.handlers = std::move(handlers);
                result[key] = std::move(info);
            }
        }
        child = child.GetNext();
    }

    return result;
}

std::vector<EventHandlerStep> EventHandlerParser::ParseHandlerArray(const JsonValue& array)
{
    std::vector<EventHandlerStep> handlers;
    int size = array.GetArraySize();
    for (int i = 0; i < size; ++i) {
        JsonValue item = array.GetArrayItem(i);
        if (!item.IsObject()) {
            LOG_A2UI(LOG_WARN, "EventHandlerParser: handler[%{public}d] is not an object, skipping", i);
            continue;
        }

        EventHandlerStep step;

        JsonValue callValue = item.GetItem("call");
        if (!callValue.IsString()) {
            LOG_A2UI(LOG_WARN, "EventHandlerParser: handler[%{public}d] 'call' is not a string, skipping", i);
            continue;
        }
        step.call = callValue.GetStringValue();

        if (item.Has("args")) {
            step.args = item.GetItem("args");
        }

        JsonValue conditionValue = item.GetItem("condition");
        if (conditionValue.IsString()) {
            step.condition = conditionValue.GetStringValue();
        } else if (conditionValue.IsObject()) {
            LOG_A2UI(LOG_WARN, "EventHandlerParser: handler[%{public}d] 'condition' is an object, skipping", i);
            continue;
        }

        JsonValue asValue = item.GetItem("as");
        if (asValue.IsValid() && !asValue.IsNull()) {
            if (!asValue.IsString()) {
                LOG_A2UI(LOG_WARN, "EventHandlerParser: handler[%{public}d] 'as' is not a string, skipping", i);
                continue;
            }
            std::string asName = asValue.GetStringValue();
            if (!IsValidLocalVariableName(asName)) {
                LOG_A2UI(LOG_WARN,
                    "EventHandlerParser: handler[%{public}d] 'as'='%{public}s' is invalid, binding dropped", i,
                    asName.c_str());
            } else {
                step.as = asName;
            }
        }

        if (IsValidHandler(step)) {
            handlers.push_back(std::move(step));
        }
    }
    return handlers;
}

bool EventHandlerParser::IsValidHandler(const EventHandlerStep& step)
{
    return !step.call.empty();
}

} // namespace NativeModule
