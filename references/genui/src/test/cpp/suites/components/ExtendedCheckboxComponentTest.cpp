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

#include "components/extended/ExtendedCheckboxComponent.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/checkbox/CheckboxTheme.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/RenderContext.h"
#include "styles/StyleResolver.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCheckboxCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Checkbox", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

} // namespace

class ExtendedCheckboxComponentTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-cb-test");
        slot_.SetRenderId(1);
        ThemeContext themeContext;
        themeContext.colorMode = ThemeMode::LIGHT;
        slot_.InitializeThemeManager(themeContext);
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedCheckboxComponentTest, L0_should_create_with_defaults_when_no_properties)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetType(), "Checkbox");
    EXPECT_FALSE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "");
    EXPECT_EQ(cb->GetGroupForTest(), "");
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cb->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_all_private_attributes_and_styles)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "Accept",
            "group": "myGroup",
            "styles": {
                "selectedColor": "#112233",
                "unselectedColor": "#445566",
                "mark": {
                    "strokeColor": "#AABBCC",
                    "size": 18,
                    "strokeWidth": 3.0
                },
                "shape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "Accept");
    EXPECT_EQ(cb->GetGroupForTest(), "myGroup");
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 18.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 3.0f);
    EXPECT_EQ(cb->GetShapeForTest(), 1);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_to_defaults_when_properties_removed)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "Yes",
            "group": "grp",
            "selectedColor": "#112233",
            "unselectedColor": "#445566",
            "mark": {"strokeColor": "#AABBCC", "size": 15, "strokeWidth": 4.0},
            "shape": "rounded_square"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FALSE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "");
    EXPECT_EQ(cb->GetGroupForTest(), "");
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cb->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_mark_without_size)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "strokeColor": "#DDEEFF",
                    "strokeWidth": 4.0
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFDDEEFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 4.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_use_circle_shape_for_rounded_square_camel_case_value)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "shape": "roundedSquare"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_style_colors)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#CCAABB",
                "unselectedColor": "#DDCCBB"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDCCBBu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_keep_style_colors_on_delta_update)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#CCAABB",
                "unselectedColor": "#DDCCBB"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#112233"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_only_invalid_delta_style_and_preserve_missing_styles)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#CCAABB",
                "unselectedColor": "#DDCCBB",
                "mark": {
                    "strokeColor": "#FF0000",
                    "size": 25,
                    "strokeWidth": 4.0
                },
                "shape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cb->GetShapeForTest(), 1);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFF0000u);

    auto selectedColor = JsonAdapter::Parse("true");
    ASSERT_NE(selectedColor, nullptr);
    std::static_pointer_cast<Component>(cb)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("selectedColor"), selectedColor->GetRoot());

    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cb->GetShapeForTest(), 1);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 25.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 4.0f);
    EXPECT_FALSE(cb->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cb->HasUnselectedColorOverrideForTest());
    EXPECT_TRUE(cb->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_invalid_mark_and_shape_delta_and_preserve_other_styles)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#CCAABB",
                "unselectedColor": "#DDCCBB",
                "mark": {
                    "strokeColor": "#FF0000",
                    "size": 25,
                    "strokeWidth": 4.0
                },
                "shape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    auto invalidShape = JsonAdapter::Parse(R"("triangle")");
    ASSERT_NE(invalidShape, nullptr);
    std::static_pointer_cast<Component>(cb)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("shape"), invalidShape->GetRoot());

    EXPECT_EQ(cb->GetShapeForTest(), 0);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFF0000u);

    auto invalidMark = JsonAdapter::Parse("true");
    ASSERT_NE(invalidMark, nullptr);
    std::static_pointer_cast<Component>(cb)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("mark"), invalidMark->GetRoot());

    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cb->GetShapeForTest(), 0);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_TRUE(cb->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cb->HasUnselectedColorOverrideForTest());
    EXPECT_FALSE(cb->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_dispatch_change_listener)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_NE(cb->GetCheckboxNodeForTest(), nullptr);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_sync_value_to_data_model_on_binding)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": {"path": "/checked"},
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "cbChange"
                }
            }
        }],
        "dataModel": {"checked": false}
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_NE(cb->GetCheckboxNodeForTest(), nullptr);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_invalid_mark_gracefully)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "mark": 42
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_mark_with_partial_properties)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "strokeColor": "#FF0000"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_mark_with_size_only)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "size": 24
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 24.0f);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_factory_should_create_checkbox_component)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Extended.Checkbox"));
    EXPECT_TRUE(factory.IsExtendedComponent("Checkbox"));

    auto comp = factory.CreateComponent("Extended.Checkbox");
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->GetType(), "Checkbox");
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_update_individual_properties)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": false,
            "label": "No",
            "group": "g1"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "Yes",
            "group": "g2"
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "Yes");
    EXPECT_EQ(cb->GetGroupForTest(), "g2");
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_construct_with_null_api_gracefully)
{
    ExtendedCheckboxComponent cb;
    EXPECT_EQ(cb.GetType(), "Checkbox");
    EXPECT_FALSE(cb.GetSelectForTest());
    EXPECT_EQ(cb.GetLabelForTest(), "");
    EXPECT_EQ(cb.GetGroupForTest(), "");
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_dispatch_change_event_when_checkbox_toggled)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": false,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(cb->GetCheckboxNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_skip_change_when_same_value)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_keep_change_event_registered_when_listener_removed)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    auto& regEvents = mockArkUIPtr_->registeredNodeEvents_[cb->GetCheckboxNodeForTest()];
    EXPECT_TRUE(regEvents.count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_TRUE(regEvents.count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_wrong_event_type_in_handle_node_event)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, static_cast<ArkUI_NodeEventType>(9999));

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_null_component_event_in_handle_node_event)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, nullptr);
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_do_nothing_on_property_removed_for_unknown_property)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "test"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "test");
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_shape_on_delta_update_without_shape)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#112233",
                "shape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetShapeForTest(), 1);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#445566"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetShapeForTest(), 0);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF445566u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_use_fallback_color_when_style_color_missing)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_non_object_mark_in_styles)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": "not_an_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_sync_value_to_bound_data_model_on_change)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": {"path": "/checked"},
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "cbChanged"
                }
            }
        }],
        "dataModel": {"checked": false}
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 1;

    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_ignore_event_when_user_data_cleared)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    mockArkUIPtr_->nodeUserData_.erase(cb->GetCheckboxNodeForTest());

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_register_event_receiver_on_checkbox_node)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    auto nodeHandle = cb->GetCheckboxNodeForTest();
    ASSERT_NE(nodeHandle, nullptr);

    EXPECT_EQ(mockArkUIPtr_->nodeEventReceivers_.count(nodeHandle), 1u);
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[nodeHandle].count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_toggle_from_true_to_false)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": false
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FALSE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_shape_value_correctly)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"shape": "circle"}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetShapeForTest(), 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"shape": "rounded_square"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetShapeForTest(), 1);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_accept_numeric_value_for_value_property)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": 1
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": 0
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_FALSE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_update_mark_properties_individually)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {"strokeColor": "#00FF00"}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFF00FF00u);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {"size": 20}
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_empty_mark_object)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"mark": {}}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_mark_to_default_on_delta_without_mark)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "strokeColor": "#FF0000",
                    "size": 25,
                    "strokeWidth": 4.0
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 25.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 4.0f);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_boolean_mark_properties)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "size": true,
                    "strokeWidth": false
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_use_default_values_when_native_api_null)
{
    ExtendedCheckboxComponent cb;
    EXPECT_FALSE(cb.GetSelectForTest());
    EXPECT_EQ(cb.GetLabelForTest(), "");
    EXPECT_EQ(cb.GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb.GetUnselectedColorForTest(), 0x33FFFFFFu);
    EXPECT_EQ(cb.GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb.GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cb.GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cb.GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_not_crash_with_null_user_data)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    mockArkUIPtr_->nodeUserData_.clear();

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_dispatch_change_with_correct_value_context)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": false,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "cbChange"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 1;

    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_not_dispatch_change_for_same_value)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "cbChange"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 1;

    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_negative_mark_values_gracefully)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "size": -5,
                    "strokeWidth": -2.5
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), -5.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), -2.5f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_group_on_property_removed)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "group": "testGroup"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetGroupForTest(), "testGroup");

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox"
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetGroupForTest(), "");
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_all_properties_in_single_update)
{
    slot_.SetCatalog(BuildCheckboxCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "Complete Test",
            "group": "group1",
            "styles": {
                "selectedColor": "#112233",
                "unselectedColor": "#445566",
                "shape": "rounded_square",
                "mark": {
                    "strokeColor": "#AABBCC",
                    "size": 18,
                    "strokeWidth": 3.0
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_TRUE(cb->GetSelectForTest());
    EXPECT_EQ(cb->GetLabelForTest(), "Complete Test");
    EXPECT_EQ(cb->GetGroupForTest(), "group1");
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(cb->GetShapeForTest(), 1);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 18.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 3.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_use_theme_colors_from_light_context)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_update_theme_colors_on_config_change)
{
    ThemeContext lightContext;
    lightContext.colorMode = ThemeMode::LIGHT;
    CheckboxTheme theme(lightContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF317AF7u);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66000000u);

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    theme.OnConfigChange(darkContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF0A59F7u);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66000000u);
    EXPECT_EQ(theme.GetMarkStrokeColor(), 0xFFFFFFFFu);

    theme.OnConfigChange(lightContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF317AF7u);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66000000u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_mark_colors_as_overridden_when_user_colors_set)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#FF0000",
                "unselectedColor": "#00FF00"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFF00FF00u);
    EXPECT_TRUE(cb->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cb->HasUnselectedColorOverrideForTest());
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFF00FF00u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_mark_style_as_overridden_when_user_mark_set)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {"strokeColor": "#AABBCC", "size": 10, "strokeWidth": 3.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 10.0f);
    EXPECT_TRUE(cb->HasMarkOverrideForTest());
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 10.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_update_non_overridden_colors_on_config_change)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#FF0000"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_TRUE(cb->HasSelectedColorOverrideForTest());
    EXPECT_FALSE(cb->HasUnselectedColorOverrideForTest());
    EXPECT_FALSE(cb->HasMarkOverrideForTest());

    themeManager->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_reset_mark_override_when_non_object_mark_in_delta)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": "not_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FALSE(cb->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_set_mark_with_zero_size_when_native_api_null)
{
    ExtendedCheckboxComponent cb;
    EXPECT_FLOAT_EQ(cb.GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cb.GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_call_setters_on_null_api_without_crash)
{
    ExtendedCheckboxComponent cb;
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "label": "test",
            "group": "g1",
            "styles": {
                "selectedColor": "#112233",
                "unselectedColor": "#445566",
                "shape": "rounded_square",
                "mark": {"strokeColor": "#AABBCC", "size": 15, "strokeWidth": 3.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_not_crash_when_component_replaced_with_listener)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {"type": "event", "name": "cb"}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto node =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"))->GetCheckboxNodeForTest();
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_not_dispatch_change_event_without_binding)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": false
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FALSE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_style_with_only_selected_color)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "selectedColor": "#AABBCC"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFFAABBCCu);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0x66000000u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_style_with_only_unselected_color)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "unselectedColor": "#DDEEFF"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetUnselectedColorForTest(), 0xFFDDEEFFu);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_delta_update_preserving_shape)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"shape": "rounded_square"}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetShapeForTest(), 1);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetShapeForTest(), 0);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_delta_update_preserving_mark)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"mark": {"strokeColor": "#FF0000", "size": 10}}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_get_theme_return_null_when_no_theme_manager)
{
    ExtendedCheckboxComponent cb;
    auto theme = cb.GetTheme();
    EXPECT_EQ(theme, nullptr);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_onconfigchange_do_nothing_when_no_theme)
{
    ExtendedCheckboxComponent cb;
    ThemeContext ctx;
    ctx.colorMode = ThemeMode::DARK;
    cb.OnConfigChange(ctx);
    EXPECT_EQ(cb.GetSelectedColorForTest(), 0xFF317AF7u);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_apply_mark_with_stroke_width_only)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {"strokeWidth": 5.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_FLOAT_EQ(cb->GetMarkStrokeWidthForTest(), 5.0f);
    EXPECT_EQ(cb->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cb->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_handle_non_object_styles_gracefully)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": "not_an_object"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->GetSelectedColorForTest(), 0xFF317AF7u);
    EXPECT_EQ(cb->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_sync_value_when_binding_path_empty)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "select": true,
            "listeners": {
                "onChange": {"type": "event", "name": "cb"}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 0;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);
    mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_FALSE(cb->GetSelectForTest());
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_register_change_event_without_listener)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    auto node = cb->GetCheckboxNodeForTest();
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "listeners": {
                "onChange": {"type": "event", "name": "cb"}
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_EVENT_ON_CHANGE) > 0);
}

TEST_F(ExtendedCheckboxComponentTest, L0_should_dispose_internal_nodes_on_destruction)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "Checkbox"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    slot_.GetAllComponents().erase("root");

    SUCCEED();
}
