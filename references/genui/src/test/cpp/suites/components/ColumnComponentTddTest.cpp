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

#include "components/A2UI/column/ColumnComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class ColumnComponentTddTest : public A2UIComponentTddTest {};

namespace {

class ColumnComponentProbe : public ColumnComponent {
public:
    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }
};

} // namespace

TEST_F(ColumnComponentTddTest, L0_column_should_create_column_node_and_report_type)
{
    auto column = std::make_shared<ColumnComponent>();
    ASSERT_NE(column, nullptr);

    EXPECT_EQ(column->GetType(), "Column");
    EXPECT_EQ(column->GetNativeView(), FindCreatedNode(ARKUI_NODE_COLUMN));
}

TEST_F(ColumnComponentTddTest, L0_column_should_apply_alignment_descriptor_values)
{
    auto column = std::make_shared<ColumnComponent>();
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","align":"end","justify":"spaceBetween"})");
    ASSERT_NE(descriptor, nullptr);

    column->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_END);
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN);
}

TEST_F(ColumnComponentTddTest, L0_column_should_fallback_invalid_alignment_tokens)
{
    auto column = std::make_shared<ColumnComponent>();
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","align":"bad","justify":"bad"})");
    ASSERT_NE(descriptor, nullptr);

    column->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_START);
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_START);
}

TEST_F(ColumnComponentTddTest, L0_column_should_apply_public_alignment_setters)
{
    auto column = std::make_shared<ColumnComponent>();

    column->SetAlignItems(A2UIHorizontalAlignment::START);
    column->SetJustifyContent(A2UIFlexAlignment::SPACE_EVENLY);

    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_START);
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY);
}

TEST_F(ColumnComponentTddTest, L0_column_should_apply_remaining_alignment_tokens)
{
    auto column = std::make_shared<ColumnComponent>();
    auto centerDescriptor = ParseJson(R"({"id":"col","component":"Column","align":"center","justify":"center"})");
    ASSERT_NE(centerDescriptor, nullptr);
    auto aroundDescriptor = ParseJson(R"({"id":"col","component":"Column","align":"start","justify":"spaceAround"})");
    ASSERT_NE(aroundDescriptor, nullptr);

    column->ApplyDescriptor(centerDescriptor->GetRoot());
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_CENTER);
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_CENTER);

    column->ApplyDescriptor(aroundDescriptor->GetRoot());
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_START);
    ExpectI32Attribute(column->GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_AROUND);
}

TEST_F(ColumnComponentTddTest, L0_column_should_collect_static_child_ids)
{
    ColumnComponentProbe column;
    auto descriptor = ParseJson(R"({"id":"col","component":"Column","children":["first","second"]})");
    ASSERT_NE(descriptor, nullptr);

    column.InvokeCollectChildListDescriptor(descriptor->GetRoot());

    ASSERT_EQ(column.GetChildListDescriptor().staticChildIds.size(), 2U);
    EXPECT_EQ(column.GetChildListDescriptor().staticChildIds.front(), "first");
    EXPECT_EQ(column.GetChildListDescriptor().staticChildIds.back(), "second");
}

TEST_F(ColumnComponentTddTest, L0_column_should_return_theme_when_surface_context_is_available)
{
    auto column = std::make_shared<ColumnComponent>();
    PrepareThemeContext(*column);

    EXPECT_NE(column->GetTheme(), nullptr);
}
