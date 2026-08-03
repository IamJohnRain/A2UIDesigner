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

#include "components/NativeComponentFactory.h"

#include <gtest/gtest.h>

#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

class NativeComponentFactoryTest : public A2UITest {};

TEST_F(NativeComponentFactoryTest, should_resolve_component_type_from_component_field_first)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(R"({"component":"Button","type":"Text"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(NativeComponentFactory::ResolveComponentType(adapter->GetRoot()), "Button");
}

TEST_F(NativeComponentFactoryTest, should_fallback_to_type_field_when_component_field_is_empty)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(R"({"component":"","type":"Text"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(NativeComponentFactory::ResolveComponentType(adapter->GetRoot()), "Text");
}

TEST_F(NativeComponentFactoryTest, should_return_empty_string_when_both_type_fields_missing)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(R"({"id":"x"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(NativeComponentFactory::ResolveComponentType(adapter->GetRoot()), "");
}

TEST_F(NativeComponentFactoryTest, should_create_known_native_component)
{
    std::shared_ptr<Component> component = NativeComponentFactory::CreateComponent("Text");
    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->GetType(), "Text");
}

TEST_F(NativeComponentFactoryTest, should_return_null_for_unknown_component_type)
{
    std::shared_ptr<Component> component = NativeComponentFactory::CreateComponent("UnknownType");
    EXPECT_EQ(component, nullptr);
}

} // namespace
