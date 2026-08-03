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

#define private public

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedDividerComponent.h"
#include "styles/StyleApplyUtils.h"
#include "utils/DisplayDensityUtils.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Divider", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

class TestableExtendedDividerComponent : public ExtendedDividerComponent {
public:
    using ExtendedDividerComponent::ApplyComponentSpecificStyles;
    using ExtendedDividerComponent::GetPrivatePropertyDeclaration;
};

int32_t FindLastSetOperationIndex(ArkUI_NodeHandle node, int32_t attribute)
{
    for (int32_t index = static_cast<int32_t>(g_tracker.attributeOperations.size()) - 1; index >= 0; --index) {
        const NativeAttributeOperation& operation = g_tracker.attributeOperations[static_cast<size_t>(index)];
        if (!operation.isReset && operation.setCall.node == node && operation.setCall.attribute == attribute) {
            return index;
        }
    }
    return -1;
}

bool HasSetAttributeValue(ArkUI_NodeHandle node, int32_t attribute, float expected)
{
    for (const auto& call : g_tracker.attributeCalls) {
        if (call.node != node || call.attribute != attribute || call.values.empty()) {
            continue;
        }
        if (call.values[0].f32 == expected) {
            return true;
        }
    }
    return false;
}

class ExtendedDividerComponentTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        DisplayDensityUtils::GetInstance().densityByRenderId_.clear();
        slot_.SetSurfaceId("surface-extended-divider");
        slot_.SetRenderId(12);
    }

    void TearDown() override
    {
        DisplayDensityUtils::GetInstance().densityByRenderId_.clear();
        RenderManager::GetInstance().RemoveRenderSlot(slot_.GetRenderId());
        A2UIComponentTddTest::TearDown();
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedDividerComponentTest, L0_should_apply_default_divider_properties_when_styles_missing)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);

    EXPECT_EQ(divider->GetType(), "Divider");
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0x33000000u);
}

TEST_F(ExtendedDividerComponentTest, L0_should_create_divider_as_row_node)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);

    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node, FindCreatedNode(ARKUI_NODE_ROW));
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_dark_default_divider_color_when_styles_missing)
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
                "component": "Divider"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(divider->GetColorForTest(), 0x33FFFFFFu);
}

TEST_F(ExtendedDividerComponentTest, L0_should_update_default_divider_color_on_theme_mode_change)
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
                "component": "Divider"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(divider->GetColorForTest(), 0x33000000u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(divider->GetColorForTest(), 0x33FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(divider->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0x33FFFFFFu);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_default_color_on_direct_config_change_when_using_theme_default)
{
    TestableExtendedDividerComponent component;
    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;

    component.OnConfigChange(darkContext);

    EXPECT_EQ(component.GetColorForTest(), 0x33FFFFFFu);
    const NativeAttributeCall* call = FindLastAttributeCall(component.GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0x33FFFFFFu);
}

TEST_F(ExtendedDividerComponentTest, L0_should_parse_px_vp_fp_percent_and_numeric_stroke_width_values)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    const std::pair<std::string, std::string> cases[] = { { R"("1px")", "px" }, { R"("6vp")", "vp" },
        { R"("12fp")", "fp" }, { R"("25%")", "%" }, { "3", "vp" } };

    for (const auto& testCase : cases) {
        std::string json = R"({
            "components": [
                {
                    "id": "root",
                    "component": "Divider",
                    "styles": {
                        "strokeWidth": )" +
                           testCase.first + R"(,
                        "vertical": false,
                        "color": "#112233"
                    }
                }
            ]
        })";
        std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(json);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

        std::shared_ptr<ExtendedDividerComponent> divider =
            std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
        ASSERT_NE(divider, nullptr);
        EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), testCase.second);
        EXPECT_EQ(divider->GetColorForTest(), 0xFF112233u);
    }
}

TEST_F(ExtendedDividerComponentTest, L0_should_fallback_invalid_divider_style_values_to_defaults)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "8vp",
                    "vertical": true,
                    "color": "#AA112233"
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
                "component": "Divider",
                "styles": {
                    "strokeWidth": "-2vp",
                    "color": "invalidColor"
                }
            }
        ]
    })");
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(invalid->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0x33000000u);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_divider_geometry_after_state_update)
{
    TestableExtendedDividerComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle dividerNode = component.GetNativeView();

    PropertyDeclaration strokeWidthDeclaration = component.GetPrivatePropertyDeclaration("strokeWidth");
    PropertyDeclaration verticalDeclaration = component.GetPrivatePropertyDeclaration("vertical");
    PropertyDeclaration colorDeclaration = component.GetPrivatePropertyDeclaration("color");
    ASSERT_TRUE(static_cast<bool>(strokeWidthDeclaration.applyValue));
    ASSERT_TRUE(static_cast<bool>(verticalDeclaration.applyValue));
    ASSERT_TRUE(static_cast<bool>(colorDeclaration.applyValue));

    std::unique_ptr<JsonAdapter> percentStrokeWidth = JsonAdapter::Parse(R"("20%")");
    ASSERT_NE(percentStrokeWidth, nullptr);
    strokeWidthDeclaration.applyValue(percentStrokeWidth->GetRoot());
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_WIDTH_PERCENT), nullptr);
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_HEIGHT_PERCENT), nullptr);
    component.ApplyComponentSpecificStyles(JsonValue(), nodeAdapter);

    EXPECT_FALSE(HasResetAttributeCall(dividerNode, NODE_WIDTH));
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_WIDTH_PERCENT), nullptr);
    EXPECT_TRUE(HasResetAttributeCall(dividerNode, NODE_HEIGHT));
    ExpectF32Attribute(dividerNode, NODE_HEIGHT_PERCENT, 0.2F);

    ResetTracker();
    std::unique_ptr<JsonAdapter> verticalTrue = JsonAdapter::Parse("true");
    ASSERT_NE(verticalTrue, nullptr);
    verticalDeclaration.applyValue(verticalTrue->GetRoot());
    component.ApplyComponentSpecificStyles(JsonValue(), nodeAdapter);

    EXPECT_FALSE(HasResetAttributeCall(dividerNode, NODE_HEIGHT));
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_HEIGHT_PERCENT), nullptr);
    EXPECT_TRUE(HasResetAttributeCall(dividerNode, NODE_WIDTH));
    ExpectF32Attribute(dividerNode, NODE_WIDTH_PERCENT, 0.2F);

    std::unique_ptr<JsonAdapter> dividerColor = JsonAdapter::Parse(R"("#AA112233")");
    ASSERT_NE(dividerColor, nullptr);
    colorDeclaration.applyValue(dividerColor->GetRoot());

    ExpectU32Attribute(dividerNode, NODE_BACKGROUND_COLOR, 0xAA112233u);
}

TEST_F(ExtendedDividerComponentTest, L0_should_convert_px_stroke_width_to_vp_before_native_apply)
{
    TestableExtendedDividerComponent component;
    ArkUI_NodeHandle dividerNode = component.GetNativeView();

    PropertyDeclaration strokeWidthDeclaration = component.GetPrivatePropertyDeclaration("strokeWidth");
    ASSERT_TRUE(static_cast<bool>(strokeWidthDeclaration.applyValue));

    component.SetRenderId(0);
    DisplayDensityUtils::GetInstance().SetDisplayDensity(0, 2.0F);

    std::unique_ptr<JsonAdapter> pxStrokeWidth = JsonAdapter::Parse(R"("1px")");
    ASSERT_NE(pxStrokeWidth, nullptr);
    strokeWidthDeclaration.applyValue(pxStrokeWidth->GetRoot());
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    component.ApplyComponentSpecificStyles(JsonValue(), nodeAdapter);

    EXPECT_FALSE(HasResetAttributeCall(dividerNode, NODE_WIDTH));
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_WIDTH_PERCENT), nullptr);
    EXPECT_TRUE(HasResetAttributeCall(dividerNode, NODE_HEIGHT_PERCENT));
    ExpectF32Attribute(dividerNode, NODE_HEIGHT, 0.5F);
    EXPECT_EQ(FindLastAttributeCall(dividerNode, NODE_WIDTH), nullptr);
}

TEST_F(ExtendedDividerComponentTest, L0_should_return_empty_property_declaration_for_unknown_divider_property)
{
    TestableExtendedDividerComponent component;

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("unknown");

    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));
}

TEST_F(ExtendedDividerComponentTest, L0_should_parse_divider_stroke_width_px_and_reject_invalid_token)
{
    float strokeWidth = 0.0F;
    std::string unit;
    std::unique_ptr<JsonAdapter> pxValue = JsonAdapter::Parse(R"("1px")");
    ASSERT_NE(pxValue, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(pxValue->GetRoot(), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 1.0F);
    EXPECT_EQ(unit, "px");

    std::unique_ptr<JsonAdapter> invalidValue = JsonAdapter::Parse("false");
    ASSERT_NE(invalidValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseDividerStrokeWidth(invalidValue->GetRoot(), strokeWidth, unit));
}

TEST_F(ExtendedDividerComponentTest, L0_should_accept_fp_stroke_width_with_vertical_and_color_from_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "100fp",
                    "vertical": true,
                    "color": "#112233"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 100.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "fp");
    EXPECT_TRUE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedDividerComponentTest, L0_should_accept_numeric_vp_stroke_width_and_ignore_unknown_line_cap)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": 2,
                    "lineCap": "round",
                    "color": "#FF0000"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 2.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "vp");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0xFFFF0000u);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_divider_and_common_styles_on_single_node_horizontal)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "2vp",
                    "vertical": false,
                    "color": "#112233",
                    "width": 120
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);

    ExpectF32Attribute(node, NODE_WIDTH, 120.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 2.0F);
    ExpectU32Attribute(node, NODE_BACKGROUND_COLOR, 0xFF112233u);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 2.0F));
    EXPECT_LT(FindLastSetOperationIndex(node, NODE_WIDTH), FindLastSetOperationIndex(node, NODE_HEIGHT));
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_divider_and_common_styles_on_single_node_vertical)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "3vp",
                    "vertical": true,
                    "color": "#445566"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);

    ExpectF32Attribute(node, NODE_WIDTH, 3.0F);
    ExpectU32Attribute(node, NODE_BACKGROUND_COLOR, 0xFF445566u);
    EXPECT_EQ(FindLastAttributeCall(node, NODE_HEIGHT_PERCENT), nullptr);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_stroke_width_when_updated_styles_do_not_include_axis_field)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "8vp",
                    "vertical": true,
                    "width": 240,
                    "height": 120
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);
    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 8.0F));

    ResetTracker();
    std::unique_ptr<JsonAdapter> verticalFalse = JsonAdapter::Parse("false");
    ASSERT_NE(verticalFalse, nullptr);
    divider->OnDataUpdate("styles.vertical", verticalFalse->GetRoot());

    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 8.0F));
    ExpectF32Attribute(node, NODE_HEIGHT, 8.0F);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_horizontal_stroke_when_width_update_has_no_height_field)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "4vp",
                    "vertical": false,
                    "width": 240,
                    "height": 120
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);
    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_HEIGHT, 4.0F));

    std::unique_ptr<JsonAdapter> widthUpdate = JsonAdapter::Parse("300");
    ASSERT_NE(widthUpdate, nullptr);
    divider->OnDataUpdate("styles.width", widthUpdate->GetRoot());

    ExpectF32Attribute(node, NODE_WIDTH, 300.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 4.0F));
    ExpectF32Attribute(node, NODE_HEIGHT, 4.0F);
}

TEST_F(ExtendedDividerComponentTest, L0_should_skip_stroke_width_when_common_height_field_exists_for_horizontal)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "4vp",
                    "vertical": false,
                    "width": 240,
                    "height": "bad"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);

    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_HEIGHT, 4.0F));
}

TEST_F(ExtendedDividerComponentTest, L0_should_skip_stroke_width_when_common_width_field_exists_for_vertical)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "4vp",
                    "vertical": true,
                    "width": "bad",
                    "height": 120
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();
    ASSERT_NE(node, nullptr);

    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 4.0F));
}

TEST_F(ExtendedDividerComponentTest, L0_should_convert_px_stroke_width_using_render_id_density)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    DisplayDensityUtils::GetInstance().SetDisplayDensity(slot_.GetRenderId(), 3.0F);

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "6px",
                    "vertical": false,
                    "color": "#FF0000"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "px");

    ArkUI_NodeHandle node = divider->GetNativeView();
    ExpectF32Attribute(node, NODE_HEIGHT, 2.0F);
}

TEST_F(ExtendedDividerComponentTest, L0_should_fallback_px_to_raw_when_density_not_set)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    DisplayDensityUtils::GetInstance().densityByRenderId_.clear();

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "6px",
                    "vertical": false,
                    "color": "#FF0000"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);

    ArkUI_NodeHandle node = divider->GetNativeView();
    ExpectF32Attribute(node, NODE_HEIGHT, 6.0F);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_percent_stroke_width_for_vertical)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "50%",
                    "vertical": true,
                    "color": "#112233"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();

    ExpectF32Attribute(node, NODE_WIDTH_PERCENT, 0.5F);
    EXPECT_EQ(FindLastAttributeCall(node, NODE_HEIGHT_PERCENT), nullptr);
}

TEST_F(ExtendedDividerComponentTest, L0_should_apply_vp_stroke_width_and_common_styles_on_single_node)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "2vp",
                    "vertical": false,
                    "color": "#FF0000",
                    "width": 200
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    ArkUI_NodeHandle node = divider->GetNativeView();

    ExpectF32Attribute(node, NODE_WIDTH, 200.0F);
    ExpectU32Attribute(node, NODE_BACKGROUND_COLOR, 0xFFFF0000u);
    ExpectF32Attribute(node, NODE_HEIGHT, 2.0F);
}

TEST_F(ExtendedDividerComponentTest, L0_should_not_update_theme_color_when_explicit_color_set)
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
                "component": "Divider",
                "styles": {
                    "color": "#AABBCC"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(divider->GetColorForTest(), 0xFFAABBCCu);

    uint32_t colorBefore = divider->GetColorForTest();
    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(divider->GetColorForTest(), colorBefore);
}

} // namespace
