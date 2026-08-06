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

#include <cfloat>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>

#include "components/Component.h"
#include "components/extended/ExtendedStyleResolver.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

extern "C" void SetStubDisplayDensityPixels(float densityPixels);

const MockArkUINativeProvider::SetAttributeRecord* FindLastConstraintRecord(
    const MockArkUINativeProvider* provider, ArkUI_NodeHandle node)
{
    for (auto iterator = provider->setAttributeRecords_.rbegin(); iterator != provider->setAttributeRecords_.rend();
         ++iterator) {
        if (iterator->nodeHandle == node && iterator->attribute == NODE_CONSTRAINT_SIZE) {
            return &(*iterator);
        }
    }
    return nullptr;
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeRecord(
    const MockArkUINativeProvider* provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto iterator = provider->setAttributeRecords_.rbegin(); iterator != provider->setAttributeRecords_.rend();
         ++iterator) {
        if (iterator->nodeHandle == node && iterator->attribute == attribute) {
            return &(*iterator);
        }
    }
    return nullptr;
}

class ConstraintSizePercentTddTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        applier_ = std::make_unique<ArkUINodeApiAdapter>([this]() { return testNode_; },
            []() { return std::string("constraint-test"); }, ArkUINodeApiAdapter::EdgeSetter(), []() {},
            [](const std::function<void()>&) {});
    }

    void TearDown() override
    {
        applier_.reset();
        SetStubDisplayDensityPixels(1.0F);
        A2UITest::TearDown();
    }

    void Apply(const char* styleJson, int32_t apiVersion = 23)
    {
        ConstraintDispatchContext context;
        context.renderId = 1;
        context.componentId = "constraint-test";
        context.nodeUniqueId = 101;
        context.componentType = "Row";
        context.apiVersion = apiVersion;
        auto styles = JsonAdapter::Parse(styleJson);
        ASSERT_NE(styles, nullptr);
        ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), *applier_, context);
    }

    ArkUI_NodeHandle GetWrapper() const
    {
        if (mockArkUIPtr_->createdNodes_.empty()) {
            return nullptr;
        }
        return mockArkUIPtr_->createdNodes_.back().nodeHandle;
    }

    ArkUI_NodeHandle testNode_ = reinterpret_cast<ArkUI_NodeHandle>(0xA580);
    std::unique_ptr<ArkUINodeApiAdapter> applier_;
};

TEST_F(ConstraintSizePercentTddTest, should_resolve_percent_values_against_measure_percent_reference)
{
    Apply(R"({"constraintSize":{"minWidth":"10%","maxWidth":"80%","minHeight":"20%","maxHeight":"90%"}})");

    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_EQ(mockArkUIPtr_->createdNodes_.back().nodeType, ARKUI_NODE_CUSTOM);
    ASSERT_EQ(mockArkUIPtr_->nodeParents_[testNode_], wrapper);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[0].f32, 100.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, 800.0F);
    EXPECT_FLOAT_EQ(record->values[2].f32, 100.0F);
    EXPECT_FLOAT_EQ(record->values[3].f32, 450.0F);
}

TEST_F(ConstraintSizePercentTddTest, should_recalculate_when_measure_percent_reference_changes)
{
    Apply(R"({"constraintSize":{"minWidth":"10%","maxWidth":"80%","minHeight":"20%","maxHeight":"90%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 800, 400));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[0].f32, 80.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, 640.0F);
    EXPECT_FLOAT_EQ(record->values[2].f32, 80.0F);
    EXPECT_FLOAT_EQ(record->values[3].f32, 360.0F);
}

TEST_F(ConstraintSizePercentTddTest, should_convert_px_reference_to_vp_before_resolving_percent)
{
    SetStubDisplayDensityPixels(2.0F);
    Apply(R"({"constraintSize":{"minWidth":"10%","maxWidth":"80%","minHeight":"20%","maxHeight":"90%"}})");

    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[0].f32, 50.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, 400.0F);
    EXPECT_FLOAT_EQ(record->values[2].f32, 50.0F);
    EXPECT_FLOAT_EQ(record->values[3].f32, 225.0F);
}

TEST_F(ConstraintSizePercentTddTest, should_resolve_mixed_vp_fp_and_percent_values)
{
    Apply(R"({"constraintSize":{"minWidth":"16fp","maxWidth":"80%","minHeight":"20vp","maxHeight":"90%"}})");

    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[0].f32, 16.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, 800.0F);
    EXPECT_FLOAT_EQ(record->values[2].f32, 20.0F);
    EXPECT_FLOAT_EQ(record->values[3].f32, 450.0F);
}

TEST_F(ConstraintSizePercentTddTest, should_not_repeat_native_attribute_when_resolved_values_are_unchanged)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    const size_t recordCount = mockArkUIPtr_->setAttributeRecords_.size();

    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    EXPECT_EQ(mockArkUIPtr_->setAttributeRecords_.size(), recordCount);
}

TEST_F(ConstraintSizePercentTddTest, should_keep_transparent_wrapper_when_constraint_size_is_reset)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ExtendedStyleResolver::Reset(
        { .rawName = "constraintSize", .name = StylePropertyName::CONSTRAINT_SIZE }, *applier_, 23);
    mockArkUIPtr_->setAttributeRecords_.clear();
    mockArkUIPtr_->measuredSizes_[testNode_] = { 320, 180 };

    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    EXPECT_EQ(FindLastConstraintRecord(mockArkUIPtr_, testNode_), nullptr);
    EXPECT_EQ(mockArkUIPtr_->measuredSizes_[wrapper].width, 320);
    EXPECT_EQ(mockArkUIPtr_->measuredSizes_[wrapper].height, 180);
}

TEST_F(ConstraintSizePercentTddTest, should_clear_measure_subscription_when_absolute_values_override_percent)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);

    Apply(R"({"constraintSize":{"maxWidth":"240vp"}})");

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[1].f32, 240.0F);
    mockArkUIPtr_->setAttributeRecords_.clear();
    EXPECT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    EXPECT_EQ(FindLastConstraintRecord(mockArkUIPtr_, testNode_), nullptr);
}

TEST_F(ConstraintSizePercentTddTest, should_use_field_default_when_percent_resolution_overflows)
{
    Apply(R"({"constraintSize":{"minWidth":"1e38%","maxWidth":"1e38%"}})");

    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, std::numeric_limits<int32_t>::max(), 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, FLT_MAX);
}

TEST_F(ConstraintSizePercentTddTest, should_use_native_measure_path_on_minimum_supported_api)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})", 13);

    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    EXPECT_FLOAT_EQ(record->values[1].f32, 800.0F);
}

TEST_F(ConstraintSizePercentTddTest, should_measure_and_layout_content_through_transparent_wrapper)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);
    mockArkUIPtr_->measuredSizes_[testNode_] = { 320, 180 };

    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    ASSERT_FALSE(mockArkUIPtr_->measuredNodes_.empty());
    EXPECT_EQ(mockArkUIPtr_->measuredNodes_.back(), testNode_);
    EXPECT_EQ(mockArkUIPtr_->measuredSizes_[wrapper].width, 320);
    EXPECT_EQ(mockArkUIPtr_->measuredSizes_[wrapper].height, 180);

    ASSERT_TRUE(mockArkUIPtr_->DispatchLayoutEvent(wrapper, 40, 60));
    ASSERT_GE(mockArkUIPtr_->layoutRecords_.size(), 2U);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_[mockArkUIPtr_->layoutRecords_.size() - 2].nodeHandle, wrapper);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_[mockArkUIPtr_->layoutRecords_.size() - 2].x, 40);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_[mockArkUIPtr_->layoutRecords_.size() - 2].y, 60);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_.back().nodeHandle, testNode_);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_.back().x, 0);
    EXPECT_EQ(mockArkUIPtr_->layoutRecords_.back().y, 0);
}

TEST_F(ConstraintSizePercentTddTest, should_expose_wrapper_only_as_component_mount_node)
{
    Component component(testNode_, false);
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);

    EXPECT_EQ(component.GetNativeView(), wrapper);
    EXPECT_EQ(component.GetHandle(), testNode_);
}

TEST_F(ConstraintSizePercentTddTest, should_preserve_parent_layout_attributes_on_wrapper)
{
    ASSERT_EQ(ArkUINodeApiAdapter::SetNodeMargin(testNode_, 8.0F, 12.0F, 16.0F, 20.0F), 0);
    ASSERT_EQ(ArkUINodeApiAdapter::SetNodeLayoutWeight(testNode_, 2U), 0);

    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);

    const auto* margin = FindLastAttributeRecord(mockArkUIPtr_, wrapper, NODE_MARGIN);
    ASSERT_NE(margin, nullptr);
    ASSERT_EQ(margin->values.size(), 4U);
    EXPECT_FLOAT_EQ(margin->values[0].f32, 8.0F);
    EXPECT_FLOAT_EQ(margin->values[3].f32, 20.0F);
    const auto* weight = FindLastAttributeRecord(mockArkUIPtr_, wrapper, NODE_LAYOUT_WEIGHT);
    ASSERT_NE(weight, nullptr);
    ASSERT_EQ(weight->values.size(), 1U);
    EXPECT_EQ(weight->values[0].u32, 2U);

    ASSERT_EQ(ArkUINodeApiAdapter::SetNodeFlexShrink(testNode_, 0.5F), 0);
    const auto* contentShrink = FindLastAttributeRecord(mockArkUIPtr_, testNode_, NODE_FLEX_SHRINK);
    const auto* wrapperShrink = FindLastAttributeRecord(mockArkUIPtr_, wrapper, NODE_FLEX_SHRINK);
    ASSERT_NE(contentShrink, nullptr);
    ASSERT_NE(wrapperShrink, nullptr);
    EXPECT_FLOAT_EQ(contentShrink->values[0].f32, 0.5F);
    EXPECT_FLOAT_EQ(wrapperShrink->values[0].f32, 0.5F);
}

TEST_F(ConstraintSizePercentTddTest, should_release_wrapper_events_and_node_with_applier_lifecycle)
{
    Apply(R"({"constraintSize":{"maxWidth":"80%"}})");
    ArkUI_NodeHandle wrapper = GetWrapper();
    ASSERT_NE(wrapper, nullptr);

    applier_.reset();

    EXPECT_FALSE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));
    EXPECT_NE(std::find(mockArkUIPtr_->disposedNodes_.begin(), mockArkUIPtr_->disposedNodes_.end(), wrapper),
        mockArkUIPtr_->disposedNodes_.end());
    EXPECT_EQ(mockArkUIPtr_->nodeParents_.find(testNode_), mockArkUIPtr_->nodeParents_.end());
}

} // namespace
