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

#include "functions/extended/NativeNavigateFunction.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/NavContainerComponent.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

std::shared_ptr<Catalog> BuildExtendedCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto navItem = std::make_shared<CatalogItem>("NavContainer", CatalogItemType::COMPONENT);
    navItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(navItem);
    auto textItem = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    textItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(textItem);
    return catalog;
}

class NativeNavigateFunctionTest : public A2UITest {
protected:
    static constexpr int32_t RENDER_ID = 904;
    static constexpr const char* SURFACE_ID = "navigate_test_surface";

    void SetUp() override
    {
        A2UITest::SetUp();
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(RENDER_ID);
        SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(SURFACE_ID);
        surface.SetCatalog(BuildExtendedCatalog());
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

} // namespace

/**
 * @tc.name: navigate 函数名
 * @tc.desc: 覆盖 GetName 分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_navigate_name)
{
    NativeNavigateFunction func;
    EXPECT_EQ(func.GetName(), "navigate");
}

/**
 * @tc.name: navigate 直接调用回退
 * @tc.desc: 覆盖直接 Execute 早退分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_for_direct_execute)
{
    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"nav","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);

    EXPECT_FALSE(func.Execute(args->GetRoot()).GetBoolValue(true));
}

/**
 * @tc.name: navigate surface 缺失
 * @tc.desc: 覆盖 surface 不存在时的失败分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_surface_is_missing)
{
    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"nav","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = "missing_surface";

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 非对象入参
 * @tc.desc: 覆盖 componentId/targetComponentId 解析函数的非对象早退分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_args_are_not_object)
{
    NativeNavigateFunction func;
    auto args = ParseJson(R"("not_object")");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 非字符串字段
 * @tc.desc: 覆盖 componentId/targetComponentId 非字符串时的早退分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_args_fields_are_not_string)
{
    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":123,"targetComponentId":true})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 目标 id 为空
 * @tc.desc: 覆盖 targetComponentId 为空时的短路分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_target_component_id_is_empty)
{
    auto* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    auto message = ParseJson(R"({
        "components": [
            {"id":"nav","component":"NavContainer","children":["page-a"],"currentIndex":0},
            {"id":"page-a","component":"Text","text":"A"}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface->UpdateComponents(message->GetRoot()));

    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"nav","targetComponentId":""})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 组件 id 为空
 * @tc.desc: 覆盖 componentId 为空时的短路分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_component_id_is_empty)
{
    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 非 NavContainer
 * @tc.desc: 覆盖目标组件类型不匹配时的失败分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_target_component_is_not_nav_container)
{
    auto* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    auto message = ParseJson(R"({
        "components": [
            {"id":"root","component":"Text","text":"hello"}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface->UpdateComponents(message->GetRoot()));

    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"root","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 目标子节点缺失
 * @tc.desc: 覆盖目标子节点找不到时的失败分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_return_false_when_target_child_is_missing)
{
    auto* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    auto message = ParseJson(R"({
        "components": [
            {"id":"nav","component":"NavContainer","children":["page-a"],"currentIndex":0},
            {"id":"page-a","component":"Text","text":"A"}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface->UpdateComponents(message->GetRoot()));

    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"nav","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    EXPECT_FALSE(func.ExecuteWithContext(args->GetRoot(), context).GetBoolValue(true));
}

/**
 * @tc.name: navigate 成功切换
 * @tc.desc: 覆盖目标子节点存在时的成功切换与可见性更新分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_navigate_to_target_child_and_update_visibility)
{
    auto* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    auto message = ParseJson(R"({
        "components": [
            {"id":"nav","component":"NavContainer","children":["page-a","page-b"],"currentIndex":0},
            {"id":"page-a","component":"Text","text":"A"},
            {"id":"page-b","component":"Text","text":"B"}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface->UpdateComponents(message->GetRoot()));

    NativeNavigateFunction func;
    auto args = ParseJson(R"({"componentId":"nav","targetComponentId":"page-b"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context;
    context.renderId = RENDER_ID;
    context.surfaceId = SURFACE_ID;

    FunctionResult result = func.ExecuteWithContext(args->GetRoot(), context);
    EXPECT_TRUE(result.GetBoolValue(false));

    auto nav = std::dynamic_pointer_cast<NavContainerComponent>(surface->FindComponentById("nav"));
    ASSERT_NE(nav, nullptr);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
}

/**
 * @tc.name: navigate 注册校验
 * @tc.desc: 覆盖函数注册表中 navigate 的存在性分支。
 * @tc.type: FUNC
 */
TEST_F(NativeNavigateFunctionTest, L0_should_be_registered_in_registry)
{
    EXPECT_TRUE(NativeFunctionRegistry::GetInstance().HasFunction("navigate"));
}
