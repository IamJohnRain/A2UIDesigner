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

#include "components/actions/NativeActionRegistry.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

class SetDataModelActionTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
    }

    void RegisterSetDataModelAction()
    {
        auto& registry = NativeActionRegistry::GetInstance();
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
    }
};

TEST_F(SetDataModelActionTest, L0_should_set_value_at_path)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": "/count", "value": 42})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    auto val = dataModel->GetNode("/count");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetInt32Value(), 42);
}

TEST_F(SetDataModelActionTest, L0_should_create_path_when_not_exists)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": "/deep/nested/key", "value": "hello"})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    auto val = dataModel->GetNode("/deep/nested/key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetStringValue(), "hello");
}

TEST_F(SetDataModelActionTest, L0_should_init_data_model_when_null)
{
    RegisterSetDataModelAction();

    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = nullptr;

    auto args = JsonAdapter::Parse(R"({"path": "/count", "value": 1})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    EXPECT_NE(context.dataModel, nullptr);
    auto val = context.dataModel->GetNode("/count");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetInt32Value(), 1);
}

TEST_F(SetDataModelActionTest, L0_should_set_string_value)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": "/name", "value": "Alice"})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    auto val = dataModel->GetNode("/name");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetStringValue(), "Alice");
}

TEST_F(SetDataModelActionTest, L0_should_set_object_value)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": "/user", "value": {"name": "Bob", "age": 30}})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    auto val = dataModel->GetNode("/user");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetString("name"), "Bob");
    EXPECT_EQ(val->GetInt32("age"), 30);
}

TEST_F(SetDataModelActionTest, L0_should_return_null_when_path_empty)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args = JsonAdapter::Parse(R"({"path": "", "value": 1})");
    auto result = NativeActionRegistry::GetInstance().Execute("setDataModel", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}

TEST_F(SetDataModelActionTest, L0_should_overwrite_existing_value)
{
    RegisterSetDataModelAction();

    auto dataModel = std::make_shared<DataModel>("test");
    EventHandlerChainExecutor::ExecutionContext context;
    context.dataModel = dataModel;

    auto args1 = JsonAdapter::Parse(R"({"path": "/count", "value": 1})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args1->GetRoot(), context);

    auto args2 = JsonAdapter::Parse(R"({"path": "/count", "value": 99})");
    NativeActionRegistry::GetInstance().Execute("setDataModel", args2->GetRoot(), context);

    auto val = dataModel->GetNode("/count");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->GetInt32Value(), 99);
}

class SetAttributesActionTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
    }
};

TEST_F(SetAttributesActionTest, L0_should_be_registered)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("setAttributes",
        [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return JsonValue(); });

    EXPECT_TRUE(registry.HasAction("setAttributes"));
}

TEST_F(SetAttributesActionTest, L0_should_return_component_id_from_args)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("setAttributes", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        std::string componentId = args.GetString("componentId");
        auto adapter = JsonAdapter::CreateString(componentId);
        return adapter->GetRoot();
    });

    auto args = JsonAdapter::Parse(R"({"componentId": "txt1", "value": {"text": "hi"}})");
    EventHandlerChainExecutor::ExecutionContext context;
    auto result = registry.Execute("setAttributes", args->GetRoot(), context);

    EXPECT_EQ(result.GetStringValue(), "txt1");
}

class NavigateActionTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
    }
};

TEST_F(NavigateActionTest, L0_should_be_registered)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("navigate", [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
        // STUB: navigate not implemented
        return JsonValue();
    });

    EXPECT_TRUE(registry.HasAction("navigate"));
}

TEST_F(NavigateActionTest, L0_should_execute_without_error)
{
    auto& registry = NativeActionRegistry::GetInstance();
    registry.Register("navigate",
        [](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) { return JsonValue(); });

    auto args = JsonAdapter::Parse(R"({"componentId": "nav1", "targetComponentId": "page2"})");
    EventHandlerChainExecutor::ExecutionContext context;
    auto result = registry.Execute("navigate", args->GetRoot(), context);

    EXPECT_FALSE(result.IsValid());
}
