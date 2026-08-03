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
#include <limits>
#include <memory>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/RenderContext.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"
#define protected public
#define private public
#include "components/extended/ExtendedTextComponent.h"
#undef private
#undef protected
#include "styles/StyleApplyUtils.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

class TestableExtendedTextComponent : public ExtendedTextComponent {
public:
    using ExtendedTextComponent::ApplyComponentSpecificStyles;
    using ExtendedTextComponent::ApplyDecorationState;
    using ExtendedTextComponent::ApplyDefaultTextStyles;
    using ExtendedTextComponent::GetPrivatePropertyDeclaration;

    ArkUI_NodeHandle GetNativeViewHandleRaw() const
    {
        return nativeView_;
    }

    void SetNativeViewHandleRaw(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }

    ArkUI_NativeNodeAPI_1* GetNativeNodeApiRaw() const
    {
        return nativeNodeApi_;
    }

    void SetNativeNodeApiRaw(ArkUI_NativeNodeAPI_1* nativeNodeApi)
    {
        nativeNodeApi_ = nativeNodeApi;
    }
};

class ExtendedTextComponentTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        slot_.SetSurfaceId("surface-extended-text");
        slot_.SetRenderId(11);
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(slot_.GetRenderId());
        A2UIComponentTddTest::TearDown();
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedTextComponentTest, L0_should_capture_default_native_text_attributes_on_construction)
{
    TestableExtendedTextComponent component;

    EXPECT_EQ(component.GetType(), "Text");
    EXPECT_EQ(component.GetTextValueForTest(), "");
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    ExpectU32Attribute(component.GetNativeView(), NODE_FONT_COLOR, 0xE5000000u);
    ExpectF32Attribute(component.GetNativeView(), NODE_FONT_SIZE, 16.0F);
    ExpectI32Attribute(component.GetNativeView(), NODE_FONT_WEIGHT, ARKUI_FONT_WEIGHT_W400);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_ALIGN, 0);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_OVERFLOW, 1);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_WORD_BREAK, ARKUI_WORD_BREAK_BREAK_WORD);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_light_default_font_color_when_font_color_missing)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme light",
                "styles": {
                    "fontSize": 18
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_dark_default_font_color_when_font_color_missing)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme dark",
                "styles": {
                    "fontSize": 18
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_update_default_font_color_on_theme_mode_change)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    managedSlot.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme switch",
                "styles": {
                    "fontSize": 18
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(text->GetFontColorForTest(), 0x99FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(text->GetNativeView(), NODE_FONT_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_light_default_font_color_when_font_color_invalid)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme light invalid color",
                "styles": {
                    "fontColor": "invalid",
                    "fontSize": 18
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_dark_default_font_color_when_font_color_invalid)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme dark invalid color",
                "styles": {
                    "fontColor": "invalid",
                    "fontSize": 18
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_apply_private_and_text_style_properties_from_descriptor)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "extended text",
                "styles": {
                    "fontColor": "#FF112233",
                    "fontSize": "18",
                    "fontWeight": 700,
                    "maxLines": "2",
                    "minFontSize": "12",
                    "maxFontSize": 24,
                    "textOverflow": "ellipsis",
                    "textAlign": "center",
                    "wordBreak": "break_all",
                    "decoration": {
                        "type": "underline",
                        "color": "#ff007dff",
                        "style": "solid",
                        "thicknessScale": 1.5
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetTextValueForTest(), "extended text");
    EXPECT_EQ(text->GetFontColorForTest(), 0xFF112233u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 18.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(text->GetMaxLinesForTest(), 2);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), 12.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), 24.0F);
    EXPECT_EQ(text->GetTextOverflowForTest(), 2);
    EXPECT_EQ(text->GetTextAlignForTest(), 1);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_ALL);

    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF007DFFu);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.5F);

    ExpectStringAttribute(text->GetNativeView(), NODE_TEXT_CONTENT, "extended text");
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(ExtendedTextComponentTest, L0_should_resolve_literal_expressions_for_extended_text_content_and_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "{{ 'Hello' + ', ' + 'World' }}",
                "styles": {
                    "fontSize": "{{ 8 + 10 }}"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetTextValueForTest(), "Hello, World");
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 18.0F);
    ExpectStringAttribute(text->GetNativeView(), NODE_TEXT_CONTENT, "Hello, World");
    ExpectF32Attribute(text->GetNativeView(), NODE_FONT_SIZE, 18.0F);
}
#endif

TEST_F(ExtendedTextComponentTest, L0_should_preserve_existing_text_style_state_on_style_delta_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "delta update",
                "styles": {
                    "fontColor": "#FF112233",
                    "fontSize": 20,
                    "maxLines": 3,
                    "minFontSize": 10,
                    "maxFontSize": 28,
                    "wordBreak": "breakWord"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    std::unique_ptr<JsonAdapter> invalidWordBreak = JsonAdapter::Parse("true");
    ASSERT_NE(invalidWordBreak, nullptr);
    text->OnDataUpdate(StyleResolver::BuildStyleBindingProperty("wordBreak"), invalidWordBreak->GetRoot());

    EXPECT_EQ(text->GetFontColorForTest(), 0xFF112233u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 20.0F);
    EXPECT_EQ(text->GetMaxLinesForTest(), 3);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), 10.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), 28.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    std::unique_ptr<JsonAdapter> newWordBreak = JsonAdapter::Parse(R"("hyphenation")");
    ASSERT_NE(newWordBreak, nullptr);
    text->OnDataUpdate(StyleResolver::BuildStyleBindingProperty("wordBreak"), newWordBreak->GetRoot());
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_HYPHENATION);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 20.0F);
    EXPECT_EQ(text->GetMaxLinesForTest(), 3);
}

TEST_F(ExtendedTextComponentTest, L0_should_reset_invalid_full_text_style_updates_to_defaults)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "before reset",
                "styles": {
                    "fontColor": "#FF334455",
                    "fontSize": 19,
                    "fontWeight": 700,
                    "maxLines": 2,
                    "minFontSize": 12,
                    "maxFontSize": 30,
                    "textOverflow": "ellipsis",
                    "textAlign": "end",
                    "wordBreak": "breakAll",
                    "decoration": {
                        "type": "linethrough",
                        "style": "wavy",
                        "thicknessScale": 2
                    }
                }
            }
        ]
    })");
    ASSERT_NE(initial, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initial->GetRoot()));

    std::unique_ptr<JsonAdapter> invalid = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "after reset",
                "styles": {
                    "fontColor": "broken",
                    "fontSize": 0,
                    "fontWeight": "unsupported",
                    "maxLines": -1,
                    "minFontSize": 0,
                    "maxFontSize": -2,
                    "textOverflow": "bad",
                    "textAlign": "bad",
                    "wordBreak": true,
                    "decoration": {
                        "type": "unsupported"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(invalid->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetTextValueForTest(), "after reset");
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(text->GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetTextOverflowForTest(), 1);
    EXPECT_EQ(text->GetTextAlignForTest(), 0);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF000000u);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);
}

TEST_F(ExtendedTextComponentTest, L0_should_apply_default_text_styles_via_applier_when_initialized)
{
    TestableExtendedTextComponent component;
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Text",
        "content": "with-applier",
        "styles": {}
    })");
    ASSERT_NE(descriptor, nullptr);

    RenderContext context;
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    component.ApplyDefaultTextStyles();

    ExpectU32Attribute(component.GetNativeView(), NODE_FONT_COLOR, 0xE5000000u);
    ExpectF32Attribute(component.GetNativeView(), NODE_FONT_SIZE, 16.0F);
    ExpectI32Attribute(component.GetNativeView(), NODE_FONT_WEIGHT, ARKUI_FONT_WEIGHT_W400);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_ALIGN, 0);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_OVERFLOW, 1);
    ExpectI32Attribute(component.GetNativeView(), NODE_TEXT_WORD_BREAK, ARKUI_WORD_BREAK_BREAK_WORD);
    ExpectStringAttribute(component.GetNativeView(), NODE_TEXT_CONTENT, "with-applier");
}

TEST_F(ExtendedTextComponentTest, L0_should_return_empty_property_declaration_for_unknown_text_property)
{
    TestableExtendedTextComponent component;

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("unknown");

    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));
}

TEST_F(ExtendedTextComponentTest, L0_should_parse_word_break_aliases_and_reject_non_string_values)
{
    int32_t wordBreak = -1;
    std::unique_ptr<JsonAdapter> aliasValue = JsonAdapter::Parse(R"("break_word")");
    ASSERT_NE(aliasValue, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseWordBreak(aliasValue->GetRoot(), wordBreak));
    EXPECT_EQ(wordBreak, ARKUI_WORD_BREAK_BREAK_WORD);

    std::unique_ptr<JsonAdapter> invalidValue = JsonAdapter::Parse("false");
    ASSERT_NE(invalidValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseWordBreak(invalidValue->GetRoot(), wordBreak));
}

TEST_F(ExtendedTextComponentTest, L0_should_route_extended_protocol_text_to_extended_component)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "fontSize": 16,
                    "fontWeight": 700,
                    "maxLines": 2,
                    "textOverflow": "ellipsis",
                    "fontColor": "#FF112233",
                    "decoration": {
                        "type": "underline",
                        "color": "#ff007dff",
                        "style": "solid",
                        "thicknessScale": 1.5
                    },
                    "textAlign": "center"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Text");
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedTextComponent>(root), nullptr);
}

TEST_F(ExtendedTextComponentTest, L0_should_parse_supported_text_align_and_text_overflow_values_from_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "maxLines": 1,
                    "textOverflow": "marquee",
                    "textAlign": "end"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetMaxLinesForTest(), 1);
    EXPECT_EQ(text->GetTextOverflowForTest(), 3);
    EXPECT_EQ(text->GetTextAlignForTest(), 2);
}

TEST_F(ExtendedTextComponentTest, L0_should_accept_zero_text_max_lines_without_fallback)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "zero max lines",
                "styles": {
                    "maxLines": 0
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetMaxLinesForTest(), 0);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_default_text_state_when_values_are_outside_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "fontColor": "#FF112233",
                "fontSize": 18,
                "fontWeight": 700,
                "maxLines": 2,
                "minFontSize": 12,
                "maxFontSize": 24,
                "wordBreak": "breakWord",
                "textOverflow": "ellipsis",
                "decoration": {
                    "type": "underline",
                    "color": "#ff007dff",
                    "style": "solid",
                    "thicknessScale": 1.5
                },
                "textAlign": "center"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetTextValueForTest(), "");
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(text->GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetTextOverflowForTest(), 1);
    EXPECT_EQ(text->GetTextAlignForTest(), 0);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_FALSE(decoration.hasColor);
    EXPECT_FALSE(decoration.hasStyle);
    EXPECT_FALSE(decoration.hasThicknessScale);
}

TEST_F(ExtendedTextComponentTest, L0_should_accept_common_extended_styles_and_reset_them_on_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "flexShrink": 0.5,
                    "backgroundImage": "https://example.com/image.png",
                    "clip": true
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_GT(CountAttributeCall(text->GetNativeView(), NODE_FLEX_SHRINK), 0);
    EXPECT_GT(CountAttributeCall(text->GetNativeView(), NODE_BACKGROUND_IMAGE), 0);
    EXPECT_GT(CountAttributeCall(text->GetNativeView(), NODE_CLIP), 0);

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello2",
                "styles": {
                    "flexShrink": -0.3,
                    "backgroundImage": "",
                    "clip": false
                }
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_TRUE(HasResetAttributeCall(text->GetNativeView(), NODE_BACKGROUND_IMAGE));
}

TEST_F(ExtendedTextComponentTest, L0_should_parse_text_min_max_font_aliases_and_normalized_word_break)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "minFontSize": " 10 ",
                    "maxFontSize": "26",
                    "wordBreak": "  HYPHENATION  "
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), 10.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), 26.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_HYPHENATION);
}

TEST_F(ExtendedTextComponentTest, L0_should_ignore_invalid_text_min_max_font_and_word_break_inputs)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "minFontSize": 0,
                    "maxFontSize": -3,
                    "wordBreak": true
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
}

TEST_F(ExtendedTextComponentTest, L0_should_update_text_style_binding_from_data_model)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "fontSize": {
                        "path": "/size"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"value":{"size":24}})");
    ASSERT_NE(data, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(data->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 24.0F);
}

TEST_F(ExtendedTextComponentTest, L0_should_update_multiple_text_style_bindings_from_data_model)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "fontColor": {
                        "path": "/color"
                    },
                    "fontWeight": {
                        "path": "/weight"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"value":{"color":"#FF0000","weight":700}})");
    ASSERT_NE(data, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(data->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
}

TEST_F(ExtendedTextComponentTest, L0_should_route_bound_content_updates_to_text_property)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": {
                    "path": "/textContent"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"value":{"textContent":"updated"}})");
    ASSERT_NE(data, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(data->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetTextValueForTest(), "updated");
}

TEST_F(ExtendedTextComponentTest, L0_should_apply_text_decoration_style_payloads)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "decoration": {
                        "type": "underline",
                        "color": "#FF0000",
                        "style": "solid"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFFFF0000u);
    EXPECT_TRUE(decoration.hasStyle);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_light_default_decoration_color_when_color_missing)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "decoration": {
                        "type": "underline"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF000000u);

    const NativeAttributeCall* call = FindLastAttributeCall(text->GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(call, nullptr);
    ASSERT_GE(call->values.size(), 2u);
    EXPECT_EQ(call->values[0].i32, 1);
    EXPECT_EQ(call->values[1].u32, 0xFF000000u);
}

TEST_F(ExtendedTextComponentTest, L0_should_fallback_to_dark_default_decoration_color_when_color_missing)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "decoration": {
                        "type": "underline"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0x99FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(text->GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(call, nullptr);
    ASSERT_GE(call->values.size(), 2u);
    EXPECT_EQ(call->values[0].i32, 1);
    EXPECT_EQ(call->values[1].u32, 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_update_default_decoration_color_on_theme_mode_change)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    managedSlot.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "decoration switch",
                "styles": {
                    "decoration": {
                        "type": "underline"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetDecorationForTest().color, 0xFF000000u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0x99FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(text->GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(call, nullptr);
    ASSERT_GE(call->values.size(), 2u);
    EXPECT_EQ(call->values[1].u32, 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_cover_text_decoration_value_slot_branches)
{
    TestableExtendedTextComponent component;
    component.nodeApplier_ = CreateSharedNodeApiAdapter(component);

    TextDecorationState styleOnly;
    styleOnly.type = 1;
    styleOnly.hasStyle = true;
    styleOnly.style = 0;
    component.ApplyDecorationState(styleOnly);
    const NativeAttributeCall* styleCall = FindLastAttributeCall(component.GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(styleCall, nullptr);
    ASSERT_GE(styleCall->values.size(), 3u);
    EXPECT_EQ(styleCall->values[0].i32, 1);
    EXPECT_EQ(styleCall->values[1].u32, 0u);
    EXPECT_EQ(styleCall->values[2].i32, 0);

    TextDecorationState thicknessOnly;
    thicknessOnly.type = 1;
    thicknessOnly.hasThicknessScale = true;
    thicknessOnly.thicknessScale = 1.25F;
    component.ApplyDecorationState(thicknessOnly);
    const NativeAttributeCall* thicknessCall = FindLastAttributeCall(component.GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(thicknessCall, nullptr);
    ASSERT_GE(thicknessCall->values.size(), 4u);
    EXPECT_EQ(thicknessCall->values[1].u32, 0u);

    TextDecorationState typeOnly;
    typeOnly.type = 1;
    component.ApplyDecorationState(typeOnly);
    const NativeAttributeCall* typeOnlyCall = FindLastAttributeCall(component.GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(typeOnlyCall, nullptr);
    ASSERT_EQ(typeOnlyCall->values.size(), 1u);
    EXPECT_EQ(typeOnlyCall->values[0].i32, 1);
}

TEST_F(ExtendedTextComponentTest, L0_should_cover_text_decoration_apply_paths_for_applier_and_native_fallback)
{
    TestableExtendedTextComponent component;
    TextDecorationState decoration;
    decoration.type = 1;
    decoration.hasColor = true;
    decoration.color = 0xFF556677u;

    component.nodeApplier_ = CreateSharedNodeApiAdapter(component);
    ArkUI_NodeHandle nativeView = component.GetNativeViewHandleRaw();

    component.SetNativeViewHandleRaw(nullptr);
    component.ApplyDecorationState(decoration);
    component.SetNativeViewHandleRaw(nativeView);

    std::shared_ptr<ArkUINodeApiAdapter> savedApplier = component.nodeApplier_;
    component.nodeApplier_.reset();

    component.SetNativeViewHandleRaw(nullptr);
    component.ApplyDecorationState(decoration);
    component.SetNativeViewHandleRaw(nativeView);

    ArkUI_NativeNodeAPI_1* nativeNodeApi = component.GetNativeNodeApiRaw();
    component.SetNativeNodeApiRaw(nullptr);
    component.ApplyDecorationState(decoration);
    component.SetNativeNodeApiRaw(nativeNodeApi);

    component.ApplyDecorationState(decoration);
    component.nodeApplier_ = savedApplier;

    const NativeAttributeCall* nativeCall = FindLastAttributeCall(component.GetNativeView(), NODE_TEXT_DECORATION);
    ASSERT_NE(nativeCall, nullptr);
    ASSERT_GE(nativeCall->values.size(), 2u);
    EXPECT_EQ(nativeCall->values[0].i32, 1);
    EXPECT_EQ(nativeCall->values[1].u32, 0xFF556677u);
}

TEST_F(ExtendedTextComponentTest, L0_should_clamp_text_font_scale_values_and_reset_non_finite_inputs)
{
    TestableExtendedTextComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> negativeStyles = JsonAdapter::Parse(R"({
        "fontScaleMode": "custom",
        "minFontScale": -0.5,
        "maxFontScale": 0.5
    })");
    ASSERT_NE(negativeStyles, nullptr);
    component.ApplyComponentSpecificStyles(negativeStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 1.0F);

    std::unique_ptr<JsonAdapter> normalStyles = JsonAdapter::Parse(R"({
        "fontScaleMode": "custom",
        "minFontScale": 0.6,
        "maxFontScale": 2.0
    })");
    ASSERT_NE(normalStyles, nullptr);
    component.ApplyComponentSpecificStyles(normalStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.6F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 2.0F);

    std::unique_ptr<JsonAdapter> nonFiniteStyles = JsonAdapter::CreateObject();
    ASSERT_NE(nonFiniteStyles, nullptr);
    JsonValue nonFiniteRoot = nonFiniteStyles->GetRoot();
    ASSERT_TRUE(nonFiniteRoot.PutString("fontScaleMode", "custom"));
    ASSERT_TRUE(nonFiniteRoot.PutNumber("minFontScale", std::numeric_limits<double>::quiet_NaN()));
    ASSERT_TRUE(nonFiniteRoot.PutNumber("maxFontScale", std::numeric_limits<double>::quiet_NaN()));
    component.ApplyComponentSpecificStyles(nonFiniteRoot, applier);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 0.0F);
}

TEST_F(ExtendedTextComponentTest, L0_should_not_override_custom_font_color_on_config_change)
{
    TestableExtendedTextComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "fontColor": "#FF123456"
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);
    EXPECT_EQ(component.GetFontColorForTest(), 0xFF123456u);

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    component.OnConfigChange(darkContext);

    EXPECT_EQ(component.GetFontColorForTest(), 0xFF123456u);
}

TEST_F(ExtendedTextComponentTest, L0_should_update_default_decoration_color_on_direct_config_change)
{
    TestableExtendedTextComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "underline"
        }
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);
    EXPECT_EQ(component.GetDecorationForTest().color, 0xFF000000u);

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    component.OnConfigChange(darkContext);

    TextDecorationState decoration = component.GetDecorationForTest();
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0x99FFFFFFu);
}

TEST_F(ExtendedTextComponentTest, L0_should_ignore_invalid_text_decoration_style_payload)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "decoration": "invalid"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF000000u);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);
}

TEST_F(ExtendedTextComponentTest, L0_should_apply_text_style_alias_and_warning_smoke_cases)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> first = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "fontWeight": 700,
                    "textAlign": "center",
                    "backgroundImageSize": "cover",
                    "backgroundimageSize": "contain",
                    "clip": true
                }
            }
        ]
    })");
    ASSERT_NE(first, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(first->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(text->GetTextAlignForTest(), 1);

    std::unique_ptr<JsonAdapter> second = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "flexShrink": "bad",
                    "layoutWeight": "bad",
                    "clip": "bad",
                    "opacity": "bad"
                }
            }
        ]
    })");
    ASSERT_NE(second, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(second->GetRoot()));
}

TEST_F(ExtendedTextComponentTest, L0_should_reset_text_common_and_font_styles_when_styles_are_removed)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "fontSize": 24,
                    "backgroundColor": "#FF0000",
                    "borderWidth": 2,
                    "borderColor": "#00FF00",
                    "width": 100,
                    "height": 200,
                    "padding": 10,
                    "margin": 5,
                    "borderRadius": 8,
                    "fontWeight": 700,
                    "textAlign": "center",
                    "maxLines": 3,
                    "textOverflow": "ellipsis",
                    "visibility": "visible",
                    "opacity": 0.8,
                    "layoutWeight": 1,
                    "flexShrink": 0.5,
                    "constraintSize": {
                        "minWidth": 10,
                        "maxWidth": 200,
                        "minHeight": 10,
                        "maxHeight": 200
                    },
                    "fontColor": "#FF0000",
                    "minFontSize": 10,
                    "maxFontSize": 20,
                    "wordBreak": "breakAll",
                    "decoration": {
                        "type": "underline"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(initial, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initial->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> originalText =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(originalText, nullptr);
    ArkUI_NodeHandle originalView = originalText->GetNativeView();

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test"
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    ArkUI_NodeHandle currentView = text->GetNativeView();
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(text->GetTextAlignForTest(), 0);
    EXPECT_EQ(text->GetTextOverflowForTest(), 1);
    EXPECT_EQ(text->GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    if (currentView == originalView) {
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_FONT_SIZE));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_FONT_COLOR));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_FONT_WEIGHT));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_ALIGN));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_OVERFLOW));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_MIN_FONT_SIZE));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_MAX_FONT_SIZE));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_DECORATION));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_TEXT_WORD_BREAK));
    } else {
        EXPECT_NE(currentView, originalView);
    }
}

TEST_F(ExtendedTextComponentTest, L0_should_reset_text_visual_styles_when_styles_are_removed)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test",
                "styles": {
                    "shadow": {
                        "style": 1
                    },
                    "backgroundImage": "https://example.com/bg.png",
                    "linearGradient": {
                        "colors": ["#FF0000", "#00FF00"],
                        "stops": [0, 1]
                    },
                    "clip": true
                }
            }
        ]
    })");
    ASSERT_NE(initial, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initial->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> originalText =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(originalText, nullptr);
    ArkUI_NodeHandle originalView = originalText->GetNativeView();

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "test"
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    ArkUI_NodeHandle currentView = text->GetNativeView();
    if (currentView == originalView) {
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_SHADOW));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_CUSTOM_SHADOW));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_BACKGROUND_IMAGE));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_LINEAR_GRADIENT));
        EXPECT_TRUE(HasResetAttributeCall(currentView, NODE_CLIP));
    } else {
        EXPECT_NE(currentView, originalView);
    }
}

} // namespace
