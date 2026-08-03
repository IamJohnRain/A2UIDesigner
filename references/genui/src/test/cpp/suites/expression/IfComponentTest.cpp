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
#include "components/extended/if/IfComponent.h"
#include "composition/ChildListDescriptor.h"
#include "utils/JsonAdapter.h"
#undef private

#include "data/BindingEngine.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SchemaWarningTestHelper.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"
#include "expression/ExpressionEngine.h"

using namespace NativeModule;

namespace {

constexpr int32_t IF_COMPONENT_TEST_RENDER_ID = 920301;

class TestableIfComponent : public IfComponent {
public:
    using IfComponent::IsKnownAdditionalDescriptorKey;
    using IfComponent::ReconcileBranchChildren;
    using IfComponent::SelectBranch;
};

JsonValue StrVal(const std::string& s)
{
    return JsonAdapter::CreateString(s)->GetRoot();
}

JsonValue NumVal(double v)
{
    return JsonAdapter::CreateNumber(v)->GetRoot();
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

SurfaceSlot& CreateManagedSurface(const std::string& surfaceId)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(IF_COMPONENT_TEST_RENDER_ID);
    return renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
}

RenderContext BuildRenderContext(SurfaceSlot& slot)
{
    RenderContext ctx;
    ctx.renderId = slot.GetRenderId();
    ctx.surfaceId = slot.GetSurfaceId();
    ctx.bindingEngine = slot.GetBindingEngine();
    ctx.dataModel = slot.GetOrCreateDataModel();
    return ctx;
}

class NativeViewComponent : public Component {
public:
    explicit NativeViewComponent(ArkUI_NodeHandle nativeView) : Component(nativeView, false, false) {}

    std::string GetType() const override
    {
        return "NativeViewTest";
    }
};

struct PassthroughCallTracker {
    std::vector<std::tuple<ArkUI_NodeHandle, ArkUI_NodeHandle, int32_t>> insertChildAtCalls;
    std::vector<std::pair<ArkUI_NodeHandle, ArkUI_NodeHandle>> removeChildCalls;
};

PassthroughCallTracker g_passthroughTracker;

int32_t PassthroughInsertChildAt(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    g_passthroughTracker.insertChildAtCalls.emplace_back(parent, child, index);
    return 0;
}

int32_t PassthroughRemoveChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    g_passthroughTracker.removeChildCalls.emplace_back(parent, child);
    return 0;
}

class NativeApiTrackerScope {
public:
    NativeApiTrackerScope()
    {
        api_ = reinterpret_cast<ArkUI_NativeNodeAPI_1*>(ArkUINodeApiAdapter::GetNativeNodeAPI());
        if (api_ != nullptr) {
            origInsertChildAt_ = api_->insertChildAt;
            origRemoveChild_ = api_->removeChild;
            api_->insertChildAt = PassthroughInsertChildAt;
            api_->removeChild = PassthroughRemoveChild;
        }
        g_passthroughTracker = {};
    }

    ~NativeApiTrackerScope()
    {
        if (api_ != nullptr) {
            api_->insertChildAt = origInsertChildAt_;
            api_->removeChild = origRemoveChild_;
        }
    }

    bool HasInsertChildAtCall(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index) const
    {
        for (const auto& call : g_passthroughTracker.insertChildAtCalls) {
            if (std::get<0>(call) == parent && std::get<1>(call) == child && std::get<2>(call) == index) {
                return true;
            }
        }
        return false;
    }

    bool HasRemoveChildCall(ArkUI_NodeHandle parent, ArkUI_NodeHandle child) const
    {
        for (const auto& call : g_passthroughTracker.removeChildCalls) {
            if (call.first == parent && call.second == child) {
                return true;
            }
        }
        return false;
    }

    size_t InsertChildAtCallCount() const
    {
        return g_passthroughTracker.insertChildAtCalls.size();
    }

    size_t RemoveChildCallCount() const
    {
        return g_passthroughTracker.removeChildCalls.size();
    }

private:
    ArkUI_NativeNodeAPI_1* api_ = nullptr;
    int32_t (*origInsertChildAt_)(ArkUI_NodeHandle, ArkUI_NodeHandle, int32_t) = nullptr;
    int32_t (*origRemoveChild_)(ArkUI_NodeHandle, ArkUI_NodeHandle) = nullptr;
};

} // namespace

class IfComponentTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        RenderManager::GetInstance().RemoveRenderSlot(IF_COMPONENT_TEST_RENDER_ID);
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(IF_COMPONENT_TEST_RENDER_ID);
        A2UITest::TearDown();
    }
};

TEST_F(IfComponentTest, should_returnCorrectType_when_getType)
{
    IfComponent comp;
    EXPECT_EQ("If", comp.GetType());
}

TEST_F(IfComponentTest, should_succeed_when_initFromDescriptor)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if0", "true", { "childA" }, { "childB" });
    RenderContext ctx;
    bool result = comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(result);
}

TEST_F(IfComponentTest, should_haveNullNativeView_when_constructed)
{
    IfComponent comp;
    EXPECT_EQ(nullptr, comp.GetNativeView());
}

TEST_F(IfComponentTest, should_selectIfBranch_when_conditionTrue)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if1", "true", { "childA", "childB" }, { "childC" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(2u, childIds.size());
    auto it = childIds.begin();
    EXPECT_EQ("childA", *it++);
    EXPECT_EQ("childB", *it);
}

TEST_F(IfComponentTest, should_selectElseBranch_when_conditionFalse)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if2", "false", { "childA" }, { "childC", "childD" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(2u, childIds.size());
    auto it = childIds.begin();
    EXPECT_EQ("childC", *it++);
    EXPECT_EQ("childD", *it);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_conditionIsUnsupportedNonStringType)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<IfComponent>();
    RenderContext ctx = RenderContext::Create(1001, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "ifWarning");
    root.PutString("component", "If");
    root.PutBool("condition", true);

    comp->InitFromDescriptor(root, ctx);

    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "ifWarning.condition"), 1U);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_initialConditionEvaluationFails)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<IfComponent>();
    RenderContext ctx = RenderContext::Create(1002, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    auto descriptor = BuildIfDescriptor("ifEvalWarning", "{{ missingVar }}", { "childA" }, { "childB" });

    comp->InitFromDescriptor(descriptor, ctx);

    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "ifEvalWarning.condition"), 1U);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_conditionEvaluatesToInvalidFalsyNonBoolean)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto verifyCase = [this](const std::string& componentId, const std::string& condition, int32_t renderId) {
        auto comp = std::make_shared<IfComponent>();
        RenderContext ctx =
            RenderContext::Create(renderId, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
        auto descriptor = BuildIfDescriptor(componentId, condition, { "childA" }, { "childB" });

        comp->InitFromDescriptor(descriptor, ctx);

        EXPECT_FALSE(comp->currentBranch_) << condition;
        EXPECT_EQ(
            TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", componentId + ".condition"), 1U)
            << condition;
    };

    verifyCase("ifFalsyZero", "0", 1010);
    verifyCase("ifFalsyEmptyString", "''", 1011);
    verifyCase("ifFalsyNull", "null", 1012);
    verifyCase("ifFalsyUndefined", "undefined", 1013);
    verifyCase("ifFalsyNaN", "NaN", 1014);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_numberConditionFieldIsZero)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<IfComponent>();
    RenderContext ctx = RenderContext::Create(1015, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "ifFalsyNumberFieldZero");
    root.PutString("component", "If");
    root.PutNumber("condition", 0);
    JsonValue childrenIf = root.PutArray("childrenIf");
    childrenIf.Append(StrVal("childA"));
    JsonValue childrenElse = root.PutArray("childrenElse");
    childrenElse.Append(StrVal("childB"));

    comp->InitFromDescriptor(root, ctx);

    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("childB", comp->GetChildListDescriptor().staticChildIds.front());
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "ifFalsyNumberFieldZero.condition"),
        1U);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_childrenIfIsNotArray)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<IfComponent>();
    RenderContext ctx = RenderContext::Create(1003, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "ifChildrenWarning");
    root.PutString("component", "If");
    root.PutString("condition", "true");
    root.PutString("childrenIf", "childA");

    comp->InitFromDescriptor(root, ctx);

    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "ifChildrenWarning.childrenIf"),
        1U);
}

TEST_F(IfComponentTest, should_dispatchSchemaWarning_when_branchChildIdIsMissing)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifMissingChild", "true", { "missingChild" });
    RenderContext ctx = RenderContext::Create(1004, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    std::map<std::string, std::shared_ptr<Component>> allComponents;

    comp->InitFromDescriptor(descriptor, ctx);
    comp->ReconcileBranchChildren(allComponents);

    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "ifMissingChild.childrenIf"), 1U);
}

TEST_F(IfComponentTest, should_defaultToElseBranch_when_emptyCondition)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if3", "", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(1u, childIds.size());
    EXPECT_EQ("childB", childIds.front());
}

TEST_F(IfComponentTest, should_defaultToElseBranch_when_evaluationFails)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if4", "undefinedVar", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_haveEmptyChildList_when_emptyBranch)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if5", "true", {}, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_FALSE(comp->GetChildListDescriptor().IsValid());
}

TEST_F(IfComponentTest, should_evaluateComparison_when_conditionIsExpression)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if6", "1 > 0", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("childA", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_evaluateEquality_when_conditionIsStringComparison)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if7", "'hello' == 'hello'", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_treatNonBooleanAsBool_when_conditionEvaluatesToNonBoolean)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if8", "42", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("childA", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_treatZeroAsFalse_when_conditionEvaluatesToZero)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if9", "0", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("childB", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_storeConditionExpression_when_initialized)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if10", "1 + 1 == 2", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_EQ("1 + 1 == 2", comp->conditionExpression_);
}

TEST_F(IfComponentTest, should_storeChildIdLists_when_initialized)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if11", "true", { "a", "b" }, { "c", "d", "e" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    ASSERT_EQ(2u, comp->childrenIfIds_.size());
    EXPECT_EQ("a", comp->childrenIfIds_.front());
    ASSERT_EQ(3u, comp->childrenElseIds_.size());
    EXPECT_EQ("c", comp->childrenElseIds_.front());
}

TEST_F(IfComponentTest, should_dispatchSchemaWarningAndSelectIfBranch_when_numberConditionFieldIsNonZero)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto comp = std::make_shared<IfComponent>();
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "if12");
    root.PutString("component", "If");
    root.Put("condition", NumVal(42.0));
    JsonValue arr = root.PutArray("childrenIf");
    arr.Append(StrVal("childA"));
    JsonValue elseArr = root.PutArray("childrenElse");
    elseArr.Append(StrVal("childB"));
    RenderContext ctx = RenderContext::Create(1016, "if-warning-surface", nullptr, nullptr, 1.0F, 0, ThemeMode::LIGHT);
    comp->InitFromDescriptor(root, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("childA", comp->GetChildListDescriptor().staticChildIds.front());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "if12.condition"), 1U);
}

TEST_F(IfComponentTest, should_treatChildrenAsEmpty_when_childrenIfIsNotArray)
{
    auto comp = std::make_shared<IfComponent>();
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "if13");
    root.PutString("component", "If");
    root.PutString("condition", "true");
    root.PutString("childrenIf", "not-an-array");
    RenderContext ctx;
    comp->InitFromDescriptor(root, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_FALSE(comp->GetChildListDescriptor().IsValid());
}

TEST_F(IfComponentTest, should_skipNonStringElements_when_childrenIfContainsNonStrings)
{
    auto comp = std::make_shared<IfComponent>();
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "if14");
    root.PutString("component", "If");
    root.PutString("condition", "true");
    JsonValue arr = root.PutArray("childrenIf");
    arr.Append(StrVal("childA"));
    arr.Append(NumVal(42.0));
    arr.Append(StrVal("childC"));
    RenderContext ctx;
    comp->InitFromDescriptor(root, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(2u, childIds.size());
    auto it = childIds.begin();
    EXPECT_EQ("childA", *it++);
    EXPECT_EQ("childC", *it);
}

TEST_F(IfComponentTest, should_collectDependencies_when_conditionHasGlobalVariables)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if15", "$__widthBreakpoint == 'sm'", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    ASSERT_EQ(1u, comp->dependencies_.size());
    EXPECT_EQ("__widthBreakpoint", comp->dependencies_[0].variableName);
}

TEST_F(IfComponentTest, should_evaluateWithEmptyString_when_noConditionField)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if16", "", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("childB", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_alwaysMountSharedId_when_sameIdInBothBranches)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if17", "true", { "sharedChild", "ifOnly" }, { "sharedChild", "elseOnly" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    EXPECT_TRUE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(2u, childIds.size());
    EXPECT_EQ("sharedChild", childIds.front());

    comp->currentBranch_ = false;
    comp->conditionExpression_ = "false";
    comp->ReevaluateAndSwitch();
    EXPECT_FALSE(comp->currentBranch_);
    const auto& elseIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(2u, elseIds.size());
    EXPECT_EQ("sharedChild", elseIds.front());
}

TEST_F(IfComponentTest, should_coerceEmptyStringToFalse_when_jsFalsyRules)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if18", "''", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_coerceNullToFalse_when_jsFalsyRules)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if19", "null", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_coerceNonZeroNumberToTrue_when_jsFalsyRules)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if20", "42", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_coerceNonEmptyStringToTrue_when_jsFalsyRules)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if21", "'hello'", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_maintainDslOrderIndependence_when_childrenOutOfOrder)
{
    auto comp = std::make_shared<IfComponent>();
    auto adapter = JsonAdapter::CreateObject();
    JsonValue root = adapter->GetRoot();
    root.PutString("id", "if22");
    root.PutString("component", "If");
    root.PutString("condition", "true");
    JsonValue arr = root.PutArray("childrenIf");
    arr.Append(StrVal("childZ"));
    arr.Append(StrVal("childA"));
    arr.Append(StrVal("childM"));
    RenderContext ctx;
    comp->InitFromDescriptor(root, ctx);

    EXPECT_TRUE(comp->currentBranch_);
    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(3u, childIds.size());
    auto it = childIds.begin();
    EXPECT_EQ("childZ", *it++);
    EXPECT_EQ("childA", *it++);
    EXPECT_EQ("childM", *it);
}

TEST_F(IfComponentTest, should_handleNestedIf_when_innerIfEvaluates)
{
    auto outerIf = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("outerIf", "true", { "innerIf" }, { "outerElse" });
    RenderContext ctx;
    outerIf->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(outerIf->currentBranch_);
    ASSERT_EQ(1u, outerIf->GetChildListDescriptor().staticChildIds.size());
    EXPECT_EQ("innerIf", outerIf->GetChildListDescriptor().staticChildIds.front());

    auto innerIf = std::make_shared<IfComponent>();
    auto innerDesc = BuildIfDescriptor("innerIf", "false", { "innerIfChild" }, { "innerElseChild" });
    innerIf->InitFromDescriptor(innerDesc, ctx);
    EXPECT_FALSE(innerIf->currentBranch_);
    ASSERT_EQ(1u, innerIf->GetChildListDescriptor().staticChildIds.size());
    EXPECT_EQ("innerElseChild", innerIf->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_delegateOnAddChildToAncestor_when_noNativeView)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if23", "true", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    auto child = std::make_shared<IfComponent>();
    comp->OnAddChild(child, 0);

    EXPECT_EQ(nullptr, comp->GetNativeView());
}

TEST_F(IfComponentTest, should_delegateOnRemoveChildToAncestor_when_noNativeView)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if24", "true", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    auto child = std::make_shared<IfComponent>();
    comp->OnRemoveChild(child);

    EXPECT_EQ(nullptr, comp->GetNativeView());
}

TEST_F(IfComponentTest, should_preserveCurrentBranch_when_subsequentEvaluationFails)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if25", "true", { "ifChild" }, { "elseChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("ifChild", comp->GetChildListDescriptor().staticChildIds.front());

    comp->conditionExpression_ = "undefinedVar == 42";
    comp->ReevaluateAndSwitch();
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("ifChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_switchToElseOnFirstFailure_when_initialEvaluationFails)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if26", "undefinedVar == 42", { "ifChild" }, { "elseChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("elseChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_resolveWidthBreakpointVariable_when_defaultBreakpointSM)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if27", "$__widthBreakpoint == 'sm'", { "smChild" }, { "otherChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("smChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_switchBranch_when_breakpointChangesViaConfigChange)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if28", "$__widthBreakpoint == 'sm'", { "smChild" }, { "lgChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("smChild", comp->GetChildListDescriptor().staticChildIds.front());

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    comp->OnConfigChange(lgContext);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("lgChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_treatWindowBreakpointAliasAsUndefined_when_conditionUsesLegacyAlias)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if29", "$__WindowBreakpoint == 'lg'", { "lgChild" }, { "otherChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("otherChild", comp->GetChildListDescriptor().staticChildIds.front());

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    comp->OnConfigChange(lgContext);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("otherChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_shortCircuit_when_reevaluateReturnsSameBranch)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if30", "1 == 1", { "ifChild" }, { "elseChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    auto originalDescriptor = comp->GetChildListDescriptor();
    comp->ReevaluateAndSwitch();
    EXPECT_TRUE(comp->currentBranch_);

    const auto& childIds = comp->GetChildListDescriptor().staticChildIds;
    ASSERT_EQ(1u, childIds.size());
    EXPECT_EQ("ifChild", childIds.front());
}

TEST_F(IfComponentTest, should_notCrash_when_onRemoveChildWithNullChild)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if31", "true", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    comp->OnRemoveChild(nullptr);
    SUCCEED();
}

TEST_F(IfComponentTest, should_notReevaluate_when_propertyIsSubstringNotExactMatch)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if32", "1 == 1", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    comp->dependencies_.push_back(Dependency { "flag", "" });
    comp->currentBranch_ = false;

    JsonValue dummy = JsonAdapter::CreateObject()->GetRoot();
    comp->OnDataUpdate("flags", dummy);
    EXPECT_FALSE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_reevaluate_when_propertyIsExactMatch)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if33", "1 == 1", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    comp->dependencies_.push_back(Dependency { "flag", "" });
    comp->currentBranch_ = false;

    JsonValue dummy = JsonAdapter::CreateObject()->GetRoot();
    comp->OnDataUpdate("flag", dummy);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_reevaluate_when_propertyIsDotPrefixMatch)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if34", "1 == 1", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    comp->dependencies_.push_back(Dependency { "flag", "" });
    comp->currentBranch_ = false;

    JsonValue dummy = JsonAdapter::CreateObject()->GetRoot();
    comp->OnDataUpdate("flag.value", dummy);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_evaluateCorrectly_when_conditionAlreadyHasBraces)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if35", "{{ 1 == 1 }}", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("childA", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_evaluateElseBranch_when_conditionAlreadyHasBracesAndFalse)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if36", "{{ 1 == 2 }}", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("childB", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_preserveSharedChild_when_branchSwitchReconcile)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if37", "1 == 1", { "shared", "ifOnly" }, { "shared", "elseOnly" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    comp->AddChildAt(sharedChild, 0);
    comp->AddChildAt(ifOnlyChild, 1);
    comp->childIds_ = { "shared", "ifOnly" };

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["shared"] = sharedChild;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->SelectBranch(false);
    comp->ReconcileBranchChildren(allComponents);

    const auto& resultChildren = comp->GetChildren();
    ASSERT_EQ(2u, resultChildren.size());

    std::vector<std::string> resultIds;
    for (const auto& child : resultChildren) {
        if (child) {
            resultIds.push_back(child->GetComponentId());
        }
    }
    ASSERT_EQ(2u, resultIds.size());
    EXPECT_EQ("shared", resultIds[0]);
    EXPECT_EQ("elseOnly", resultIds[1]);

    bool sharedPtrMatches = false;
    for (const auto& child : resultChildren) {
        if (child && child.get() == sharedChild.get()) {
            sharedPtrMatches = true;
            break;
        }
    }
    EXPECT_TRUE(sharedPtrMatches);

    const auto& ids = comp->childIds_;
    ASSERT_EQ(2u, ids.size());
    EXPECT_EQ("shared", ids.front());
    EXPECT_EQ("elseOnly", ids.back());
}

TEST_F(IfComponentTest, should_notRemoveAnyChildren_when_reconcileSameBranch)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if38", "1 == 1", { "childA", "childB" }, { "childC" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    auto childA = std::make_shared<IfComponent>();
    childA->SetComponentId("childA");
    auto childB = std::make_shared<IfComponent>();
    childB->SetComponentId("childB");

    comp->AddChildAt(childA, 0);
    comp->AddChildAt(childB, 1);
    comp->childIds_ = { "childA", "childB" };

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["childA"] = childA;
    allComponents["childB"] = childB;

    comp->SelectBranch(true);
    comp->ReconcileBranchChildren(allComponents);

    const auto& resultChildren = comp->GetChildren();
    ASSERT_EQ(2u, resultChildren.size());
    EXPECT_EQ(childA.get(), resultChildren.front().get());
    EXPECT_EQ(childB.get(), resultChildren.back().get());
}

// ==================== P4-4: global variable update matching ====================

TEST_F(IfComponentTest, should_reevaluate_when_dataModelGlobalVariableChanges)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if39", "1 == 1", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    comp->dependencies_.push_back(Dependency { "__dataModel", "/flag" });
    comp->currentBranch_ = false;

    JsonValue dummy = JsonAdapter::CreateObject()->GetRoot();
    comp->OnDataUpdate("__dataModel", dummy);
    EXPECT_TRUE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_notReevaluate_when_differentGlobalVariableChanges)
{
    auto comp = std::make_shared<IfComponent>();
    auto descriptor = BuildIfDescriptor("if40", "1 == 1", { "childA" }, { "childB" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    comp->dependencies_.push_back(Dependency { "__dataModel", "/flag" });
    comp->currentBranch_ = false;

    JsonValue dummy = JsonAdapter::CreateObject()->GetRoot();
    comp->OnDataUpdate("__widthBreakpoint", dummy);
    EXPECT_FALSE(comp->currentBranch_);
}

TEST_F(IfComponentTest, should_reevaluate_when_widthBreakpointGlobalVariableChanges)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifGlobalWidth", "$__widthBreakpoint == 'lg'", { "lgChild" }, { "otherChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("otherChild", comp->GetChildListDescriptor().staticChildIds.front());

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    comp->lastThemeContext_ = lgContext;
    comp->themeContextValid_ = true;

    comp->OnDataUpdate("__widthBreakpoint", JsonValue());
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("lgChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_reevaluate_when_colorModeGlobalVariableChanges)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifGlobalColor", "$__colorMode == 'dark'", { "darkChild" }, { "lightChild" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_FALSE(comp->currentBranch_);
    EXPECT_EQ("lightChild", comp->GetChildListDescriptor().staticChildIds.front());

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;
    comp->lastThemeContext_ = darkContext;
    comp->themeContextValid_ = true;

    comp->OnDataUpdate("__colorMode", JsonValue());
    EXPECT_TRUE(comp->currentBranch_);
    EXPECT_EQ("darkChild", comp->GetChildListDescriptor().staticChildIds.front());
}

TEST_F(IfComponentTest, should_remountBranch_when_surfaceGlobalVariableNotificationChangesWidthBreakpoint)
{
    SurfaceSlot& slot = CreateManagedSurface("if-surface-width");

    auto comp = std::make_shared<TestableIfComponent>();
    comp->SetComponentId("ifSurfaceWidth");

    auto lgChild = std::make_shared<IfComponent>();
    lgChild->SetComponentId("lgChild");
    auto otherChild = std::make_shared<IfComponent>();
    otherChild->SetComponentId("otherChild");

    auto& components = slot.GetAllComponents();
    components["ifSurfaceWidth"] = comp;
    components["lgChild"] = lgChild;
    components["otherChild"] = otherChild;

    auto descriptor =
        BuildIfDescriptor("ifSurfaceWidth", "$__widthBreakpoint == 'lg'", { "lgChild" }, { "otherChild" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);
    slot.GetBindingEngine()->RegisterComponent(comp);
    comp->ReconcileBranchChildren(components);

    ASSERT_EQ(1u, comp->GetChildren().size());
    EXPECT_EQ("otherChild", comp->GetChildren().front()->GetComponentId());

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    comp->lastThemeContext_ = lgContext;
    comp->themeContextValid_ = true;

    slot.GetBindingEngine()->NotifyGlobalVariableChanged("__widthBreakpoint");

    ASSERT_EQ(1u, comp->GetChildren().size());
    EXPECT_EQ("lgChild", comp->GetChildren().front()->GetComponentId());
}

TEST_F(IfComponentTest, should_switchBranch_when_surfaceDataModelUpdateNotifiesIfDependency)
{
    SurfaceSlot& slot = CreateManagedSurface("if-data-model-surface");

    auto initialData = JsonAdapter::Parse(R"({"value":{"showIf":true}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(slot.UpdateDataModel(initialData->GetRoot()));

    auto comp = std::make_shared<TestableIfComponent>();
    comp->SetSurfaceId(slot.GetSurfaceId());
    comp->SetComponentId("ifDataModel");

    auto ifChild = std::make_shared<IfComponent>();
    ifChild->SetComponentId("ifChild");
    auto elseChild = std::make_shared<IfComponent>();
    elseChild->SetComponentId("elseChild");

    auto& components = slot.GetAllComponents();
    components["ifDataModel"] = comp;
    components["ifChild"] = ifChild;
    components["elseChild"] = elseChild;

    auto descriptor = BuildIfDescriptor("ifDataModel", "$__dataModel.showIf == true", { "ifChild" }, { "elseChild" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);
    slot.GetBindingEngine()->RegisterComponent(comp);
    comp->ReconcileBranchChildren(components);

    ASSERT_EQ(1u, comp->GetChildren().size());
    EXPECT_EQ("ifChild", comp->GetChildren().front()->GetComponentId());

    auto updateData = JsonAdapter::Parse(R"({"path":"/showIf","value":false})");
    ASSERT_NE(updateData, nullptr);
    ASSERT_TRUE(slot.UpdateDataModel(updateData->GetRoot()));

    ASSERT_EQ(1u, comp->GetChildren().size());
    EXPECT_EQ("elseChild", comp->GetChildren().front()->GetComponentId());
}

// ==================== P4-3: AC-11/AC-12 Child retention and re-mount ====================

TEST_F(IfComponentTest, should_retainRemovedChildInAllComponents_when_branchSwitchIfToElse)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if41", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    comp->AddChildAt(ifOnlyChild, 0);
    comp->AddChildAt(sharedChild, 1);
    comp->childIds_ = { "ifOnly", "shared" };

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    std::weak_ptr<Component> ifOnlyWeak(ifOnlyChild);

    comp->SelectBranch(false);
    comp->ReconcileBranchChildren(allComponents);

    EXPECT_FALSE(ifOnlyWeak.expired());

    auto it = allComponents.find("ifOnly");
    ASSERT_NE(allComponents.end(), it);
    EXPECT_EQ(ifOnlyChild.get(), it->second.get());

    const auto& children = comp->GetChildren();
    bool ifOnlyInChildren = false;
    for (const auto& child : children) {
        if (child && child->GetComponentId() == "ifOnly") {
            ifOnlyInChildren = true;
        }
    }
    EXPECT_FALSE(ifOnlyInChildren);
}

TEST_F(IfComponentTest, should_reuseSamePointer_when_switchingBackToIfBranch)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if42", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    comp->AddChildAt(ifOnlyChild, 0);
    comp->AddChildAt(sharedChild, 1);
    comp->childIds_ = { "ifOnly", "shared" };

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    Component* originalIfOnlyPtr = ifOnlyChild.get();

    comp->SelectBranch(false);
    comp->ReconcileBranchChildren(allComponents);

    bool ifOnlyStillInChildren = false;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "ifOnly") {
            ifOnlyStillInChildren = true;
        }
    }
    EXPECT_FALSE(ifOnlyStillInChildren);

    comp->SelectBranch(true);
    comp->ReconcileBranchChildren(allComponents);

    bool ifOnlyRemounted = false;
    Component* remountedPtr = nullptr;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "ifOnly") {
            ifOnlyRemounted = true;
            remountedPtr = child.get();
        }
    }
    EXPECT_TRUE(ifOnlyRemounted);
    EXPECT_EQ(originalIfOnlyPtr, remountedPtr);
}

TEST_F(IfComponentTest, should_reuseSamePointer_when_switchingBackToElseBranch)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if43", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);
    EXPECT_TRUE(comp->currentBranch_);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    comp->AddChildAt(ifOnlyChild, 0);
    comp->AddChildAt(sharedChild, 1);
    comp->childIds_ = { "ifOnly", "shared" };

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    Component* originalElseOnlyPtr = elseOnlyChild.get();

    comp->SelectBranch(false);
    comp->ReconcileBranchChildren(allComponents);

    bool elseOnlyMounted = false;
    Component* mountedPtr = nullptr;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "elseOnly") {
            elseOnlyMounted = true;
            mountedPtr = child.get();
        }
    }
    EXPECT_TRUE(elseOnlyMounted);
    EXPECT_EQ(originalElseOnlyPtr, mountedPtr);

    comp->SelectBranch(true);
    comp->ReconcileBranchChildren(allComponents);

    bool elseOnlyStillInChildren = false;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "elseOnly") {
            elseOnlyStillInChildren = true;
        }
    }
    EXPECT_FALSE(elseOnlyStillInChildren);

    EXPECT_NE(allComponents.end(), allComponents.find("elseOnly"));
    EXPECT_EQ(originalElseOnlyPtr, allComponents["elseOnly"].get());
}

TEST_F(IfComponentTest, should_retainAllChildrenInAllComponents_afterFullRoundTrip)
{
    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("if44", "1 == 1", { "a1", "a2" }, { "b1", "b2" });
    RenderContext ctx;
    comp->InitFromDescriptor(descriptor, ctx);

    auto a1 = std::make_shared<IfComponent>();
    a1->SetComponentId("a1");
    auto a2 = std::make_shared<IfComponent>();
    a2->SetComponentId("a2");

    comp->AddChildAt(a1, 0);
    comp->AddChildAt(a2, 1);
    comp->childIds_ = { "a1", "a2" };

    auto b1 = std::make_shared<IfComponent>();
    b1->SetComponentId("b1");
    auto b2 = std::make_shared<IfComponent>();
    b2->SetComponentId("b2");

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["a1"] = a1;
    allComponents["a2"] = a2;
    allComponents["b1"] = b1;
    allComponents["b2"] = b2;

    std::weak_ptr<Component> a1Weak(a1);
    std::weak_ptr<Component> a2Weak(a2);

    comp->SelectBranch(false);
    comp->ReconcileBranchChildren(allComponents);
    EXPECT_FALSE(a1Weak.expired());
    EXPECT_FALSE(a2Weak.expired());

    comp->SelectBranch(true);
    comp->ReconcileBranchChildren(allComponents);

    EXPECT_EQ(2u, comp->GetChildren().size());
    auto it = comp->GetChildren().begin();
    EXPECT_EQ("a1", (*it)->GetComponentId());
    EXPECT_EQ(a1.get(), it->get());
    ++it;
    EXPECT_EQ("a2", (*it)->GetComponentId());
    EXPECT_EQ(a2.get(), it->get());
}

TEST_F(IfComponentTest, should_recognizeConditionAsKnownField_when_isKnownAdditionalDescriptorKey)
{
    TestableIfComponent comp;
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("condition"));
}

TEST_F(IfComponentTest, should_recognizeChildrenIfAsKnownField_when_isKnownAdditionalDescriptorKey)
{
    TestableIfComponent comp;
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("childrenIf"));
}

TEST_F(IfComponentTest, should_recognizeChildrenElseAsKnownField_when_isKnownAdditionalDescriptorKey)
{
    TestableIfComponent comp;
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("childrenElse"));
}

TEST_F(IfComponentTest, should_rejectUnknownField_when_isKnownAdditionalDescriptorKey)
{
    TestableIfComponent comp;
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey("unknownField"));
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey("fontSize"));
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey(""));
}

// ==================== BR-02: Virtual Node Passthrough (OnAddChild/OnRemoveChild) ====================

TEST_F(IfComponentTest, should_callInsertChildAtOnAncestor_when_onAddChildWithRealViews)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnAddChild(child, 0);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, childView, 0));
    EXPECT_EQ(1u, tracker.InsertChildAtCallCount());
}

TEST_F(IfComponentTest, should_callRemoveChildOnAncestor_when_onRemoveChildWithRealViews)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnRemoveChild(child);

    EXPECT_TRUE(tracker.HasRemoveChildCall(ancestorView, childView));
    EXPECT_EQ(1u, tracker.RemoveChildCallCount());
}

TEST_F(IfComponentTest, should_passCorrectIndex_when_onAddChildWithRealViews)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnAddChild(child, 3);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, childView, 3));
}

TEST_F(IfComponentTest, should_findAncestorThroughMultipleVirtualLevels_when_nestedIf)
{
    NativeApiTrackerScope tracker;

    auto grandparentView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto grandparent = std::make_shared<NativeViewComponent>(grandparentView);
    auto parentIf = std::make_shared<IfComponent>();
    parentIf->SetParent(grandparent);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(parentIf);

    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnAddChild(child, 0);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(grandparentView, childView, 0));
    EXPECT_EQ(1u, tracker.InsertChildAtCallCount());
}

TEST_F(IfComponentTest, should_notCallInsertChildAt_when_childHasNoNativeView)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<IfComponent>();
    ifComp->OnAddChild(child, 0);

    EXPECT_EQ(0u, tracker.InsertChildAtCallCount());
}

TEST_F(IfComponentTest, should_notCallRemoveChild_when_childHasNoNativeView)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<IfComponent>();
    ifComp->OnRemoveChild(child);

    EXPECT_EQ(0u, tracker.RemoveChildCallCount());
}

TEST_F(IfComponentTest, should_notCallInsertChildAt_when_noAncestorWithNativeView)
{
    NativeApiTrackerScope tracker;

    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto ifComp = std::make_shared<IfComponent>();
    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnAddChild(child, 0);

    EXPECT_EQ(0u, tracker.InsertChildAtCallCount());
}

TEST_F(IfComponentTest, should_callBothInsertAndRemove_when_addingThenRemovingWithRealViews)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto childView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto child = std::make_shared<NativeViewComponent>(childView);
    ifComp->OnAddChild(child, 0);
    ifComp->OnRemoveChild(child);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, childView, 0));
    EXPECT_TRUE(tracker.HasRemoveChildCall(ancestorView, childView));
    EXPECT_EQ(1u, tracker.InsertChildAtCallCount());
    EXPECT_EQ(1u, tracker.RemoveChildCallCount());
}

TEST_F(IfComponentTest, should_insertNativeDescendants_when_onAddChildWithVirtualChild)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto firstChildView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);
    auto secondChildView = reinterpret_cast<ArkUI_NodeHandle>(0x3000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto virtualChild = std::make_shared<IfComponent>();
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(firstChildView), 0);
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(secondChildView), 1);

    ifComp->AddChildAt(virtualChild, 0);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, firstChildView, 0));
    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, secondChildView, 1));
    EXPECT_EQ(2u, tracker.InsertChildAtCallCount());
}

TEST_F(IfComponentTest, should_offsetByVirtualNativeDescendants_when_addSiblingAfterVirtualChild)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto firstChildView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);
    auto secondChildView = reinterpret_cast<ArkUI_NodeHandle>(0x3000);
    auto siblingView = reinterpret_cast<ArkUI_NodeHandle>(0x4000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto virtualChild = std::make_shared<IfComponent>();
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(firstChildView), 0);
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(secondChildView), 1);
    ifComp->AddChildAt(virtualChild, 0);

    auto sibling = std::make_shared<NativeViewComponent>(siblingView);
    ifComp->AddChildAt(sibling, 1);

    EXPECT_TRUE(tracker.HasInsertChildAtCall(ancestorView, siblingView, 2));
}

TEST_F(IfComponentTest, should_removeNativeDescendants_when_clearChildrenWithVirtualChild)
{
    NativeApiTrackerScope tracker;

    auto ancestorView = reinterpret_cast<ArkUI_NodeHandle>(0x1000);
    auto firstChildView = reinterpret_cast<ArkUI_NodeHandle>(0x2000);
    auto secondChildView = reinterpret_cast<ArkUI_NodeHandle>(0x3000);

    auto ancestor = std::make_shared<NativeViewComponent>(ancestorView);
    auto ifComp = std::make_shared<IfComponent>();
    ifComp->SetParent(ancestor);

    auto virtualChild = std::make_shared<IfComponent>();
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(firstChildView), 0);
    virtualChild->AddChildAt(std::make_shared<NativeViewComponent>(secondChildView), 1);
    ifComp->AddChildAt(virtualChild, 0);

    ifComp->ClearChildren();

    EXPECT_TRUE(tracker.HasRemoveChildCall(ancestorView, firstChildView));
    EXPECT_TRUE(tracker.HasRemoveChildCall(ancestorView, secondChildView));
    EXPECT_EQ(2u, tracker.RemoveChildCallCount());
}

// ==================== BR-02: ReevaluateAndSwitch with SurfaceSlot (BuildChildren path) ====================

TEST_F(IfComponentTest, should_reconcileChildrenViaReevaluate_when_branchSwitchWithSurfaceSlot)
{
    SurfaceSlot& slot = CreateManagedSurface("if-slot-reconcile");

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifSlot1", "1 == 1", { "shared", "ifOnly" }, { "shared", "elseOnly" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    auto& allComponents = slot.GetAllComponents();
    allComponents["shared"] = sharedChild;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->BuildChildren(slot);
    ASSERT_EQ(2u, comp->GetChildren().size());
    auto it = comp->GetChildren().begin();
    EXPECT_EQ("shared", (*it)->GetComponentId());
    ++it;
    EXPECT_EQ("ifOnly", (*it)->GetComponentId());

    comp->conditionExpression_ = "1 == 2";
    comp->ReevaluateAndSwitch();

    EXPECT_FALSE(comp->currentBranch_);
    ASSERT_EQ(2u, comp->GetChildren().size());
    it = comp->GetChildren().begin();
    EXPECT_EQ("shared", (*it)->GetComponentId());
    ++it;
    EXPECT_EQ("elseOnly", (*it)->GetComponentId());
}

TEST_F(IfComponentTest, should_preserveSharedChildPointer_when_branchSwitchWithSurfaceSlot)
{
    SurfaceSlot& slot = CreateManagedSurface("if-slot-preserve-shared");

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifSlot2", "1 == 1", { "shared", "ifOnly" }, { "shared", "elseOnly" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    auto& allComponents = slot.GetAllComponents();
    allComponents["shared"] = sharedChild;
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->BuildChildren(slot);

    comp->conditionExpression_ = "1 == 2";
    comp->ReevaluateAndSwitch();

    Component* sharedPtrAfter = nullptr;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "shared") {
            sharedPtrAfter = child.get();
        }
    }
    ASSERT_NE(nullptr, sharedPtrAfter);
    EXPECT_EQ(sharedChild.get(), sharedPtrAfter);
}

TEST_F(IfComponentTest, should_reconcileBackAndForth_when_multipleBranchSwitchesWithSurfaceSlot)
{
    SurfaceSlot& slot = CreateManagedSurface("if-slot-roundtrip");

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifSlot3", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    auto& allComponents = slot.GetAllComponents();
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->BuildChildren(slot);
    ASSERT_EQ(2u, comp->GetChildren().size());

    comp->conditionExpression_ = "1 == 2";
    comp->ReevaluateAndSwitch();
    EXPECT_FALSE(comp->currentBranch_);
    ASSERT_EQ(2u, comp->GetChildren().size());
    auto it = comp->GetChildren().begin();
    EXPECT_EQ("elseOnly", (*it)->GetComponentId());
    ++it;
    EXPECT_EQ("shared", (*it)->GetComponentId());

    comp->conditionExpression_ = "1 == 1";
    comp->ReevaluateAndSwitch();
    EXPECT_TRUE(comp->currentBranch_);
    ASSERT_EQ(2u, comp->GetChildren().size());
    it = comp->GetChildren().begin();
    EXPECT_EQ("ifOnly", (*it)->GetComponentId());
    ++it;
    EXPECT_EQ("shared", (*it)->GetComponentId());

    Component* ifOnlyPtr = nullptr;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "ifOnly") {
            ifOnlyPtr = child.get();
        }
    }
    EXPECT_EQ(ifOnlyChild.get(), ifOnlyPtr);
}

// ==================== AC-11/AC-12: Child retention + re-mount via ReevaluateAndSwitch with SurfaceSlot
// ====================

TEST_F(IfComponentTest, should_retainRemovedChildInAllComponents_when_branchSwitchViaReevaluate)
{
    SurfaceSlot& slot = CreateManagedSurface("if-ac11-retain");

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifAc11", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    auto& allComponents = slot.GetAllComponents();
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->BuildChildren(slot);

    comp->conditionExpression_ = "1 == 2";
    comp->ReevaluateAndSwitch();

    bool ifOnlyInChildren = false;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "ifOnly") {
            ifOnlyInChildren = true;
        }
    }
    EXPECT_FALSE(ifOnlyInChildren);

    auto it = allComponents.find("ifOnly");
    ASSERT_NE(allComponents.end(), it);
    EXPECT_EQ(ifOnlyChild.get(), it->second.get());
}

TEST_F(IfComponentTest, should_remountSamePointer_when_switchingBackViaReevaluate)
{
    SurfaceSlot& slot = CreateManagedSurface("if-ac12-remount");

    auto comp = std::make_shared<TestableIfComponent>();
    auto descriptor = BuildIfDescriptor("ifAc12", "1 == 1", { "ifOnly", "shared" }, { "elseOnly", "shared" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    auto ifOnlyChild = std::make_shared<IfComponent>();
    ifOnlyChild->SetComponentId("ifOnly");
    auto sharedChild = std::make_shared<IfComponent>();
    sharedChild->SetComponentId("shared");
    auto elseOnlyChild = std::make_shared<IfComponent>();
    elseOnlyChild->SetComponentId("elseOnly");

    auto& allComponents = slot.GetAllComponents();
    allComponents["ifOnly"] = ifOnlyChild;
    allComponents["shared"] = sharedChild;
    allComponents["elseOnly"] = elseOnlyChild;

    comp->BuildChildren(slot);
    Component* originalIfOnlyPtr = ifOnlyChild.get();

    comp->conditionExpression_ = "1 == 2";
    comp->ReevaluateAndSwitch();

    comp->conditionExpression_ = "1 == 1";
    comp->ReevaluateAndSwitch();

    Component* remountedPtr = nullptr;
    for (const auto& child : comp->GetChildren()) {
        if (child && child->GetComponentId() == "ifOnly") {
            remountedPtr = child.get();
        }
    }
    ASSERT_NE(nullptr, remountedPtr);
    EXPECT_EQ(originalIfOnlyPtr, remountedPtr);
}

// ==================== condition expression binding registration ====================

TEST_F(IfComponentTest, should_registerConditionDataModelBinding_when_surfaceSlotAvailableDuringInit)
{
    SurfaceSlot& slot = CreateManagedSurface("if-binding-surface");

    auto comp = std::make_shared<TestableIfComponent>();
    comp->SetSurfaceId(slot.GetSurfaceId());

    auto descriptor = BuildIfDescriptor("ifBinding1", "$__dataModel.flag == 1", { "ifChild" }, { "elseChild" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    bool foundConditionPathBinding = false;
    for (const auto& binding : comp->GetDataBindings()) {
        if (binding.propertyName_ == "condition" && binding.dataPath_ == "/flag") {
            foundConditionPathBinding = true;
            break;
        }
    }
    EXPECT_TRUE(foundConditionPathBinding);
}

TEST_F(IfComponentTest, should_registerConditionGlobalExpressionBinding_when_surfaceSlotAvailableDuringInit)
{
    SurfaceSlot& slot = CreateManagedSurface("if-binding-global-surface");

    auto comp = std::make_shared<TestableIfComponent>();

    auto descriptor = BuildIfDescriptor("ifBinding2", "$__widthBreakpoint == 'lg'", { "ifChild" }, { "elseChild" });
    RenderContext ctx = BuildRenderContext(slot);
    comp->InitFromDescriptor(descriptor, ctx);

    bool foundConditionGlobalBinding = false;
    for (const auto& binding : comp->GetDataBindings()) {
        if (binding.propertyName_ != "condition" || binding.type_ != BindingType::EXPRESSION) {
            continue;
        }
        for (const auto& dep : binding.globalVarDeps_) {
            if (dep == "__widthBreakpoint") {
                foundConditionGlobalBinding = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundConditionGlobalBinding);
}
