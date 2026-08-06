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

#include "components/actions/EventHandlerChainExecutor.h"

#include <cctype>
#include <cstdlib>
#include <optional>

#include "components/actions/ActionDispatcher.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "utils/LogA2UI.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "expression/EvalResult.h"
#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/ExpressionEngine.h"
#endif

namespace NativeModule {

namespace {

std::map<std::string, JsonValue> BuildEventLocalVariables(const EventHandlerChainExecutor::ExecutionContext& context)
{
    std::map<std::string, JsonValue> localVariables;
    if (context.eventContext.IsValid()) {
        localVariables["context"] = context.eventContext;
    }
    for (const auto& [name, value] : context.localVariables) {
        if (!name.empty() && value.IsValid()) {
            localVariables[name] = value;
        }
    }
    return localVariables;
}

DynamicResolveContext BuildResolveContext(const EventHandlerChainExecutor::ExecutionContext& context)
{
    return { .renderId = context.renderId,
        .surfaceId = context.surfaceId,
        .componentId = context.componentId,
        .allowExpression = true,
        .localVariables = BuildEventLocalVariables(context) };
}

bool HasOnlyPathDescriptorKey(const JsonValue& value)
{
    if (!value.IsObject() || !value.Has("path")) {
        return false;
    }
    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        if (child.GetKey() != "path") {
            return false;
        }
    }
    return true;
}

bool ResolveJsonValueRecursively(
    const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue);

bool ResolveDynamicJsonValue(const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    ResolvedValue resolved = DynamicValueResolver::Resolve(value, context);
    if (!resolved.success || !resolved.value.IsValid()) {
        return false;
    }
    resolvedValue = resolved.value;
    return true;
}

bool ResolveJsonObjectRecursively(
    const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    std::unique_ptr<JsonAdapter> objectAdapter = JsonAdapter::CreateObject();
    if (objectAdapter == nullptr) {
        return false;
    }
    JsonValue objectValue = objectAdapter->GetRoot();
    for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        JsonValue childValue;
        if (!ResolveJsonValueRecursively(child, context, childValue)) {
            return false;
        }
        if (!objectValue.Put(key.c_str(), childValue)) {
            return false;
        }
    }
    resolvedValue = objectValue;
    return true;
}

bool ResolveJsonArrayRecursively(const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    std::unique_ptr<JsonAdapter> arrayAdapter = JsonAdapter::CreateArray();
    if (arrayAdapter == nullptr) {
        return false;
    }
    JsonValue arrayValue = arrayAdapter->GetRoot();
    int itemCount = value.GetArraySize();
    for (int index = 0; index < itemCount; ++index) {
        JsonValue itemValue;
        if (!ResolveJsonValueRecursively(value.GetArrayItem(index), context, itemValue)) {
            return false;
        }
        if (!arrayValue.Append(itemValue)) {
            return false;
        }
    }
    resolvedValue = arrayValue;
    return true;
}

bool ResolveJsonValueRecursively(const JsonValue& value, const DynamicResolveContext& context, JsonValue& resolvedValue)
{
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsObject() && !value.Has("call") && !HasOnlyPathDescriptorKey(value)) {
        return ResolveJsonObjectRecursively(value, context, resolvedValue);
    }
    if (value.IsArray()) {
        return ResolveJsonArrayRecursively(value, context, resolvedValue);
    }
    return ResolveDynamicJsonValue(value, context, resolvedValue);
}

#ifndef ENABLE_EXPRESSION_ENGINE
std::string TrimCopy(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::optional<std::string> ExtractConditionExpression(const std::string& condition)
{
    std::string trimmed = TrimCopy(condition);
    constexpr size_t wrapperSize = 4;
    if (trimmed.size() < wrapperSize || trimmed.rfind("{{", 0) != 0 ||
        trimmed.compare(trimmed.size() - 2, 2, "}}") != 0) {
        return std::nullopt;
    }
    return TrimCopy(trimmed.substr(2, trimmed.size() - wrapperSize));
}

std::optional<size_t> FindEqualityOperator(const std::string& expression, std::string& op)
{
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    for (size_t i = 0; i + 1 < expression.size(); ++i) {
        char ch = expression[i];
        if (ch == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (ch == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (inSingleQuote || inDoubleQuote) {
            continue;
        }
        std::string candidate = expression.substr(i, 2);
        if (candidate == "==" || candidate == "!=") {
            op = candidate;
            return i;
        }
    }
    return std::nullopt;
}

EvalResult ResolveFallbackVariable(
    const std::string& operand, const EventHandlerChainExecutor::ExecutionContext& context)
{
    if (operand.size() < 2 || operand[0] != '$') {
        return EvalResult::Undefined();
    }
    std::string path = operand.substr(1);
    size_t dot = path.find('.');
    std::string variableName = dot == std::string::npos ? path : path.substr(0, dot);
    std::map<std::string, JsonValue> localVariables = BuildEventLocalVariables(context);
    auto variable = localVariables.find(variableName);
    if (variable == localVariables.end() || !variable->second.IsValid()) {
        return EvalResult::Undefined();
    }
    JsonValue value = variable->second;
    while (dot != std::string::npos) {
        size_t nextDot = path.find('.', dot + 1);
        std::string member = path.substr(dot + 1, nextDot == std::string::npos ? std::string::npos : nextDot - dot - 1);
        if (member.empty() || !value.IsObject()) {
            return EvalResult::Undefined();
        }
        value = value.GetItem(member);
        if (!value.IsValid()) {
            return EvalResult::Undefined();
        }
        dot = nextDot;
    }
    return EvalResult::FromJson(value);
}

EvalResult ResolveFallbackOperand(
    const std::string& rawOperand, const EventHandlerChainExecutor::ExecutionContext& context)
{
    std::string operand = TrimCopy(rawOperand);
    if (operand.empty()) {
        return EvalResult::Undefined();
    }
    if ((operand.front() == '\'' && operand.back() == '\'') || (operand.front() == '"' && operand.back() == '"')) {
        return operand.size() >= 2 ? EvalResult::FromString(operand.substr(1, operand.size() - 2))
                                   : EvalResult::FromString("");
    }
    if (operand == "true") {
        return EvalResult::FromBool(true);
    }
    if (operand == "false") {
        return EvalResult::FromBool(false);
    }
    if (operand == "null") {
        return EvalResult::Null();
    }
    if (operand[0] == '$') {
        return ResolveFallbackVariable(operand, context);
    }
    char* parseEnd = nullptr;
    double number = std::strtod(operand.c_str(), &parseEnd);
    if (parseEnd != operand.c_str() && parseEnd != nullptr && *parseEnd == '\0') {
        return EvalResult::FromNumber(number);
    }
    return EvalResult::Undefined();
}

bool EvaluateFallbackCondition(
    const std::string& expression, const EventHandlerChainExecutor::ExecutionContext& context)
{
    std::string op;
    std::optional<size_t> opPosition = FindEqualityOperator(expression, op);
    if (!opPosition.has_value()) {
        EvalResult result = ResolveFallbackOperand(expression, context);
        return !result.IsUndefined() && result.AsBool();
    }
    EvalResult left = ResolveFallbackOperand(expression.substr(0, opPosition.value()), context);
    EvalResult right = ResolveFallbackOperand(expression.substr(opPosition.value() + op.size()), context);
    if (left.IsUndefined() || right.IsUndefined()) {
        return false;
    }
    bool isEqual = left.AsString() == right.AsString();
    return op == "==" ? isEqual : !isEqual;
}
#endif

bool ResolveCondition(const EventHandlerStep& step, const EventHandlerChainExecutor::ExecutionContext& context)
{
    if (step.condition.empty()) {
        return true;
    }
#ifdef ENABLE_EXPRESSION_ENGINE
    if (!ExpressionEngine::IsExpression(step.condition)) {
        return false;
    }
    EvaluationContext evalContext;
    evalContext.SetRenderId(context.renderId);
    evalContext.SetSurfaceId(context.surfaceId);
    evalContext.SetComponentId(context.componentId);
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(context.renderId);
    if (renderSlot != nullptr && renderSlot->GetSurfaceManager() != nullptr) {
        evalContext.SetThemeContext(&renderSlot->GetSurfaceManager()->GetThemeContext());
    }
    if (context.dataModel != nullptr) {
        evalContext.SetDataModel(context.dataModel.get());
    }
    evalContext.PushScope();
    for (const auto& [name, value] : BuildEventLocalVariables(context)) {
        if (!name.empty() && value.IsValid()) {
            evalContext.SetLocalVariable(name, EvalResult::FromJson(value));
        }
    }
    EvalResult result = ExpressionEngine::GetInstance().Evaluate(step.condition, evalContext);
    return !result.IsUndefined() && result.AsBool();
#else
    std::optional<std::string> expression = ExtractConditionExpression(step.condition);
    return expression.has_value() && EvaluateFallbackCondition(expression.value(), context);
#endif
}

JsonValue ResolveArgs(const EventHandlerStep& step, const EventHandlerChainExecutor::ExecutionContext& context)
{
    if (!step.args.IsValid()) {
        return JsonValue();
    }
    JsonValue resolvedArgs;
    if (!ResolveJsonValueRecursively(step.args, BuildResolveContext(context), resolvedArgs)) {
        LOG_A2UI(LOG_WARN, "EventHandlerChainExecutor: args resolve failed, call='%{public}s', componentId=%{public}s",
            step.call.c_str(), context.componentId.c_str());
        return JsonValue();
    }
    return resolvedArgs;
}

bool IsEventContextObject(const JsonValue& value)
{
    return value.IsObject() && (value.Has("componentId") || value.Has("eventData"));
}

JsonValue BuildEventContextValue(const std::string& componentId, const JsonValue& eventData)
{
    std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::CreateObject();
    if (contextAdapter == nullptr) {
        return JsonValue();
    }

    JsonValue contextRoot = contextAdapter->GetRoot();
    if (!contextRoot.PutString("componentId", componentId)) {
        return JsonValue();
    }
    if (eventData.IsValid() && !contextRoot.Put("eventData", eventData)) {
        return JsonValue();
    }
    return contextRoot;
}

JsonValue BuildDispatchEventContext(const std::string& componentId, const JsonValue& extraContext)
{
    if (IsEventContextObject(extraContext)) {
        std::unique_ptr<JsonAdapter> contextAdapter = JsonAdapter::Clone(extraContext);
        if (contextAdapter == nullptr) {
            return BuildEventContextValue(componentId, JsonValue());
        }
        JsonValue contextRoot = contextAdapter->GetRoot();
        if (!contextRoot.Has("componentId") && !contextRoot.PutString("componentId", componentId)) {
            return JsonValue();
        }
        return contextRoot;
    }
    return BuildEventContextValue(componentId, extraContext);
}

JsonValue DispatchHandlerCall(const std::string& call, const JsonValue& resolvedArgs,
    EventHandlerChainExecutor::ExecutionContext& context, ActionDispatcherList& dispatchers, size_t handlerIndex)
{
    for (auto& dispatcher : dispatchers) {
        if (dispatcher->CanDispatch(call)) {
            return dispatcher->Dispatch(call, resolvedArgs, context);
        }
    }
    BridgeFunctionDispatcher bridgeDispatcher;
    JsonValue result = bridgeDispatcher.Dispatch(call, resolvedArgs, context);
    LOG_A2UI(LOG_WARN,
        "EventHandlerChainExecutor: unknown call '%{public}s' at handler[%{public}zu], componentId=%{public}s",
        call.c_str(), handlerIndex, context.componentId.c_str());
    return result;
}

} // namespace

void EventHandlerChainExecutor::ExecuteChain(const std::vector<EventHandlerStep>& handlers, ExecutionContext& context)
{
    ActionDispatcherList dispatchers = CreateDefaultDispatchers();

    for (size_t i = 0; i < handlers.size(); ++i) {
        const auto& step = handlers[i];

        if (!ResolveCondition(step, context)) {
            LOG_A2UI(LOG_DEBUG, "EventHandlerChainExecutor: skip handler[%{public}zu] by condition", i);
            continue;
        }

        if (step.call == "break") {
            LOG_A2UI(LOG_DEBUG, "EventHandlerChainExecutor: break at handler[%{public}zu]", i);
            break;
        }

        JsonValue result;
        JsonValue resolvedArgs = ResolveArgs(step, context);
        try {
            result = DispatchHandlerCall(step.call, resolvedArgs, context, dispatchers, i);
        } catch (const std::exception& e) {
            LOG_A2UI(LOG_ERROR,
                "EventHandlerChainExecutor: exception at handler[%{public}zu], call='%{public}s', "
                "componentId=%{public}s, what=%{public}s",
                i, step.call.c_str(), context.componentId.c_str(), e.what());
            break;
        } catch (...) {
            LOG_A2UI(LOG_ERROR,
                "EventHandlerChainExecutor: unknown exception at handler[%{public}zu], call='%{public}s', "
                "componentId=%{public}s",
                i, step.call.c_str(), context.componentId.c_str());
            break;
        }

        if (!step.as.empty() && result.IsValid()) {
            context.localVariables[step.as] = result;
        }
    }
}

bool DispatchEventToHandlers(const DispatchParams& params)
{
    auto handlerIt = params.handlers.find(params.eventName);
    if (handlerIt == params.handlers.end() || handlerIt->second.handlers.empty()) {
        LOG_A2UI(LOG_DEBUG, "DispatchEventToHandlers: handler not found, componentId=%{public}s, event=%{public}s",
            params.componentId.c_str(), params.eventName.c_str());
        return false;
    }

    std::shared_ptr<DataModel> dataModel;
    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(params.renderId, params.surfaceId);
    if (surface != nullptr) {
        dataModel = surface->GetOrCreateDataModel();
    } else {
        LOG_A2UI(LOG_WARN, "DispatchEventToHandlers: surface not found, surfaceId=%{public}s, componentId=%{public}s",
            params.surfaceId.c_str(), params.componentId.c_str());
    }

    EventHandlerChainExecutor::ExecutionContext context;
    context.renderId = params.renderId;
    context.surfaceId = params.surfaceId;
    context.componentId = params.componentId;
    context.dataModel = dataModel;
    context.eventContext = BuildDispatchEventContext(params.componentId, params.extraContext);
    context.externalEventContext = params.extraContext;
    context.hasExternalEventContext = true;
    if (params.localVariables != nullptr) {
        context.localVariables = *params.localVariables;
    }

    EventHandlerChainExecutor::ExecuteChain(handlerIt->second.handlers, context);
    return true;
}

} // namespace NativeModule
