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
#include <memory>
#include <string>

#include "composition/ChildListParser.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

TEST(ChildListParserBranchTest, should_skip_empty_and_non_string_ids_in_children_array)
{
    auto adapter = ParseJson(R"({"children": ["first", "", 1, "second"]})");
    ASSERT_NE(adapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));

    EXPECT_EQ(descriptor.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(descriptor.staticChildIds.size(), 2U);
    auto it = descriptor.staticChildIds.begin();
    EXPECT_EQ(*it, "first");
    ++it;
    EXPECT_EQ(*it, "second");
}

TEST(ChildListParserBranchTest, should_return_invalid_for_object_children_with_missing_fields)
{
    auto missingPath = ParseJson(R"({"children": {"componentId": "tmpl"}})");
    auto missingComponentId = ParseJson(R"({"children": {"path": "/items"}})");
    ASSERT_NE(missingPath, nullptr);
    ASSERT_NE(missingComponentId, nullptr);

    ChildListDescriptor descriptor1 = ChildListParser::ParseChildren(missingPath->GetRoot().GetItem("children"));
    ChildListDescriptor descriptor2 = ChildListParser::ParseChildren(missingComponentId->GetRoot().GetItem("children"));

    EXPECT_EQ(descriptor1.type, ChildListType::INVALID);
    EXPECT_EQ(descriptor2.type, ChildListType::INVALID);
}

TEST(ChildListParserBranchTest, should_parse_single_child_string)
{
    std::unique_ptr<JsonAdapter> stringAdapter = JsonAdapter::CreateString("child_1");
    ASSERT_NE(stringAdapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChild(stringAdapter->GetRoot());

    EXPECT_EQ(descriptor.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(descriptor.staticChildIds.size(), 1U);
    EXPECT_EQ(descriptor.staticChildIds.front(), "child_1");
}

TEST(ChildListParserBranchTest, should_return_invalid_for_empty_or_non_string_child)
{
    std::unique_ptr<JsonAdapter> emptyString = JsonAdapter::CreateString("");
    std::unique_ptr<JsonAdapter> number = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(emptyString, nullptr);
    ASSERT_NE(number, nullptr);

    ChildListDescriptor emptyDescriptor = ChildListParser::ParseChild(emptyString->GetRoot());
    ChildListDescriptor nonStringDescriptor = ChildListParser::ParseChild(number->GetRoot());

    EXPECT_EQ(emptyDescriptor.type, ChildListType::INVALID);
    EXPECT_EQ(nonStringDescriptor.type, ChildListType::INVALID);
}

TEST(ChildListParserBranchTest, should_parse_template_path_children_when_both_fields_present)
{
    auto adapter = ParseJson(R"({"children": {"componentId": "tmpl", "path": "/items"}})");
    ASSERT_NE(adapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));
    EXPECT_EQ(descriptor.type, ChildListType::TEMPLATE_PATH);
    EXPECT_EQ(descriptor.templateComponentId, "tmpl");
    EXPECT_EQ(descriptor.templatePath, "/items");
}

TEST(ChildListParserBranchTest, should_parse_valid_custom_loop_variable_names)
{
    auto adapter =
        ParseJson(R"({"children": {"componentId": "tmpl", "path": "/items", "indexVar": "idx", "itemVar": "entry"}})");
    ASSERT_NE(adapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));

    EXPECT_EQ(descriptor.type, ChildListType::TEMPLATE_PATH);
    EXPECT_EQ(descriptor.resolvedIndexVarName, "idx");
    EXPECT_EQ(descriptor.resolvedItemVarName, "entry");
    EXPECT_FALSE(descriptor.useDefaultIndexVar);
    EXPECT_FALSE(descriptor.useDefaultItemVar);
}

TEST(ChildListParserBranchTest, should_fallback_invalid_loop_variable_names_independently)
{
    auto invalidIndex =
        ParseJson(R"({"children": {"componentId": "tmpl", "path": "/items", "indexVar": "1idx", "itemVar": "entry"}})");
    auto invalidItem =
        ParseJson(R"({"children": {"componentId": "tmpl", "path": "/items", "indexVar": "idx", "itemVar": "$entry"}})");
    ASSERT_NE(invalidIndex, nullptr);
    ASSERT_NE(invalidItem, nullptr);

    ChildListDescriptor invalidIndexDescriptor =
        ChildListParser::ParseChildren(invalidIndex->GetRoot().GetItem("children"));
    ChildListDescriptor invalidItemDescriptor =
        ChildListParser::ParseChildren(invalidItem->GetRoot().GetItem("children"));

    EXPECT_EQ(invalidIndexDescriptor.resolvedIndexVarName, "index");
    EXPECT_EQ(invalidIndexDescriptor.resolvedItemVarName, "entry");
    EXPECT_TRUE(invalidIndexDescriptor.useDefaultIndexVar);
    EXPECT_FALSE(invalidIndexDescriptor.useDefaultItemVar);

    EXPECT_EQ(invalidItemDescriptor.resolvedIndexVarName, "idx");
    EXPECT_EQ(invalidItemDescriptor.resolvedItemVarName, "item");
    EXPECT_FALSE(invalidItemDescriptor.useDefaultIndexVar);
    EXPECT_TRUE(invalidItemDescriptor.useDefaultItemVar);
}

TEST(ChildListParserBranchTest, should_fallback_system_prefixed_loop_variable_names)
{
    auto adapter = ParseJson(
        R"({"children": {"componentId": "tmpl", "path": "/items", "indexVar": "__widthBreakpoint", "itemVar": "__dataModel"}})");
    ASSERT_NE(adapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));

    EXPECT_EQ(descriptor.resolvedIndexVarName, "index");
    EXPECT_EQ(descriptor.resolvedItemVarName, "item");
    EXPECT_TRUE(descriptor.useDefaultIndexVar);
    EXPECT_TRUE(descriptor.useDefaultItemVar);
}

TEST(ChildListParserBranchTest, should_fallback_both_custom_loop_variable_names_when_conflicted)
{
    auto adapter = ParseJson(
        R"({"children": {"componentId": "tmpl", "path": "/items", "indexVar": "entry", "itemVar": "entry"}})");
    ASSERT_NE(adapter, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));

    EXPECT_EQ(descriptor.resolvedIndexVarName, "index");
    EXPECT_EQ(descriptor.resolvedItemVarName, "item");
    EXPECT_TRUE(descriptor.useDefaultIndexVar);
    EXPECT_TRUE(descriptor.useDefaultItemVar);
}

TEST(ChildListParserBranchTest, should_return_invalid_for_non_array_non_object_children)
{
    std::unique_ptr<JsonAdapter> number = JsonAdapter::CreateNumber(3.0);
    ASSERT_NE(number, nullptr);

    ChildListDescriptor descriptor = ChildListParser::ParseChildren(number->GetRoot());
    EXPECT_EQ(descriptor.type, ChildListType::INVALID);
    EXPECT_TRUE(descriptor.staticChildIds.empty());
}

} // namespace
