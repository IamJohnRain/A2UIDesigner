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

#include "components/extended/ExtendedCheckboxGroupComponent.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/checkbox/CheckboxGroupTheme.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/RenderContext.h"
#include "styles/StyleResolver.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCheckboxGroupCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("CheckboxGroup", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

} // namespace

class ExtendedCheckboxGroupComponentTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-cbg-test");
        slot_.SetRenderId(1);
        ThemeContext themeContext;
        themeContext.colorMode = ThemeMode::LIGHT;
        slot_.InitializeThemeManager(themeContext);
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_create_with_defaults_when_no_properties)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetType(), "CheckboxGroup");
    EXPECT_FALSE(cbg->GetSelectAllForTest());
    EXPECT_EQ(cbg->GetGroupForTest(), "");
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_all_private_attributes)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "myGroup",
            "selectAll": true,
            "styles": {
                "selectedColor": "#112233",
                "unSelectedColor": "#445566",
                "mark": {
                    "strokeColor": "#AABBCC",
                    "size": 18,
                    "strokeWidth": 3.0
                },
                "checkboxShape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_TRUE(cbg->GetSelectAllForTest());
    EXPECT_EQ(cbg->GetGroupForTest(), "myGroup");
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 18.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 3.0f);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_select_all_to_same_group_without_overriding_explicit_select)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "CheckboxGroup",
                "group": "groupA",
                "selectAll": true
            },
            {
                "id": "checkboxImplicit",
                "component": "Checkbox",
                "group": "groupA",
                "value": "implicit"
            },
            {
                "id": "checkboxExplicitFalse",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitFalse",
                "select": false
            },
            {
                "id": "checkboxExplicitTrue",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitTrue",
                "select": true
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto implicitCheckbox =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("checkboxImplicit"));
    auto explicitFalseCheckbox =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("checkboxExplicitFalse"));
    auto explicitTrueCheckbox =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("checkboxExplicitTrue"));
    ASSERT_NE(implicitCheckbox, nullptr);
    ASSERT_NE(explicitFalseCheckbox, nullptr);
    ASSERT_NE(explicitTrueCheckbox, nullptr);
    EXPECT_TRUE(implicitCheckbox->GetSelectForTest());
    EXPECT_FALSE(explicitFalseCheckbox->GetSelectForTest());
    EXPECT_TRUE(explicitTrueCheckbox->GetSelectForTest());

    auto update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "CheckboxGroup",
                "group": "groupA",
                "selectAll": true
            },
            {
                "id": "checkboxExplicitFalse",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitFalse"
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
    EXPECT_TRUE(explicitFalseCheckbox->GetSelectForTest());
}

TEST_F(
    ExtendedCheckboxGroupComponentTest, L0_should_apply_checkbox_shape_to_same_group_without_overriding_explicit_shape)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "CheckboxGroup",
                "group": "groupA",
                "styles": {
                    "checkboxShape": "rounded_square"
                }
            },
            {
                "id": "checkboxImplicitShape",
                "component": "Checkbox",
                "group": "groupA",
                "value": "implicitShape"
            },
            {
                "id": "checkboxExplicitShape",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitShape",
                "styles": {
                    "shape": "circle"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto implicitShapeCheckbox =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("checkboxImplicitShape"));
    auto explicitShapeCheckbox =
        std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("checkboxExplicitShape"));
    ASSERT_NE(implicitShapeCheckbox, nullptr);
    ASSERT_NE(explicitShapeCheckbox, nullptr);
    EXPECT_EQ(implicitShapeCheckbox->GetShapeForTest(), 1);
    EXPECT_EQ(explicitShapeCheckbox->GetShapeForTest(), 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "CheckboxGroup",
                "group": "groupA",
                "styles": {
                    "checkboxShape": "circle"
                }
            },
            {
                "id": "checkboxImplicitShape",
                "component": "Checkbox",
                "group": "groupA",
                "value": "implicitShape"
            },
            {
                "id": "checkboxExplicitShape",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitShape",
                "styles": {
                    "shape": "rounded_square"
                }
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(implicitShapeCheckbox->GetShapeForTest(), 0);
    EXPECT_EQ(explicitShapeCheckbox->GetShapeForTest(), 1);

    auto removeExplicitShapeUpdate = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "CheckboxGroup",
                "group": "groupA",
                "styles": {
                    "checkboxShape": "rounded_square"
                }
            },
            {
                "id": "checkboxExplicitShape",
                "component": "Checkbox",
                "group": "groupA",
                "value": "explicitShape"
            }
        ]
    })");
    ASSERT_NE(removeExplicitShapeUpdate, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(removeExplicitShapeUpdate->GetRoot()));
    EXPECT_EQ(explicitShapeCheckbox->GetShapeForTest(), 1);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_to_defaults_when_properties_removed)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "grp",
            "selectAll": true,
            "selectedColor": "#112233",
            "unSelectedColor": "#445566",
            "mark": {"strokeColor": "#AABBCC", "size": 15, "strokeWidth": 4.0},
            "checkboxShape": "rounded_square"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_FALSE(cbg->GetSelectAllForTest());
    EXPECT_EQ(cbg->GetGroupForTest(), "");
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_mark_without_size)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
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

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFDDEEFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 4.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_use_circle_shape_for_rounded_square_camel_case_value)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "checkboxShape": "roundedSquare"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_style_colors)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#CCAABB",
                "unSelectedColor": "#DDCCBB"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDCCBBu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_ignore_legacy_style_names_without_alias)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#112233",
                "unselectedColor": "#445566",
                "shape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_keep_colors_on_delta_update)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#CCAABB",
                "unSelectedColor": "#DDCCBB"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#112233"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_only_invalid_delta_style_and_preserve_missing_styles)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#CCAABB",
                "unSelectedColor": "#DDCCBB",
                "mark": {
                    "strokeColor": "#FF0000",
                    "size": 25,
                    "strokeWidth": 4.0
                },
                "checkboxShape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFF0000u);

    auto selectedColor = JsonAdapter::Parse("true");
    ASSERT_NE(selectedColor, nullptr);
    std::static_pointer_cast<Component>(cbg)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("selectedColor"), selectedColor->GetRoot());

    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 25.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 4.0f);
    EXPECT_FALSE(cbg->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cbg->HasUnselectedColorOverrideForTest());
    EXPECT_TRUE(cbg->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_invalid_mark_and_shape_delta_and_preserve_other_styles)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#CCAABB",
                "unSelectedColor": "#DDCCBB",
                "mark": {
                    "strokeColor": "#FF0000",
                    "size": 25,
                    "strokeWidth": 4.0
                },
                "checkboxShape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    auto invalidShape = JsonAdapter::Parse(R"("triangle")");
    ASSERT_NE(invalidShape, nullptr);
    std::static_pointer_cast<Component>(cbg)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("checkboxShape"), invalidShape->GetRoot());

    EXPECT_EQ(cbg->GetShapeForTest(), 0);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFF0000u);

    auto invalidMark = JsonAdapter::Parse("true");
    ASSERT_NE(invalidMark, nullptr);
    std::static_pointer_cast<Component>(cbg)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("mark"), invalidMark->GetRoot());

    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFCCAABBu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDCCBBu);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_TRUE(cbg->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cbg->HasUnselectedColorOverrideForTest());
    EXPECT_FALSE(cbg->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_register_change_listener)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_NE(cbg->GetCheckboxGroupNodeForTest(), nullptr);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_invalid_mark_gracefully)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "mark": 42
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_mark_with_partial_properties)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "mark": {
                    "strokeColor": "#FF0000"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_factory_should_create_checkbox_group)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Extended.CheckboxGroup"));
    EXPECT_TRUE(factory.IsExtendedComponent("CheckboxGroup"));

    auto comp = factory.CreateComponent("Extended.CheckboxGroup");
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->GetType(), "CheckboxGroup");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_individual_properties)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "selectAll": false
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g2",
            "selectAll": true
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetGroupForTest(), "g2");
    EXPECT_TRUE(cbg->GetSelectAllForTest());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_construct_with_null_api_gracefully)
{
    ExtendedCheckboxGroupComponent cbg;
    EXPECT_EQ(cbg.GetType(), "CheckboxGroup");
    EXPECT_FALSE(cbg.GetSelectAllForTest());
    EXPECT_EQ(cbg.GetGroupForTest(), "");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_dispatch_change_event_when_triggered)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    ASSERT_NE(cbg->GetCheckboxGroupNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_ignore_wrong_event_type)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, static_cast<ArkUI_NodeEventType>(9999));

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_unregister_change_event_when_listener_removed)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    auto node = cbg->GetCheckboxGroupNodeForTest();
    auto regEventsIt = mockArkUIPtr_->registeredNodeEvents_.find(node);
    ASSERT_NE(regEventsIt, mockArkUIPtr_->registeredNodeEvents_.end());
    EXPECT_TRUE(regEventsIt->second.count(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE) > 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    regEventsIt = mockArkUIPtr_->registeredNodeEvents_.find(node);
    EXPECT_EQ(regEventsIt, mockArkUIPtr_->registeredNodeEvents_.end());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_do_nothing_on_property_removed_for_unknown_property)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "selectAll": true
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_TRUE(cbg->GetSelectAllForTest());
    EXPECT_EQ(cbg->GetGroupForTest(), "g1");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_shape_on_delta_update_without_shape)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#112233",
                "checkboxShape": "rounded_square"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "selectedColor": "#445566"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetShapeForTest(), 0);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF445566u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_use_fallback_color_when_style_color_missing)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_non_object_mark_in_styles)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "mark": "not_an_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_ignore_event_when_user_data_cleared)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    mockArkUIPtr_->nodeUserData_.erase(cbg->GetCheckboxGroupNodeForTest());

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_register_event_receiver_on_checkbox_group_node)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    auto nodeHandle = cbg->GetCheckboxGroupNodeForTest();
    ASSERT_NE(nodeHandle, nullptr);

    EXPECT_EQ(mockArkUIPtr_->nodeEventReceivers_.count(nodeHandle), 1u);
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[nodeHandle].count(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE) > 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_shape_value_correctly)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {"checkboxShape": "circle"}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {"checkboxShape": "rounded_square"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetShapeForTest(), 1);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_accept_numeric_value_for_selectall_property)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "selectAll": 1
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_TRUE(cbg->GetSelectAllForTest());

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "selectAll": 0
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_FALSE(cbg->GetSelectAllForTest());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_mark_properties_individually)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "mark": {"strokeColor": "#00FF00"}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFF00FF00u);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {
                "mark": {"size": 20}
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_empty_mark_object)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {"mark": {}}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_mark_to_default_on_delta_without_mark)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
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

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFF0000u);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 25.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 4.0f);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_boolean_mark_properties)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
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

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_use_default_values_when_native_api_null)
{
    ExtendedCheckboxGroupComponent cbg;
    EXPECT_FALSE(cbg.GetSelectAllForTest());
    EXPECT_EQ(cbg.GetGroupForTest(), "");
    EXPECT_EQ(cbg.GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg.GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg.GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg.GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FLOAT_EQ(cbg.GetMarkSizeForTest(), 20.0f);
    EXPECT_EQ(cbg.GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_not_crash_with_null_user_data)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    mockArkUIPtr_->nodeUserData_.clear();

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_dispatch_change_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChange"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_negative_mark_values_gracefully)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
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

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), -5.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), -2.5f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_group_on_property_removed)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "testGroup"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetGroupForTest(), "testGroup");

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup"
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetGroupForTest(), "");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_all_properties_in_single_update)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "group1",
            "selectAll": true,
            "styles": {
                "selectedColor": "#112233",
                "unSelectedColor": "#445566",
                "checkboxShape": "rounded_square",
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

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_TRUE(cbg->GetSelectAllForTest());
    EXPECT_EQ(cbg->GetGroupForTest(), "group1");
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 18.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 3.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_have_default_select_all_status_none)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_capture_status_all_from_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 0;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_capture_status_part_from_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 1);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_capture_status_none_from_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent compEvent = {};
    compEvent.data[0].i32 = 2;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &compEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_set_status_0_from_string_async_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "Status:0";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_set_status_1_from_string_async_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "Status:1";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 1);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_set_status_2_from_string_async_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "Status:2";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_keep_status_when_status_prefix_not_found)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "SomeOtherData";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_keep_status_when_status_string_truncated)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "Status:";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_keep_status_when_status_value_out_of_range)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char statusStr[] = "Status:9";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = statusStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_not_crash_when_string_async_event_has_null_pStr)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "groupChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = nullptr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_use_theme_colors_from_light_context)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);

    auto message = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup", "group": "g1"}]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_theme_colors_on_config_change)
{
    ThemeContext lightContext;
    lightContext.colorMode = ThemeMode::LIGHT;
    CheckboxGroupTheme theme(lightContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF007DFFu);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66182431u);

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    theme.OnConfigChange(darkContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF3F97E9u);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66FFFFFFu);
    EXPECT_EQ(theme.GetMarkStrokeColor(), 0xFFFFFFFFu);

    theme.OnConfigChange(lightContext);
    EXPECT_EQ(theme.GetSelectedColor(), 0xFF007DFFu);
    EXPECT_EQ(theme.GetUnselectedColor(), 0x66182431u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_mark_colors_as_overridden_when_user_colors_set)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "selectedColor": "#FF0000",
                "unSelectedColor": "#00FF00"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFF00FF00u);
    EXPECT_TRUE(cbg->HasSelectedColorOverrideForTest());
    EXPECT_TRUE(cbg->HasUnselectedColorOverrideForTest());
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFF00FF00u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_unregister_event_in_destructor)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto node = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"))
                    ->GetCheckboxGroupNodeForTest();
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE) > 0);

    auto update = JsonAdapter::Parse(R"({
        "components": [{"id": "root", "component": "CheckboxGroup", "group": "g1"}]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_get_theme_return_null_when_no_theme_manager)
{
    ExtendedCheckboxGroupComponent cbg;
    auto theme = cbg.GetTheme();
    EXPECT_EQ(theme, nullptr);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_onconfigchange_do_nothing_when_no_theme)
{
    ExtendedCheckboxGroupComponent cbg;
    ThemeContext ctx;
    ctx.colorMode = ThemeMode::DARK;
    cbg.OnConfigChange(ctx);
    EXPECT_EQ(cbg.GetSelectedColorForTest(), 0xFF007DFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_style_with_only_selected_color)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "selectedColor": "#AABBCC"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFAABBCCu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_style_with_only_unselected_color)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "unSelectedColor": "#DDEEFF"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0xFFDDEEFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_non_object_styles_gracefully)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": "not_an_object"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(cbg->GetShapeForTest(), 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_mark_with_stroke_width_only)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "mark": {"strokeWidth": 5.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 5.0f);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_registration_when_listener_added_later)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    auto node = cbg->GetCheckboxGroupNodeForTest();
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE), 0u);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));
    EXPECT_TRUE(mockArkUIPtr_->registeredNodeEvents_[node].count(NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE) > 0);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_delta_update_preserving_shape)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {"checkboxShape": "rounded_square"}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetShapeForTest(), 1);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetShapeForTest(), 0);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_apply_delta_update_preserving_mark)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {"mark": {"strokeColor": "#FF0000", "size": 10}}
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    auto update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {"selectedColor": "#112233"}
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFF112233u);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_call_setters_on_null_api_without_crash)
{
    ExtendedCheckboxGroupComponent cbg;
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "selectAll": true,
            "styles": {
                "selectedColor": "#112233",
                "unSelectedColor": "#445566",
                "checkboxShape": "rounded_square",
                "mark": {"strokeColor": "#AABBCC", "size": 15, "strokeWidth": 3.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_mark_style_as_overridden_when_user_mark_set)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "mark": {"strokeColor": "#AABBCC", "size": 10, "strokeWidth": 3.0}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 10.0f);
    EXPECT_TRUE(cbg->HasMarkOverrideForTest());
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFAABBCCu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 10.0f);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_non_overridden_colors_on_config_change)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "selectedColor": "#FF0000"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_TRUE(cbg->HasSelectedColorOverrideForTest());
    EXPECT_FALSE(cbg->HasUnselectedColorOverrideForTest());
    EXPECT_FALSE(cbg->HasMarkOverrideForTest());

    themeManager->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(cbg->GetSelectedColorForTest(), 0xFFFF0000u);
    EXPECT_EQ(cbg->GetUnselectedColorForTest(), 0x66182431u);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_reset_mark_override_when_non_object_mark_in_delta)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "mark": "not_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);
    EXPECT_EQ(cbg->GetMarkStrokeColorForTest(), 0xFFFFFFFFu);
    EXPECT_FLOAT_EQ(cbg->GetMarkSizeForTest(), 20.0f);
    EXPECT_FLOAT_EQ(cbg->GetMarkStrokeWidthForTest(), 2.0f);
    EXPECT_FALSE(cbg->HasMarkOverrideForTest());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_handle_null_component_event_and_null_string_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "group": "g1",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_parse_names_from_string_async_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char eventStr[] = "Name:cb1,cb2,cb3;Status:0";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = eventStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 0);

    auto names = cbg->GetSelectedNamesForTest();
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "cb1");
    EXPECT_EQ(names[1], "cb2");
    EXPECT_EQ(names[2], "cb3");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_parse_single_name_from_string_async_event)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char eventStr[] = "Name:itemA;Status:1";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = eventStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 1);

    auto names = cbg->GetSelectedNamesForTest();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "itemA");
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_have_empty_names_when_no_name_prefix)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char eventStr[] = "Status:2";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = eventStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
    EXPECT_TRUE(cbg->GetSelectedNamesForTest().empty());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_have_empty_names_when_name_is_empty)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    char eventStr[] = "Name:;Status:2";
    ArkUI_StringAsyncEvent stringEvent = {};
    stringEvent.pStr = eventStr;
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 2);
    EXPECT_TRUE(cbg->GetSelectedNamesForTest().empty());
}

TEST_F(ExtendedCheckboxGroupComponentTest, L0_should_update_names_on_subsequent_events)
{
    slot_.SetCatalog(BuildCheckboxGroupCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "CheckboxGroup",
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "cbg"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cbg, nullptr);

    {
        ArkUI_NodeEvent fakeEvent = {};
        char eventStr[] = "Name:cb1,cb2;Status:0";
        ArkUI_StringAsyncEvent stringEvent = {};
        stringEvent.pStr = eventStr;
        mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
        mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
        mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);
        mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    }

    ASSERT_EQ(cbg->GetSelectedNamesForTest().size(), 2u);
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 0);

    {
        ArkUI_NodeEvent fakeEvent = {};
        char eventStr[] = "Name:cb1;Status:1";
        ArkUI_StringAsyncEvent stringEvent = {};
        stringEvent.pStr = eventStr;
        mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
        mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);
        mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);
        mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    }

    auto names = cbg->GetSelectedNamesForTest();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "cb1");
    EXPECT_EQ(cbg->GetSelectAllStatusForTest(), 1);
}
