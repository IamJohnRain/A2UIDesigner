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

#include "components/A2UI/card/CardComponent.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class CardComponentTddTest : public A2UIComponentTddTest {};

namespace {

class CardComponentProbe : public CardComponent {
public:
    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }

    void InvokeOnConfigChange(const ThemeContext& context)
    {
        OnConfigChange(context);
    }
};

} // namespace

TEST_F(CardComponentTddTest, L0_card_should_create_column_node_and_report_type)
{
    auto card = std::make_shared<CardComponent>();
    ASSERT_NE(card, nullptr);

    EXPECT_EQ(card->GetType(), "Card");
    EXPECT_EQ(card->GetNativeView(), FindCreatedNode(ARKUI_NODE_COLUMN));
}

TEST_F(CardComponentTddTest, L0_card_should_apply_default_style_and_explicit_size)
{
    auto card = std::make_shared<CardComponent>();
    PrepareThemeContext(*card);

    auto descriptor = ParseJson(R"({"id":"card1","component":"Card","width":120,"height":64})");
    ASSERT_NE(descriptor, nullptr);

    card->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle cardNode = card->GetNativeView();
    ExpectF32Attribute(cardNode, NODE_BORDER_RADIUS, 8.0F);
    ExpectU32Attribute(cardNode, NODE_BACKGROUND_COLOR, 0xFFFFFFFFU);
    ExpectI32Attribute(cardNode, NODE_SHADOW, ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    ExpectU32Attribute(cardNode, NODE_BORDER_COLOR, 0xFFE0E0E0U);
    ExpectF32Attribute(cardNode, NODE_WIDTH, 120.0F);
    ExpectF32Attribute(cardNode, NODE_HEIGHT, 64.0F);

    const NativeAttributeCall* paddingCall = FindLastAttributeCall(cardNode, NODE_PADDING);
    ASSERT_NE(paddingCall, nullptr);
    ASSERT_EQ(paddingCall->values.size(), 4U);
    EXPECT_FLOAT_EQ(paddingCall->values[0].f32, 16.0F);
    EXPECT_FLOAT_EQ(paddingCall->values[1].f32, 16.0F);

    const NativeAttributeCall* borderWidthCall = FindLastAttributeCall(cardNode, NODE_BORDER_WIDTH);
    ASSERT_NE(borderWidthCall, nullptr);
    ASSERT_EQ(borderWidthCall->values.size(), 4U);
    EXPECT_FLOAT_EQ(borderWidthCall->values[0].f32, 1.0F);
    EXPECT_FLOAT_EQ(borderWidthCall->values[3].f32, 1.0F);
}

TEST_F(CardComponentTddTest, L0_card_should_apply_public_shadow_and_border_setters)
{
    auto card = std::make_shared<CardComponent>();

    card->SetShadow(12.0F, 0x66000000U, 1.0F, 2.0F);
    card->SetBorderWidth(1.0F, 2.0F, 3.0F, 4.0F);
    card->SetBorderColor(0xFFABCDEFU);

    const NativeAttributeCall* shadowCall = FindLastAttributeCall(card->GetNativeView(), NODE_SHADOW);
    ASSERT_NE(shadowCall, nullptr);
    ASSERT_EQ(shadowCall->values.size(), 4U);
    EXPECT_FLOAT_EQ(shadowCall->values[0].f32, 12.0F);
    EXPECT_EQ(shadowCall->values[1].u32, 0x66000000U);
    EXPECT_FLOAT_EQ(shadowCall->values[2].f32, 1.0F);
    EXPECT_FLOAT_EQ(shadowCall->values[3].f32, 2.0F);

    const NativeAttributeCall* borderWidthCall = FindLastAttributeCall(card->GetNativeView(), NODE_BORDER_WIDTH);
    ASSERT_NE(borderWidthCall, nullptr);
    ASSERT_EQ(borderWidthCall->values.size(), 4U);
    EXPECT_FLOAT_EQ(borderWidthCall->values[0].f32, 1.0F);
    EXPECT_FLOAT_EQ(borderWidthCall->values[3].f32, 4.0F);
    ExpectU32Attribute(card->GetNativeView(), NODE_BORDER_COLOR, 0xFFABCDEFU);
}

TEST_F(CardComponentTddTest, L0_card_should_collect_single_child_descriptor)
{
    CardComponentProbe card;
    auto descriptor = ParseJson(R"({"id":"card","component":"Card","child":"body"})");
    ASSERT_NE(descriptor, nullptr);

    card.InvokeCollectChildListDescriptor(descriptor->GetRoot());

    ASSERT_EQ(card.GetChildListDescriptor().staticChildIds.size(), 1U);
    EXPECT_EQ(card.GetChildListDescriptor().staticChildIds.front(), "body");
}

TEST_F(CardComponentTddTest, L0_card_should_ignore_config_change_when_theme_context_is_missing)
{
    CardComponentProbe card;
    ThemeContext context;

    card.InvokeOnConfigChange(context);

    EXPECT_EQ(card.GetTheme(), nullptr);
}

TEST_F(CardComponentTddTest, L0_card_should_return_theme_when_surface_context_is_available)
{
    auto card = std::make_shared<CardComponent>();
    PrepareThemeContext(*card);

    EXPECT_NE(card->GetTheme(), nullptr);
}
