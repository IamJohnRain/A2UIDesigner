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

#include "functions/NativeGetSelectValueFunction.h"

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

std::shared_ptr<Catalog> BuildSelectCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Select", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

} // namespace

TEST(NativeGetSelectValueFunctionTest, L0_should_return_correct_name)
{
    NativeGetSelectValueFunction func;
    EXPECT_EQ(func.GetName(), "getSelectValue");
}

TEST(NativeGetSelectValueFunctionTest, L0_should_return_empty_string_when_execute_without_context)
{
    NativeGetSelectValueFunction func;
    auto args = ParseJson("{}");
    FunctionResult result = func.Execute(args);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST(NativeGetSelectValueFunctionTest, L0_should_return_empty_string_when_surface_id_empty)
{
    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "comp1"})json");
    DynamicResolveContext context;
    context.surfaceId = "";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST(NativeGetSelectValueFunctionTest, L0_should_return_null_when_component_id_missing)
{
    NativeGetSelectValueFunction func;
    auto args = ParseJson("{}");
    DynamicResolveContext context;
    context.surfaceId = "surf1";
    context.componentId = "";
    FunctionResult result = func.ExecuteWithContext(args, context);
    EXPECT_TRUE(result.IsNull());
}

TEST(NativeGetSelectValueFunctionTest, L0_should_return_empty_string_when_surface_not_found)
{
    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "comp1"})json");
    DynamicResolveContext context;
    context.surfaceId = "nonexistent_surface";
    context.componentId = "comp1";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST(NativeGetSelectValueFunctionTest, L0_should_be_registered_in_registry)
{
    NativeFunctionRegistry& reg = NativeFunctionRegistry::GetInstance();
    EXPECT_TRUE(reg.HasFunction("getSelectValue"));
}

class NativeGetSelectValueFunctionIntegrationTest : public A2UITest {
protected:
    static constexpr int32_t RENDER_ID = 800;
    static constexpr const char* SURFACE_ID = "select_test_surface";

    void SetUp() override
    {
        A2UITest::SetUp();
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(RENDER_ID);
        SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(SURFACE_ID);
        surface.SetCatalog(BuildSelectCatalog());
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

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_component_not_found)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{"id": "sel0", "component": "Select"}]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "nonexistent_component"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_no_selection_made)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{"id": "sel_default", "component": "Select"}]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "sel_default"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_component_is_not_custom)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{"id": "col1", "component": "Column"}]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "col1"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(
    NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_component_is_custom_but_not_select)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto customText = std::make_shared<CustomComponent>("Extended.Text");
    customText->SetRuntimeCustomProperty("value", ParseJson(R"json("unexpected")json"));
    slot->GetAllComponents()["textLike"] = customText;

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "textLike"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_value_after_select_change)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "sel1",
            "component": "Select",
            "onSelect": [{"call": "selectChanged"}]
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("sel1");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto extraContext = ParseJson(R"json({"index": 2, "value": "Option C"})json");
    customComp->SetRuntimeCustomProperty("value", extraContext.GetItem("value"));
    customComp->DispatchEvent("onSelect", extraContext);

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "sel1"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "Option C");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_lookup_select_by_component_id_arg)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "selectA",
            "component": "Select"
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("selectA");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto extraContext = ParseJson(R"json({"index": 1, "value": "Shanghai"})json");
    customComp->SetRuntimeCustomProperty("value", extraContext.GetItem("value"));

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "selectA"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "selectFuncBtn";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "Shanghai");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_initial_selected_option_value)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "selectA",
            "component": "Select",
            "options": [
                { "value": "北京" },
                { "value": "上海" }
            ],
            "selected": 0
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "selectA"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "selectFuncBtn";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(""), "北京");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_value_over_multiple_changes)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "sel2",
            "component": "Select",
            "onSelect": [{"call": "selectChanged"}]
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("sel2");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto ctx1 = ParseJson(R"json({"index": 0, "value": "Alpha"})json");
    customComp->SetRuntimeCustomProperty("value", ctx1.GetItem("value"));
    customComp->DispatchEvent("onSelect", ctx1);

    NativeGetSelectValueFunction func;
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel2";

    auto result1 = func.ExecuteWithContext(ParseJson(R"json({"componentId": "sel2"})json"), context);
    ASSERT_TRUE(result1.IsString());
    EXPECT_EQ(result1.GetStringValue(""), "Alpha");

    auto ctx2 = ParseJson(R"json({"index": 3, "value": "Delta"})json");
    customComp->SetRuntimeCustomProperty("value", ctx2.GetItem("value"));
    customComp->DispatchEvent("onSelect", ctx2);

    auto result2 = func.ExecuteWithContext(ParseJson(R"json({"componentId": "sel2"})json"), context);
    ASSERT_TRUE(result2.IsString());
    EXPECT_EQ(result2.GetStringValue(""), "Delta");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_value_is_not_string)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "sel_num",
            "component": "Select",
            "onSelect": [{"call": "selectChanged"}]
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("sel_num");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto extraContext = ParseJson(R"json({"index": 1, "value": 42})json");
    customComp->DispatchEvent("onSelect", extraContext);

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "sel_num"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel_num";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_value_is_null)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "sel_null",
            "component": "Select",
            "onSelect": [{"call": "selectChanged"}]
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("sel_null");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto extraContext = ParseJson(R"json({"index": 0, "value": null})json");
    customComp->DispatchEvent("onSelect", extraContext);

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "sel_null"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel_null";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_registry_execute_should_return_ok)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{"id": "sel_rt", "component": "Select"}]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel_rt";

    auto result = NativeFunctionRegistry::GetInstance().Execute(
        "getSelectValue", ParseJson(R"json({"componentId": "sel_rt"})json"), context, "string");
    EXPECT_TRUE(result.success);
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_registry_execute_should_fail_when_component_id_missing)
{
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel_rt";

    auto result = NativeFunctionRegistry::GetInstance().Execute("getSelectValue", ParseJson("{}"), context, "string");
    EXPECT_FALSE(result.success);
}

TEST_F(
    NativeGetSelectValueFunctionIntegrationTest, L0_registry_execute_should_fail_when_using_uppercase_component_id_arg)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{"id": "sel_upper", "component": "Select"}]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "source";

    auto result = NativeFunctionRegistry::GetInstance().Execute(
        "getSelectValue", ParseJson(R"json({"componentID": "sel_upper"})json"), context, "string");
    EXPECT_FALSE(result.success);
}

TEST_F(NativeGetSelectValueFunctionIntegrationTest, L0_should_return_empty_string_when_value_is_empty_string)
{
    auto* slot = GetSurface();
    ASSERT_NE(slot, nullptr);
    auto message = JsonAdapter::Parse(R"json({
        "components": [{
            "id": "sel_empty",
            "component": "Select",
            "onSelect": [{"call": "selectChanged"}]
        }]
    })json");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot->UpdateComponents(message->GetRoot()));

    auto comp = slot->FindComponentById("sel_empty");
    ASSERT_NE(comp, nullptr);
    auto customComp = std::dynamic_pointer_cast<CustomComponent>(comp);
    ASSERT_NE(customComp, nullptr);

    auto extraContext = ParseJson(R"json({"index": -1, "value": ""})json");
    customComp->DispatchEvent("onSelect", extraContext);

    NativeGetSelectValueFunction func;
    auto args = ParseJson(R"json({"componentId": "sel_empty"})json");
    DynamicResolveContext context;
    context.surfaceId = SURFACE_ID;
    context.componentId = "sel_empty";
    FunctionResult result = func.ExecuteWithContext(args, context);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue("X"), "");
}
