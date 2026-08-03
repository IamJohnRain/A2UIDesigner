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
#include <string>
#include <vector>

#include "components/extended/ExtendedStyleResolver.h"
#include "components/extended/RenderContext.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "utils/JsonAdapter.h"

#include "A2UIArkUITypeConverter.h"
#include "ArkUINodeApiAdapter.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::string GetRequestStringProperty(const MockNapiProvider* mockNapi, napi_value object, const char* key)
{
    auto objectIt = mockNapi->objectProperties_.find(object);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return "";
    }
    auto valueIt = objectIt->second.find(key);
    if (valueIt == objectIt->second.end()) {
        return "";
    }
    auto stringIt = mockNapi->stringValues_.find(valueIt->second);
    return stringIt != mockNapi->stringValues_.end() ? stringIt->second : "";
}

double GetRequestNumberProperty(const MockNapiProvider* mockNapi, napi_value object, const char* key)
{
    auto objectIt = mockNapi->objectProperties_.find(object);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return 0.0;
    }
    auto valueIt = objectIt->second.find(key);
    if (valueIt == objectIt->second.end()) {
        return 0.0;
    }
    auto numberIt = mockNapi->numberValues_.find(valueIt->second);
    return numberIt != mockNapi->numberValues_.end() ? numberIt->second : 0.0;
}

// CrossLanguageAttributeBridge keeps its napi callback in private members; this mirror lets the
// fixture clear the singleton between tests so registrations do not leak across cases.
struct CrossLanguageAttributeBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref callbackRef_ = nullptr;
};

void ResetCrossLanguageAttributeBridge()
{
    auto* bridge = reinterpret_cast<CrossLanguageAttributeBridgeMirror*>(&CrossLanguageAttributeBridge::GetInstance());
    bridge->napiEnv_ = nullptr;
    bridge->callbackRef_ = nullptr;
}

// Minimal applier: returns a non-null root handle so ApplyDimension proceeds to the layout-policy
// path. ArkUINodeApiAdapter::GetRootNode() is non-virtual, so the fake root is injected through the
// public constructor's RootNodeGetter callback rather than by subclassing. Native attribute calls
// flow through the mocked ArkUI native provider (A2UITest base).
ArkUI_NodeHandle FakeRootNode()
{
    static ArkUI_Node rootNode {};
    return &rootNode;
}

ArkUINodeApiAdapter MakeFakeRootApplier()
{
    return ArkUINodeApiAdapter([]() { return FakeRootNode(); }, // RootNodeGetter
        []() { return std::string(); },                         // ComponentIdGetter
        [](float, float, float, float) {},                      // EdgeSetter (margin)
        []() {},                                                // ResetAction (resetCommonMargin)
        [](const std::function<void()>&) {});                   // ActionRegistrar (onClick)
}

ConstraintDispatchContext MakeDispatchContext(
    int32_t apiVersion, const std::string& componentId = "child", const std::string& componentType = "Column")
{
    ConstraintDispatchContext context;
    context.renderId = 1;
    context.componentId = componentId;
    context.nodeUniqueId = 10;
    context.componentType = componentType;
    context.apiVersion = apiVersion;
    return context;
}

} // namespace

class LayoutPolicyFallbackTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        ResetCrossLanguageAttributeBridge();
    }

    void TearDown() override
    {
        ResetCrossLanguageAttributeBridge();
        A2UITest::TearDown();
    }

    // Drives width/height through the resolver with the given apiVersion; a bridge callback is
    // registered first so any layoutPolicy dispatch is captured in the mock napi call history.
    void ApplyWithBridge(const std::string& json, int32_t apiVersion, const std::string& componentId = "child",
        const std::string& componentType = "Column")
    {
        CrossLanguageAttributeBridge::GetInstance().RegisterCrossLanguageCallback(env_, CreateCallback());
        auto adapter = JsonAdapter::Parse(json);
        ASSERT_NE(adapter, nullptr);
        auto applier = MakeFakeRootApplier();
        std::vector<DescriptorValidationIssue> issues;
        ExtendedStyleResolver::ResolveAndApply(
            adapter->GetRoot(), applier, MakeDispatchContext(apiVersion, componentId, componentType), issues);
    }

    // Drives the resolver with no dispatch context (nullopt): apiVersion degrades to 0 and isRootNode
    // to false. Exercises the has_value()==false branches of ApplyLayoutPolicyDimension.
    void ApplyWithoutContext(const std::string& json)
    {
        auto adapter = JsonAdapter::Parse(json);
        ASSERT_NE(adapter, nullptr);
        auto applier = MakeFakeRootApplier();
        ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier, std::nullopt);
    }

    bool WasLayoutPolicyDispatched() const
    {
        return WasAttributeDispatched("layoutPolicy");
    }

    int32_t LayoutPolicyDispatchCount() const
    {
        int32_t count = 0;
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (!args.empty() && GetRequestStringProperty(mockNapiPtr_, args[0], "attributeName") == "layoutPolicy") {
                ++count;
            }
        }
        return count;
    }

    // Returns the payloadJson of the most recent layoutPolicy dispatch, or "" when none occurred.
    std::string LastLayoutPolicyPayload() const
    {
        for (auto it = mockNapiPtr_->callFunctionArgsHistory_.rbegin();
             it != mockNapiPtr_->callFunctionArgsHistory_.rend(); ++it) {
            if (it->empty()) {
                continue;
            }
            if (GetRequestStringProperty(mockNapiPtr_, (*it)[0], "attributeName") == "layoutPolicy") {
                return GetRequestStringProperty(mockNapiPtr_, (*it)[0], "payloadJson");
            }
        }
        return "";
    }

    // ===== Native (ArkUI) attribute inspection helpers =====
    // Native set/reset calls flow through the mocked ArkUI native provider and land in these records.

    bool NativeAttributeWasSet(int32_t attribute) const
    {
        for (const auto& record : mockArkUIPtr_->setAttributeRecords_) {
            if (record.attribute == attribute) {
                return true;
            }
        }
        return false;
    }

    // Returns the i32 payload of the most recent setAttribute for the given attribute (e.g. a layout
    // policy value), or -1 when absent. Set via SetInt32Attribute -> { .i32 = value }.
    int32_t NativeAttributeInt32Value(int32_t attribute) const
    {
        for (auto it = mockArkUIPtr_->setAttributeRecords_.rbegin(); it != mockArkUIPtr_->setAttributeRecords_.rend();
             ++it) {
            if (it->attribute == attribute && !it->values.empty()) {
                return it->values.front().i32;
            }
        }
        return -1;
    }

    // Returns the f32 payload of the most recent setAttribute for the given attribute (e.g. a percent
    // ratio), or -1.0 when absent. Set via SetFloatAttribute -> { .f32 = value }.
    float NativeAttributeFloatValue(int32_t attribute) const
    {
        for (auto it = mockArkUIPtr_->setAttributeRecords_.rbegin(); it != mockArkUIPtr_->setAttributeRecords_.rend();
             ++it) {
            if (it->attribute == attribute && !it->values.empty()) {
                return it->values.front().f32;
            }
        }
        return -1.0F;
    }

    bool NativeAttributeWasReset(int32_t attribute) const
    {
        for (const auto& record : mockArkUIPtr_->resetAttributeRecords_) {
            if (record.attribute == attribute) {
                return true;
            }
        }
        return false;
    }

    bool WasAttributeDispatched(const std::string& attributeName) const
    {
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            if (GetRequestStringProperty(mockNapiPtr_, args[0], "attributeName") == attributeName) {
                return true;
            }
        }
        return false;
    }

    std::string LastAttributePayload(const std::string& attributeName) const
    {
        for (auto it = mockNapiPtr_->callFunctionArgsHistory_.rbegin();
             it != mockNapiPtr_->callFunctionArgsHistory_.rend(); ++it) {
            if (it->empty()) {
                continue;
            }
            if (GetRequestStringProperty(mockNapiPtr_, (*it)[0], "attributeName") == attributeName) {
                return GetRequestStringProperty(mockNapiPtr_, (*it)[0], "payloadJson");
            }
        }
        return "";
    }

    napi_env env_ = reinterpret_cast<napi_env>(0x300);

private:
    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        EXPECT_EQ(mockNapiPtr_->CreateFunction(env_, "bridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
        return callback;
    }
};

TEST_F(LayoutPolicyFallbackTest, ButtonBorderRadiusDispatchesToCrossLanguageBridge)
{
    ApplyWithBridge(R"({"borderRadius":{"topLeft":4,"topRight":8,"bottomRight":16,"bottomLeft":24}})",
        MIN_API_VERSION_LAYOUT_POLICY, "button", "Button");

    EXPECT_TRUE(WasAttributeDispatched("borderRadius"));
    auto payloadAdapter = JsonAdapter::Parse(LastAttributePayload("borderRadius"));
    ASSERT_NE(payloadAdapter, nullptr);
    JsonValue payload = payloadAdapter->GetRoot();
    EXPECT_DOUBLE_EQ(payload.GetNumber("topLeft", 0.0), 4.0);
    EXPECT_DOUBLE_EQ(payload.GetNumber("topRight", 0.0), 8.0);
    EXPECT_DOUBLE_EQ(payload.GetNumber("bottomRight", 0.0), 16.0);
    EXPECT_DOUBLE_EQ(payload.GetNumber("bottomLeft", 0.0), 24.0);
}

// ==================== Constant ====================

/**
 * @tc.name: LayoutPolicyFallbackTest001
 * @tc.desc: Verify MIN_API_VERSION_LAYOUT_POLICY constant equals 21.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest001)
{
    EXPECT_EQ(MIN_API_VERSION_LAYOUT_POLICY, 21);
}

// ==================== Old-API dispatch path (0 < apiVersion < 21) ====================

/**
 * @tc.name: LayoutPolicyFallbackTest002
 * @tc.desc: Verify matchParent width dispatches layoutPolicy payload axis=width, policy=0 on old API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest002)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 18);
    EXPECT_TRUE(WasLayoutPolicyDispatched());
    std::string payload = LastLayoutPolicyPayload();
    EXPECT_NE(payload.find(R"("axis":"width")"), std::string::npos);
    EXPECT_NE(payload.find(R"("policy":)" +
                           std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT))),
        std::string::npos);
}

/**
 * @tc.name: LayoutPolicyFallbackTest003
 * @tc.desc: Verify matchParent height dispatches payload axis=height, policy=0.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest003)
{
    ApplyWithBridge(R"({"height": "matchParent"})", 20);
    EXPECT_TRUE(WasLayoutPolicyDispatched());
    std::string payload = LastLayoutPolicyPayload();
    EXPECT_NE(payload.find(R"("axis":"height")"), std::string::npos);
    EXPECT_NE(payload.find(R"("policy":)" +
                           std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT))),
        std::string::npos);
}

/**
 * @tc.name: LayoutPolicyFallbackTest004
 * @tc.desc: Verify wrapContent dispatches payload policy=1 on old API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest004)
{
    ApplyWithBridge(R"({"width": "wrapContent"})", 15);
    EXPECT_TRUE(WasLayoutPolicyDispatched());
    EXPECT_NE(LastLayoutPolicyPayload().find(
                  R"("policy":)" +
                  std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT))),
        std::string::npos);
}

/**
 * @tc.name: LayoutPolicyFallbackTest005
 * @tc.desc: Verify fixAtIdealSize dispatches payload policy=2 on old API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest005)
{
    ApplyWithBridge(R"({"height": "fixAtIdealSize"})", 1);
    EXPECT_TRUE(WasLayoutPolicyDispatched());
    std::string payload = LastLayoutPolicyPayload();
    EXPECT_NE(payload.find(R"("axis":"height")"), std::string::npos);
    EXPECT_NE(payload.find(R"("policy":)" + std::to_string(A2UIArkUITypeConverter::ToArkUILayoutPolicy(
                                                A2UILayoutPolicy::FIX_AT_IDEAL_SIZE))),
        std::string::npos);
}

/**
 * @tc.name: LayoutPolicyFallbackTest006
 * @tc.desc: Verify the dispatched request carries the dispatch context (renderId/nodeUniqueId/componentType).
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest006)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 18);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_.back()[0];
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, request, "renderId"), 1.0);
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, request, "nodeUniqueId"), 10.0);
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, request, "componentType"), "Column");
}

// ==================== Native / legacy paths do not dispatch ====================

/**
 * @tc.name: LayoutPolicyFallbackTest007
 * @tc.desc: Verify apiVersion == 0 takes the native policy path and does not dispatch.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest007)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 0);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
}

/**
 * @tc.name: LayoutPolicyFallbackTest008
 * @tc.desc: Verify apiVersion >= 21 takes the native policy path and does not dispatch.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest008)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 21);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    ApplyWithBridge(R"({"width": "matchParent"})", 25);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
}

/**
 * @tc.name: LayoutPolicyFallbackTest009
 * @tc.desc: Verify percent units take the legacy path and do not dispatch even on old API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest009)
{
    ApplyWithBridge(R"({"width": "50%"})", 18);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
}

/**
 * @tc.name: LayoutPolicyFallbackTest010
 * @tc.desc: Verify absolute (VP) units take the legacy path and do not dispatch even on old API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest010)
{
    ApplyWithBridge(R"({"width": 200})", 18);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
}

/**
 * @tc.name: LayoutPolicyFallbackTest011
 * @tc.desc: Verify width+height both dispatch on old API (two layoutPolicy requests).
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest011)
{
    ApplyWithBridge(R"({"width": "matchParent", "height": "wrapContent"})", 18);
    EXPECT_EQ(LayoutPolicyDispatchCount(), 2);
}

// ==================== Root-node special case (componentId == "root") ====================

/**
 * @tc.name: LayoutPolicyFallbackTest012
 * @tc.desc: Verify MATCH_PARENT width on a root node (componentId=="root") takes the root special
 *           case: SetWidthPercent(1.0F) sets NODE_WIDTH_PERCENT to 1.0, no dispatch, no native
 *           layout policy. Covers the (MATCH_PARENT && isRootNode) true branch and its width side.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest012)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 21, "root");
    // Root special case is taken before the apiVersion check, so nothing is dispatched.
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_WIDTH_PERCENT));
    EXPECT_FLOAT_EQ(NativeAttributeFloatValue(NODE_WIDTH_PERCENT), 1.0F);
    EXPECT_FALSE(NativeAttributeWasSet(NODE_WIDTH_LAYOUTPOLICY));
}

/**
 * @tc.name: LayoutPolicyFallbackTest013
 * @tc.desc: Verify MATCH_PARENT height on a root node takes the root special case: SetHeightPercent
 *           (1.0F) sets NODE_HEIGHT_PERCENT to 1.0. Covers the isWidth==false side of the root case.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest013)
{
    ApplyWithBridge(R"({"height": "matchParent"})", 21, "root");
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_HEIGHT_PERCENT));
    EXPECT_FLOAT_EQ(NativeAttributeFloatValue(NODE_HEIGHT_PERCENT), 1.0F);
    EXPECT_FALSE(NativeAttributeWasSet(NODE_HEIGHT_LAYOUTPOLICY));
}

/**
 * @tc.name: LayoutPolicyFallbackTest014
 * @tc.desc: Verify MATCH_PARENT on a root node short-circuits before apiVersion, so even an old API
 *           (apiVersion<21) does NOT dispatch for the root. Covers the && short-circuit where the
 *           first operand (MATCH_PARENT) and second (isRootNode) are both true, independent of API.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest014)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 18, "root");
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_WIDTH_PERCENT));
    EXPECT_FLOAT_EQ(NativeAttributeFloatValue(NODE_WIDTH_PERCENT), 1.0F);
}

// ==================== Native layout-policy path (apiVersion == 0 or >= 21) ====================

/**
 * @tc.name: LayoutPolicyFallbackTest015
 * @tc.desc: Verify a non-root height on new API (>=21) applies the native height layout policy
 *           (NODE_HEIGHT_LAYOUTPOLICY) with value 1 (WRAP_CONTENT) and does not dispatch. Covers the
 *           native path isWidth==false branch (SetNodeHeightLayoutPolicy).
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest015)
{
    ApplyWithBridge(R"({"height": "wrapContent"})", 21);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_HEIGHT_LAYOUTPOLICY));
    EXPECT_EQ(NativeAttributeInt32Value(NODE_HEIGHT_LAYOUTPOLICY),
        A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT));
}

/**
 * @tc.name: LayoutPolicyFallbackTest016
 * @tc.desc: Verify apiVersion==0 width MATCH_PARENT applies the native width layout policy with
 *           value 0 (MATCH_PARENT). Covers the native path isWidth==true branch with a value check.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest016)
{
    ApplyWithBridge(R"({"width": "matchParent"})", 0);
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_WIDTH_LAYOUTPOLICY));
    EXPECT_EQ(NativeAttributeInt32Value(NODE_WIDTH_LAYOUTPOLICY),
        A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT));
}

/**
 * @tc.name: LayoutPolicyFallbackTest017
 * @tc.desc: Verify a missing dispatch context (nullopt) degrades to apiVersion=0 / isRootNode=
 *           false, so MATCH_PARENT width takes the native layout-policy path without dispatching.
 *           Covers the dispatchContext.has_value()==false branches of ApplyLayoutPolicyDimension.
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest017)
{
    ApplyWithoutContext(R"({"width": "matchParent"})");
    EXPECT_FALSE(WasLayoutPolicyDispatched());
    EXPECT_TRUE(NativeAttributeWasSet(NODE_WIDTH_LAYOUTPOLICY));
    EXPECT_EQ(NativeAttributeInt32Value(NODE_WIDTH_LAYOUTPOLICY),
        A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT));
}

// ==================== Reset path exercises the new layout-policy resets ====================

/**
 * @tc.name: LayoutPolicyFallbackTest018
 * @tc.desc: Verify an invalid width value triggers Reset(WIDTH), which now also resets
 *           NODE_WIDTH_LAYOUTPOLICY. Covers the new ResetNodeWidthLayoutPolicy line in Reset().
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest018)
{
    ApplyWithBridge(R"({"width": "abc"})", 21);
    EXPECT_TRUE(NativeAttributeWasReset(NODE_WIDTH_LAYOUTPOLICY));
}

/**
 * @tc.name: LayoutPolicyFallbackTest019
 * @tc.desc: Verify an invalid height value triggers Reset(HEIGHT), which now also resets
 *           NODE_HEIGHT_LAYOUTPOLICY. Covers the new ResetNodeHeightLayoutPolicy line in Reset().
 * @tc.type: FUNC
 */
TEST_F(LayoutPolicyFallbackTest, LayoutPolicyFallbackTest019)
{
    ApplyWithBridge(R"({"height": "abc"})", 21);
    EXPECT_TRUE(NativeAttributeWasReset(NODE_HEIGHT_LAYOUTPOLICY));
}
