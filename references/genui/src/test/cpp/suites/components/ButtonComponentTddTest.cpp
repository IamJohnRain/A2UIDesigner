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

#include "components/A2UI/button/ButtonComponent.h"
#include "components/A2UI/text/TextComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

namespace {

constexpr uint32_t PRIMARY_BACKGROUND_COLOR = 0xFF0A59F7U;
constexpr uint32_t PRIMARY_TEXT_COLOR = 0xFFFFFFFFU;
constexpr uint32_t BORDERLESS_BACKGROUND_COLOR = 0x00000000U;
constexpr uint32_t BORDERLESS_TEXT_COLOR = 0xFF0A59F7U;
constexpr uint32_t DEFAULT_BACKGROUND_COLOR = 0x0C000000U;

class ButtonComponentProbe : public ButtonComponent {
public:
    bool InvokeIsKnownAdditionalDescriptorKey(const std::string& propertyName) const
    {
        return IsKnownAdditionalDescriptorKey(propertyName);
    }

    void InvokeOnConfigChange(const ThemeContext& context)
    {
        OnConfigChange(context);
    }
};

class ButtonComponentTddTest : public A2UIComponentTddTest {};

} // namespace

TEST_F(ButtonComponentTddTest, L0_should_create_button_node_and_register_native_receiver)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    ASSERT_NE(button, nullptr);

    ArkUI_NodeHandle buttonNode = FindCreatedNode(ARKUI_NODE_BUTTON);
    ASSERT_NE(buttonNode, nullptr);
    EXPECT_EQ(button->GetNativeView(), buttonNode);
    EXPECT_EQ(button->GetType(), "Button");
    EXPECT_EQ(g_tracker.setUserDataCount, 1);
    EXPECT_EQ(g_tracker.addNodeEventReceiverCount, 1);
    EXPECT_TRUE(HasAddNodeEventReceiverCall(buttonNode));
    EXPECT_NE(FindLastAttributeCall(buttonNode, NODE_ACCESSIBILITY_GROUP), nullptr);
}

TEST_F(ButtonComponentTddTest, L0_should_recognize_action_and_checks_as_button_descriptor_keys)
{
    ButtonComponentProbe button;

    EXPECT_TRUE(button.InvokeIsKnownAdditionalDescriptorKey("action"));
    EXPECT_TRUE(button.InvokeIsKnownAdditionalDescriptorKey("checks"));
    EXPECT_FALSE(button.InvokeIsKnownAdditionalDescriptorKey("borderRadius"));
    EXPECT_FALSE(button.InvokeIsKnownAdditionalDescriptorKey("unknownButtonKey"));
}

TEST_F(ButtonComponentTddTest, L0_should_apply_primary_variant_and_propagate_text_color)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto text = std::make_shared<TextComponent>();
    PrepareThemeContext(*button);

    button->AddChild(text);
    auto descriptor = ParseJson(R"({"id":"primaryButton","component":"Button","action":{"event":{"name":"tap"}},)"
                                R"("variant":"primary"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle buttonNode = button->GetNativeView();
    const NativeAttributeCall* typeCall = FindLastAttributeCall(buttonNode, NODE_BUTTON_TYPE);
    const NativeAttributeCall* backgroundCall = FindLastAttributeCall(buttonNode, NODE_BACKGROUND_COLOR);
    const NativeAttributeCall* textColorCall = FindLastAttributeCall(text->GetNativeView(), NODE_FONT_COLOR);
    const NativeAttributeCall* paddingCall = FindLastAttributeCall(buttonNode, NODE_PADDING);
    ASSERT_NE(typeCall, nullptr);
    ASSERT_NE(backgroundCall, nullptr);
    ASSERT_NE(textColorCall, nullptr);
    ASSERT_NE(paddingCall, nullptr);
    ASSERT_FALSE(typeCall->values.empty());
    ASSERT_FALSE(backgroundCall->values.empty());
    ASSERT_FALSE(textColorCall->values.empty());
    ASSERT_EQ(paddingCall->values.size(), 4U);
    EXPECT_EQ(typeCall->values[0].i32, ARKUI_BUTTON_TYPE_CAPSULE);
    EXPECT_EQ(backgroundCall->values[0].u32, PRIMARY_BACKGROUND_COLOR);
    EXPECT_EQ(textColorCall->values[0].u32, PRIMARY_TEXT_COLOR);
    EXPECT_FLOAT_EQ(paddingCall->values[0].f32, 8.0F);
    EXPECT_FLOAT_EQ(paddingCall->values[1].f32, 16.0F);
    EXPECT_TRUE(HasRegisterNodeEventCall(buttonNode, NODE_ON_CLICK));
}

TEST_F(ButtonComponentTddTest, L0_should_apply_borderless_variant_after_primary_variant)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto text = std::make_shared<TextComponent>();
    PrepareThemeContext(*button);
    button->AddChild(text);

    auto primaryDescriptor = ParseJson(
        R"({"id":"switchButton","component":"Button","action":{"event":{"name":"tap"}},"variant":"primary"})");
    ASSERT_NE(primaryDescriptor, nullptr);
    button->ApplyDescriptor(primaryDescriptor->GetRoot());

    auto borderlessDescriptor = ParseJson(
        R"({"id":"switchButton","component":"Button","action":{"event":{"name":"tap"}},"variant":"borderless"})");
    ASSERT_NE(borderlessDescriptor, nullptr);
    button->ApplyDescriptor(borderlessDescriptor->GetRoot());

    const NativeAttributeCall* typeCall = FindLastAttributeCall(button->GetNativeView(), NODE_BUTTON_TYPE);
    const NativeAttributeCall* backgroundCall = FindLastAttributeCall(button->GetNativeView(), NODE_BACKGROUND_COLOR);
    const NativeAttributeCall* textColorCall = FindLastAttributeCall(text->GetNativeView(), NODE_FONT_COLOR);
    ASSERT_NE(typeCall, nullptr);
    ASSERT_NE(backgroundCall, nullptr);
    ASSERT_NE(textColorCall, nullptr);
    ASSERT_FALSE(typeCall->values.empty());
    ASSERT_FALSE(backgroundCall->values.empty());
    ASSERT_FALSE(textColorCall->values.empty());
    EXPECT_EQ(typeCall->values[0].i32, ARKUI_BUTTON_TYPE_CAPSULE);
    EXPECT_EQ(backgroundCall->values[0].u32, BORDERLESS_BACKGROUND_COLOR);
    EXPECT_EQ(textColorCall->values[0].u32, BORDERLESS_TEXT_COLOR);
}

TEST_F(ButtonComponentTddTest, L0_should_fallback_invalid_variant_to_default_theme_style)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    PrepareThemeContext(*button);

    auto descriptor = ParseJson(R"({"id":"defaultButton","component":"Button","action":{"event":{"name":"tap"}},)"
                                R"("variant":"invalidVariant"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    const NativeAttributeCall* backgroundCall = FindLastAttributeCall(button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(backgroundCall, nullptr);
    ASSERT_FALSE(backgroundCall->values.empty());
    EXPECT_EQ(FindLastAttributeCall(button->GetNativeView(), NODE_BUTTON_TYPE), nullptr);
    EXPECT_EQ(backgroundCall->values[0].u32, DEFAULT_BACKGROUND_COLOR);
}

TEST_F(ButtonComponentTddTest, L0_should_disable_button_when_checks_fail_and_refresh_on_check_update)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto descriptor =
        ParseJson(R"({"id":"guardedButton","component":"Button","action":{"event":{"name":"tap"}},)"
                  R"("checks":[{"condition":{"call":"required","args":{"value":""}},"message":"required"}]})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    const NativeAttributeCall* enabledCall = FindLastAttributeCall(button->GetNativeView(), NODE_ENABLED);
    ASSERT_NE(enabledCall, nullptr);
    ASSERT_FALSE(enabledCall->values.empty());
    EXPECT_EQ(enabledCall->values[0].i32, 0);

    int32_t enabledCallCount = CountAttributeCall(button->GetNativeView(), NODE_ENABLED);
    auto updateValue = JsonAdapter::CreateString("trigger-refresh");
    ASSERT_NE(updateValue, nullptr);
    Component* component = static_cast<Component*>(button.get());
    component->OnDataUpdate("__checks_dep_1", updateValue->GetRoot());

    EXPECT_EQ(CountAttributeCall(button->GetNativeView(), NODE_ENABLED), enabledCallCount + 1);
    const NativeAttributeCall* refreshedEnabledCall = FindLastAttributeCall(button->GetNativeView(), NODE_ENABLED);
    ASSERT_NE(refreshedEnabledCall, nullptr);
    ASSERT_FALSE(refreshedEnabledCall->values.empty());
    EXPECT_EQ(refreshedEnabledCall->values[0].i32, 0);
}

TEST_F(ButtonComponentTddTest, L0_should_ignore_non_text_children_and_accept_text_children_only)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto leaf = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x7200));
    auto text = std::make_shared<TextComponent>();

    button->AddChild(leaf);
    EXPECT_TRUE(g_tracker.insertChildAtCalls.empty());

    button->AddChild(text);
    EXPECT_TRUE(HasInsertChildAtCall(button->GetNativeView(), text->GetNativeView(), 1));
    EXPECT_EQ(g_tracker.insertChildAtCalls.size(), 1U);
}

TEST_F(ButtonComponentTddTest, L0_should_apply_explicit_font_color_to_text_child_added_later)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    PrepareThemeContext(*button);
    auto descriptor = ParseJson(
        R"({"id":"lateTextButton","component":"Button","action":{"event":{"name":"tap"}},"variant":"primary"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    auto text = std::make_shared<TextComponent>();
    button->AddChild(text);

    const NativeAttributeCall* textColorCall = FindLastAttributeCall(text->GetNativeView(), NODE_FONT_COLOR);
    ASSERT_NE(textColorCall, nullptr);
    ASSERT_FALSE(textColorCall->values.empty());
    EXPECT_EQ(textColorCall->values[0].u32, PRIMARY_TEXT_COLOR);
}

TEST_F(ButtonComponentTddTest, L0_should_reset_text_child_font_color_when_removed)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto text = std::make_shared<TextComponent>();
    PrepareThemeContext(*button);
    button->AddChild(text);
    auto descriptor = ParseJson(
        R"({"id":"removeTextButton","component":"Button","action":{"event":{"name":"tap"}},"variant":"primary"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    button->ClearChildren();

    EXPECT_TRUE(HasResetAttributeCall(text->GetNativeView(), NODE_FONT_COLOR));
}

TEST_F(ButtonComponentTddTest, L0_should_reapply_theme_style_on_config_change)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto text = std::make_shared<TextComponent>();
    PrepareThemeContext(*button);
    button->AddChild(text);
    auto descriptor =
        ParseJson(R"({"id":"themeButton","component":"Button","action":{"event":{"name":"tap"}},"variant":"primary"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    ThemeContext context;
    button->InvokeOnConfigChange(context);

    const NativeAttributeCall* backgroundCall = FindLastAttributeCall(button->GetNativeView(), NODE_BACKGROUND_COLOR);
    const NativeAttributeCall* textColorCall = FindLastAttributeCall(text->GetNativeView(), NODE_FONT_COLOR);
    ASSERT_NE(backgroundCall, nullptr);
    ASSERT_NE(textColorCall, nullptr);
    ASSERT_FALSE(backgroundCall->values.empty());
    ASSERT_FALSE(textColorCall->values.empty());
    EXPECT_EQ(backgroundCall->values[0].u32, PRIMARY_BACKGROUND_COLOR);
    EXPECT_EQ(textColorCall->values[0].u32, PRIMARY_TEXT_COLOR);
}

TEST_F(ButtonComponentTddTest, L0_should_not_register_click_event_when_action_is_invalid)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto descriptor = ParseJson(R"({"id":"invalidAction","component":"Button","action":{"event":{}}})");
    ASSERT_NE(descriptor, nullptr);

    button->ApplyDescriptor(descriptor->GetRoot());

    EXPECT_FALSE(HasRegisterNodeEventCall(button->GetNativeView(), NODE_ON_CLICK));
    const NativeAttributeCall* enabledCall = FindLastAttributeCall(button->GetNativeView(), NODE_ENABLED);
    ASSERT_NE(enabledCall, nullptr);
    ASSERT_FALSE(enabledCall->values.empty());
    EXPECT_EQ(enabledCall->values[0].i32, 1);
}
