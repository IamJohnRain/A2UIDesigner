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

#include "functions/NativeGetCheckboxGroupValuesFunction.h"

#include <gtest/gtest.h>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/custom/CustomComponent.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

JsonValue ParseJson(const std::string& json)
{
    auto adapter = JsonAdapter::Parse(json);
    if (adapter == nullptr) {
        return JsonValue();
    }
    return adapter->GetRoot();
}

std::shared_ptr<Catalog> BuildCheckboxCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto checkboxItem = std::make_shared<CatalogItem>("Checkbox", CatalogItemType::COMPONENT);
    checkboxItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(checkboxItem);
    auto groupItem = std::make_shared<CatalogItem>("CheckboxGroup", CatalogItemType::COMPONENT);
    groupItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(groupItem);
    return catalog;
}

void AssertEmptyArray(const FunctionResult& result)
{
    ASSERT_TRUE(result.IsJsonValue());
    JsonValue json = result.GetJsonValue();
    ASSERT_TRUE(json.IsArray());
    EXPECT_EQ(json.GetArraySize(), 0);
    EXPECT_EQ(json.ToJsonLiteral(), "[]");
}

void AssertArrayLiteral(const FunctionResult& result, const std::string& expected)
{
    ASSERT_TRUE(result.IsJsonValue());
    JsonValue json = result.GetJsonValue();
    ASSERT_TRUE(json.IsArray());
    EXPECT_EQ(json.ToJsonLiteral(), expected);
}

void StoreCheckboxRuntimeState(SurfaceSlot& slot, const std::string& key, const std::string& json)
{
    JsonValue state = ParseJson(json);
    ASSERT_TRUE(state.IsValid());
    slot.StoreRuntimeState("ExtendedCheckbox.select", key, state);
}

} // namespace

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_correct_name)
{
    NativeGetCheckboxGroupValuesFunction func;
    EXPECT_EQ(func.GetName(), "getCheckboxGroupValues");
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_empty_array_when_execute_without_context)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson("{}");
    FunctionResult result = func.Execute(args);
    AssertEmptyArray(result);
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_null_when_group_arg_missing)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson("{}");
    DynamicResolveContext context;
    context.surfaceId = "surf1";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    EXPECT_TRUE(result.IsNull());
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_null_when_group_arg_empty_string)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": ""})json");
    DynamicResolveContext context;
    context.surfaceId = "surf1";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    EXPECT_TRUE(result.IsNull());
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_null_when_group_arg_null)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": null})json");
    DynamicResolveContext context;
    context.surfaceId = "surf1";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    EXPECT_TRUE(result.IsNull());
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_null_when_group_arg_not_string)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": 123})json");
    DynamicResolveContext context;
    context.surfaceId = "surf1";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    EXPECT_TRUE(result.IsNull());
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_return_empty_array_when_surface_not_found)
{
    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.surfaceId = "nonexistent_surface";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST(NativeGetCheckboxGroupValuesFunctionTest, L0_should_be_registered_in_registry)
{
    NativeFunctionRegistry& reg = NativeFunctionRegistry::GetInstance();
    EXPECT_TRUE(reg.HasFunction("getCheckboxGroupValues"));
}

class NativeGetCheckboxGroupValuesFunctionIntegrationTest : public A2UITest {
protected:
    static constexpr int32_t RENDER_ID = 700;
    static constexpr const char* SURFACE_ID = "integration_test_surface";

    void SetUp() override
    {
        A2UITest::SetUp();
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(RENDER_ID);
        SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(SURFACE_ID);
        surface.SetCatalog(BuildCheckboxCatalog());
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(RENDER_ID);
        A2UITest::TearDown();
    }

    SurfaceSlot* GetSurface()
    {
        return RenderManager::GetInstance().FindSurface(SURFACE_ID);
    }
};

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_return_empty_array_when_no_checkboxes)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_group_does_not_match)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("other_group")json"));
    customCb->SetRuntimeCustomProperty("select", ParseJson(R"json(true)json"));
    customCb->SetRuntimeCustomProperty("label", ParseJson(R"json("Label")json"));
    slot->GetAllComponents()["customCb2"] = customCb;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_value_is_not_bool)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCb->SetRuntimeCustomProperty("value", ParseJson(R"json("not_a_bool")json"));
    customCb->SetRuntimeCustomProperty("label", ParseJson(R"json("Label")json"));
    slot->GetAllComponents()["customCb3"] = customCb;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_value_is_false)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCb->SetRuntimeCustomProperty("value", ParseJson(R"json(false)json"));
    customCb->SetRuntimeCustomProperty("label", ParseJson(R"json("Label")json"));
    slot->GetAllComponents()["customCb4"] = customCb;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_value_is_empty)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCb->SetRuntimeCustomProperty("select", ParseJson(R"json(true)json"));
    customCb->SetRuntimeCustomProperty("value", ParseJson(R"json("")json"));
    slot->GetAllComponents()["customCb5"] = customCb;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_value_is_not_string)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCb->SetRuntimeCustomProperty("select", ParseJson(R"json(true)json"));
    customCb->SetRuntimeCustomProperty("value", ParseJson(R"json(42)json"));
    slot->GetAllComponents()["customCb6"] = customCb;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_custom_component_when_type_is_not_checkbox)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCard = std::make_shared<CustomComponent>("Extended.Card");
    customCard->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCard->SetRuntimeCustomProperty("value", ParseJson(R"json(true)json"));
    customCard->SetRuntimeCustomProperty("label", ParseJson(R"json("Label")json"));
    slot->GetAllComponents()["customCard1"] = customCard;

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertEmptyArray(result);
}

TEST_F(
    NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_collect_multiple_labels_from_mixed_component_types)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto customCb = std::make_shared<CustomComponent>("Extended.Checkbox");
    customCb->SetRuntimeCustomProperty("group", ParseJson(R"json("g1")json"));
    customCb->SetRuntimeCustomProperty("select", ParseJson(R"json(true)json"));
    customCb->SetRuntimeCustomProperty("value", ParseJson(R"json("CustomA")json"));
    slot->GetAllComponents()["mixedCb1"] = customCb;

    auto message = JsonAdapter::Parse(R"json({
        "components": [
            {"id": "mixedCb2", "component": "Checkbox", "value": "ExtendedB", "select": true, "group": "g1"}
        ]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsJsonValue());
    JsonValue json = result.GetJsonValue();
    ASSERT_TRUE(json.IsArray());
    EXPECT_EQ(json.GetArraySize(), 2);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_skip_non_custom_non_extended_component)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto message = JsonAdapter::Parse(R"json({
        "components": [
            {"id": "col1", "component": "Column"},
            {"id": "cb1", "component": "Checkbox", "value": "Checked", "select": true, "group": "g1"}
        ]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertArrayLiteral(result, R"(["Checked"])");
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_return_null_when_args_are_not_object)
{
    NativeGetCheckboxGroupValuesFunction func;
    JsonValue nonObject = JsonValue();
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    FunctionResult result = func.ExecuteWithContext(nonObject, context);
    EXPECT_TRUE(result.IsNull());
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_registry_execute_should_fail_when_group_missing)
{
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";
    ResolvedValue result =
        NativeFunctionRegistry::GetInstance().Execute("getCheckboxGroupValues", ParseJson("{}"), context, "array");
    EXPECT_FALSE(result.success);
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_collect_values_from_runtime_state_store)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    StoreCheckboxRuntimeState(*slot, "g1\nHiddenA", R"json({"group":"g1","value":"HiddenA","select":true})json");
    StoreCheckboxRuntimeState(*slot, "g1\nHiddenB", R"json({"group":"g1","value":"HiddenB","select":false})json");
    StoreCheckboxRuntimeState(*slot, "other\nHiddenC", R"json({"group":"other","value":"HiddenC","select":true})json");
    StoreCheckboxRuntimeState(*slot, "g1\nNoValue", R"json({"group":"g1","value":"","select":true})json");
    StoreCheckboxRuntimeState(*slot, "g1\nBadSelect", R"json({"group":"g1","value":"BadSelect","select":"true"})json");
    StoreCheckboxRuntimeState(*slot, "g1\nNotObject", R"json([true])json");

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";

    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertArrayLiteral(result, R"(["HiddenA"])");
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_merge_duplicate_runtime_values_without_overwrite)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    StoreCheckboxRuntimeState(*slot, "g1\n/items/0/cb", R"json({"group":"g1","value":"Same","select":true})json");
    StoreCheckboxRuntimeState(*slot, "g1\n/items/1/cb", R"json({"group":"g1","value":"Same","select":false})json");

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";

    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertArrayLiteral(result, R"(["Same"])");
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_should_merge_duplicate_visible_values_without_overwrite)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    auto message = JsonAdapter::Parse(R"json({
        "components": [
            {"id": "dupCb1", "component": "Checkbox", "value": "Same", "select": true, "group": "g1"},
            {"id": "dupCb2", "component": "Checkbox", "value": "Same", "select": false, "group": "g1"}
        ]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";

    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertArrayLiteral(result, R"(["Same"])");
}

TEST_F(NativeGetCheckboxGroupValuesFunctionIntegrationTest, L0_runtime_state_should_override_visible_checkbox_value)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);

    StoreCheckboxRuntimeState(*slot, "g1\nvisibleCb", R"json({"group":"g1","value":"VisibleA","select":false})json");

    auto message = JsonAdapter::Parse(R"json({
        "components": [
            {"id": "visibleCb", "component": "Checkbox", "value": "VisibleA", "select": true, "group": "g1"},
            {"id": "visibleCb2", "component": "Checkbox", "value": "VisibleB", "select": true, "group": "g1"}
        ]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetCheckboxGroupValuesFunction func;
    auto args = ParseJson(R"json({"group": "g1"})json");
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;
    context.componentId = "any";

    FunctionResult result = func.ExecuteWithContext(args, context);
    AssertArrayLiteral(result, R"(["VisibleB"])");
}
