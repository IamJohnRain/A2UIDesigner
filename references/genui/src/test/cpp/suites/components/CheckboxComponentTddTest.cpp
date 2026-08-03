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

#include "components/A2UI/checkbox/CheckboxComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

namespace {

std::vector<ArkUI_NodeHandle> g_checkboxCreateNodeResults;
size_t g_checkboxCreateNodeIndex = 0;

ArkUI_NodeHandle CreateCheckboxNodeFromSequence(ArkUI_NodeType type)
{
    if (g_checkboxCreateNodeIndex < g_checkboxCreateNodeResults.size()) {
        return g_checkboxCreateNodeResults[g_checkboxCreateNodeIndex++];
    }
    ++g_checkboxCreateNodeIndex;
    return TrackCreateNode(type);
}

void UseCheckboxCreateNodeSequence(ArkUI_NativeNodeAPI_1* api, const std::vector<ArkUI_NodeHandle>& results)
{
    g_checkboxCreateNodeResults = results;
    g_checkboxCreateNodeIndex = 0;
    api->createNode = CreateCheckboxNodeFromSequence;
}

int32_t CountCheckboxAddChildCalls(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.addChildCalls) {
        if (call.first == parent && call.second == child) {
            ++count;
        }
    }
    return count;
}

class CheckboxComponentProbe : public CheckboxComponent {
public:
    void InvokeOnAttachToParent()
    {
        OnAttachToParent();
    }
};

} // namespace

class CheckboxComponentTddTest : public A2UIComponentTddTest {};

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_create_internal_nodes_and_report_type)
{
    auto checkbox = std::make_shared<CheckboxComponent>();
    ASSERT_NE(checkbox, nullptr);
    ArkUI_NodeHandle rowNode = checkbox->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);

    EXPECT_EQ(checkbox->GetType(), "CheckBox");
    EXPECT_EQ(rowNode, FindCreatedNode(ARKUI_NODE_ROW));
    ASSERT_NE(textNode, nullptr);
    ASSERT_NE(checkboxNode, nullptr);
    EXPECT_FALSE(HasAddChildCall(rowNode, textNode));
    EXPECT_FALSE(HasAddChildCall(rowNode, checkboxNode));

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(checkbox);

    EXPECT_TRUE(HasAddChildCall(rowNode, textNode));
    EXPECT_TRUE(HasAddChildCall(rowNode, checkboxNode));
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_apply_label_value_style_and_enabled_state)
{
    auto checkbox = std::make_shared<CheckboxComponent>();
    ArkUI_NodeHandle rowNode = checkbox->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);
    auto descriptor = ParseJson(R"({"id":"agree","component":"CheckBox","label":"Agree","value":true})");
    ASSERT_NE(descriptor, nullptr);

    checkbox->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(textNode, NODE_TEXT_CONTENT, "Agree");
    ExpectI32Attribute(checkboxNode, NODE_CHECKBOX_SELECT, 1);
    ExpectU32Attribute(checkboxNode, NODE_CHECKBOX_SELECT_COLOR, 0xFF007DFFU);
    ExpectI32Attribute(checkboxNode, NODE_CHECKBOX_SHAPE, ArkUI_CHECKBOX_SHAPE_CIRCLE);
    ExpectI32Attribute(checkboxNode, NODE_ENABLED, 1);
    ExpectI32Attribute(rowNode, NODE_ROW_ALIGN_ITEMS, ARKUI_VERTICAL_ALIGNMENT_CENTER);
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_disable_when_checks_fail_and_refresh_on_check_update)
{
    auto checkbox = std::make_shared<CheckboxComponent>();
    ArkUI_NodeHandle checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);
    auto descriptor =
        ParseJson(R"({"id":"guardedCheckbox","component":"CheckBox","label":"Agree","value":true,)"
                  R"("checks":[{"condition":{"call":"required","args":{"value":""}},"message":"required"}]})");
    ASSERT_NE(descriptor, nullptr);

    checkbox->ApplyDescriptor(descriptor->GetRoot());
    ExpectI32Attribute(checkboxNode, NODE_ENABLED, 0);

    auto updateValue = JsonAdapter::CreateString("refresh");
    ASSERT_NE(updateValue, nullptr);
    checkbox->OnDataUpdate("__checks_dep_1", updateValue->GetRoot());

    ExpectI32Attribute(checkboxNode, NODE_ENABLED, 0);
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_apply_public_checkbox_setters)
{
    auto checkbox = std::make_shared<CheckboxComponent>();
    ArkUI_NodeHandle checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);

    checkbox->SetSelect(false);
    checkbox->SetSelectColor(0xFF00AA00U);
    checkbox->SetCheckboxShape(A2UICheckboxShape::ROUNDED_SQUARE);

    ExpectI32Attribute(checkboxNode, NODE_CHECKBOX_SELECT, 0);
    ExpectU32Attribute(checkboxNode, NODE_CHECKBOX_SELECT_COLOR, 0xFF00AA00U);
    ExpectI32Attribute(checkboxNode, NODE_CHECKBOX_SHAPE, ArkUI_CHECKBOX_SHAPE_ROUNDED_SQUARE);
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_remove_and_dispose_internal_nodes_on_destroy)
{
    ArkUI_NodeHandle rowNode = nullptr;
    ArkUI_NodeHandle textNode = nullptr;
    ArkUI_NodeHandle checkboxNode = nullptr;
    {
        auto checkbox = std::make_shared<CheckboxComponent>();
        auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
        rowNode = checkbox->GetNativeView();
        textNode = FindCreatedNode(ARKUI_NODE_TEXT);
        checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);
        ASSERT_NE(rowNode, nullptr);
        ASSERT_NE(textNode, nullptr);
        ASSERT_NE(checkboxNode, nullptr);
        parent->AddChild(checkbox);
    }

    EXPECT_TRUE(HasAddChildCall(rowNode, textNode));
    EXPECT_TRUE(HasAddChildCall(rowNode, checkboxNode));
    ASSERT_GE(g_tracker.removeChildCalls.size(), 2U);
    EXPECT_GE(g_tracker.disposeNodeCount, 2);
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_not_attach_internal_nodes_twice)
{
    auto checkbox = std::make_shared<CheckboxComponentProbe>();
    ArkUI_NodeHandle rowNode = checkbox->GetNativeView();
    ArkUI_NodeHandle textNode = FindCreatedNode(ARKUI_NODE_TEXT);
    ArkUI_NodeHandle checkboxNode = FindCreatedNode(ARKUI_NODE_CHECKBOX);
    ASSERT_NE(rowNode, nullptr);
    ASSERT_NE(textNode, nullptr);
    ASSERT_NE(checkboxNode, nullptr);

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(checkbox);
    ASSERT_EQ(CountCheckboxAddChildCalls(rowNode, textNode), 1);
    ASSERT_EQ(CountCheckboxAddChildCalls(rowNode, checkboxNode), 1);

    checkbox->InvokeOnAttachToParent();

    EXPECT_EQ(CountCheckboxAddChildCalls(rowNode, textNode), 1);
    EXPECT_EQ(CountCheckboxAddChildCalls(rowNode, checkboxNode), 1);
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_skip_attach_when_native_view_is_missing)
{
    ArkUI_NodeHandle textNode = reinterpret_cast<ArkUI_NodeHandle>(0x810010);
    ArkUI_NodeHandle checkboxNode = reinterpret_cast<ArkUI_NodeHandle>(0x810011);
    UseCheckboxCreateNodeSequence(api_, { nullptr, textNode, checkboxNode });

    auto checkbox = std::make_shared<CheckboxComponent>();
    EXPECT_EQ(checkbox->GetNativeView(), nullptr);
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(checkbox);

    EXPECT_FALSE(HasAddChildCall(nullptr, textNode));
    EXPECT_FALSE(HasAddChildCall(nullptr, checkboxNode));
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_skip_attach_when_text_node_is_missing)
{
    ArkUI_NodeHandle rowNode = reinterpret_cast<ArkUI_NodeHandle>(0x810020);
    ArkUI_NodeHandle checkboxNode = reinterpret_cast<ArkUI_NodeHandle>(0x810021);
    UseCheckboxCreateNodeSequence(api_, { rowNode, nullptr, checkboxNode });

    auto checkbox = std::make_shared<CheckboxComponent>();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(checkbox);

    EXPECT_FALSE(HasAddChildCall(rowNode, checkboxNode));
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_skip_attach_when_checkbox_node_is_missing)
{
    ArkUI_NodeHandle rowNode = reinterpret_cast<ArkUI_NodeHandle>(0x810030);
    ArkUI_NodeHandle textNode = reinterpret_cast<ArkUI_NodeHandle>(0x810031);
    UseCheckboxCreateNodeSequence(api_, { rowNode, textNode, nullptr });

    auto checkbox = std::make_shared<CheckboxComponent>();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(checkbox);

    EXPECT_FALSE(HasAddChildCall(rowNode, textNode));
}

TEST_F(CheckboxComponentTddTest, L0_checkbox_should_return_theme_when_surface_context_is_available)
{
    auto checkbox = std::make_shared<CheckboxComponent>();
    PrepareThemeContext(*checkbox);

    EXPECT_NE(checkbox->GetTheme(), nullptr);
}
