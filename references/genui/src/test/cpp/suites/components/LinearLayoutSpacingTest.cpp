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

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "components/A2UI/column/ColumnComponent.h"
#include "components/A2UI/row/RowComponent.h"
#include "components/Component.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

class DummyChildComponent : public Component {
public:
    explicit DummyChildComponent(ArkUI_NodeHandle handle) : Component(handle, false) {}
    ~DummyChildComponent() override = default;
    std::string GetType() const override
    {
        return "Dummy";
    }
};

bool FindLastMargin(const MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, std::array<float, 4>& margin)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle != nodeHandle || it->attribute != NODE_MARGIN || it->values.size() != 4) {
            continue;
        }
        margin = { it->values[0].f32, it->values[1].f32, it->values[2].f32, it->values[3].f32 };
        return true;
    }
    return false;
}

bool FindLastAttribute(
    const MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, int32_t attribute, ArkUI_NumberValue& value)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle != nodeHandle || it->attribute != attribute || it->values.empty()) {
            continue;
        }
        value = it->values[0];
        return true;
    }
    return false;
}

std::unique_ptr<JsonAdapter> ParseJson(const std::string& content)
{
    return JsonAdapter::Parse(content);
}

} // namespace

class LinearLayoutSpacingTest : public A2UITest {};

TEST_F(LinearLayoutSpacingTest, should_apply_row_fallback_alignment_and_justify_from_descriptor)
{
    auto row = std::make_shared<RowComponent>();
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","align":"invalid","justify":"invalid"})");
    ASSERT_NE(descriptor, nullptr);
    row->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NumberValue alignValue = {};
    ArkUI_NumberValue justifyValue = {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, alignValue));
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, justifyValue));
    EXPECT_EQ(alignValue.i32, ARKUI_VERTICAL_ALIGNMENT_TOP);
    EXPECT_EQ(justifyValue.i32, ARKUI_FLEX_ALIGNMENT_START);
}

TEST_F(LinearLayoutSpacingTest, should_apply_column_fallback_alignment_and_justify_from_descriptor)
{
    auto column = std::make_shared<ColumnComponent>();
    auto descriptor = ParseJson(R"({"id":"column","component":"Column","align":"invalid","justify":"invalid"})");
    ASSERT_NE(descriptor, nullptr);
    column->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NumberValue alignValue = {};
    ArkUI_NumberValue justifyValue = {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, alignValue));
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, justifyValue));
    EXPECT_EQ(alignValue.i32, ARKUI_HORIZONTAL_ALIGNMENT_START);
    EXPECT_EQ(justifyValue.i32, ARKUI_FLEX_ALIGNMENT_START);
}

TEST_F(LinearLayoutSpacingTest, should_forward_row_child_add_move_remove_to_base_logic)
{
    auto row = std::make_shared<RowComponent>();
    auto childA = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x701));
    auto childB = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x702));

    row->AddChild(childA);
    row->AddChild(childB);
    ASSERT_EQ(row->GetChildren().size(), 2U);

    row->AddChildAt(childA, 1);
    ASSERT_EQ(row->GetChildren().size(), 2U);
    auto iter = row->GetChildren().begin();
    EXPECT_EQ(*iter, childB);
    ++iter;
    EXPECT_EQ(*iter, childA);

    row->ClearChildren();
    EXPECT_EQ(row->GetChildren().size(), 0U);
}

TEST_F(LinearLayoutSpacingTest, should_forward_column_child_add_move_remove_to_base_logic)
{
    auto column = std::make_shared<ColumnComponent>();
    auto childA = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x711));
    auto childB = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x712));

    column->AddChild(childA);
    column->AddChild(childB);
    ASSERT_EQ(column->GetChildren().size(), 2U);

    column->AddChildAt(childA, 1);
    ASSERT_EQ(column->GetChildren().size(), 2U);
    auto iter = column->GetChildren().begin();
    EXPECT_EQ(*iter, childB);
    ++iter;
    EXPECT_EQ(*iter, childA);

    column->ClearChildren();
    EXPECT_EQ(column->GetChildren().size(), 0U);
}

/**
 * @tc.name: LinearLayoutSpacingTest001
 * @tc.desc: Verify column spacing distribution for weight consistency.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest001)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x11));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x12));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x13));
    column->AddChild(child1);
    column->AddChild(child2);
    column->AddChild(child3);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));

    EXPECT_FLOAT_EQ(child1Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[2], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest002
 * @tc.desc: Verify row spacing distribution for weight consistency.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest002)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x21));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x22));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x23));
    row->AddChild(child1);
    row->AddChild(child2);
    row->AddChild(child3);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));

    EXPECT_FLOAT_EQ(child1Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child1Margin[3], 0.0F);
    EXPECT_FLOAT_EQ(child2Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child2Margin[3], 8.0F);
    EXPECT_FLOAT_EQ(child3Margin[1], 0.0F);
    EXPECT_FLOAT_EQ(child3Margin[3], 8.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest003
 * @tc.desc: Verify column spacing with 2 children (evenly distributed).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest003)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x31));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x32));
    column->AddChild(child1);
    column->AddChild(child2);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));

    EXPECT_FLOAT_EQ(child1Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[2], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest004
 * @tc.desc: Verify row spacing with 2 children (evenly distributed).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest004)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x41));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x42));
    row->AddChild(child1);
    row->AddChild(child2);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));

    EXPECT_FLOAT_EQ(child1Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child1Margin[3], 0.0F);
    EXPECT_FLOAT_EQ(child2Margin[1], 0.0F);
    EXPECT_FLOAT_EQ(child2Margin[3], 8.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest005
 * @tc.desc: Verify column spacing with 1 child (no margin).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest005)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x51));
    column->AddChild(child1);

    std::array<float, 4> child1Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));

    EXPECT_FLOAT_EQ(child1Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[2], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest006
 * @tc.desc: Verify column spacing with 4 children (totalChildren > 3 branch).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest006)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2","c3","c4"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x61));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x62));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x63));
    auto child4 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x64));

    column->AddChild(child1);
    column->AddChild(child2);
    column->AddChild(child3);
    column->AddChild(child4);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    std::array<float, 4> child4Margin = {};

    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child4->GetNativeView(), child4Margin));

    EXPECT_FLOAT_EQ(child1Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child4Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child4Margin[2], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest007
 * @tc.desc: Verify column move child to same position (currentIndex == targetIndex).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest007)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x71));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x72));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x73));

    column->AddChild(child1);
    column->AddChild(child2);
    column->AddChild(child3);

    column->AddChildAt(child2, 1);

    std::array<float, 4> child2Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));

    EXPECT_FLOAT_EQ(child2Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child2Margin[2], 4.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest008
 * @tc.desc: Verify column remove all children clears spacing.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest008)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x81));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x82));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x83));

    column->AddChild(child1);
    column->AddChild(child2);
    column->AddChild(child3);

    ASSERT_EQ(column->GetChildren().size(), 3U);

    column->ClearChildren();
    ASSERT_EQ(column->GetChildren().size(), 0U);
}

/**
 * @tc.name: LinearLayoutSpacingTest013
 * @tc.desc: Verify column move child with totalChildren > 3.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest013)
{
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["c1","c2","c3","c4"]})");
    ASSERT_NE(descriptor, nullptr);

    auto column = std::make_shared<ColumnComponent>();
    column->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xA1));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xA2));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xA3));
    auto child4 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xA4));

    column->AddChild(child1);
    column->AddChild(child2);
    column->AddChild(child3);
    column->AddChild(child4);

    column->AddChildAt(child1, 3);

    auto children = column->GetChildren();
    ASSERT_EQ(children.size(), 4U);
    auto it = children.begin();
    EXPECT_EQ(*it, child2);
    ++it;
    EXPECT_EQ(*it, child3);
    ++it;
    EXPECT_EQ(*it, child4);
    ++it;
    EXPECT_EQ(*it, child1);

    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    std::array<float, 4> child4Margin = {};
    std::array<float, 4> child1Margin = {};

    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child4->GetNativeView(), child4Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));

    EXPECT_FLOAT_EQ(child2Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(child2Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child3Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child4Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child4Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(child1Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(child1Margin[2], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest014
 * @tc.desc: Verify row spacing with 1 child (no margin).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest014)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xB1));
    row->AddChild(child1);

    std::array<float, 4> child1Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));

    EXPECT_FLOAT_EQ(child1Margin[1], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[3], 0.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest015
 * @tc.desc: Verify row spacing with 4 children (totalChildren > 3 branch).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest015)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2","c3","c4"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xC1));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xC2));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xC3));
    auto child4 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xC4));

    row->AddChild(child1);
    row->AddChild(child2);
    row->AddChild(child3);
    row->AddChild(child4);

    std::array<float, 4> child1Margin = {};
    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    std::array<float, 4> child4Margin = {};

    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child4->GetNativeView(), child4Margin));

    EXPECT_FLOAT_EQ(child1Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child1Margin[3], 0.0F);
    EXPECT_FLOAT_EQ(child2Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child2Margin[3], 8.0F);
    EXPECT_FLOAT_EQ(child3Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child3Margin[3], 8.0F);
    EXPECT_FLOAT_EQ(child4Margin[1], 0.0F);
    EXPECT_FLOAT_EQ(child4Margin[3], 8.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest016
 * @tc.desc: Verify row move child to same position (currentIndex == targetIndex).
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest016)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xD1));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xD2));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xD3));

    row->AddChild(child1);
    row->AddChild(child2);
    row->AddChild(child3);

    row->AddChildAt(child2, 1);

    std::array<float, 4> child2Margin = {};
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));

    EXPECT_FLOAT_EQ(child2Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child2Margin[3], 8.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest017
 * @tc.desc: Verify row remove all children clears spacing.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest017)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2","c3"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xE1));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xE2));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xE3));

    row->AddChild(child1);
    row->AddChild(child2);
    row->AddChild(child3);

    ASSERT_EQ(row->GetChildren().size(), 3U);

    row->ClearChildren();
    ASSERT_EQ(row->GetChildren().size(), 0U);
}

/**
 * @tc.name: LinearLayoutSpacingTest018
 * @tc.desc: Verify row move child with totalChildren > 3.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest018)
{
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["c1","c2","c3","c4"]})");
    ASSERT_NE(descriptor, nullptr);

    auto row = std::make_shared<RowComponent>();
    row->ApplyDescriptor(descriptor->GetRoot());

    auto child1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF1));
    auto child2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF2));
    auto child3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF3));
    auto child4 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF4));

    row->AddChild(child1);
    row->AddChild(child2);
    row->AddChild(child3);
    row->AddChild(child4);

    row->AddChildAt(child1, 3);

    auto children = row->GetChildren();
    ASSERT_EQ(children.size(), 4U);
    auto it = children.begin();
    EXPECT_EQ(*it, child2);
    ++it;
    EXPECT_EQ(*it, child3);
    ++it;
    EXPECT_EQ(*it, child4);
    ++it;
    EXPECT_EQ(*it, child1);

    std::array<float, 4> child2Margin = {};
    std::array<float, 4> child3Margin = {};
    std::array<float, 4> child4Margin = {};
    std::array<float, 4> child1Margin = {};

    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child2->GetNativeView(), child2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child3->GetNativeView(), child3Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child4->GetNativeView(), child4Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, child1->GetNativeView(), child1Margin));

    EXPECT_FLOAT_EQ(child2Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child2Margin[3], 0.0F);
    EXPECT_FLOAT_EQ(child3Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child3Margin[3], 8.0F);
    EXPECT_FLOAT_EQ(child4Margin[1], 8.0F);
    EXPECT_FLOAT_EQ(child4Margin[3], 8.0F);
    EXPECT_FLOAT_EQ(child1Margin[1], 0.0F);
    EXPECT_FLOAT_EQ(child1Margin[3], 8.0F);
}

/**
 * @tc.name: LinearLayoutSpacingTest019
 * @tc.desc: Verify nested column spacing - inner column children should have spacing.
 * @tc.type: FUNC
 */
TEST_F(LinearLayoutSpacingTest, LinearLayoutSpacingTest019)
{
    auto outerColumn = std::make_shared<ColumnComponent>();
    outerColumn->SetComponentId("outer");

    auto innerColumn = std::make_shared<ColumnComponent>();
    innerColumn->SetComponentId("inner");

    auto innerChild1 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x1010));
    auto innerChild2 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x1020));
    auto innerChild3 = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x1030));

    innerColumn->AddChild(innerChild1);
    innerColumn->AddChild(innerChild2);
    innerColumn->AddChild(innerChild3);

    outerColumn->AddChild(innerColumn);

    std::array<float, 4> innerChild1Margin = {};
    std::array<float, 4> innerChild2Margin = {};
    std::array<float, 4> innerChild3Margin = {};

    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, innerChild1->GetNativeView(), innerChild1Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, innerChild2->GetNativeView(), innerChild2Margin));
    ASSERT_TRUE(FindLastMargin(mockArkUIPtr_, innerChild3->GetNativeView(), innerChild3Margin));

    EXPECT_FLOAT_EQ(innerChild1Margin[0], 0.0F);
    EXPECT_FLOAT_EQ(innerChild1Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(innerChild2Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(innerChild2Margin[2], 4.0F);
    EXPECT_FLOAT_EQ(innerChild3Margin[0], 4.0F);
    EXPECT_FLOAT_EQ(innerChild3Margin[2], 0.0F);
}
