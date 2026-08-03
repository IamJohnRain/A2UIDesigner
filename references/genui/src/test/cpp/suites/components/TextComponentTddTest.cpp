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

#include "components/A2UI/text/TextComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class TextComponentTddTest : public A2UIComponentTddTest {};

namespace {

class TextComponentProbe : public TextComponent {
public:
    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }
};

} // namespace

TEST_F(TextComponentTddTest, L0_text_should_create_text_node_and_report_type)
{
    auto text = std::make_shared<TextComponent>();
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetType(), "Text");
    EXPECT_EQ(text->GetNativeView(), FindCreatedNode(ARKUI_NODE_TEXT));
}

TEST_F(TextComponentTddTest, L0_text_should_apply_content_and_heading_variant)
{
    auto text = std::make_shared<TextComponent>();
    auto descriptor = ParseJson(R"({"id":"title","component":"Text","text":"Hello","variant":"h3"})");
    ASSERT_NE(descriptor, nullptr);

    text->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(text->GetNativeView(), NODE_TEXT_CONTENT, "Hello");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 24.0F);
}

TEST_F(TextComponentTddTest, L0_text_should_fallback_invalid_variant_to_body_size)
{
    auto text = std::make_shared<TextComponent>();
    auto descriptor = ParseJson(R"({"id":"body","component":"Text","text":"Body","variant":"invalid"})");
    ASSERT_NE(descriptor, nullptr);

    text->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(text->GetNativeView(), NODE_TEXT_CONTENT, "Body");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 16.0F);
}

TEST_F(TextComponentTddTest, L0_text_should_apply_each_public_text_style_setter)
{
    auto text = std::make_shared<TextComponent>();

    text->SetTextContent("Manual");
    text->SetFontColor(0xFF123456U);
    text->SetFontSize(18.0F);
    text->ResetFontColor();

    ExpectStringAttribute(text->GetNativeView(), NODE_TEXT_CONTENT, "Manual");
    ExpectU32Attribute(text->GetNativeView(), NODE_FONT_COLOR, 0xFF123456U);
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 18.0F);
    EXPECT_TRUE(HasResetAttributeCall(text->GetNativeView(), NODE_FONT_COLOR));
}

TEST_F(TextComponentTddTest, L0_text_should_resolve_all_supported_variant_sizes)
{
    auto text = std::make_shared<TextComponent>();

    text->SetVariant("h1");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 32.0F);
    text->SetVariant("h2");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 28.0F);
    text->SetVariant("h4");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 20.0F);
    text->SetVariant("h5");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 18.0F);
    text->SetVariant("caption");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 12.0F);
}

TEST_F(TextComponentTddTest, L0_text_should_ignore_child_descriptor_object_for_text_component)
{
    TextComponentProbe text;
    auto descriptor = ParseJson(R"({"id":"text","component":"Text","child":{"componentId":"ignored"}})");
    ASSERT_NE(descriptor, nullptr);

    text.InvokeCollectChildListDescriptor(descriptor->GetRoot());

    EXPECT_TRUE(text.GetChildListDescriptor().staticChildIds.empty());
}

TEST_F(TextComponentTddTest, L0_text_should_return_theme_when_surface_context_is_available)
{
    auto text = std::make_shared<TextComponent>();
    PrepareThemeContext(*text);

    EXPECT_NE(text->GetTheme(), nullptr);
}
