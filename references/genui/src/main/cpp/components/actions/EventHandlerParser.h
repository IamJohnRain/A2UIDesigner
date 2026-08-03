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

#ifndef A2UI_EVENT_HANDLER_PARSER_H
#define A2UI_EVENT_HANDLER_PARSER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "utils/JsonAdapter.h"

namespace NativeModule {

struct EventHandlerStep {
    std::string call;
    JsonValue args;
    std::string condition;
    std::string as;
};

struct EventListenerInfo {
    std::string eventName;
    std::vector<EventHandlerStep> handlers;
};

using EventHandlerMap = std::map<std::string, EventListenerInfo>;

class EventHandlerParser final {
public:
    static const std::set<std::string> KNOWN_EVENT_NAMES;

    static EventHandlerMap Parse(const JsonValue& descriptor);

private:
    static std::vector<EventHandlerStep> ParseHandlerArray(const JsonValue& array);
    static bool IsValidHandler(const EventHandlerStep& step);
};

} // namespace NativeModule

#endif // A2UI_EVENT_HANDLER_PARSER_H
