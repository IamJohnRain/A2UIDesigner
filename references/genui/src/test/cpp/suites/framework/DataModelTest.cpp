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

#include "data/DataModel.h"

#include <gtest/gtest.h>

#include "utils/JsonAdapter.h"

using namespace NativeModule;

namespace {

JsonValue ParseJsonOrInvalid(const std::string& json)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(json);
    if (adapter == nullptr) {
        return JsonValue();
    }
    return adapter->GetRoot();
}

} // namespace

class DataModelTest : public ::testing::Test {
protected:
    DataModel model_ { "test-surface" };
};

TEST_F(DataModelTest, should_return_surface_id)
{
    EXPECT_EQ(model_.GetSurfaceId(), "test-surface");
}

TEST_F(DataModelTest, should_replace_all_and_read_node)
{
    model_.ReplaceAll(ParseJsonOrInvalid(R"({"user":"Alice","age":30})"));

    std::optional<JsonValue> user = model_.GetNode("/user");
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->GetStringValue(""), "Alice");

    std::optional<JsonValue> age = model_.GetNode("/age");
    ASSERT_TRUE(age.has_value());
    EXPECT_DOUBLE_EQ(age->GetNumberValue(0.0), 30.0);
}

TEST_F(DataModelTest, should_update_nested_path)
{
    model_.ReplaceAll(ParseJsonOrInvalid(R"({})"));
    model_.UpdateByPath("/a/b/c", ParseJsonOrInvalid(R"("deep")"));

    std::optional<JsonValue> value = model_.GetNode("/a/b/c");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value->GetStringValue(""), "deep");
}

TEST_F(DataModelTest, should_delete_path)
{
    model_.ReplaceAll(ParseJsonOrInvalid(R"({"name":"Alice","age":30})"));
    model_.DeleteByPath("/age");

    std::optional<JsonValue> age = model_.GetNode("/age");
    EXPECT_FALSE(age.has_value());

    std::optional<JsonValue> name = model_.GetNode("/name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name->GetStringValue(""), "Alice");
}

TEST_F(DataModelTest, should_support_array_index_in_get_node)
{
    model_.ReplaceAll(ParseJsonOrInvalid(R"({"items":[10,20,30]})"));

    std::optional<JsonValue> item = model_.GetNode("/items/1");
    ASSERT_TRUE(item.has_value());
    EXPECT_DOUBLE_EQ(item->GetNumberValue(0.0), 20.0);
}

TEST_F(DataModelTest, measure_json_depth_flat)
{
    JsonValue flat = ParseJsonOrInvalid(R"({"a":1,"b":2})");
    EXPECT_EQ(DataModel::MeasureJsonDepth(flat), 1);
}

TEST_F(DataModelTest, measure_json_depth_nested)
{
    JsonValue nested = ParseJsonOrInvalid(R"({"a":{"b":{"c":3}}})");
    EXPECT_EQ(DataModel::MeasureJsonDepth(nested), 3);
}

TEST_F(DataModelTest, measure_json_depth_array)
{
    JsonValue arr = ParseJsonOrInvalid(R"({"items":[{"x":1},{"y":{"z":2}}]})");
    EXPECT_EQ(DataModel::MeasureJsonDepth(arr), 4);
}

TEST_F(DataModelTest, measure_json_depth_exactly_20_accepted)
{
    std::string json = R"({"a":)";
    for (int i = 0; i < 19; ++i) {
        json += R"({"a":)";
    }
    json += "1";
    for (int i = 0; i < 19; ++i) {
        json += "}";
    }
    json += "}";
    JsonValue value20 = ParseJsonOrInvalid(json);
    ASSERT_TRUE(value20.IsValid());
    EXPECT_EQ(DataModel::MeasureJsonDepth(value20), 20);
    EXPECT_LE(DataModel::MAX_DATA_MODEL_DEPTH, 20);
}

TEST_F(DataModelTest, measure_json_depth_21_exceeds_limit)
{
    std::string json = R"({"a":)";
    for (int i = 0; i < 20; ++i) {
        json += R"({"a":)";
    }
    json += "1";
    for (int i = 0; i < 20; ++i) {
        json += "}";
    }
    json += "}";
    JsonValue value21 = ParseJsonOrInvalid(json);
    ASSERT_TRUE(value21.IsValid());
    EXPECT_EQ(DataModel::MeasureJsonDepth(value21), 21);
    EXPECT_GT(DataModel::MeasureJsonDepth(value21), DataModel::MAX_DATA_MODEL_DEPTH);
}
