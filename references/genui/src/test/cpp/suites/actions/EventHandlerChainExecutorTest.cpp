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

#include <atomic>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "components/actions/BuiltInActions.h"
#include "components/actions/NativeActionRegistry.h"
#include "data/DataModel.h"
#include "functions/NativeFunctionBase.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

struct CapturedNativeFunctionContext {
    int32_t renderId = -1;
    std::string surfaceId;
    std::string componentId;
};

class CaptureContextNativeFunction final : public NativeFunctionBase {
public:
    explicit CaptureContextNativeFunction(std::shared_ptr<CapturedNativeFunctionContext> captured)
        : captured_(std::move(captured))
    {}

    std::string GetName() const override
    {
        return "captureNativeContext";
    }

    FunctionResult Execute(const JsonValue& resolvedArgs) override
    {
        static_cast<void>(resolvedArgs);
        return FunctionResult(std::string("missing context"));
    }

    FunctionResult ExecuteWithContext(const JsonValue& resolvedArgs, const DynamicResolveContext& context) override
    {
        static_cast<void>(resolvedArgs);
        if (captured_ != nullptr) {
            captured_->renderId = context.renderId;
            captured_->surfaceId = context.surfaceId;
            captured_->componentId = context.componentId;
        }
        return FunctionResult(std::string("ok"));
    }

private:
    std::shared_ptr<CapturedNativeFunctionContext> captured_;
};

} // namespace

class EventHandlerChainExecutorTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        for (int32_t renderId : createdRenderIds_) {
            RenderManager::GetInstance().RemoveRenderSlot(renderId);
        }
        createdRenderIds_.clear();
        NativeActionRegistry::GetInstance().Clear();
    }

    std::vector<EventHandlerStep> MakeHandlers(const std::vector<std::string>& calls)
    {
        std::vector<EventHandlerStep> handlers;
        for (const auto& call : calls) {
            EventHandlerStep step;
            step.call = call;
            handlers.push_back(std::move(step));
        }
        return handlers;
    }

    EventHandlerChainExecutor::ExecutionContext emptyContext;

    SurfaceSlot& CreateSurfaceForTest(int32_t renderId, const std::string& surfaceId)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
        createdRenderIds_.push_back(renderId);
        std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
        return surfaceManager->CreateSurface(surfaceId);
    }

private:
    std::vector<int32_t> createdRenderIds_;
};

// ===== AC-002: 链式执行 =====

TEST_F(EventHandlerChainExecutorTest, L0_should_execute_handlers_in_order)
{
    std::vector<std::string> executionOrder;

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("step1", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("step1");
        return JsonValue();
    });
    registry.Register("step2", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("step2");
        return JsonValue();
    });
    registry.Register("step3", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("step3");
        return JsonValue();
    });

    auto handlers = MakeHandlers({ "step1", "step2", "step3" });
    EventHandlerChainExecutor::ExecuteChain(handlers, emptyContext);

    ASSERT_EQ(executionOrder.size(), 3u);
    EXPECT_EQ(executionOrder[0], "step1");
    EXPECT_EQ(executionOrder[1], "step2");
    EXPECT_EQ(executionOrder[2], "step3");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_execute_empty_handler_list)
{
    std::vector<EventHandlerStep> empty;
    EventHandlerChainExecutor::ExecuteChain(empty, emptyContext);
}

// ===== AC-003: break 中断 =====

TEST_F(EventHandlerChainExecutorTest, L0_should_break_at_first_handler)
{
    std::vector<std::string> executionOrder;

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("step1", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("step1");
        return JsonValue();
    });

    auto handlers = MakeHandlers({ "break", "step1" });
    EventHandlerChainExecutor::ExecuteChain(handlers, emptyContext);

    EXPECT_TRUE(executionOrder.empty());
}

TEST_F(EventHandlerChainExecutorTest, L0_should_break_in_middle)
{
    std::vector<std::string> executionOrder;

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("before", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("before");
        return JsonValue();
    });
    registry.Register("after", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("after");
        return JsonValue();
    });

    auto handlers = MakeHandlers({ "before", "break", "after" });
    EventHandlerChainExecutor::ExecuteChain(handlers, emptyContext);

    ASSERT_EQ(executionOrder.size(), 1u);
    EXPECT_EQ(executionOrder[0], "before");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_not_break_when_break_is_last)
{
    std::vector<std::string> executionOrder;

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("step1", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        executionOrder.push_back("step1");
        return JsonValue();
    });

    auto handlers = MakeHandlers({ "step1", "break" });
    EventHandlerChainExecutor::ExecuteChain(handlers, emptyContext);

    ASSERT_EQ(executionOrder.size(), 1u);
    EXPECT_EQ(executionOrder[0], "step1");
}

// ===== AC-009: as 变量存储 =====

TEST_F(EventHandlerChainExecutorTest, L0_should_store_result_in_as_variable)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("getValue", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        auto adapter = JsonAdapter::CreateString("hello");
        return adapter->GetRoot();
    });

    EventHandlerStep step;
    step.call = "getValue";
    step.as = "myVar";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    ASSERT_TRUE(context.localVariables.count("myVar") > 0);
    EXPECT_EQ(context.localVariables.at("myVar").GetStringValue(), "hello");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_not_store_when_as_is_empty)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("getValue", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        auto adapter = JsonAdapter::CreateString("hello");
        return adapter->GetRoot();
    });

    EventHandlerStep step;
    step.call = "getValue";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_TRUE(context.localVariables.empty());
}

TEST_F(EventHandlerChainExecutorTest, L0_should_pass_execution_context_to_native_function_handler)
{
    auto captured = std::make_shared<CapturedNativeFunctionContext>();
    NativeFunctionRegistry::GetInstance().Register(
        "captureNativeContext", std::make_shared<CaptureContextNativeFunction>(captured));

    EventHandlerStep step;
    step.call = "captureNativeContext";
    step.as = "result";

    EventHandlerChainExecutor::ExecutionContext context;
    context.renderId = 42;
    context.surfaceId = "surfaceA";
    context.componentId = "componentB";

    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_EQ(captured->renderId, 42);
    EXPECT_EQ(captured->surfaceId, "surfaceA");
    EXPECT_EQ(captured->componentId, "componentB");
    ASSERT_TRUE(context.localVariables.count("result") > 0);
    EXPECT_EQ(context.localVariables.at("result").GetStringValue(""), "ok");
}

// ===== AC-010: condition 表达式求值 =====

TEST_F(EventHandlerChainExecutorTest, L0_should_execute_handler_when_condition_is_true)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("withCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "withCondition";
    step.condition = "{{ true }}";

    EventHandlerChainExecutor::ExecuteChain({ step }, emptyContext);

    EXPECT_EQ(callCount.load(), 1);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_handler_when_condition_is_false)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("withCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "withCondition";
    step.condition = "{{ false }}";

    EventHandlerChainExecutor::ExecuteChain({ step }, emptyContext);

    EXPECT_EQ(callCount.load(), 0);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_evaluate_comparison_condition)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("withCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep stepTrue;
    stepTrue.call = "withCondition";
    stepTrue.condition = "{{ 1 == 1 }}";

    EventHandlerChainExecutor::ExecuteChain({ stepTrue }, emptyContext);
    EXPECT_EQ(callCount.load(), 1);

    callCount = 0;
    EventHandlerStep stepFalse;
    stepFalse.call = "withCondition";
    stepFalse.condition = "{{ 1 == 2 }}";

    EventHandlerChainExecutor::ExecuteChain({ stepFalse }, emptyContext);
    EXPECT_EQ(callCount.load(), 0);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_handler_when_condition_is_invalid_expression)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("withCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "withCondition";
    step.condition = "{{ undefinedVar }}";

    EventHandlerChainExecutor::ExecuteChain({ step }, emptyContext);

    EXPECT_EQ(callCount.load(), 0);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_continue_chain_after_skipped_condition)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("step1", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });
    registry.Register("step2", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount += 10;
        return JsonValue();
    });
    registry.Register("step3", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount += 100;
        return JsonValue();
    });

    EventHandlerStep s1;
    s1.call = "step1";
    s1.condition = "{{ true }}";

    EventHandlerStep s2;
    s2.call = "step2";
    s2.condition = "{{ false }}";

    EventHandlerStep s3;
    s3.call = "step3";
    s3.condition = "{{ true }}";

    EventHandlerChainExecutor::ExecuteChain({ s1, s2, s3 }, emptyContext);

    EXPECT_EQ(callCount.load(), 101);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_break_when_condition_is_false)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("afterBreak", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep breakStep;
    breakStep.call = "break";
    breakStep.condition = "{{ false }}";

    EventHandlerStep afterStep;
    afterStep.call = "afterBreak";

    EventHandlerChainExecutor::ExecuteChain({ breakStep, afterStep }, emptyContext);

    EXPECT_EQ(callCount.load(), 1);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_handler_when_condition_has_no_wrapper)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("withCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "withCondition";
    step.condition = "plain text without wrapper";

    EventHandlerChainExecutor::ExecuteChain({ step }, emptyContext);

    EXPECT_EQ(callCount.load(), 0);
}

// ===== AC-002: 未知 call =====

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_unknown_call)
{
    std::atomic<int> callCount { 0 };

    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("known", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    auto handlers = MakeHandlers({ "unknown", "known" });
    EventHandlerChainExecutor::ExecuteChain(handlers, emptyContext);

    EXPECT_EQ(callCount.load(), 1);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_dispatch_data_model_update_to_render_scoped_surface)
{
    constexpr int32_t oldRenderId = 8101;
    constexpr int32_t newRenderId = 8102;
    const std::string surfaceId = "shared-surface";
    const std::string buttonId = "shared-button";

    SurfaceSlot& oldSurface = CreateSurfaceForTest(oldRenderId, surfaceId);
    SurfaceSlot& newSurface = CreateSurfaceForTest(newRenderId, surfaceId);
    std::shared_ptr<DataModel> oldDataModel = oldSurface.GetOrCreateDataModel();
    std::shared_ptr<DataModel> newDataModel = newSurface.GetOrCreateDataModel();
    auto zeroAdapter = JsonAdapter::CreateNumber(0.0);
    ASSERT_NE(zeroAdapter, nullptr);
    oldDataModel->UpdateByPath("/count", zeroAdapter->GetRoot());
    newDataModel->UpdateByPath("/count", zeroAdapter->GetRoot());

    RegisterBuiltInActions(NativeActionRegistry::GetInstance());
    auto argsAdapter = JsonAdapter::Parse(R"({"path":"/count","value":1})");
    ASSERT_NE(argsAdapter, nullptr);

    EventHandlerStep step;
    step.call = "setDataModel";
    step.args = argsAdapter->GetRoot();

    EventListenerInfo info;
    info.eventName = "onClick";
    info.handlers.push_back(step);
    EventHandlerMap handlers;
    handlers["onClick"] = info;

    auto contextAdapter = JsonAdapter::Parse(R"({"componentId":"shared-button","eventData":{}})");
    ASSERT_NE(contextAdapter, nullptr);
    DispatchParams params { handlers, "onClick", surfaceId, buttonId, newRenderId, contextAdapter->GetRoot() };

    EXPECT_TRUE(DispatchEventToHandlers(params));

    std::optional<JsonValue> oldCount = oldDataModel->GetNode("/count");
    std::optional<JsonValue> newCount = newDataModel->GetNode("/count");
    ASSERT_TRUE(oldCount.has_value());
    ASSERT_TRUE(newCount.has_value());
    EXPECT_EQ(oldCount->GetInt32Value(-1), 0);
    EXPECT_EQ(newCount->GetInt32Value(-1), 1);
}

#ifdef ENABLE_EXPRESSION_ENGINE

TEST_F(EventHandlerChainExecutorTest, L0_should_resolve_context_and_as_variables_in_later_handler_args)
{
    std::string capturedSummary;
    std::string capturedComponentId;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("getUser", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        return JsonAdapter::Parse(R"({"name":"Alice","tier":"gold"})")->GetRoot();
    });
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedSummary = args.GetString("summary", "");
        capturedComponentId = args.GetString("component", "");
        return JsonValue();
    });

    EventHandlerStep getUser;
    getUser.call = "getUser";
    getUser.as = "user";
    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({
        "summary": "{{ $user.name + ':' + $context.eventData.value }}",
        "component": "{{ $context.componentId }}"
    })")
                       ->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.eventContext =
        JsonAdapter::Parse(R"({"componentId":"button1","eventData":{"value":"clicked"}})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ getUser, capture }, context);

    EXPECT_EQ(capturedSummary, "Alice:clicked");
    EXPECT_EQ(capturedComponentId, "button1");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_wrap_dispatch_extra_context_as_event_context)
{
    std::string capturedComponentId;
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedComponentId = args.GetString("component", "");
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({
        "component": "{{ $context.componentId }}",
        "value": "{{ $context.eventData.value }}"
    })")
                       ->GetRoot();
    EventListenerInfo info;
    info.eventName = "onChange";
    info.handlers.push_back(capture);
    EventHandlerMap handlers;
    handlers["onChange"] = info;

    auto eventDataAdapter = JsonAdapter::Parse(R"({"value":"clicked"})");
    ASSERT_NE(eventDataAdapter, nullptr);

    DispatchParams params { handlers, "onChange", "surface", "input1", 0, eventDataAdapter->GetRoot() };
    EXPECT_TRUE(DispatchEventToHandlers(params));

    EXPECT_EQ(capturedComponentId, "input1");
    EXPECT_EQ(capturedValue, "clicked");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_fill_component_id_when_dispatch_context_has_event_data_only)
{
    std::string capturedComponentId;
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedComponentId = args.GetString("component", "");
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({
        "component": "{{ $context.componentId }}",
        "value": "{{ $context.eventData.value }}"
    })")
                       ->GetRoot();
    EventListenerInfo info;
    info.eventName = "onChange";
    info.handlers.push_back(capture);
    EventHandlerMap handlers;
    handlers["onChange"] = info;

    auto eventContextAdapter = JsonAdapter::Parse(R"({"eventData":{"value":"clicked"}})");
    ASSERT_NE(eventContextAdapter, nullptr);

    DispatchParams params { handlers, "onChange", "surface", "input1", 0, eventContextAdapter->GetRoot() };
    EXPECT_TRUE(DispatchEventToHandlers(params));

    EXPECT_EQ(capturedComponentId, "input1");
    EXPECT_EQ(capturedValue, "clicked");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_ignore_invalid_local_variable_entries_when_resolving_args)
{
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({"value":"{{ $item.name }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.localVariables[""] = JsonAdapter::Parse(R"({"name":"ignored"})")->GetRoot();
    context.localVariables["invalid"] = JsonValue();
    context.localVariables["item"] = JsonAdapter::Parse(R"({"name":"Alice"})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ capture }, context);

    EXPECT_EQ(capturedValue, "Alice");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_evaluate_condition_with_data_model_context)
{
    std::atomic<int> callCount { 0 };
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        ++callCount;
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.condition = "{{ true }}";

    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "surface";
    context.dataModel = std::make_shared<DataModel>("surface");
    EventHandlerChainExecutor::ExecuteChain({ capture }, context);

    EXPECT_EQ(callCount.load(), 1);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_not_share_as_variable_between_event_chains)
{
    std::atomic<int> callCount { 0 };
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        ++callCount;
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.condition = "{{ $result == 'ok' }}";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ capture }, context);

    EXPECT_EQ(callCount.load(), 0);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_skip_handler_when_condition_resolves_false)
{
    std::atomic<int> callCount { 0 };
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        ++callCount;
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.condition = "{{ $context.eventData.enabled }}";

    EventHandlerChainExecutor::ExecutionContext context;
    context.eventContext = JsonAdapter::Parse(R"({"componentId":"toggle1","eventData":{"enabled":false}})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ capture }, context);

    EXPECT_EQ(callCount.load(), 0);
}

TEST_F(EventHandlerChainExecutorTest, L0_should_not_bind_as_when_handler_returns_invalid_value)
{
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("voidStep",
        [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return JsonValue(); });
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedValue = args.GetString("value", "not-called");
        return JsonValue();
    });

    EventHandlerStep voidStep;
    voidStep.call = "voidStep";
    voidStep.as = "result";
    EventHandlerStep capture;
    capture.call = "capture";
    capture.condition = "{{ $result == 'ok' }}";
    capture.args = JsonAdapter::Parse(R"({"value":"{{ $result }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ voidStep, capture }, context);

    EXPECT_TRUE(context.localVariables.find("result") == context.localVariables.end());
    EXPECT_TRUE(capturedValue.empty());
}

TEST_F(EventHandlerChainExecutorTest, L0_should_prefer_as_binding_when_as_name_is_context)
{
    std::string beforeValue;
    std::string afterValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("captureBefore", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        beforeValue = args.GetString("value", "");
        return JsonValue();
    });
    registry.Register("makeContext", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        return JsonAdapter::Parse(R"({"componentId":"shadowed"})")->GetRoot();
    });
    registry.Register("captureAfter", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        afterValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep before;
    before.call = "captureBefore";
    before.args = JsonAdapter::Parse(R"({"value":"{{ $context.componentId }}"})")->GetRoot();
    EventHandlerStep makeContext;
    makeContext.call = "makeContext";
    makeContext.as = "context";
    EventHandlerStep after;
    after.call = "captureAfter";
    after.args = JsonAdapter::Parse(R"({"value":"{{ $context.componentId }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.eventContext = JsonAdapter::Parse(R"({"componentId":"button1","eventData":{}})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ before, makeContext, after }, context);

    EXPECT_EQ(beforeValue, "button1");
    EXPECT_EQ(afterValue, "shadowed");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_allow_template_variable_and_as_binding_together)
{
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("getResult", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        return JsonAdapter::Parse(R"({"label":"selected"})")->GetRoot();
    });
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep getResult;
    getResult.call = "getResult";
    getResult.as = "result";
    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({"value":"{{ $item.name + ':' + $result.label }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.localVariables["item"] = JsonAdapter::Parse(R"({"name":"Alice"})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ getResult, capture }, context);

    EXPECT_EQ(capturedValue, "Alice:selected");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_shadow_template_variable_after_as_binding_with_same_name)
{
    std::string beforeValue;
    std::string afterValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("captureBefore", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        beforeValue = args.GetString("value", "");
        return JsonValue();
    });
    registry.Register("getItem", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        return JsonAdapter::Parse(R"({"name":"ResultItem"})")->GetRoot();
    });
    registry.Register("captureAfter", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        afterValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep before;
    before.call = "captureBefore";
    before.args = JsonAdapter::Parse(R"({"value":"{{ $item.name }}"})")->GetRoot();
    EventHandlerStep getItem;
    getItem.call = "getItem";
    getItem.as = "item";
    EventHandlerStep after;
    after.call = "captureAfter";
    after.args = JsonAdapter::Parse(R"({"value":"{{ $item.name }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.localVariables["item"] = JsonAdapter::Parse(R"({"name":"TemplateItem"})")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ before, getItem, after }, context);

    EXPECT_EQ(beforeValue, "TemplateItem");
    EXPECT_EQ(afterValue, "ResultItem");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_not_shadow_system_global_variable_with_local_variable)
{
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({"value":"{{ $__widthBreakpoint }}"})")->GetRoot();

    EventHandlerChainExecutor::ExecutionContext context;
    context.localVariables["__widthBreakpoint"] = JsonAdapter::CreateString("hacked")->GetRoot();
    context.localVariables["widthBreakpoint"] = JsonAdapter::CreateString("local")->GetRoot();
    EventHandlerChainExecutor::ExecuteChain({ capture }, context);

    EXPECT_EQ(capturedValue, "sm");
}

TEST_F(EventHandlerChainExecutorTest, L0_should_pass_template_local_variables_through_dispatch_params)
{
    std::string capturedValue;
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("capture", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        capturedValue = args.GetString("value", "");
        return JsonValue();
    });

    EventHandlerStep capture;
    capture.call = "capture";
    capture.args = JsonAdapter::Parse(R"({"value":"{{ $item.name + ':' + $index }}"})")->GetRoot();
    EventListenerInfo info;
    info.eventName = "onClick";
    info.handlers.push_back(capture);
    EventHandlerMap handlers;
    handlers["onClick"] = info;

    auto itemAdapter = JsonAdapter::Parse(R"({"name":"Alice"})");
    auto indexAdapter = JsonAdapter::CreateNumber(3.0);
    ASSERT_NE(itemAdapter, nullptr);
    ASSERT_NE(indexAdapter, nullptr);
    std::map<std::string, JsonValue> localVariables;
    localVariables["item"] = itemAdapter->GetRoot();
    localVariables["index"] = indexAdapter->GetRoot();
    auto contextAdapter = JsonAdapter::Parse(R"({"componentId":"row1","eventData":{}})");
    ASSERT_NE(contextAdapter, nullptr);

    DispatchParams params { handlers, "onClick", "surface", "row1", 0, contextAdapter->GetRoot(), &localVariables };
    EXPECT_TRUE(DispatchEventToHandlers(params));

    EXPECT_EQ(capturedValue, "Alice:3");
}
#endif
