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
#include "components/extended/if/IfComponent.h"
#include "composition/ChildListDescriptor.h"
#include "utils/JsonAdapter.h"
#undef private

#include "../components/A2UIComponentTddTestHelper.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"
#include "expression/DependencyCollector.h"
#include "expression/EvaluationContext.h"
#include "expression/ExpressionEngine.h"

using namespace NativeModule;

namespace {

JsonValue StrVal(const std::string& s)
{
    return JsonAdapter::CreateString(s)->GetRoot();
}

JsonValue BuildIfDescriptor(const std::string& id, const std::string& condition,
    const std::vector<std::string>& childrenIf, const std::vector<std::string>& childrenElse = {})
{
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", id);
    root.PutString("component", "If");
    if (!condition.empty()) {
        root.PutString("condition", condition);
    }
    if (!childrenIf.empty()) {
        JsonValue arr = root.PutArray("childrenIf");
        for (const auto& cid : childrenIf) {
            arr.Append(StrVal(cid));
        }
    }
    if (!childrenElse.empty()) {
        JsonValue arr = root.PutArray("childrenElse");
        for (const auto& cid : childrenElse) {
            arr.Append(StrVal(cid));
        }
    }
    return root;
}

std::shared_ptr<Catalog> CreateExtendedProtocolCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* componentName : { "Column", "Text", "If" }) {
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

bool HasRemoveChildCallForIf(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    for (const auto& call : g_tracker.removeChildCalls) {
        if (call.first == parent && call.second == child) {
            return true;
        }
    }
    return false;
}

} // namespace

class IfComponentIntegrationTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(IfComponentIntegrationTest, should_evaluateConditionAndCollectDeps_when_fullChain)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("int-if1", "true == true", { "ifChild" }, { "elseChild" });

    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    EXPECT_TRUE(comp->currentBranch_);

    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(1u, childIds.size());
    EXPECT_EQ("ifChild", childIds.front());
}

TEST_F(IfComponentIntegrationTest, should_switchBranch_when_reevaluate)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("int-if4", "1 == 1", { "ifChild" }, { "elseChild" });

    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("ifChild", comp->GetChildListDescriptor().staticChildIds.front());

    comp->ReevaluateAndSwitch();
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("ifChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentIntegrationTest, should_handleMultipleVariablesInCondition)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor(
        "int-if5", "$__widthBreakpoint == 'sm' && $__colorMode == 'light'", { "ifChild" }, { "elseChild" });

    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    EXPECT_GE(comp->dependencies_.size(), 2u);

    std::set<std::string> varNames;
    for (const auto& dep : comp->dependencies_) {
        varNames.insert(dep.variableName);
    }
    EXPECT_TRUE(varNames.count("__widthBreakpoint") > 0);
    EXPECT_TRUE(varNames.count("__colorMode") > 0);
}

class IfComponentSurfaceSlotIntegrationTest : public A2UIComponentTddTest {
protected:
    void SetUp() override
    {
        A2UIComponentTddTest::SetUp();
        RenderManager::GetInstance().RemoveRenderSlot(COMPONENT_TDD_RENDER_ID);
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }

    SurfaceSlot& CreateManagedSurface(const std::string& surfaceId)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
        return renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    }
};

TEST_F(IfComponentSurfaceSlotIntegrationTest, should_switchToElse_when_n4DataModelPathUpdateRunsThroughSurfaceSlot)
{
    SurfaceSlot& surfaceSlot = CreateManagedSurface("if-component-manual-surface");
    surfaceSlot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog());

    auto initialData = ParseJson(R"({"value":{"showIf":true}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto componentMessage = ParseJson(R"({"components":[)"
                                      R"({"id":"root","component":"Column","children":["title","if1"]},)"
                                      R"({"id":"title","component":"Text","content":"N4 switch by updateDataModel"},)"
                                      R"({"id":"if1","component":"If","condition":"$__dataModel.showIf == true",)"
                                      R"("childrenIf":["dmIfChild"],"childrenElse":["dmElseChild"]},)"
                                      R"({"id":"dmIfChild","component":"Text","content":"DM_IF_BRANCH"},)"
                                      R"({"id":"dmElseChild","component":"Text","content":"DM_ELSE_AFTER_UPDATE"})"
                                      R"(]})");
    ASSERT_NE(componentMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(componentMessage->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    auto ifNode = std::dynamic_pointer_cast<IfComponent>(surfaceSlot.FindComponentById("if1"));
    auto ifChild = surfaceSlot.FindComponentById("dmIfChild");
    auto elseChild = surfaceSlot.FindComponentById("dmElseChild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(ifNode, nullptr);
    ASSERT_NE(ifChild, nullptr);
    ASSERT_NE(elseChild, nullptr);
    ASSERT_EQ(1u, ifNode->GetChildren().size());
    EXPECT_EQ("dmIfChild", ifNode->GetChildren().front()->GetComponentId());

    auto updateData = ParseJson(R"({"path":"/showIf","value":false})");
    ASSERT_NE(updateData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updateData->GetRoot()));

    ASSERT_EQ(1u, ifNode->GetChildren().size());
    EXPECT_EQ("dmElseChild", ifNode->GetChildren().front()->GetComponentId());
    EXPECT_TRUE(HasRemoveChildCallForIf(root->GetNativeView(), ifChild->GetNativeView()));
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), elseChild->GetNativeView(), 1));
}

TEST_F(IfComponentSurfaceSlotIntegrationTest, should_switchBranch_when_n10SystemColorModeChangesThroughSurfaceManager)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    surfaceManager->UpdateThemeMode(ThemeMode::LIGHT);

    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface("if-component-manual-surface");
    surfaceSlot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog());

    auto componentMessage =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":["title","if1"]},)"
                  R"({"id":"title","component":"Text","content":"N10 $__colorMode follows system colorMode",)"
                  R"("styles":{"fontColor":"#FF0000"}},)"
                  R"({"id":"if1","component":"If","condition":"$__colorMode == 'dark'",)"
                  R"("childrenIf":["darkChild"],"childrenElse":["lightChild"]},)"
                  R"({"id":"darkChild","component":"Text","content":"COLOR_DARK_BRANCH",)"
                  R"("styles":{"fontColor":"#FF0000"}},)"
                  R"({"id":"lightChild","component":"Text","content":"COLOR_LIGHT_BRANCH",)"
                  R"("styles":{"fontColor":"#FF0000"}})"
                  R"(]})");
    ASSERT_NE(componentMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(componentMessage->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    auto ifNode = std::dynamic_pointer_cast<IfComponent>(surfaceSlot.FindComponentById("if1"));
    auto darkChild = surfaceSlot.FindComponentById("darkChild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(ifNode, nullptr);
    ASSERT_NE(darkChild, nullptr);
    ASSERT_EQ(1u, ifNode->GetChildren().size());
    EXPECT_EQ("lightChild", ifNode->GetChildren().front()->GetComponentId());

    surfaceManager->UpdateThemeMode(ThemeMode::DARK);

    ASSERT_EQ(1u, ifNode->GetChildren().size());
    EXPECT_EQ("darkChild", ifNode->GetChildren().front()->GetComponentId());
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), darkChild->GetNativeView(), 1));
}
