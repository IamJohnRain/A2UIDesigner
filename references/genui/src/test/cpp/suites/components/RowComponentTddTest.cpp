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

#include "components/A2UI/row/RowComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class RowComponentTddTest : public A2UIComponentTddTest {};

namespace {

class RowComponentProbe : public RowComponent {
public:
    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }
};

} // namespace

TEST_F(RowComponentTddTest, L0_row_should_create_row_node_and_report_type)
{
    auto row = std::make_shared<RowComponent>();
    ASSERT_NE(row, nullptr);

    EXPECT_EQ(row->GetType(), "Row");
    EXPECT_EQ(row->GetNativeView(), FindCreatedNode(ARKUI_NODE_ROW));
}

TEST_F(RowComponentTddTest, L0_row_should_apply_alignment_descriptor_values)
{
    auto row = std::make_shared<RowComponent>();
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","align":"start","justify":"spaceEvenly"})");
    ASSERT_NE(descriptor, nullptr);

    row->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_TOP);
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY);
}

TEST_F(RowComponentTddTest, L0_row_should_fallback_invalid_alignment_tokens)
{
    auto row = std::make_shared<RowComponent>();
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","align":"bad","justify":"bad"})");
    ASSERT_NE(descriptor, nullptr);

    row->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_TOP);
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_START);
}

TEST_F(RowComponentTddTest, L0_row_should_apply_public_alignment_setters)
{
    auto row = std::make_shared<RowComponent>();

    row->SetAlignItems(A2UIVerticalAlignment::BOTTOM);
    row->SetJustifyContent(A2UIFlexAlignment::END);

    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_BOTTOM);
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_END);
}

TEST_F(RowComponentTddTest, L0_row_should_apply_remaining_alignment_tokens)
{
    auto row = std::make_shared<RowComponent>();
    auto centerDescriptor = ParseJson(R"({"id":"row","component":"Row","align":"center","justify":"center"})");
    ASSERT_NE(centerDescriptor, nullptr);
    auto aroundDescriptor = ParseJson(R"({"id":"row","component":"Row","align":"end","justify":"spaceAround"})");
    ASSERT_NE(aroundDescriptor, nullptr);
    auto betweenDescriptor = ParseJson(R"({"id":"row","component":"Row","align":"center","justify":"spaceBetween"})");
    ASSERT_NE(betweenDescriptor, nullptr);

    row->ApplyDescriptor(centerDescriptor->GetRoot());
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_CENTER);
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_CENTER);

    row->ApplyDescriptor(aroundDescriptor->GetRoot());
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_BOTTOM);
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_AROUND);

    row->ApplyDescriptor(betweenDescriptor->GetRoot());
    ExpectI32Attribute(row->GetNativeView(), NODE_ROW_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN);
}

TEST_F(RowComponentTddTest, L0_row_should_collect_static_child_ids)
{
    RowComponentProbe row;
    auto descriptor = ParseJson(R"({"id":"row","component":"Row","children":["first","second"]})");
    ASSERT_NE(descriptor, nullptr);

    row.InvokeCollectChildListDescriptor(descriptor->GetRoot());

    ASSERT_EQ(row.GetChildListDescriptor().staticChildIds.size(), 2U);
    EXPECT_EQ(row.GetChildListDescriptor().staticChildIds.front(), "first");
    EXPECT_EQ(row.GetChildListDescriptor().staticChildIds.back(), "second");
}

TEST_F(RowComponentTddTest, L0_row_should_return_theme_when_surface_context_is_available)
{
    auto row = std::make_shared<RowComponent>();
    PrepareThemeContext(*row);

    EXPECT_NE(row->GetTheme(), nullptr);
}
