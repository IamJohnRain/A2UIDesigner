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

/**
 * @file ArkUINodeApiAdapterBranchCoverageTest.cpp
 * @brief Branch-coverage tests for the REAL adapter/ArkUINodeApiAdapter.cpp.
 *
 * Compiled into NodeApiAdapterBranchCoverageTest (does NOT link a2ui_lib) so the
 * real adapter is instrumented. TDD_BUILD routes GetNativeNodeAPI() to the mock
 * provider injected by A2UITest::SetUp().
 */

#include <functional>
#include <string>
#include <vector>

// Expose MockArkUINativeProvider::mockNodeAPI_/mockDialogAPI_ (private statics)
// so tests can null out individual function pointers.
#define private public
#include "include/mock_arkui_native_provider.h"
#undef private

#include "A2UIArkUITypes.h"
#include "ArkUINodeApiAdapter.h"
#include "TestFixture.h"

using namespace NativeModule;

// ParseImageObjectFit: every token branch, matrix apiVersion branches, unknown
// fallback, and NormalizeImageObjectFitToken (hyphen/underscore/space/uppercase).
TEST_F(A2UITest, ParseImageObjectFit_AllTokensAndBranches)
{
    struct Case {
        std::string in;
        A2UIObjectFit exp;
    };
    const std::vector<Case> cases = {
        { "contain", A2UIObjectFit::CONTAIN },
        { "cover", A2UIObjectFit::COVER },
        { "auto", A2UIObjectFit::AUTO },
        { "fill", A2UIObjectFit::FILL },
        { "scaledown", A2UIObjectFit::SCALE_DOWN },
        { "none", A2UIObjectFit::NONE },
        { "topstart", A2UIObjectFit::NONE_AND_ALIGN_TOP_START },
        { "top", A2UIObjectFit::NONE_AND_ALIGN_TOP },
        { "topend", A2UIObjectFit::NONE_AND_ALIGN_TOP_END },
        { "start", A2UIObjectFit::NONE_AND_ALIGN_START },
        { "center", A2UIObjectFit::NONE_AND_ALIGN_CENTER },
        { "end", A2UIObjectFit::NONE_AND_ALIGN_END },
        { "bottomstart", A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START },
        { "bottom", A2UIObjectFit::NONE_AND_ALIGN_BOTTOM },
        { "bottomend", A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END },
    };
    for (const auto& c : cases) {
        EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit(c.in, A2UIObjectFit::CONTAIN), c.exp) << "input=" << c.in;
    }
    // unknown token -> fallback
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("bogus", A2UIObjectFit::COVER), A2UIObjectFit::COVER);
    // matrix: apiVersion < 21 -> fallback, >= 21 -> NONE_MATRIX
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("matrix", A2UIObjectFit::FILL, 20), A2UIObjectFit::FILL);
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("matrix", A2UIObjectFit::FILL, 21), A2UIObjectFit::NONE_MATRIX);
    // normalization: hyphen / underscore / surrounding spaces / uppercase
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("Top-Start", A2UIObjectFit::CONTAIN),
        A2UIObjectFit::NONE_AND_ALIGN_TOP_START);
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("COVER", A2UIObjectFit::NONE), A2UIObjectFit::COVER);
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("  contain  ", A2UIObjectFit::NONE), A2UIObjectFit::CONTAIN);
    EXPECT_EQ(ArkUINodeApiAdapter::ParseImageObjectFit("TOP_START", A2UIObjectFit::NONE),
        A2UIObjectFit::NONE_AND_ALIGN_TOP_START);
}

// Thin Set* wrappers: exercising a broad set auto-covers SetFloat/Uint32/Int32/
// Bool/String attribute helpers and the success path of SetAttributeInternal.
TEST_F(A2UITest, SetNode_ThinWrappers_RecordAttributes)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    const auto prevSize = mockArkUIPtr_->setAttributeRecords_.size();

    ArkUINodeApiAdapter::SetNodeWidth(node, 100.0F);
    ArkUINodeApiAdapter::SetNodeHeight(node, 200.0F);
    ArkUINodeApiAdapter::SetNodeWidthPercent(node, 0.5F);
    ArkUINodeApiAdapter::SetNodeHeightPercent(node, 0.5F);
    ArkUINodeApiAdapter::SetNodeFontSize(node, 16.0F);
    ArkUINodeApiAdapter::SetNodeOpacity(node, 0.8F);
    ArkUINodeApiAdapter::SetNodeAspectRatio(node, 1.5F);
    ArkUINodeApiAdapter::SetNodeBackgroundColor(node, 0xFF000000U);
    ArkUINodeApiAdapter::SetNodeFontColor(node, 0xFFFFFFFFU);
    ArkUINodeApiAdapter::SetNodeBorderColor(node, 0x000000FFU);
    ArkUINodeApiAdapter::SetNodeBorderRadius(node, 8.0F);
    ArkUINodeApiAdapter::SetNodeBorderWidth(node, 2.0F);
    ArkUINodeApiAdapter::SetNodeProgressColor(node, 0xFF00FF00U);
    ArkUINodeApiAdapter::SetNodeTextContent(node, "hello");
    ArkUINodeApiAdapter::SetNodeImageSrc(node, "http://x.png");
    ArkUINodeApiAdapter::SetNodeButtonLabel(node, "OK");
    ArkUINodeApiAdapter::SetNodeId(node, "node-1");
    ArkUINodeApiAdapter::SetNodeEnabled(node, true);
    ArkUINodeApiAdapter::SetNodeClip(node, false);
    ArkUINodeApiAdapter::SetNodeVisibility(node, A2UIVisibility::VISIBLE);
    ArkUINodeApiAdapter::SetNodeButtonType(node, A2UIButtonType::NORMAL);
    ArkUINodeApiAdapter::SetNodeFontWeight(node, A2UIFontWeight::BOLD);
    ArkUINodeApiAdapter::SetNodeCheckboxShape(node, A2UICheckboxShape::CIRCLE);
    ArkUINodeApiAdapter::SetNodeCheckboxSelect(node, true);
    ArkUINodeApiAdapter::SetNodeRadioChecked(node, true);
    ArkUINodeApiAdapter::SetNodeToggleValue(node, false);
    ArkUINodeApiAdapter::SetNodeTextInputType(node, A2UITextInputType::NORMAL);
    ArkUINodeApiAdapter::SetNodeTextAlign(node, 0);
    ArkUINodeApiAdapter::SetNodeTextMaxLines(node, 3);
    ArkUINodeApiAdapter::SetNodeLayoutWeight(node, 1);
    ArkUINodeApiAdapter::SetNodeFlexShrink(node, 1.0F);
    ArkUINodeApiAdapter::SetNodeImageObjectFit(node, A2UIObjectFit::CONTAIN);
    ArkUINodeApiAdapter::SetNodeSliderValue(node, 5.0F);
    ArkUINodeApiAdapter::SetNodeProgressValue(node, 50.0F);
    ArkUINodeApiAdapter::SetNodePadding(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeMargin(node, 1.0F, 2.0F, 3.0F, 4.0F);

    EXPECT_GT(mockArkUIPtr_->setAttributeRecords_.size(), prevSize);
}

TEST_F(A2UITest, ResetNode_ThinWrappers_RecordResets)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::ROW);
    ASSERT_NE(node, nullptr);
    const auto prev = mockArkUIPtr_->resetAttributeRecords_.size();

    ArkUINodeApiAdapter::ResetNodeWidth(node);
    ArkUINodeApiAdapter::ResetNodeHeight(node);
    ArkUINodeApiAdapter::ResetNodeBackgroundColor(node);
    ArkUINodeApiAdapter::ResetNodeFontSize(node);
    ArkUINodeApiAdapter::ResetNodeFontColor(node);
    ArkUINodeApiAdapter::ResetNodeBorderWidth(node);
    ArkUINodeApiAdapter::ResetNodeOpacity(node);
    ArkUINodeApiAdapter::ResetNodeVisibility(node);
    ArkUINodeApiAdapter::ResetNodeFontWeight(node);
    ArkUINodeApiAdapter::ResetNodeImageObjectFit(node);
    ArkUINodeApiAdapter::ResetNodeTextContent(node);
    ArkUINodeApiAdapter::ResetNodePadding(node);

    EXPECT_GT(mockArkUIPtr_->resetAttributeRecords_.size(), prev);
}

// SetNodeLinearGradient boundary branches (null colors/stops, count<=0, mismatch).
TEST_F(A2UITest, SetNodeLinearGradient_BoundaryBranches)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    uint32_t colors[] = { 0xFF0000FFU, 0xFFFF0000U };
    float stops[] = { 0.0F, 1.0F };

    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeLinearGradient(node, 90.0F, 0, false, colors, 2, stops, 2), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeLinearGradient(node, 90.0F, 0, false, nullptr, 2, stops, 2), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeLinearGradient(node, 90.0F, 0, false, colors, 2, nullptr, 2), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeLinearGradient(node, 90.0F, 0, false, colors, 0, stops, 0), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeLinearGradient(node, 90.0F, 0, false, colors, 2, stops, 1), -1);
}

// ListNodeAdapter null-handle early return.
TEST_F(A2UITest, SetNodeListNodeAdapter_NullHandle_ReturnsError)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeListNodeAdapter(node, nullptr), -1);
}

// Dialog lifecycle drives created/closed dialog recording.
TEST_F(A2UITest, Dialog_Lifecycle)
{
    A2UINativeDialogHandle handle = ArkUINodeApiAdapter::DialogCreate();
    ASSERT_NE(handle, nullptr);
    EXPECT_FALSE(mockArkUIPtr_->createdDialogs_.empty());

    ArkUI_NodeHandle content = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContent(handle, content), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetModalMode(handle, true), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetAutoCancel(handle, false), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogEnableCustomStyle(handle, false), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogShow(handle, false), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogClose(handle), 0);

    ArkUINodeApiAdapter::DialogDispose(handle);
}

// Instance methods on a valid root node.
TEST_F(A2UITest, InstanceMethods_OnValidRoot)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    ArkUINodeApiAdapter adapter([node]() { return node; }, []() { return std::string("comp-id"); },
        ArkUINodeApiAdapter::EdgeSetter(), []() {}, [](const std::function<void()>&) {});

    EXPECT_EQ(adapter.GetRootNode(), node);
    adapter.SetWidth(100.0F);
    adapter.SetHeight(50.0F);
    adapter.SetBackgroundColor(0xFF112233U);
    adapter.SetBorderRadius(4.0F);
    adapter.SetPadding(1.0F, 2.0F, 3.0F, 4.0F);
    adapter.SetMargin(1.0F, 2.0F, 3.0F, 4.0F);
    adapter.ResetNodeMargin(node); // nodeHandle==GetRootNode() && resetCommonMargin_!=nullptr -> true branch
}

// Instance methods on a null root node -> SetAttributeInternal node==null branch.
TEST_F(A2UITest, InstanceMethods_OnNullRoot)
{
    ArkUINodeApiAdapter adapter([]() { return nullptr; }, []() { return std::string("comp-id"); },
        ArkUINodeApiAdapter::EdgeSetter(), []() {}, [](const std::function<void()>&) {});

    EXPECT_EQ(adapter.GetRootNode(), nullptr);
    adapter.SetWidth(100.0F);
    adapter.SetBackgroundColor(0U);
}

// Static setters with a null node -> SetAttributeInternal/ResetAttributeInternal
// node==null short-circuit branch.
TEST_F(A2UITest, StaticSet_NullNode_ReturnsError)
{
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidth(nullptr, 1.0F), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeBackgroundColor(nullptr, 0U), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeWidth(nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeBackgroundColor(nullptr), -1);
}

// Availability and native API accessors.
TEST_F(A2UITest, IsAvailable_AndNativeApiAccessors)
{
    EXPECT_TRUE(ArkUINodeApiAdapter::IsAvailable());
    EXPECT_NE(ArkUINodeApiAdapter::GetNativeNodeAPI(), nullptr);
    EXPECT_NE(ArkUINodeApiAdapter::GetNativeDialogAPI(), nullptr);
}

// Node tree / event management operations, including null-node guards.
TEST_F(A2UITest, NodeManagement_OperationsAndNullGuards)
{
    ArkUI_NodeHandle parent = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ArkUI_NodeHandle child = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    ASSERT_NE(parent, nullptr);
    ASSERT_NE(child, nullptr);

    EXPECT_EQ(ArkUINodeApiAdapter::AddChild(parent, child), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::AddChild(nullptr, child), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::AddChild(parent, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::InsertChildAt(parent, child, 0), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::InsertChildAt(nullptr, child, 0), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveChild(parent, child), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveChild(nullptr, child), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetUserData(parent, reinterpret_cast<void*>(0x1)), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::SetUserData(nullptr, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::GetUserData(parent), reinterpret_cast<void*>(0x1));
    EXPECT_EQ(ArkUINodeApiAdapter::GetUserData(nullptr), nullptr);

    void (*cb)(A2UINodeEvent*) = [](A2UINodeEvent*) {};
    EXPECT_EQ(ArkUINodeApiAdapter::AddNodeEventReceiver(parent, cb), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::AddNodeEventReceiver(nullptr, cb), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::AddNodeEventReceiver(parent, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveNodeEventReceiver(parent, cb), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveNodeEventReceiver(nullptr, cb), -1);
    const auto evt = static_cast<A2UINodeEventType>(1);
    EXPECT_EQ(ArkUINodeApiAdapter::RegisterNodeEvent(parent, evt, 1, nullptr), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::RegisterNodeEvent(nullptr, evt, 1, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::UnregisterNodeEvent(parent, evt), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::UnregisterNodeEvent(nullptr, evt), -1);

    ArkUINodeApiAdapter::DisposeNode(parent);  // success path
    ArkUINodeApiAdapter::DisposeNode(nullptr); // null guard (void return)
}

// Complex wrappers with internal branch logic (has-* combinations, apiVersion
// guards, ternaries) and a broad set of array-backed setters.
TEST_F(A2UITest, SetNode_ComplexWrappers_AllBranches)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    const auto cancelStyle = static_cast<A2UICancelButtonStyle>(0);

    // SetNodeTextDecoration: every has-* combination path.
    ArkUINodeApiAdapter::SetNodeTextDecoration(node, 1, false, 0U, false, 0, false, 0.0F);
    ArkUINodeApiAdapter::SetNodeTextDecoration(node, 1, true, 0xFF000000U, false, 0, false, 0.0F);
    ArkUINodeApiAdapter::SetNodeTextDecoration(node, 1, false, 0U, true, 1, false, 0.0F);
    ArkUINodeApiAdapter::SetNodeTextDecoration(node, 1, false, 0U, false, 0, true, 0.5F);
    ArkUINodeApiAdapter::SetNodeTextDecoration(node, 1, true, 0xFFU, true, 1, true, 0.5F);

    // SetNodeTextInputCancelButton: every has-* combination path.
    ArkUINodeApiAdapter::SetNodeTextInputCancelButton(node, cancelStyle, false, 0.0F, false, 0U, false, "");
    ArkUINodeApiAdapter::SetNodeTextInputCancelButton(node, cancelStyle, true, 16.0F, false, 0U, false, "");
    ArkUINodeApiAdapter::SetNodeTextInputCancelButton(node, cancelStyle, false, 0.0F, true, 0xFFU, false, "");
    ArkUINodeApiAdapter::SetNodeTextInputCancelButton(node, cancelStyle, true, 16.0F, true, 0xFFU, true, "src");

    // SetNodeCustomShadow: useColorStrategy / fill ternary branches.
    ArkUINodeApiAdapter::SetNodeCustomShadow(node, 5.0F, true, 1.0F, 2.0F, 0, 0xFFU, true);
    ArkUINodeApiAdapter::SetNodeCustomShadow(node, 5.0F, false, 1.0F, 2.0F, 0, 0xFFU, false);

    // Array-backed setters (exercise SetNumberArrayAttribute success path).
    ArkUINodeApiAdapter::SetNodeFlexOption(node, static_cast<A2UIFlexDirection>(0), static_cast<A2UIFlexWrap>(0),
        static_cast<A2UIFlexAlignment>(0), static_cast<A2UIItemAlignment>(0), static_cast<A2UIFlexAlignment>(0));
    ArkUINodeApiAdapter::SetNodeFlexSpace(node, 1.0F, 2.0F);
    ArkUINodeApiAdapter::SetNodeMarginPercent(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodePaddingPercent(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeRadioStyle(node, 1U, 2U, 3U);
    ArkUINodeApiAdapter::SetNodeScrollNestedScroll(
        node, static_cast<A2UIScrollNestedMode>(0), static_cast<A2UIScrollNestedMode>(1));
    ArkUINodeApiAdapter::SetNodeShadow(node, 5.0F, 0xFFU, 1.0F, 2.0F);
    ArkUINodeApiAdapter::SetNodeShadow(node, 1);
    ArkUINodeApiAdapter::SetNodeTextInputUnderlineColor(node, 1U, 2U, 3U, 4U);
    ArkUINodeApiAdapter::SetNodeBorderRadius(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeBorderRadiusPercent(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeBorderWidth(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeBorderWidthPercent(node, 1.0F);
    ArkUINodeApiAdapter::SetNodeConstraintSize(node, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(node, 0xFFU);
    ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(node, 0xFFU, 16.0F, 2.0F);
    ArkUINodeApiAdapter::SetNodeCheckboxMark(node, 0xFFU);
    ArkUINodeApiAdapter::SetNodeCheckboxMark(node, 0xFFU, 16.0F, 2.0F);
    ArkUINodeApiAdapter::SetNodeProgressType(node, 0);
    ArkUINodeApiAdapter::SetNodeProgressTotal(node, 100.0F);

    // LayoutPolicy: apiVersion 0 (set), 0<v<21 (-1), >=21 (set).
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidthLayoutPolicy(node, 1, 0), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidthLayoutPolicy(node, 1, 10), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidthLayoutPolicy(node, 1, 21), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeHeightLayoutPolicy(node, 1, 10), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeHeightLayoutPolicy(node, 1, 21), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeWidthLayoutPolicy(node, 10), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeHeightLayoutPolicy(node, 10), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeWidthLayoutPolicy(node, 21), 0);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeHeightLayoutPolicy(node, 0), 0);
}

// Dialog methods with a null handle -> guard short-circuit branches.
TEST_F(A2UITest, Dialog_NullHandleGuards)
{
    ArkUI_NodeHandle content = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContent(nullptr, content), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContent(nullptr, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContentAlignment(nullptr, static_cast<A2UIAlignment>(0), 0.0F, 0.0F), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetModalMode(nullptr, true), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetAutoCancel(nullptr, false), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogShow(nullptr, false), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogClose(nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogEnableCustomStyle(nullptr, false), -1);
    void (*cb)(A2UIDialogDismissEvent*) = [](A2UIDialogDismissEvent*) {};
    EXPECT_EQ(ArkUINodeApiAdapter::DialogRegisterOnWillDismissWithUserData(nullptr, nullptr, cb), -1);
    ArkUINodeApiAdapter::DialogDispose(nullptr); // null guard (void return)
}

// ResetNodeMargin with a null resetAction -> condition false branch.
TEST_F(A2UITest, InstanceMethods_NullResetAction)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    ArkUINodeApiAdapter adapter([node]() { return node; }, []() { return std::string("id"); },
        ArkUINodeApiAdapter::EdgeSetter(), nullptr, [](const std::function<void()>&) {});
    EXPECT_EQ(adapter.ResetNodeMargin(node), 0); // resetCommonMargin_==nullptr -> false branch
}

// With a null native-API provider, every guard's api==nullptr branch fires.
TEST_F(A2UITest, NativeApiNullProvider_AllGuardsReturnError)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN); // created with full provider
    ASSERT_NE(node, nullptr);
    ArkUINativeAPI::SetProvider(nullptr); // GetNativeNodeAPI/GetNativeDialogAPI now return nullptr

    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidth(node, 1.0F), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeBackgroundColor(node, 0U), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeWidth(node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeBackgroundColor(node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN), nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::AddChild(node, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveChild(node, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::InsertChildAt(node, node, 0), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetUserData(node, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::GetUserData(node), nullptr);
    void (*cb)(A2UINodeEvent*) = [](A2UINodeEvent*) {};
    EXPECT_EQ(ArkUINodeApiAdapter::AddNodeEventReceiver(node, cb), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RegisterNodeEvent(node, static_cast<A2UINodeEventType>(1), 1, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::UnregisterNodeEvent(node, static_cast<A2UINodeEventType>(1)), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogCreate(), nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContent(nullptr, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogClose(nullptr), -1);

    // Restore a live provider: SetProvider(nullptr) destroyed the original mock
    // that mockArkUIPtr_ points at, so A2UITest::TearDown would dereference a
    // dangling pointer. Re-inject a fresh mock and repoint the handle.
    auto restored = MockArkUINativeProvider::Create();
    mockArkUIPtr_ = restored.get();
    ArkUINativeAPI::SetProvider(std::move(restored));
}

// With a non-null API struct whose function pointers are all null, every
// guard's api->fn==nullptr branch fires (covers the last unreachable guard arc).
TEST_F(A2UITest, NullFunctionPointers_AllGuardsReturnError)
{
    // Create node + dialog with the full API first (so handles are valid).
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    A2UINativeDialogHandle dlg = ArkUINodeApiAdapter::DialogCreate();
    ASSERT_NE(node, nullptr);
    ASSERT_NE(dlg, nullptr);
    void (*cb)(A2UINodeEvent*) = [](A2UINodeEvent*) {};
    void (*dcb)(A2UIDialogDismissEvent*) = [](A2UIDialogDismissEvent*) {};

    auto& nodeApi = MockArkUINativeProvider::mockNodeAPI_;
    auto& dialogApi = MockArkUINativeProvider::mockDialogAPI_;
    const ArkUI_NativeNodeAPI_1 savedNodeApi = nodeApi;
    const ArkUI_NativeDialogAPI_1 savedDialogApi = dialogApi;
    nodeApi = ArkUI_NativeNodeAPI_1 {}; // non-null struct, null function pointers
    dialogApi = ArkUI_NativeDialogAPI_1 {};

    EXPECT_EQ(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN), nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeWidth(node, 1.0F), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeBackgroundColor(node, 0U), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::ResetNodeWidth(node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::AddChild(node, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveChild(node, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::InsertChildAt(node, node, 0), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetUserData(node, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::GetUserData(node), nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::AddNodeEventReceiver(node, cb), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RemoveNodeEventReceiver(node, cb), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::RegisterNodeEvent(node, static_cast<A2UINodeEventType>(1), 1, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::UnregisterNodeEvent(node, static_cast<A2UINodeEventType>(1)), -1);
    ArkUINodeApiAdapter::DisposeNode(node); // void, fn-null guard
    EXPECT_EQ(ArkUINodeApiAdapter::DialogCreate(), nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContent(dlg, node), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetContentAlignment(dlg, static_cast<A2UIAlignment>(0), 0.0F, 0.0F), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetModalMode(dlg, true), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogSetAutoCancel(dlg, false), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogShow(dlg, false), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogClose(dlg), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogEnableCustomStyle(dlg, false), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogRegisterOnWillDismissWithUserData(dlg, nullptr, dcb), -1);
    ArkUINodeApiAdapter::DialogDispose(dlg); // void, fn-null guard

    nodeApi = savedNodeApi;
    dialogApi = savedDialogApi;
}

// SetNodeGridNodeAdapter: null vs valid adapter-handle branches.
TEST_F(A2UITest, SetNodeGridNodeAdapter_NullAndValidHandle)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::GRID);
    ASSERT_NE(node, nullptr);
    auto adapterHandle = reinterpret_cast<A2UINodeAdapterHandle>(0x1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeGridNodeAdapter(node, nullptr), -1);
    EXPECT_EQ(ArkUINodeApiAdapter::SetNodeGridNodeAdapter(node, adapterHandle), 0);
}

// RegisterOnClick branches: registrar present (invokes) vs absent (no-op).
TEST_F(A2UITest, InstanceMethods_RegisterOnClickBranches)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN);
    ASSERT_NE(node, nullptr);
    bool clicked = false;
    ArkUINodeApiAdapter withRegistrar([node]() { return node; }, []() { return std::string("id"); },
        ArkUINodeApiAdapter::EdgeSetter(), []() {}, [&](const std::function<void()>&) { clicked = true; });
    withRegistrar.RegisterOnClick([]() {});
    EXPECT_TRUE(clicked);

    ArkUINodeApiAdapter withoutRegistrar(
        []() { return nullptr; }, nullptr, ArkUINodeApiAdapter::EdgeSetter(), []() {}, nullptr);
    withoutRegistrar.RegisterOnClick([]() {}); // onClickRegistrar_==nullptr -> no-op
}

// Additional setters + valid adapter-handle paths.
TEST_F(A2UITest, SetNode_AdditionalWrappers)
{
    ArkUI_NodeHandle node = ArkUINodeApiAdapter::CreateNode(A2UINodeType::IMAGE);
    ASSERT_NE(node, nullptr);
    ArkUINodeApiAdapter::SetNodeBackgroundImageSize(node, 100.0F, 200.0F);
    ArkUINodeApiAdapter::SetNodeBackgroundImageSizeWithStyle(node, 1);
    ArkUINodeApiAdapter::SetNodeListNodeAdapter(node, reinterpret_cast<A2UINodeAdapterHandle>(0x1));
}

// Dialog callback-null guard branch.
TEST_F(A2UITest, Dialog_NullCallbackGuard)
{
    A2UINativeDialogHandle dlg = ArkUINodeApiAdapter::DialogCreate();
    ASSERT_NE(dlg, nullptr);
    EXPECT_EQ(ArkUINodeApiAdapter::DialogRegisterOnWillDismissWithUserData(dlg, nullptr, nullptr), -1);
}
