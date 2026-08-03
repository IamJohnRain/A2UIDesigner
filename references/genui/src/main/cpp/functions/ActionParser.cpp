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

#include "ActionParser.h"

#include <optional>

#include "utils/LogA2UI.h"

#include "../SurfaceErrorCodes.h"
#include "RuntimeErrorDispatchBridge.h"

namespace NativeModule {

namespace {

constexpr int32_t MAX_EVENT_CONTEXT_DEPTH = 20;

void DispatchActionParseError(const ActionParseContext* parseContext, const std::string& message)
{
    if (parseContext == nullptr) {
        return;
    }
    RuntimeErrorDispatchBridge::GetInstance().Dispatch(parseContext->renderId, parseContext->surfaceId,
        parseContext->componentId, SURFACE_ERROR_ACTION_PARSE_FAILED, message, "ActionParser");
}

bool ValidateContextNode(const JsonValue& node, int32_t depth, bool* maxDepthExceeded)
{
    if (!node.IsValid()) {
        return true;
    }
    if (depth > MAX_EVENT_CONTEXT_DEPTH) {
        if (maxDepthExceeded != nullptr) {
            *maxDepthExceeded = true;
        }
        return false;
    }

    if (!node.IsObject() && !node.IsArray()) {
        return true;
    }

    for (JsonValue child = node.GetChild(); child.IsValid(); child = child.GetNext()) {
        if (!ValidateContextNode(child, depth + 1, maxDepthExceeded)) {
            return false;
        }
    }
    return true;
}

std::optional<JsonValue> CloneJsonValue(const JsonValue& value)
{
    if (!value.IsValid()) {
        return std::nullopt;
    }
    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::Clone(value);
    if (valueAdapter == nullptr) {
        return std::nullopt;
    }
    return valueAdapter->GetRoot();
}

bool ValidateEventContext(const JsonValue& contextValue, bool* maxDepthExceeded)
{
    if (!contextValue.IsObject()) {
        return false;
    }

    std::optional<JsonValue> contextCopy = CloneJsonValue(contextValue);
    if (!contextCopy.has_value()) {
        return false;
    }
    return contextCopy->IsObject() && ValidateContextNode(contextCopy.value(), 1, maxDepthExceeded);
}

std::shared_ptr<FunctionCallInfo> ParseFunctionCall(const JsonValue& functionCallValue)
{
    if (!functionCallValue.IsObject()) {
        LOG_A2UI(LOG_WARN, "ParseFunctionCall: functionCall is not an object");
        return nullptr;
    }

    std::string functionName = functionCallValue.GetString("call", "");
    if (functionName.empty()) {
        LOG_A2UI(LOG_WARN, "ParseFunctionCall: call is empty");
        return nullptr;
    }

    JsonValue args;
    JsonValue argsValue = functionCallValue.GetItem("args");
    if (argsValue.IsValid()) {
        std::unique_ptr<JsonAdapter> argsCopy = JsonAdapter::Clone(argsValue);
        if (argsCopy == nullptr) {
            LOG_A2UI(LOG_WARN, "ParseFunctionCall: args conversion failed");
            return nullptr;
        }
        args = argsCopy->GetRoot();
    }

    std::string returnType = functionCallValue.GetString("returnType", "void");
    return std::make_shared<FunctionCallInfo>(functionName, args, returnType);
}

std::shared_ptr<ActionInfo> ParseEventAction(const JsonValue& eventValue, const ActionParseContext* parseContext)
{
    if (!eventValue.IsObject()) {
        LOG_A2UI(LOG_WARN, "ParseEventAction: event is not an object");
        return nullptr;
    }

    std::string eventName = eventValue.GetString("name", "");
    if (eventName.empty()) {
        LOG_A2UI(LOG_WARN, "ParseEventAction: event.name is empty");
        return nullptr;
    }

    JsonValue contextValue = eventValue.GetItem("context");
    if (contextValue.IsValid()) {
        if (contextValue.IsObject()) {
            bool maxDepthExceeded = false;
            if (!ValidateEventContext(contextValue, &maxDepthExceeded)) {
                if (maxDepthExceeded) {
                    DispatchActionParseError(
                        parseContext, "action.event.context depth exceeds maximum supported limit");
                }
                LOG_A2UI(LOG_WARN, "ParseEventAction: event.context is invalid, drop action");
                return nullptr;
            }
        } else {
            LOG_A2UI(LOG_WARN, "ParseEventAction: event.context is not an object, fallback to {}");
        }
    }

    return std::make_shared<ActionInfo>(eventName, contextValue);
}

} // namespace

std::shared_ptr<ActionInfo> ActionParser::Parse(const JsonValue& descriptor)
{
    return Parse(descriptor, ActionParseContext());
}

std::shared_ptr<ActionInfo> ActionParser::Parse(const JsonValue& descriptor, const ActionParseContext& context)
{
    const ActionParseContext* parseContext = nullptr;
    if (context.renderId > 0) {
        parseContext = &context;
    }

    if (!descriptor.IsObject()) {
        return nullptr;
    }

    JsonValue actionValue = descriptor;
    if (descriptor.Has("action")) {
        actionValue = descriptor.GetItem("action");
    }
    if (!actionValue.IsObject()) {
        LOG_A2UI(LOG_WARN, "ActionParser::Parse: action is not an object");
        return nullptr;
    }

    if (actionValue.Has("functionCall") && actionValue.Has("event")) {
        LOG_A2UI(LOG_WARN, "ActionParser::Parse: functionCall and event both exist, drop action");
        return nullptr;
    }

    if (actionValue.Has("functionCall")) {
        JsonValue functionCallValue = actionValue.GetItem("functionCall");
        std::shared_ptr<FunctionCallInfo> functionCall = ParseFunctionCall(functionCallValue);
        if (functionCall != nullptr) {
            return std::make_shared<ActionInfo>(functionCall, functionCallValue);
        }
    } else if (actionValue.Has("event")) {
        JsonValue eventValue = actionValue.GetItem("event");
        return ParseEventAction(eventValue, parseContext);
    }
    return nullptr;
}

} // namespace NativeModule
