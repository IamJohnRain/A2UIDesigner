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

#include <gtest/gtest.h>

#include "utils/JsonAdapter.h"

using namespace NativeModule;

class EventHandlerParserTest : public ::testing::Test {
protected:
    JsonValue ParseJson(const std::string& json)
    {
        auto adapter = JsonAdapter::Parse(json);
        if (adapter == nullptr) {
            return JsonValue();
        }
        return adapter->GetRoot();
    }
};

// ===== AC-001: 事件注册解析 =====

TEST_F(EventHandlerParserTest, L0_should_parse_single_onClick_handler)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "setDataModel", "args": { "path": "/count", "value": 1 } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    ASSERT_TRUE(result.count("onClick") > 0);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "setDataModel");
}

TEST_F(EventHandlerParserTest, L0_should_parse_multiple_handlers_in_one_event)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "setDataModel", "args": { "path": "/a", "value": 1 } },
            { "call": "setAttributes", "args": { "componentId": "txt", "value": { "text": "hi" } } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 2u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "setDataModel");
    EXPECT_EQ(result.at("onClick").handlers[1].call, "setAttributes");
}

TEST_F(EventHandlerParserTest, L0_should_parse_handler_with_all_fields)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "myFunc", "args": { "x": 1 }, "condition": "someExpr", "as": "result" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    const auto& handler = result.at("onClick").handlers[0];
    EXPECT_EQ(handler.call, "myFunc");
    EXPECT_EQ(handler.condition, "someExpr");
    EXPECT_EQ(handler.as, "result");
}

TEST_F(EventHandlerParserTest, L0_should_return_empty_when_array_is_empty)
{
    auto descriptor = ParseJson(R"({ "onClick": [] })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_return_empty_when_no_event_properties)
{
    auto descriptor = ParseJson(R"({ "id": "btn", "text": "click" })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_return_empty_when_descriptor_is_null)
{
    auto descriptor = ParseJson("null");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_return_empty_when_descriptor_is_not_object)
{
    auto descriptor = ParseJson(R"("just a string")");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_with_missing_call)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "args": { "path": "/x" } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_with_non_string_call)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": 123 }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_ignore_unknown_event_name)
{
    auto descriptor = ParseJson(R"({
        "onHover": [
            { "call": "doSomething" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_parse_multiple_event_types)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1" }
        ],
        "onAppear": [
            { "call": "fn2" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 2u);
    ASSERT_TRUE(result.count("onClick") > 0);
    ASSERT_TRUE(result.count("onAppear") > 0);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
    EXPECT_EQ(result.at("onAppear").handlers.size(), 1u);
}

TEST_F(EventHandlerParserTest, L0_should_parse_all_known_event_names)
{
    auto descriptor = ParseJson(R"({
        "onClick": [{ "call": "fn1" }],
        "onAppear": [{ "call": "fn2" }],
        "onChange": [{ "call": "fn3" }],
        "onSelect": [{ "call": "fn4" }],
        "onReachStart": [{ "call": "fn5" }],
        "onReachEnd": [{ "call": "fn6" }]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 6u);
    EXPECT_TRUE(result.count("onClick") > 0);
    EXPECT_TRUE(result.count("onAppear") > 0);
    EXPECT_TRUE(result.count("onChange") > 0);
    EXPECT_TRUE(result.count("onSelect") > 0);
    EXPECT_TRUE(result.count("onReachStart") > 0);
    EXPECT_TRUE(result.count("onReachEnd") > 0);
}

TEST_F(EventHandlerParserTest, L0_should_skip_non_array_event_value)
{
    auto descriptor = ParseJson(R"({
        "onClick": { "call": "fn1" }
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_non_object_handler_in_array)
{
    auto descriptor = ParseJson(R"({
        "onClick": ["notAnObject", { "call": "fn1" }]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "fn1");
}

TEST_F(EventHandlerParserTest, L0_should_parse_handler_with_no_args)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "break" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "break");
    EXPECT_FALSE(result.at("onClick").handlers[0].args.IsValid());
}

// ===== AC-013: call 和 as 字段禁止使用表达式 =====

TEST_F(EventHandlerParserTest, L0_should_skip_handler_when_call_is_object_expression)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": { "path": "/funcName" } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_when_call_is_function_call_expression)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": { "call": "getFunctionName", "args": {} } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_when_as_is_object_expression)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "as": { "path": "/varName" } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_when_as_is_function_call_expression)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "as": { "call": "getVarName", "args": {} } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_skip_handler_when_condition_is_object_expression)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "condition": { "path": "/flag" } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    EXPECT_EQ(result.size(), 0u);
}

TEST_F(EventHandlerParserTest, L0_should_accept_handler_with_valid_call_and_as)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "myFunc", "as": "result" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "myFunc");
    EXPECT_EQ(result.at("onClick").handlers[0].as, "result");
}

TEST_F(EventHandlerParserTest, L0_should_keep_handler_and_drop_invalid_string_as_bindings)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": "fn1", "as": "1result" },
            { "call": "fn2", "as": "bad-name" },
            { "call": "fn3", "as": "$result" },
            { "call": "fn4", "as": "bad name" },
            { "call": "fn5", "as": "__dataModel" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    ASSERT_EQ(result.at("onClick").handlers.size(), 5u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "fn1");
    EXPECT_TRUE(result.at("onClick").handlers[0].as.empty());
    EXPECT_EQ(result.at("onClick").handlers[1].call, "fn2");
    EXPECT_TRUE(result.at("onClick").handlers[1].as.empty());
    EXPECT_EQ(result.at("onClick").handlers[2].call, "fn3");
    EXPECT_TRUE(result.at("onClick").handlers[2].as.empty());
    EXPECT_EQ(result.at("onClick").handlers[3].call, "fn4");
    EXPECT_TRUE(result.at("onClick").handlers[3].as.empty());
    EXPECT_EQ(result.at("onClick").handlers[4].call, "fn5");
    EXPECT_TRUE(result.at("onClick").handlers[4].as.empty());
}

TEST_F(EventHandlerParserTest, L0_should_skip_invalid_handlers_but_keep_valid_ones)
{
    auto descriptor = ParseJson(R"({
        "onClick": [
            { "call": { "path": "/fn" } },
            { "call": "validFunc" },
            { "call": "fn2", "as": { "path": "/varName" } }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers.size(), 1u);
    EXPECT_EQ(result.at("onClick").handlers[0].call, "validFunc");
}

TEST_F(EventHandlerParserTest, L0_should_set_event_name_correctly)
{
    auto descriptor = ParseJson(R"({
        "onChange": [
            { "call": "handleChange" }
        ]
    })");

    auto result = EventHandlerParser::Parse(descriptor);

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result.at("onChange").eventName, "onChange");
}
