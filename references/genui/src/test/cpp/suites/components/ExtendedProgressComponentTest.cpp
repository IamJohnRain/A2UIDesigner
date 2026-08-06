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

#include "components/extended/ExtendedProgressComponent.h"

#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "ArkUINodeApiAdapter.h"
#include "RenderManager.h"
#include "SchemaWarningTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Progress", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

class TestableExtendedProgressComponent : public ExtendedProgressComponent {
public:
    using ExtendedProgressComponent::ApplyComponentSpecificStyles;
    using ExtendedProgressComponent::ApplyPrivateAttributes;
    using ExtendedProgressComponent::GetPrivatePropertyDeclaration;
    using ExtendedProgressComponent::OnDataUpdate;
    using ExtendedProgressComponent::SetApplyingStyleDeltaUpdateForTest;
};

class ExtendedProgressComponentTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        slot_.SetSurfaceId("surface-extended-progress");
        slot_.SetRenderId(13);
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(slot_.GetRenderId());
        A2UIComponentTddTest::TearDown();
    }

    SurfaceSlot slot_;
};

class ExtendedProgressComponentSchemaWarningTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        callbacks_ = TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    }

    TestHelpers::DispatchCallbacks callbacks_;
};

TEST_F(ExtendedProgressComponentTest, L0_should_capture_default_progress_attributes_on_construction)
{
    TestableExtendedProgressComponent component;

    EXPECT_EQ(component.GetType(), "Progress");
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);

    ExpectF32Attribute(component.GetNativeView(), NODE_PROGRESS_VALUE, 0.0F);
    ExpectF32Attribute(component.GetNativeView(), NODE_PROGRESS_TOTAL, 100.0F);
    ExpectU32Attribute(component.GetNativeView(), NODE_PROGRESS_COLOR, 0xFF0A59F7u);
    ExpectI32Attribute(component.GetNativeView(), NODE_PROGRESS_TYPE, 0);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_dark_default_color_for_linear_progress_when_style_color_missing)
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
                "component": "Progress",
                "styles": {
                    "type": "linear"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 0);
}

TEST_F(ExtendedProgressComponentTest, L0_should_update_default_linear_progress_color_on_theme_mode_change)
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
                "component": "Progress"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF0A59F7u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF317AF7u);

    const NativeAttributeCall* call = FindLastAttributeCall(progress->GetNativeView(), NODE_PROGRESS_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0xFF317AF7u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_theme_default_color_for_eclipse_progress_when_style_color_missing)
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
                "component": "Progress",
                "styles": {
                    "type": "eclipse"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 2);
    EXPECT_EQ(progress->GetColorForTest(), 0x19000000u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_update_default_eclipse_progress_color_on_theme_mode_change)
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
                "component": "Progress",
                "styles": {
                    "type": "eclipse"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetColorForTest(), 0x19000000u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(progress->GetColorForTest(), 0x19FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(progress->GetNativeView(), NODE_PROGRESS_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0x19FFFFFFu);
}

TEST_F(
    ExtendedProgressComponentTest, L0_should_apply_theme_default_color_for_scale_ring_progress_when_style_color_missing)
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
                "component": "Progress",
                "styles": {
                    "type": "scaleRing"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 3);
    EXPECT_EQ(progress->GetColorForTest(), 0x99000000u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_update_default_scale_ring_progress_color_on_theme_mode_change)
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
                "component": "Progress",
                "styles": {
                    "type": "scaleRing"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->GetColorForTest(), 0x99000000u);

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(progress->GetColorForTest(), 0x99FFFFFFu);

    const NativeAttributeCall* call = FindLastAttributeCall(progress->GetNativeView(), NODE_PROGRESS_COLOR);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, 0x99FFFFFFu);
}

TEST_F(ExtendedProgressComponentTest, L0_should_create_progress_via_factory_and_apply_descriptor)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Progress"));
    std::shared_ptr<ExtendedComponent> created = factory.CreateComponent("Extended.Progress");
    ASSERT_NE(created, nullptr);
    EXPECT_EQ(created->GetType(), "Progress");

    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": 64,
                "total": 200,
                "styles": {
                    "color": "#FF112233",
                    "type": "ring"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 64.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 200.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF112233u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 1);
}

TEST_F(ExtendedProgressComponentTest, L0_should_bind_progress_value_and_total_to_data_model_paths)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog());

    std::unique_ptr<JsonAdapter> model = JsonAdapter::Parse(R"({
        "value": {
            "progress": {
                "value": 25,
                "total": 80
            }
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(model->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": {
                    "path": "/progress/value"
                },
                "total": {
                    "path": "/progress/total"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 25.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 80.0F);
    ASSERT_EQ(progress->GetDataBindings().size(), 2U);

    std::unique_ptr<JsonAdapter> valueUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/value",
        "value": 48
    })");
    ASSERT_NE(valueUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(valueUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 48.0F);

    std::unique_ptr<JsonAdapter> totalUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/total",
        "value": 120
    })");
    ASSERT_NE(totalUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(totalUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 120.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_resolve_progress_value_and_total_expression_bindings)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog());

    std::unique_ptr<JsonAdapter> model = JsonAdapter::Parse(R"({
        "value": {
            "progress": {
                "done": 20,
                "extra": 5,
                "total": 50
            }
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(model->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": "{{ $__dataModel.progress.done + $__dataModel.progress.extra }}",
                "total": "{{ $__dataModel.progress.total * 2 }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 25.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> extraUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/extra",
        "value": 15
    })");
    ASSERT_NE(extraUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(extraUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 35.0F);

    std::unique_ptr<JsonAdapter> totalUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/total",
        "value": 60
    })");
    ASSERT_NE(totalUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(totalUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 120.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_normalize_progress_number_expression_when_result_is_bool)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": "{{ true }}",
                "total": "{{ true }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 1.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 1.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_normalize_progress_number_path_binding_when_result_is_bool)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(slot_.GetRenderId());
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(slot_.GetSurfaceId(), nullptr);
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog());

    std::unique_ptr<JsonAdapter> model = JsonAdapter::Parse(R"({
        "value": {
            "progress": {
                "value": true,
                "total": true
            }
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(model->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": {
                    "path": "/progress/value"
                },
                "total": {
                    "path": "/progress/total"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(managedSlot.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 1.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 1.0F);
    ASSERT_EQ(progress->GetDataBindings().size(), 2U);

    std::unique_ptr<JsonAdapter> valueUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/value",
        "value": 48
    })");
    ASSERT_NE(valueUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(valueUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 48.0F);

    std::unique_ptr<JsonAdapter> totalUpdate = JsonAdapter::Parse(R"({
        "path": "/progress/total",
        "value": 120
    })");
    ASSERT_NE(totalUpdate, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(totalUpdate->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 120.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_fallback_invalid_progress_values_to_defaults_on_full_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": 32,
                "total": 64,
                "styles": {
                    "color": "#AA112233",
                    "type": "capsule"
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
                "component": "Progress",
                "value": -5,
                "total": 0,
                "styles": {
                    "color": "invalidColor",
                    "type": "broken"
                }
            }
        ]
    })");
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(invalid->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 100.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF0A59F7u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 0);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_progress_attributes_and_styles_via_native_api_without_applier)
{
    TestableExtendedProgressComponent component;
    ArkUI_NodeHandle node = component.GetNativeView();

    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "value": 25,
        "total": 40
    })");
    ASSERT_NE(descriptor, nullptr);
    component.ApplyPrivateAttributes(descriptor->GetRoot());

    EXPECT_FLOAT_EQ(component.GetValueForTest(), 25.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 40.0F);
    ExpectF32Attribute(node, NODE_PROGRESS_VALUE, 25.0F);
    ExpectF32Attribute(node, NODE_PROGRESS_TOTAL, 40.0F);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "color": "#AA334455",
        "type": "scale_ring"
    })");
    ASSERT_NE(styles, nullptr);
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);
    EXPECT_EQ(component.GetProgressTypeForTest(), 3);
    ExpectU32Attribute(node, NODE_PROGRESS_COLOR, 0xAA334455u);
    ExpectI32Attribute(node, NODE_PROGRESS_TYPE, 3);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest, L0_should_fallback_progress_color_number_to_default_color)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1005);
    component.SetSurfaceId("surface-progress-color-number-warning");
    component.SetComponentId("root");
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "color": 2
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.color"), 1U);
}

TEST_F(ExtendedProgressComponentTest, L0_should_ignore_missing_progress_style_fields_and_unknown_property)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> emptyStyles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(emptyStyles, nullptr);

    component.ApplyComponentSpecificStyles(emptyStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("unknown");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));
}

TEST_F(ExtendedProgressComponentTest, L0_should_skip_progress_color_update_when_theme_color_unchanged)
{
    TestableExtendedProgressComponent component;
    ThemeContext lightContext;
    lightContext.colorMode = ThemeMode::LIGHT;

    component.OnConfigChange(lightContext);

    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_keep_explicit_progress_color_when_type_changes)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> colorStyles = JsonAdapter::Parse(R"({
        "color": "#AA334455"
    })");
    ASSERT_NE(colorStyles, nullptr);
    component.ApplyComponentSpecificStyles(colorStyles->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> typeStyles = JsonAdapter::Parse(R"({
        "type": "eclipse"
    })");
    ASSERT_NE(typeStyles, nullptr);
    component.ApplyComponentSpecificStyles(typeStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetProgressTypeForTest(), 2);
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_progress_color_style_binding_updates_as_string_hex_only)
{
    TestableExtendedProgressComponent component;
    RenderContext context =
        RenderContext::Create(13, "surface-progress-binding", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "color": "#AA334455"
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);

    std::unique_ptr<JsonAdapter> stringColorDelta = JsonAdapter::Parse(R"("#FF112233")");
    ASSERT_NE(stringColorDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("color"), stringColorDelta->GetRoot());
    EXPECT_EQ(component.GetColorForTest(), 0xFF112233u);

    std::unique_ptr<JsonAdapter> numericColorDelta = JsonAdapter::Parse("2026");
    ASSERT_NE(numericColorDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("color"), numericColorDelta->GetRoot());
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_resolve_progress_color_path_binding_to_string_hex)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> model = JsonAdapter::Parse(R"({
        "value": {
            "progressColor": "#AA102030"
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(model->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "styles": {
                    "color": {
                        "path": "/progressColor"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_EQ(progress->GetColorForTest(), 0xAA102030u);
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(ExtendedProgressComponentTest, L0_should_resolve_progress_color_expression_to_string_hex)
{
    TestableExtendedProgressComponent component;
    RenderContext context =
        RenderContext::Create(13, "surface-progress-expression", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "color": "{{ '#CC102030' }}"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    EXPECT_EQ(component.GetColorForTest(), 0xCC102030u);
}
#endif

TEST_F(ExtendedProgressComponentTest, L0_should_restore_default_progress_color_when_type_invalid_and_color_is_default)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> eclipseTypeStyles = JsonAdapter::Parse(R"({
        "type": "eclipse"
    })");
    ASSERT_NE(eclipseTypeStyles, nullptr);
    component.ApplyComponentSpecificStyles(eclipseTypeStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 2);
    EXPECT_EQ(component.GetColorForTest(), 0x19000000u);

    std::unique_ptr<JsonAdapter> invalidTypeStyles = JsonAdapter::Parse(R"({
        "type": "invalid-type"
    })");
    ASSERT_NE(invalidTypeStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidTypeStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
}

TEST_F(ExtendedProgressComponentTest, L0_should_parse_progress_type_aliases_and_reject_non_string_values)
{
    int32_t progressType = -1;
    std::unique_ptr<JsonAdapter> aliasValue = JsonAdapter::Parse(R"("scale_ring")");
    ASSERT_NE(aliasValue, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseProgressType(aliasValue->GetRoot(), progressType));
    EXPECT_EQ(progressType, 3);

    std::unique_ptr<JsonAdapter> capsuleValue = JsonAdapter::Parse(R"("capsule")");
    ASSERT_NE(capsuleValue, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseProgressType(capsuleValue->GetRoot(), progressType));
    EXPECT_EQ(progressType, 4);

    std::unique_ptr<JsonAdapter> invalidValue = JsonAdapter::Parse("false");
    ASSERT_NE(invalidValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseProgressType(invalidValue->GetRoot(), progressType));
}

TEST_F(ExtendedProgressComponentTest, L0_should_cover_progress_private_attribute_fallback_shapes_and_style_fallbacks)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> objectPayload = JsonAdapter::Parse(R"({
        "value": {},
        "total": {}
    })");
    ASSERT_NE(objectPayload, nullptr);
    component.ApplyPrivateAttributes(objectPayload->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> invalidScalarPayload = JsonAdapter::Parse(R"({
        "value": "bad",
        "total": "bad"
    })");
    ASSERT_NE(invalidScalarPayload, nullptr);
    component.ApplyPrivateAttributes(invalidScalarPayload->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "type": "ring",
        "color": "#AA334455"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 1);
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStyles(nonObjectStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);

    component.SetApplyingStyleDeltaUpdateForTest(false);
    component.ApplyComponentSpecificStyles(nonObjectStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);

    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    std::unique_ptr<JsonAdapter> invalidTypeStyles = JsonAdapter::Parse(R"({
        "type": "unsupported"
    })");
    ASSERT_NE(invalidTypeStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidTypeStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);
}

TEST_F(
    ExtendedProgressComponentTest, L0_should_cover_progress_object_total_fallback_clamp_and_explicit_color_theme_skip)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> explicitColorStyles = JsonAdapter::Parse(R"({
        "color": "#AA010203"
    })");
    ASSERT_NE(explicitColorStyles, nullptr);
    component.ApplyComponentSpecificStyles(explicitColorStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetColorForTest(), 0xAA010203u);

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    component.OnConfigChange(darkContext);
    EXPECT_EQ(component.GetColorForTest(), 0xAA010203u);

    std::unique_ptr<JsonAdapter> bindingDescriptor = JsonAdapter::Parse(R"({
        "value": 10,
        "total": {
            "path": "$.progress.total"
        }
    })");
    ASSERT_NE(bindingDescriptor, nullptr);
    component.ApplyPrivateAttributes(bindingDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 10.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> clampDescriptor = JsonAdapter::Parse(R"({
        "value": 150,
        "total": 40
    })");
    ASSERT_NE(clampDescriptor, nullptr);
    component.ApplyPrivateAttributes(clampDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 40.0F);
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 40.0F);

    PropertyDeclaration valueDeclaration = component.GetPrivatePropertyDeclaration("value");
    PropertyDeclaration totalDeclaration = component.GetPrivatePropertyDeclaration("total");
    EXPECT_DOUBLE_EQ(valueDeclaration.fallbackNumber, 0.0);
    EXPECT_DOUBLE_EQ(totalDeclaration.fallbackNumber, 100.0);
    EXPECT_TRUE(static_cast<bool>(valueDeclaration.applyValue));
    EXPECT_TRUE(static_cast<bool>(totalDeclaration.applyValue));
}

TEST_F(ExtendedProgressComponentTest,
    L0_should_cover_progress_non_object_descriptor_object_private_values_and_non_finite_total)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> nonObjectDescriptor = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectDescriptor, nullptr);
    component.ApplyPrivateAttributes(nonObjectDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> bindingCallDescriptor = JsonAdapter::Parse(R"({
        "value": {
            "call": "getValue"
        },
        "total": {
            "call": "getTotal"
        }
    })");
    ASSERT_NE(bindingCallDescriptor, nullptr);
    component.ApplyPrivateAttributes(bindingCallDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> nonFiniteTotalDescriptor = JsonAdapter::CreateObject();
    ASSERT_NE(nonFiniteTotalDescriptor, nullptr);
    JsonValue nonFiniteRoot = nonFiniteTotalDescriptor->GetRoot();
    ASSERT_TRUE(nonFiniteRoot.PutNumber("value", 10));
    ASSERT_TRUE(nonFiniteRoot.PutNumber("total", std::numeric_limits<double>::quiet_NaN()));
    component.ApplyPrivateAttributes(nonFiniteRoot);
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 10.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.ApplyComponentSpecificStyles(nonObjectStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
}

TEST_F(ExtendedProgressComponentTest, L0_should_cover_progress_missing_total_and_invalid_style_json_value_paths)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "type": "ring",
        "color": "#AA334455"
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 1);
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);

    JsonValue invalidStyles;
    component.ApplyComponentSpecificStyles(invalidStyles, applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 1);
    EXPECT_EQ(component.GetColorForTest(), 0xAA334455u);

    std::unique_ptr<JsonAdapter> missingTotalDescriptor = JsonAdapter::Parse(R"({
        "value": 12
    })");
    ASSERT_NE(missingTotalDescriptor, nullptr);
    component.ApplyPrivateAttributes(missingTotalDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 12.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_cover_progress_value_object_fallback_shape)
{
    TestableExtendedProgressComponent component;

    std::unique_ptr<JsonAdapter> pathBindingDescriptor = JsonAdapter::Parse(R"({
        "value": {
            "path": "$.progress.value"
        },
        "total": 80
    })");
    ASSERT_NE(pathBindingDescriptor, nullptr);

    component.ApplyPrivateAttributes(pathBindingDescriptor->GetRoot());

    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 80.0F);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest,
    L0_should_fallback_progress_invalid_private_values_and_dispatch_schema_warnings)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1003);
    component.SetSurfaceId("surface-progress-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidDescriptor = JsonAdapter::Parse(R"({
        "value": true,
        "total": false
    })");
    ASSERT_NE(invalidDescriptor, nullptr);
    component.ApplyPrivateAttributes(invalidDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> invalidBindingObjectDescriptor = JsonAdapter::Parse(R"({
        "value": {
            "path": "$.progress.value"
        },
        "total": {
            "call": "getTotal"
        }
    })");
    ASSERT_NE(invalidBindingObjectDescriptor, nullptr);
    component.ApplyPrivateAttributes(invalidBindingObjectDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "type": true,
        "color": false
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "value"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "total"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.type"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.color"), 1U);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest,
    L0_should_reject_progress_numeric_color_and_dispatch_type_mismatch_warning)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1005);
    component.SetSurfaceId("surface-progress-color-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> numericColorStyles = JsonAdapter::Parse(R"({
        "color": 2026
    })");
    ASSERT_NE(numericColorStyles, nullptr);
    component.ApplyComponentSpecificStyles(numericColorStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.color"), 1U);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest,
    L0_should_dispatch_progress_invalid_value_and_non_object_style_schema_warnings)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1004);
    component.SetSurfaceId("surface-progress-invalid-value-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> missingTotalDescriptor = JsonAdapter::Parse(R"({
        "value": 12
    })");
    ASSERT_NE(missingTotalDescriptor, nullptr);
    component.ApplyPrivateAttributes(missingTotalDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 12.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> invalidRangeDescriptor = JsonAdapter::Parse(R"({
        "value": -20,
        "total": 0
    })");
    ASSERT_NE(invalidRangeDescriptor, nullptr);
    component.ApplyPrivateAttributes(invalidRangeDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 100.0F);

    std::unique_ptr<JsonAdapter> overflowDescriptor = JsonAdapter::Parse(R"({
        "value": 150,
        "total": 40
    })");
    ASSERT_NE(overflowDescriptor, nullptr);
    component.ApplyPrivateAttributes(overflowDescriptor->GetRoot());
    EXPECT_FLOAT_EQ(component.GetValueForTest(), 40.0F);
    EXPECT_FLOAT_EQ(component.GetTotalForTest(), 40.0F);

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "type": "unsupported",
        "color": "abc"
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse("null");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.ApplyComponentSpecificStyles(nonObjectStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetProgressTypeForTest(), 0);
    EXPECT_EQ(component.GetColorForTest(), 0xFF0A59F7u);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "total"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "value"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.type"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.color"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles"), 1U);
}

TEST_F(ExtendedProgressComponentTest, L0_should_capture_default_stroke_width_on_construction)
{
    TestableExtendedProgressComponent component;

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 4.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_progress_stroke_width_when_number_value_provided)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "strokeWidth": 10
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 10.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_use_default_stroke_width_when_style_omitted)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> emptyStyles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(emptyStyles, nullptr);
    component.ApplyComponentSpecificStyles(emptyStyles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 4.0F);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest, L0_should_fallback_progress_stroke_width_boolean_to_default)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1006);
    component.SetSurfaceId("surface-progress-stroke-width-bool-warning");
    component.SetComponentId("root");
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "strokeWidth": true
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 4.0F);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
}

TEST_F(ExtendedProgressComponentSchemaWarningTest, L0_should_fallback_progress_stroke_width_string_to_default)
{
    TestableExtendedProgressComponent component;
    component.SetRenderId(1007);
    component.SetSurfaceId("surface-progress-stroke-width-string-warning");
    component.SetComponentId("root");
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "strokeWidth": "10"
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 4.0F);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
}

TEST_F(ExtendedProgressComponentTest, L0_should_pass_through_negative_stroke_width_without_validation)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "strokeWidth": -5
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), -5.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_pass_through_zero_stroke_width_without_validation)
{
    TestableExtendedProgressComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "strokeWidth": 0
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 0.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_retain_stroke_width_default_when_value_is_invalid)
{
    TestableExtendedProgressComponent component;

    JsonValue invalidValue;
    component.ApplyStrokeWidthValueForTest(invalidValue);

    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 4.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_resolve_progress_stroke_width_path_binding_to_number)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> model = JsonAdapter::Parse(R"({
        "value": {
            "width": 8
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(model->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "styles": {
                    "strokeWidth": {
                        "path": "/width"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetStrokeWidthForTest(), 8.0F);

    std::unique_ptr<JsonAdapter> updatedModel = JsonAdapter::Parse(R"({
        "value": {
            "width": 12
        }
    })");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(updatedModel->GetRoot()));
    EXPECT_FLOAT_EQ(progress->GetStrokeWidthForTest(), 12.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_apply_progress_stroke_width_delta_update)
{
    TestableExtendedProgressComponent component;
    RenderContext context =
        RenderContext::Create(13, "surface-progress-stroke-width-delta", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "strokeWidth": 6
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 6.0F);

    std::unique_ptr<JsonAdapter> deltaValue = JsonAdapter::Parse("15");
    ASSERT_NE(deltaValue, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("strokeWidth"), deltaValue->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 15.0F);
}

TEST_F(ExtendedProgressComponentTest, L0_should_retain_previous_stroke_width_and_warn_on_delta_string_value)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    TestableExtendedProgressComponent component;
    RenderContext context = RenderContext::Create(
        13, "surface-progress-stroke-width-delta-string", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "strokeWidth": 6
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 6.0F);

    std::unique_ptr<JsonAdapter> stringDelta = JsonAdapter::Parse(R"("10")");
    ASSERT_NE(stringDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("strokeWidth"), stringDelta->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 6.0F);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
}

TEST_F(ExtendedProgressComponentTest, L0_should_retain_previous_stroke_width_and_warn_on_delta_boolean_value)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    TestableExtendedProgressComponent component;
    RenderContext context = RenderContext::Create(
        13, "surface-progress-stroke-width-delta-boolean", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "strokeWidth": 8
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 8.0F);

    std::unique_ptr<JsonAdapter> booleanDelta = JsonAdapter::Parse("true");
    ASSERT_NE(booleanDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("strokeWidth"), booleanDelta->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 8.0F);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(ExtendedProgressComponentTest, L0_should_resolve_progress_stroke_width_expression_to_number)
{
    TestableExtendedProgressComponent component;
    RenderContext context =
        RenderContext::Create(13, "surface-progress-stroke-width-expression", nullptr, BuildExtendedProtocolCatalog());
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Progress",
        "styles": {
            "strokeWidth": "{{ 7 + 3 }}"
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    EXPECT_FLOAT_EQ(component.GetStrokeWidthForTest(), 10.0F);
}
#endif

} // namespace
