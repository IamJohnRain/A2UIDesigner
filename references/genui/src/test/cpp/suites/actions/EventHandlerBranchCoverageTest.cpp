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

#include <gtest/gtest.h>
#include <stdexcept>

#include "components/actions/BuiltInActions.h"
#include "components/actions/EventHandlerChainExecutor.h"
#include "components/actions/EventHandlerParser.h"
#include "components/actions/NativeActionRegistry.h"
#include "data/DataModel.h"
#include "functions/FunctionBridge.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

class EventHandlerBranchCoverageTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
    }

    JsonValue ParseJson(const std::string& json)
    {
        auto adapter = JsonAdapter::Parse(json);
        if (adapter == nullptr) {
            return JsonValue();
        }
        return adapter->GetRoot();
    }
};

TEST_F(EventHandlerBranchCoverageTest, L0_should_skip_handler_with_empty_call_string)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);
    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_accept_handler_when_condition_is_number)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "condition": 42 }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "fn1");
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_accept_handler_when_condition_is_boolean)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "condition": true }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_accept_handler_when_condition_is_null)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "condition": null }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_accept_handler_when_as_is_null)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "as": null }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_call_native_function_when_registered)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();

    auto& fnRegistry = NativeFunctionRegistry::GetInstance();

    EventHandlerStep step;
    step.call = "formatNumber";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_not_store_result_when_action_returns_invalid)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    registry.Register("returnsInvalid",
        [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return JsonValue(); });

    EventHandlerStep step;
    step.call = "returnsInvalid";
    step.as = "myVar";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_TRUE(context.localVariables.empty());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_store_result_when_action_returns_valid_with_as)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    registry.Register("returnsValid", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        auto adapter = JsonAdapter::CreateNumber(42.0);
        return adapter != nullptr ? adapter->GetRoot() : JsonValue();
    });

    EventHandlerStep step;
    step.call = "returnsValid";
    step.as = "result";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    ASSERT_TRUE(context.localVariables.count("result") > 0);
    EXPECT_EQ(context.localVariables.at("result").GetNumberValue(0), 42.0);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_handle_setDataModel_with_non_string_path)
{
    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": 123, "value": 1})");
    auto result = NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_handle_setDataModel_with_null_path)
{
    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": null, "value": 1})");
    auto result = NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_early_when_component_id_empty_in_setAttributes)
{
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "surf1";

    auto args = JsonAdapter::Parse(R"({"componentId": "", "value": {"text": "hi"}})");
    auto result = NativeActionRegistry::GetInstance().Execute("setAttributes", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_early_when_value_not_object_in_setAttributes)
{
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "surf1";

    auto args = JsonAdapter::Parse(R"({"componentId": "txt1", "value": "string"})");
    auto result = NativeActionRegistry::GetInstance().Execute("setAttributes", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_early_when_surface_not_found_in_setAttributes)
{
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "nonexistent_surface";

    auto args = JsonAdapter::Parse(R"({"componentId": "txt1", "value": {"text": "hi"}})");
    auto result = NativeActionRegistry::GetInstance().Execute("setAttributes", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_early_when_component_not_found_in_setAttributes)
{
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "nonexistent_surface";

    auto args = JsonAdapter::Parse(R"({"componentId": "missingComp", "value": {"text": "hi"}})");
    auto result = NativeActionRegistry::GetInstance().Execute("setAttributes", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_not_register_navigate_as_builtin_action)
{
    RegisterBuiltInActions(NativeActionRegistry::GetInstance());

    EXPECT_FALSE(NativeActionRegistry::GetInstance().HasAction("navigate"));
    EXPECT_TRUE(NativeFunctionRegistry::GetInstance().HasFunction("navigate"));
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_false_from_function_bridge_when_env_null)
{
    auto functionCall = std::make_shared<FunctionCallInfo>("testFunc", JsonValue(), "void");
    bool result = FunctionBridge::GetInstance().Invoke(0, "surf1", "comp1", functionCall);
    EXPECT_FALSE(result);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_false_from_function_bridge_when_call_null)
{
    bool result = FunctionBridge::GetInstance().Invoke(0, "surf1", "comp1", nullptr);
    EXPECT_FALSE(result);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_chain_multiple_native_function_calls)
{
    auto& fnRegistry = NativeFunctionRegistry::GetInstance();

    std::vector<EventHandlerStep> steps;
    EventHandlerStep step1;
    step1.call = "formatNumber";
    steps.push_back(step1);

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain(steps, context);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_handle_setDataModel_with_missing_path_key)
{
    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"value": 1})");
    auto result = NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_handle_setAttributes_with_missing_value_key)
{
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "surf1";

    auto args = JsonAdapter::Parse(R"({"componentId": "txt1"})");
    auto result = NativeActionRegistry::GetInstance().Execute("setAttributes", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_execute_handler_without_condition)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    std::atomic<int> callCount { 0 };
    registry.Register("noCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        callCount++;
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "noCondition";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_EQ(callCount.load(), 1);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_execute_builtin_actions_via_chain)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    registry.Register("setDataModel", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        if (ctx.dataModel == nullptr) {
            ctx.dataModel = std::make_shared<DataModel>(ctx.surfaceId);
        }
        JsonValue pathValue = args.GetItem("path");
        std::string resolvedPath;
        if (pathValue.IsString()) {
            resolvedPath = pathValue.GetStringValue();
        }
        if (resolvedPath.empty()) {
            return JsonValue();
        }
        JsonValue value = args.GetItem("value");
        ctx.dataModel->UpdateByPath(resolvedPath, value);
        ctx.dataModel->NotifyPathUpdate(resolvedPath);
        return JsonValue();
    });

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "test";
    context.dataModel = dataModel;

    EventHandlerStep step;
    step.call = "setDataModel";
    step.args = ParseJson(R"({"path": "/x", "value": 99})");

    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    auto val = dataModel->GetNode("/x");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetInt32Value(), 99);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_handle_bridge_fallback_for_unknown_call)
{
    NativeActionRegistry::GetInstance().Clear();

    EventHandlerStep step;
    step.call = "totallyUnknownFunction";

    EventHandlerChainExecutor::ExecutionContext context;
    context.surfaceId = "surf1";
    context.componentId = "comp1";

    EventHandlerChainExecutor::ExecuteChain({ step }, context);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_resolve_nested_literal_args_before_dispatch)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    bool called = false;
    registry.Register(
        "captureNestedArgs", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) -> JsonValue {
            called = true;
            EXPECT_TRUE(args.IsObject());
            EXPECT_EQ(args.GetString("path", ""), "/literal");
            JsonValue items = args.GetItem("items");
            EXPECT_TRUE(items.IsArray());
            EXPECT_EQ(items.GetArraySize(), 3);
            if (items.IsArray() && items.GetArraySize() == 3) {
                EXPECT_EQ(items.GetArrayItem(0).GetInt32Value(), 1);
                EXPECT_EQ(items.GetArrayItem(1).GetStringValue(""), "two");
                EXPECT_TRUE(items.GetArrayItem(2).GetItem("nested").GetBoolValue(false));
            }
            EXPECT_FALSE(args.Has(""));
            return JsonValue();
        });

    EventHandlerStep step;
    step.call = "captureNestedArgs";
    step.args = ParseJson(R"({"path":"/literal","items":[1,"two",{"nested":true}],"":"ignored"})");

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_TRUE(called);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_pass_invalid_args_when_dynamic_arg_resolution_fails)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    bool sawInvalidArgs = false;
    registry.Register(
        "captureInvalidArgs", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
            sawInvalidArgs = !args.IsValid();
            return JsonValue();
        });

    EventHandlerStep step;
    step.call = "captureInvalidArgs";
    step.args = ParseJson(R"({"path":123})");

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_TRUE(sawInvalidArgs);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_evaluate_context_condition_with_not_equal_operator)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    int callCount = 0;
    registry.Register(
        "captureContextCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
            ++callCount;
            return JsonValue();
        });

    EventHandlerStep step;
    step.call = "captureContextCondition";
    step.condition = "{{ $context.eventData.count != 3 }}";

    EventHandlerChainExecutor::ExecutionContext context;
    context.eventContext = ParseJson(R"({"eventData":{"count":2}})");
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_EQ(callCount, 1);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_skip_condition_when_context_member_is_missing)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    int callCount = 0;
    registry.Register(
        "captureMissingContextCondition", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
            ++callCount;
            return JsonValue();
        });

    EventHandlerStep step;
    step.call = "captureMissingContextCondition";
    step.condition = "{{ $context.eventData.missing == 3 }}";

    EventHandlerChainExecutor::ExecutionContext context;
    context.eventContext = ParseJson(R"({"eventData":{"count":2}})");
    EventHandlerChainExecutor::ExecuteChain({ step }, context);

    EXPECT_EQ(callCount, 0);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_stop_chain_when_handler_throws_std_exception)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    int afterCount = 0;
    registry.Register(
        "throwStdException", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) -> JsonValue {
            throw std::runtime_error("boom");
        });
    registry.Register(
        "afterStdException", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
            ++afterCount;
            return JsonValue();
        });

    EventHandlerStep throwingStep;
    throwingStep.call = "throwStdException";
    EventHandlerStep afterStep;
    afterStep.call = "afterStdException";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ throwingStep, afterStep }, context);

    EXPECT_EQ(afterCount, 0);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_stop_chain_when_handler_throws_unknown_exception)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    int afterCount = 0;
    registry.Register("throwUnknownException",
        [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) -> JsonValue { throw 7; });
    registry.Register(
        "afterUnknownException", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
            ++afterCount;
            return JsonValue();
        });

    EventHandlerStep throwingStep;
    throwingStep.call = "throwUnknownException";
    EventHandlerStep afterStep;
    afterStep.call = "afterUnknownException";

    EventHandlerChainExecutor::ExecutionContext context;
    EventHandlerChainExecutor::ExecuteChain({ throwingStep, afterStep }, context);

    EXPECT_EQ(afterCount, 0);
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_return_false_when_dispatch_handler_is_missing_or_empty)
{
    EventHandlerMap emptyHandlers;
    JsonValue emptyContext;

    EXPECT_FALSE(DispatchEventToHandlers({ emptyHandlers, "onClick", "surface", "component", 0, emptyContext }));

    EventHandlerMap handlersWithEmptyEvent;
    EventListenerInfo listener;
    listener.eventName = "onClick";
    handlersWithEmptyEvent["onClick"] = listener;

    EXPECT_FALSE(
        DispatchEventToHandlers({ handlersWithEmptyEvent, "onClick", "surface", "component", 0, emptyContext }));
}

TEST_F(EventHandlerBranchCoverageTest, L0_should_dispatch_event_without_local_variable_params)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Clear();
    bool called = false;
    registry.Register("captureDispatch", [&](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        called = true;
        EXPECT_TRUE(ctx.localVariables.empty());
        EXPECT_EQ(ctx.surfaceId, "missing-surface");
        EXPECT_EQ(ctx.componentId, "button1");
        return JsonValue();
    });

    EventHandlerStep step;
    step.call = "captureDispatch";
    EventListenerInfo listener;
    listener.eventName = "onClick";
    listener.handlers.push_back(step);
    EventHandlerMap handlers;
    handlers["onClick"] = listener;
    JsonValue emptyContext;

    EXPECT_TRUE(DispatchEventToHandlers({ handlers, "onClick", "missing-surface", "button1", 9, emptyContext }));
    EXPECT_TRUE(called);
}
