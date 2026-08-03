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

#include <gtest/gtest.h>

#include "utils/JsonAdapter.h"

using namespace NativeModule;

class NativeActionRegistryTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
    }
};

TEST_F(NativeActionRegistryTest, L0_should_register_and_find_action)
{
    auto& registry = NativeActionRegistry::GetInstance();

    registry.Register(
        "testAction", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return args; });

    EXPECT_TRUE(registry.HasAction("testAction"));
    EXPECT_FALSE(registry.HasAction("nonExistent"));
}

TEST_F(NativeActionRegistryTest, L0_should_execute_registered_action)
{
    auto& registry = NativeActionRegistry::GetInstance();

    registry.Register(
        "echo", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return args; });

    auto argsAdapter = JsonAdapter::Parse(R"({"key": "value"})");
    EventHandlerChainExecutor::ExecutionContext emptyContext;
    auto result = registry.Execute("echo", argsAdapter->GetRoot(), emptyContext);

    EXPECT_TRUE(result.IsValid());
    EXPECT_EQ(result.GetString("key"), "value");
}

TEST_F(NativeActionRegistryTest, L0_should_return_null_for_unknown_action)
{
    auto& registry = NativeActionRegistry::GetInstance();
    EventHandlerChainExecutor::ExecutionContext emptyContext;

    auto result = registry.Execute("unknown", JsonValue(), emptyContext);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(NativeActionRegistryTest, L0_should_overwrite_existing_action)
{
    auto& registry = NativeActionRegistry::GetInstance();

    registry.Register("myAction", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        auto adapter = JsonAdapter::CreateString("first");
        return adapter->GetRoot();
    });

    registry.Register("myAction", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        auto adapter = JsonAdapter::CreateString("second");
        return adapter->GetRoot();
    });

    EventHandlerChainExecutor::ExecutionContext emptyContext;
    auto result = registry.Execute("myAction", JsonValue(), emptyContext);
    EXPECT_EQ(result.GetStringValue(), "second");
}

TEST_F(NativeActionRegistryTest, L0_should_clear_all_actions)
{
    auto& registry = NativeActionRegistry::GetInstance();

    registry.Register(
        "a", [](const JsonValue&, const EventHandlerChainExecutor::ExecutionContext&) { return JsonValue(); });
    registry.Register(
        "b", [](const JsonValue&, const EventHandlerChainExecutor::ExecutionContext&) { return JsonValue(); });

    EXPECT_TRUE(registry.HasAction("a"));
    EXPECT_TRUE(registry.HasAction("b"));

    registry.Clear();

    EXPECT_FALSE(registry.HasAction("a"));
    EXPECT_FALSE(registry.HasAction("b"));
}
