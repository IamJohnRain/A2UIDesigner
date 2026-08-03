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

#include <array>
#include <gtest/gtest.h>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "components/extended/ExtendedStyleResolver.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleParser.h"
#include "utils/JsonAdapter.h"

#include "A2UIArkUITypeConverter.h"
#include "mock_arkui_native_provider.h"

using namespace NativeModule;

namespace {

MockArkUINativeProvider& GetRecordingProvider()
{
    MockArkUINativeProvider* activeProvider = MockArkUINativeProvider::GetActiveInstance();
    return activeProvider != nullptr ? *activeProvider : MockArkUINativeProvider::GetInstance();
}

class RecordingCommonStyleApplier : public ArkUINodeApiAdapter {
public:
    RecordingCommonStyleApplier()
        : ArkUINodeApiAdapter([this]() { return GetRootNode(); }, []() { return std::string(); },
              ArkUINodeApiAdapter::EdgeSetter(), []() {}, [](const std::function<void()>&) {})
    {
        const auto& provider = GetRecordingProvider();
        setAttributeRecordStart_ = provider.setAttributeRecords_.size();
        resetAttributeRecordStart_ = provider.resetAttributeRecords_.size();
    }

    void SetReturnNullRootNode(bool value)
    {
        returnNullRootNode_ = value;
    }

    ArkUI_NodeHandle GetRootNode() const
    {
        if (returnNullRootNode_) {
            return nullptr;
        }
        return const_cast<ArkUI_Node*>(&rootNode_);
    }

    void SetWidth(float width)
    {
        width_ = width;
        hasWidth_ = true;
    }

    void SetHeight(float height)
    {
        static_cast<void>(height);
        hasHeight_ = true;
    }

    void SetWidthPercent(float percent)
    {
        widthPercent_ = percent;
        hasWidthPercent_ = true;
    }

    void SetHeightPercent(float percent)
    {
        heightPercent_ = percent;
        hasHeightPercent_ = true;
    }

    void SetBackgroundColor(uint32_t color)
    {
        static_cast<void>(color);
    }

    void SetBorderRadius(float radius)
    {
        borderRadius_ = radius;
        hasBorderRadius_ = true;
    }

    void SetBorderRadiusPercent(float topLeft, float topRight, float bottomRight, float bottomLeft)
    {
        borderRadiusPercent_ = { topLeft, topRight, bottomRight, bottomLeft };
        hasBorderRadiusPercent_ = true;
    }

    void SetPadding(float top, float right, float bottom, float left)
    {
        hasPadding_ = true;
    }

    void SetPaddingPercent(float top, float right, float bottom, float left)
    {
        hasPaddingPercent_ = true;
    }

    void SetMargin(float top, float right, float bottom, float left)
    {
        hasMargin_ = true;
    }

    void SetMarginPercent(float top, float right, float bottom, float left)
    {
        marginPercent_ = { top, right, bottom, left };
        hasMarginPercent_ = true;
    }

    void SetBorderWidthPercent(float width)
    {
        borderWidthPercent_ = width;
        hasBorderWidthPercent_ = true;
    }

    void SetNodeFloat(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, float value)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_FLEX_SHRINK) {
            hasFlexShrink_ = true;
            flexShrink_ = value;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_BORDER_WIDTH) {
            hasBorderWidth_ = true;
            borderWidth_ = value;
        }
    }

    void SetNodeInt32(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, int32_t value)
    {
        if (nodeHandle != GetRootNode()) {
            return;
        }
        if (attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            hasBackgroundImageSizeWithStyle_ = true;
            backgroundImageSizeWithStyle_ = value;
        }
    }

    void SetNodeUint32(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, uint32_t value)
    {
        static_cast<void>(nodeHandle);
        static_cast<void>(attribute);
        static_cast<void>(value);
    }

    void SetNodeBool(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, bool value)
    {
        static_cast<void>(nodeHandle);
        static_cast<void>(attribute);
        static_cast<void>(value);
    }

    void SetNodeString(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const std::string& value)
    {
        static_cast<void>(nodeHandle);
        static_cast<void>(attribute);
        static_cast<void>(value);
    }

    void SetNodeNumberArray(
        ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const std::vector<ArkUI_NumberValue>& values)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            hasBackgroundImageSize_ = true;
            backgroundImageSizeValues_ = values;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_CONSTRAINT_SIZE) {
            hasConstraintSize_ = true;
            constraintSizeValues_ = values;
        }
    }

    void SetLinearGradient(ArkUI_NodeHandle nodeHandle, const StyleLinearGradient& gradient)
    {
        static_cast<void>(nodeHandle);
        static_cast<void>(gradient);
    }

    void ResetNodeAttribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute)
    {
        if (nodeHandle == GetRootNode()) {
            resetAttributes_.insert(attribute);
        }
    }

    void RegisterOnClick(const std::function<void()>& onClick)
    {
        static_cast<void>(onClick);
    }

    bool HasWidth() const
    {
        return hasWidth_ || HasRecordedAttribute(NODE_WIDTH);
    }

    float GetWidth() const
    {
        const auto* record = FindLastSetAttribute(NODE_WIDTH);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return width_;
    }

    bool HasWidthPercent() const
    {
        return hasWidthPercent_ || HasRecordedAttribute(NODE_WIDTH_PERCENT);
    }

    float GetWidthPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_WIDTH_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return widthPercent_;
    }

    bool HasFlexShrink() const
    {
        return hasFlexShrink_ || HasRecordedAttribute(NODE_FLEX_SHRINK);
    }

    bool HasFontWeight() const
    {
        return HasRecordedAttribute(NODE_FONT_WEIGHT);
    }

    int32_t GetFontWeight() const
    {
        const auto* record = FindLastSetAttribute(NODE_FONT_WEIGHT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32;
        }
        return 0;
    }

    bool HasBorderRadiusPercent() const
    {
        return hasBorderRadiusPercent_ || HasRecordedAttribute(NODE_BORDER_RADIUS_PERCENT);
    }

    float GetBorderRadiusPercentTopLeft() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS_PERCENT);
        if (record != nullptr && record->values.size() >= 1) {
            return record->values[0].f32;
        }
        return borderRadiusPercent_[0];
    }

    float GetBorderRadiusPercentTopRight() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS_PERCENT);
        if (record != nullptr && record->values.size() >= 2) {
            return record->values[1].f32;
        }
        return borderRadiusPercent_[1];
    }

    float GetBorderRadiusPercentBottomRight() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS_PERCENT);
        if (record != nullptr && record->values.size() >= 3) {
            return record->values[2].f32;
        }
        return borderRadiusPercent_[2];
    }

    float GetBorderRadiusPercentBottomLeft() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS_PERCENT);
        if (record != nullptr && record->values.size() >= 4) {
            return record->values[3].f32;
        }
        return borderRadiusPercent_[3];
    }

    bool HasBorderRadius() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS);
        return hasBorderRadius_ || (record != nullptr && record->values.size() == 1);
    }

    float GetBorderRadius() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_RADIUS);
        if (record != nullptr && record->values.size() == 1) {
            return record->values.front().f32;
        }
        return borderRadius_;
    }

    bool HasMarginPercent() const
    {
        return hasMarginPercent_ || HasRecordedAttribute(NODE_MARGIN_PERCENT);
    }

    float GetMarginPercentTop() const
    {
        const auto* record = FindLastSetAttribute(NODE_MARGIN_PERCENT);
        if (record != nullptr && record->values.size() >= 1) {
            return record->values[0].f32;
        }
        return marginPercent_[0];
    }

    float GetMarginPercentRight() const
    {
        const auto* record = FindLastSetAttribute(NODE_MARGIN_PERCENT);
        if (record != nullptr && record->values.size() >= 2) {
            return record->values[1].f32;
        }
        return marginPercent_[1];
    }

    float GetMarginPercentBottom() const
    {
        const auto* record = FindLastSetAttribute(NODE_MARGIN_PERCENT);
        if (record != nullptr && record->values.size() >= 3) {
            return record->values[2].f32;
        }
        return marginPercent_[2];
    }

    float GetMarginPercentLeft() const
    {
        const auto* record = FindLastSetAttribute(NODE_MARGIN_PERCENT);
        if (record != nullptr && record->values.size() >= 4) {
            return record->values[3].f32;
        }
        return marginPercent_[3];
    }

    bool HasMargin() const
    {
        return hasMargin_ || HasRecordedAttribute(NODE_MARGIN);
    }

    bool HasBorderWidthPercent() const
    {
        return hasBorderWidthPercent_ || HasRecordedAttribute(NODE_BORDER_WIDTH_PERCENT);
    }

    float GetBorderWidthPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_WIDTH_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return borderWidthPercent_;
    }

    bool HasBorderWidth() const
    {
        return hasBorderWidth_ || HasRecordedAttribute(NODE_BORDER_WIDTH);
    }

    float GetBorderWidthValue() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_WIDTH);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return borderWidth_;
    }

    bool HasPadding() const
    {
        return hasPadding_ || HasRecordedAttribute(NODE_PADDING);
    }

    bool HasPaddingPercent() const
    {
        return hasPaddingPercent_ || HasRecordedAttribute(NODE_PADDING_PERCENT);
    }

    bool HasConstraintSize() const
    {
        return hasConstraintSize_ || HasRecordedAttribute(NODE_CONSTRAINT_SIZE);
    }

    bool HasBackgroundImageSize() const
    {
        return hasBackgroundImageSize_ || HasRecordedAttribute(NODE_BACKGROUND_IMAGE_SIZE);
    }

    bool HasBackgroundImageSizeWithStyle() const
    {
        return hasBackgroundImageSizeWithStyle_ || HasRecordedAttribute(NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE);
    }

    bool HasHeight() const
    {
        return hasHeight_ || HasRecordedAttribute(NODE_HEIGHT);
    }

    bool HasHeightPercent() const
    {
        return hasHeightPercent_ || HasRecordedAttribute(NODE_HEIGHT_PERCENT);
    }

    // match_parent/wrap_content/fix_at_ideal_size now route to the ArkUI layout-policy API
    // (NODE_WIDTH_LAYOUTPOLICY / NODE_HEIGHT_LAYOUTPOLICY) instead of the legacy percent/reset path.
    bool HasLayoutPolicy(ArkUI_NodeAttributeType attribute, int32_t expectedPolicy) const
    {
        const auto* record = FindLastSetAttribute(attribute);
        return record != nullptr && !record->values.empty() && record->values.front().i32 == expectedPolicy;
    }

    float GetHeightPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_HEIGHT_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return heightPercent_;
    }

    bool WasAttributeReset(ArkUI_NodeAttributeType attribute) const
    {
        if (resetAttributes_.count(attribute) > 0) {
            return true;
        }
        const auto& resetRecords = GetRecordingProvider().resetAttributeRecords_;
        ArkUI_NodeHandle rootNode = GetRootNode();
        for (size_t index = resetRecords.size(); index > resetAttributeRecordStart_; --index) {
            const auto& record = resetRecords[index - 1];
            if (record.nodeHandle == rootNode && record.attribute == attribute) {
                return true;
            }
        }
        return false;
    }

private:
    const MockArkUINativeProvider::SetAttributeRecord* FindLastSetAttribute(ArkUI_NodeAttributeType attribute) const
    {
        const auto& records = GetRecordingProvider().setAttributeRecords_;
        ArkUI_NodeHandle rootNode = GetRootNode();
        for (size_t index = records.size(); index > setAttributeRecordStart_; --index) {
            const auto& record = records[index - 1];
            if (record.nodeHandle == rootNode && record.attribute == attribute) {
                return &record;
            }
        }
        return nullptr;
    }

    bool HasRecordedAttribute(ArkUI_NodeAttributeType attribute) const
    {
        return FindLastSetAttribute(attribute) != nullptr;
    }

    size_t setAttributeRecordStart_ = 0;
    size_t resetAttributeRecordStart_ = 0;
    bool returnNullRootNode_ = false;
    mutable ArkUI_Node rootNode_ {};
    float width_ = 0.0F;
    float widthPercent_ = 0.0F;
    float heightPercent_ = 0.0F;
    bool hasWidth_ = false;
    bool hasHeight_ = false;
    bool hasWidthPercent_ = false;
    bool hasHeightPercent_ = false;
    float flexShrink_ = 0.0F;
    bool hasFlexShrink_ = false;
    std::set<ArkUI_NodeAttributeType> resetAttributes_;
    bool hasBorderRadiusPercent_ = false;
    std::array<float, 4> borderRadiusPercent_ {};
    bool hasBorderRadius_ = false;
    float borderRadius_ = 0.0F;
    bool hasMarginPercent_ = false;
    std::array<float, 4> marginPercent_ {};
    bool hasMargin_ = false;
    bool hasBorderWidthPercent_ = false;
    float borderWidthPercent_ = 0.0F;
    bool hasBorderWidth_ = false;
    float borderWidth_ = 0.0F;
    bool hasPadding_ = false;
    bool hasPaddingPercent_ = false;
    bool hasConstraintSize_ = false;
    std::vector<ArkUI_NumberValue> constraintSizeValues_;
    bool hasBackgroundImageSize_ = false;
    std::vector<ArkUI_NumberValue> backgroundImageSizeValues_;
    bool hasBackgroundImageSizeWithStyle_ = false;
    int32_t backgroundImageSizeWithStyle_ = 0;
};

} // namespace

/**
 * @tc.name: ExtendedStyleResolverPr150Test003
 * @tc.desc: Verify fixAtIdealSize routes to the ArkUI layout-policy API (FIX_AT_IDEAL_SIZE).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverPr150Test, ExtendedStyleResolverPr150Test003)
{
    auto adapter = JsonAdapter::Parse(R"({"width": "fixAtIdealSize", "height": "fixAtIdealSize"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::FIX_AT_IDEAL_SIZE)));
    EXPECT_TRUE(applier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::FIX_AT_IDEAL_SIZE)));
}

/**
 * @tc.name: ExtendedStyleResolverPr150Test004
 * @tc.desc: Verify width applies absolute, percent, matchParent, wrapContent and fixAtIdealSize branches.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverPr150Test, ExtendedStyleResolverPr150Test004)
{
    auto absoluteAdapter = JsonAdapter::Parse(R"({"width": 120})");
    ASSERT_NE(absoluteAdapter, nullptr);
    RecordingCommonStyleApplier absoluteApplier;
    ExtendedStyleResolver::ResolveAndApply(absoluteAdapter->GetRoot(), absoluteApplier);
    ASSERT_TRUE(absoluteApplier.HasWidth());
    EXPECT_FLOAT_EQ(absoluteApplier.GetWidth(), 120.0F);
    EXPECT_FALSE(absoluteApplier.HasWidthPercent());

    auto vpAdapter = JsonAdapter::Parse(R"({"width": "80vp"})");
    ASSERT_NE(vpAdapter, nullptr);
    RecordingCommonStyleApplier vpApplier;
    ExtendedStyleResolver::ResolveAndApply(vpAdapter->GetRoot(), vpApplier);
    ASSERT_TRUE(vpApplier.HasWidth());
    EXPECT_FLOAT_EQ(vpApplier.GetWidth(), 80.0F);
    EXPECT_FALSE(vpApplier.HasWidthPercent());

    auto percentAdapter = JsonAdapter::Parse(R"({"width": "75%"})");
    ASSERT_NE(percentAdapter, nullptr);
    RecordingCommonStyleApplier percentApplier;
    ExtendedStyleResolver::ResolveAndApply(percentAdapter->GetRoot(), percentApplier);
    EXPECT_FALSE(percentApplier.HasWidth());
    ASSERT_TRUE(percentApplier.HasWidthPercent());
    EXPECT_FLOAT_EQ(percentApplier.GetWidthPercent(), 0.75F);

    auto matchParentAdapter = JsonAdapter::Parse(R"({"width": "matchParent"})");
    ASSERT_NE(matchParentAdapter, nullptr);
    RecordingCommonStyleApplier matchParentApplier;
    ExtendedStyleResolver::ResolveAndApply(matchParentAdapter->GetRoot(), matchParentApplier);
    EXPECT_FALSE(matchParentApplier.HasWidth());
    ASSERT_TRUE(matchParentApplier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));

    auto wrapContentAdapter = JsonAdapter::Parse(R"({"width": "wrapContent"})");
    ASSERT_NE(wrapContentAdapter, nullptr);
    RecordingCommonStyleApplier wrapContentApplier;
    ExtendedStyleResolver::ResolveAndApply(wrapContentAdapter->GetRoot(), wrapContentApplier);
    EXPECT_TRUE(wrapContentApplier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT)));

    auto fixAtIdealSizeAdapter = JsonAdapter::Parse(R"({"width": "fixAtIdealSize"})");
    ASSERT_NE(fixAtIdealSizeAdapter, nullptr);
    RecordingCommonStyleApplier fixAtIdealSizeApplier;
    ExtendedStyleResolver::ResolveAndApply(fixAtIdealSizeAdapter->GetRoot(), fixAtIdealSizeApplier);
    EXPECT_TRUE(fixAtIdealSizeApplier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::FIX_AT_IDEAL_SIZE)));
}

/**
 * @tc.name: StyleApplyUtilsTest007
 * @tc.desc: Verify ParseDimension covers all width protocol forms and rejects negative values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest007)
{
    StyleDimension dimension;

    auto numberAdapter = JsonAdapter::Parse("120");
    ASSERT_NE(numberAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(numberAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::VP);
    EXPECT_FLOAT_EQ(dimension.value, 120.0F);

    auto vpAdapter = JsonAdapter::Parse(R"("80vp")");
    ASSERT_NE(vpAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(vpAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::VP);
    EXPECT_FLOAT_EQ(dimension.value, 80.0F);

    auto fpAdapter = JsonAdapter::Parse(R"("24fp")");
    ASSERT_NE(fpAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(fpAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::FP);
    EXPECT_FLOAT_EQ(dimension.value, 24.0F);

    auto percentAdapter = JsonAdapter::Parse(R"("75%")");
    ASSERT_NE(percentAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(percentAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::PERCENT);
    EXPECT_FLOAT_EQ(dimension.value, 75.0F);

    auto matchParentAdapter = JsonAdapter::Parse(R"("matchParent")");
    ASSERT_NE(matchParentAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(matchParentAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::MATCH_PARENT);
    EXPECT_FLOAT_EQ(dimension.value, 100.0F);

    auto wrapContentAdapter = JsonAdapter::Parse(R"("wrapContent")");
    ASSERT_NE(wrapContentAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(wrapContentAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::WRAP_CONTENT);
    EXPECT_FLOAT_EQ(dimension.value, 0.0F);

    auto fixAtIdealAdapter = JsonAdapter::Parse(R"("fixAtIdealSize")");
    ASSERT_NE(fixAtIdealAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(fixAtIdealAdapter->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::FIX_AT_IDEAL_SIZE);
    EXPECT_FLOAT_EQ(dimension.value, 0.0F);

    auto negativeNumberAdapter = JsonAdapter::Parse("-1");
    ASSERT_NE(negativeNumberAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseDimension(negativeNumberAdapter->GetRoot(), dimension));

    auto negativeStringAdapter = JsonAdapter::Parse(R"("-2vp")");
    ASSERT_NE(negativeStringAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseDimension(negativeStringAdapter->GetRoot(), dimension));
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest005
 * @tc.desc: Verify custom shadow object without radius is rejected.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest005)
{
    auto adapter = JsonAdapter::Parse(R"({
        "shadow": {
            "offsetX": 3,
            "offsetY": 4,
            "type": "blur",
            "color": "#FF112233",
            "fill": true
        }
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBorderWidth());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest006
 * @tc.desc: Verify padding with percent values applies SetPaddingPercent.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest006)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "5% 10% 15% 20%"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasPaddingPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest007
 * @tc.desc: Verify margin with percent values applies SetMarginPercent.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest007)
{
    auto adapter = JsonAdapter::Parse(R"({
        "margin": "10% 20% 30% 40%"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasMarginPercent());
    EXPECT_FLOAT_EQ(applier.GetMarginPercentTop(), 0.10F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercentRight(), 0.20F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercentBottom(), 0.30F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercentLeft(), 0.40F);
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest008
 * @tc.desc: Verify margin with absolute VP values applies SetMargin.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest008)
{
    auto adapter = JsonAdapter::Parse(R"({
        "margin": "10 20 30 40"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasMargin());
    EXPECT_FALSE(applier.HasMarginPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest009
 * @tc.desc: Verify borderRadius with percent values applies SetBorderRadiusPercent.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest009)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {"topLeft": "5%", "topRight": "10%", "bottomRight": "15%", "bottomLeft": "20%"}
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasBorderRadiusPercent());
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercentTopLeft(), 5.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercentTopRight(), 10.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercentBottomLeft(), 15.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercentBottomRight(), 20.0F);
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest010
 * @tc.desc: Verify borderRadius with same radius value applies SetBorderRadius.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest010)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": 12
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasBorderRadius());
    EXPECT_FALSE(applier.HasBorderRadiusPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest011
 * @tc.desc: Verify borderWidth with VP dimension applies NODE_BORDER_WIDTH.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest011)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "3vp"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasBorderWidth());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest012
 * @tc.desc: Verify borderWidth with percent dimension applies SetBorderWidthPercent.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest012)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "50%"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasBorderWidthPercent());
    EXPECT_FLOAT_EQ(applier.GetBorderWidthPercent(), 0.5F);
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest013
 * @tc.desc: Verify Reset for margin resets both NODE_MARGIN and NODE_MARGIN_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest013)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "margin", StylePropertyName::MARGIN }, applier);

    EXPECT_TRUE(applier.WasAttributeReset(NODE_MARGIN));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_MARGIN_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest014
 * @tc.desc: Verify Reset for borderRadius resets both NODE_BORDER_RADIUS and NODE_BORDER_RADIUS_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest014)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "borderRadius", StylePropertyName::BORDER_RADIUS }, applier);

    EXPECT_TRUE(applier.WasAttributeReset(NODE_BORDER_RADIUS));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BORDER_RADIUS_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest015
 * @tc.desc: Verify Reset for borderWidth resets both NODE_BORDER_WIDTH and NODE_BORDER_WIDTH_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest015)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "borderWidth", StylePropertyName::BORDER_WIDTH }, applier);

    EXPECT_TRUE(applier.WasAttributeReset(NODE_BORDER_WIDTH));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BORDER_WIDTH_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest016
 * @tc.desc: Verify Reset for backgroundImageSize resets BACKGROUND_IMAGE_SIZE_WITH_STYLE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest016)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "backgroundImageSize", StylePropertyName::BACKGROUND_IMAGE_SIZE }, applier);

    EXPECT_TRUE(applier.WasAttributeReset(NODE_BACKGROUND_IMAGE_SIZE));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE));
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest017
 * @tc.desc: Verify constraintSize with valid VP dimensions applies NODE_CONSTRAINT_SIZE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest017)
{
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10vp",
            "maxWidth": "200vp",
            "minHeight": "5vp",
            "maxHeight": "100vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasConstraintSize());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest018
 * @tc.desc: Verify constraintSize with percent unit is rejected (only VP/FP allowed).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest018)
{
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "50%",
            "maxWidth": "100%",
            "minHeight": "10%",
            "maxHeight": "80%"
        }
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasConstraintSize());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest019
 * @tc.desc: Verify backgroundImageSizeWithStyle applies image size keyword.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest019)
{
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundImageSizeWithStyle": "cover"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasBackgroundImageSizeWithStyle());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest020
 * @tc.desc: Verify backgroundImageSizeWithStyle applies dimension size.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest020)
{
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundimageSizeWithStyle": {"width": "80vp", "height": "40vp"}
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasBackgroundImageSize());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest021
 * @tc.desc: Verify borderWidth with FP dimension applies NODE_BORDER_WIDTH.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest021)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "5fp"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasBorderWidth());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest022
 * @tc.desc: Verify borderWidth with invalid unit (wrapContent) is rejected with warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest022)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "wrapContent"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBorderWidth());
    EXPECT_FALSE(applier.HasBorderWidthPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest023
 * @tc.desc: Verify margin with mixed absolute and percent units is rejected.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest023)
{
    auto adapter = JsonAdapter::Parse(R"({
        "margin": "10 20% 30 40"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasMargin());
    EXPECT_FALSE(applier.HasMarginPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest024
 * @tc.desc: Verify padding with mixed absolute and percent units is rejected.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest024)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "5% 10 15% 20"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasPadding());
    EXPECT_FALSE(applier.HasPaddingPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest025
 * @tc.desc: Verify borderRadius with mixed percent and absolute units is rejected.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest025)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {"topLeft": "5%", "topRight": "10", "bottomRight": "15%", "bottomLeft": "20"}
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBorderRadius());
    EXPECT_FALSE(applier.HasBorderRadiusPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest026
 * @tc.desc: Verify borderRadius with different absolute values applies via SetNodeNumberArray.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest026)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {"topLeft": "5", "topRight": "10", "bottomRight": "15", "bottomLeft": "20"}
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBorderRadius());
    EXPECT_FALSE(applier.HasBorderRadiusPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest027
 * @tc.desc: Verify borderWidth with invalid string input logs warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest027)
{
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "notAUnit"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBorderWidth());
    EXPECT_FALSE(applier.HasBorderWidthPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest028
 * @tc.desc: Verify ResolveAndApply with non-object styles is ignored.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest028)
{
    auto adapter = JsonAdapter::Parse(R"("notAnObject")");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasWidth());
    EXPECT_FALSE(applier.HasFlexShrink());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest029
 * @tc.desc: Verify ApplyDimension with negative dimension is rejected.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest029)
{
    auto adapter = JsonAdapter::Parse(R"({"width": "-5"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasWidth());
    EXPECT_FALSE(applier.HasWidthPercent());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest030
 * @tc.desc: Verify padding with absolute VP values applies SetPadding.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest030)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "10 20 30 40"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_TRUE(applier.HasPadding());
    EXPECT_FALSE(applier.HasPaddingPercent());
}

// ==================== Additional coverage tests ====================

/**
 * @tc.name: ExtendedStyleResolverTest005
 * @tc.desc: Verify height applies absolute VP and percent values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest005)
{
    auto absoluteAdapter = JsonAdapter::Parse(R"({"height": 60})");
    ASSERT_NE(absoluteAdapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(absoluteAdapter->GetRoot(), applier);
    EXPECT_TRUE(applier.HasHeight());

    auto vpAdapter = JsonAdapter::Parse(R"({"height": "40vp"})");
    ASSERT_NE(vpAdapter, nullptr);
    RecordingCommonStyleApplier vpApplier;
    ExtendedStyleResolver::ResolveAndApply(vpAdapter->GetRoot(), vpApplier);
    EXPECT_TRUE(vpApplier.HasHeight());

    auto percentAdapter = JsonAdapter::Parse(R"({"height": "30%"})");
    ASSERT_NE(percentAdapter, nullptr);
    RecordingCommonStyleApplier percentApplier;
    ExtendedStyleResolver::ResolveAndApply(percentAdapter->GetRoot(), percentApplier);
    ASSERT_TRUE(percentApplier.HasHeightPercent());
    EXPECT_FLOAT_EQ(percentApplier.GetHeightPercent(), 0.30F);

    auto wrapContentAdapter = JsonAdapter::Parse(R"({"height": "wrapContent"})");
    ASSERT_NE(wrapContentAdapter, nullptr);
    RecordingCommonStyleApplier wrapApplier;
    ExtendedStyleResolver::ResolveAndApply(wrapContentAdapter->GetRoot(), wrapApplier);
    EXPECT_TRUE(wrapApplier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT)));

    auto matchParentAdapter = JsonAdapter::Parse(R"({"height": "matchParent"})");
    ASSERT_NE(matchParentAdapter, nullptr);
    RecordingCommonStyleApplier mpApplier;
    ExtendedStyleResolver::ResolveAndApply(matchParentAdapter->GetRoot(), mpApplier);
    ASSERT_TRUE(mpApplier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));
}

/**
 * @tc.name: ExtendedStyleResolverTest006
 * @tc.desc: Verify backgroundColor and borderColor apply correctly.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest006)
{
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundColor": "#FF112233",
        "borderColor": "#AABBCCDD"
    })");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    // Verify the styles were processed without crash
}

/**
 * @tc.name: ExtendedStyleResolverTest007
 * @tc.desc: Verify opacity and visibility apply correctly.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest007)
{
    auto adapter = JsonAdapter::Parse(R"({
        "opacity": 0.5,
        "visibility": "hidden"
    })");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest008
 * @tc.desc: Verify layoutWeight applies correctly.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest008)
{
    auto adapter = JsonAdapter::Parse(R"({"layoutWeight": 2})");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest009
 * @tc.desc: Verify invalid visibility value is ignored.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest009)
{
    auto adapter = JsonAdapter::Parse(R"({"visibility": "invalidValue"})");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest010
 * @tc.desc: Verify linearGradient applies through ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest010)
{
    auto adapter = JsonAdapter::Parse(R"({
        "linearGradient": {
            "direction": "left",
            "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
        }
    })");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest011
 * @tc.desc: Verify Reset for width resets both NODE_WIDTH and NODE_WIDTH_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest011)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "width", StylePropertyName::WIDTH }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_WIDTH));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_WIDTH_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverTest012
 * @tc.desc: Verify Reset for height resets both NODE_HEIGHT and NODE_HEIGHT_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest012)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "height", StylePropertyName::HEIGHT }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_HEIGHT));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_HEIGHT_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverTest013
 * @tc.desc: Verify Reset for padding resets both NODE_PADDING and NODE_PADDING_PERCENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest013)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "padding", StylePropertyName::PADDING }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_PADDING));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_PADDING_PERCENT));
}

/**
 * @tc.name: ExtendedStyleResolverTest014
 * @tc.desc: Verify Reset for shadow resets NODE_SHADOW and NODE_CUSTOM_SHADOW.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest014)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "shadow", StylePropertyName::SHADOW }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_SHADOW));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_CUSTOM_SHADOW));
}

/**
 * @tc.name: ExtendedStyleResolverTest015
 * @tc.desc: Verify Reset for clip resets NODE_CLIP.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest015)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "clip", StylePropertyName::CLIP }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_CLIP));
}

/**
 * @tc.name: ExtendedStyleResolverTest016
 * @tc.desc: Verify Reset for linearGradient resets NODE_LINEAR_GRADIENT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest016)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "linearGradient", StylePropertyName::LINEAR_GRADIENT }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_LINEAR_GRADIENT));
}

/**
 * @tc.name: ExtendedStyleResolverTest017
 * @tc.desc: Verify Reset for opacity resets NODE_OPACITY.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest017)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "opacity", StylePropertyName::OPACITY }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_OPACITY));
}

/**
 * @tc.name: ExtendedStyleResolverTest018
 * @tc.desc: Verify Reset for visibility resets NODE_VISIBILITY.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest018)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "visibility", StylePropertyName::VISIBILITY }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_VISIBILITY));
}

/**
 * @tc.name: ExtendedStyleResolverTest019
 * @tc.desc: Verify Reset for backgroundColor resets NODE_BACKGROUND_COLOR.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest019)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "backgroundColor", StylePropertyName::BACKGROUND_COLOR }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BACKGROUND_COLOR));
}

/**
 * @tc.name: ExtendedStyleResolverTest020
 * @tc.desc: Verify Reset for backgroundimage resets NODE_BACKGROUND_IMAGE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest020)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "backgroundimage", StylePropertyName::BACKGROUND_IMAGE }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BACKGROUND_IMAGE));
}

/**
 * @tc.name: ExtendedStyleResolverTest021
 * @tc.desc: Verify Reset for borderColor resets NODE_BORDER_COLOR.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest021)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "borderColor", StylePropertyName::BORDER_COLOR }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_BORDER_COLOR));
}

/**
 * @tc.name: ExtendedStyleResolverTest022
 * @tc.desc: Verify Reset for fontColor resets NODE_FONT_COLOR.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest022)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "fontColor", StylePropertyName::FONT_COLOR }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_FONT_COLOR));
}

/**
 * @tc.name: ExtendedStyleResolverTest023
 * @tc.desc: Verify Reset for fontSize resets NODE_FONT_SIZE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest023)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "fontSize", StylePropertyName::FONT_SIZE }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_FONT_SIZE));
}

/**
 * @tc.name: ExtendedStyleResolverTest024
 * @tc.desc: Verify Reset for fontWeight resets NODE_FONT_WEIGHT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest024)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "fontWeight", StylePropertyName::FONT_WEIGHT }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_FONT_WEIGHT));
}

/**
 * @tc.name: ExtendedStyleResolverTest025
 * @tc.desc: Verify Reset for maxLines resets NODE_TEXT_MAX_LINES and NODE_TEXT_INPUT_NUMBER_OF_LINES.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest025)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "maxLines", StylePropertyName::MAX_LINES }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_MAX_LINES));
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_INPUT_NUMBER_OF_LINES));
}

/**
 * @tc.name: ExtendedStyleResolverTest026
 * @tc.desc: Verify Reset for textOverflow resets NODE_TEXT_OVERFLOW.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest026)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "textOverflow", StylePropertyName::TEXT_OVERFLOW }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_OVERFLOW));
}

/**
 * @tc.name: ExtendedStyleResolverTest027
 * @tc.desc: Verify Reset for decoration resets NODE_TEXT_DECORATION.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest027)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "decoration", StylePropertyName::DECORATION }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_DECORATION));
}

/**
 * @tc.name: ExtendedStyleResolverTest028
 * @tc.desc: Verify Reset for flexShrink resets NODE_FLEX_SHRINK.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest028)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "flexShrink", StylePropertyName::FLEX_SHRINK }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_FLEX_SHRINK));
}

/**
 * @tc.name: ExtendedStyleResolverTest029
 * @tc.desc: Verify Reset for textAlign resets NODE_TEXT_ALIGN.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest029)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "textAlign", StylePropertyName::TEXT_ALIGN }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_ALIGN));
}

/**
 * @tc.name: ExtendedStyleResolverTest030
 * @tc.desc: Verify Reset for constraintSize resets NODE_CONSTRAINT_SIZE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest030)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "constraintSize", StylePropertyName::CONSTRAINT_SIZE }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_CONSTRAINT_SIZE));
}

/**
 * @tc.name: ExtendedStyleResolverTest031
 * @tc.desc: Verify Reset for layoutWeight resets NODE_LAYOUT_WEIGHT.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest031)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "layoutWeight", StylePropertyName::LAYOUT_WEIGHT }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_LAYOUT_WEIGHT));
}

/**
 * @tc.name: ExtendedStyleResolverTest032
 * @tc.desc: Verify Reset for placeholderColor resets NODE_TEXT_INPUT_PLACEHOLDER_COLOR.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest032)
{
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::Reset({ "placeholderColor", StylePropertyName::PLACEHOLDER_COLOR }, applier);
    EXPECT_TRUE(applier.WasAttributeReset(NODE_TEXT_INPUT_PLACEHOLDER_COLOR));
}

/**
 * @tc.name: ExtendedStyleResolverTest033
 * @tc.desc: Verify fontColor and fontSize apply through ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest033)
{
    auto adapter = JsonAdapter::Parse(R"({
        "fontColor": "#FF000000",
        "fontSize": 14
    })");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest034
 * @tc.desc: Verify constraintSize with FP unit applies NODE_CONSTRAINT_SIZE.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest034)
{
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10fp",
            "maxWidth": "200fp",
            "minHeight": "5fp",
            "maxHeight": "100fp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    EXPECT_TRUE(applier.HasConstraintSize());
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest001
 * @tc.desc: Verify ParseShadow from number value applies style shadow.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest001)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"(1)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::STYLE);
    EXPECT_TRUE(shadow.valid);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest002
 * @tc.desc: Verify ParseShadow from string keyword.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest002)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"("outerFloatingSM")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::STYLE);
    EXPECT_EQ(shadow.style, ARKUI_SHADOW_STYLE_OUTER_FLOATING_SM);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest003
 * @tc.desc: Verify ParseShadow from string number.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest003)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"("2")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::STYLE);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest004
 * @tc.desc: Verify ParseShadow rejects invalid string.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest004)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"("invalidShadow")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest005
 * @tc.desc: Verify ParseShadow rejects out-of-range number.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest005)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"(999)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest006
 * @tc.desc: Verify ParseShadow from object with type number.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest006)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 10,
        "offsetX": 3,
        "offsetY": 4,
        "type": 0,
        "color": "#FF112233",
        "fill": true
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::CUSTOM);
    EXPECT_TRUE(shadow.valid);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest007
 * @tc.desc: Verify ParseShadow from object with type string.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest007)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 5,
        "type": "blur",
        "color": "#FF000000"
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::CUSTOM);
    EXPECT_EQ(shadow.type, ARKUI_SHADOW_TYPE_BLUR);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest008
 * @tc.desc: Verify ParseShadow from object accepts missing radius with default radius.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest008)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "offsetX": 3,
        "offsetY": 4
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_TRUE(shadow.valid);
    EXPECT_EQ(shadow.kind, StyleShadowKind::CUSTOM);
    EXPECT_FLOAT_EQ(shadow.radius, 0.0F);
    EXPECT_FLOAT_EQ(shadow.offsetX, 3.0F);
    EXPECT_FLOAT_EQ(shadow.offsetY, 4.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest009
 * @tc.desc: Verify ParseShadow rejects object with path binding.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest009)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({"radius": 10, "path": "/shadow"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest010
 * @tc.desc: Verify ParseShadow rejects invalid type value.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest010)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 10,
        "type": "invalidType"
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest011
 * @tc.desc: Verify ParseShadow from object with invalid type (non-finite number).
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest011)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 10,
        "type": 1.7
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest012
 * @tc.desc: Verify ParseShadow from object rejects invalid fill.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest012)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 10,
        "fill": "notABool"
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest013
 * @tc.desc: Verify ParseBackgroundImageSize rejects object with path.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest013)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"({"width": "10vp", "height": "20vp", "path": "/img"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest014
 * @tc.desc: Verify ParseBackgroundImageSize accepts "auto" keyword.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest014)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"("auto")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
    EXPECT_EQ(size.kind, StyleBackgroundImageSizeKind::IMAGE_SIZE);
    EXPECT_EQ(size.imageSize, 0);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest015
 * @tc.desc: Verify ParseBackgroundImageSize rejects invalid string.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest015)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"("invalidSize")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest016
 * @tc.desc: Verify ParseBackgroundImageSize accepts percent dimensions.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest016)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"({"width": "50%", "height": "75%"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
    EXPECT_EQ(size.kind, StyleBackgroundImageSizeKind::SIZE);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest017
 * @tc.desc: Verify ParseBackgroundImageSize rejects negative dimensions.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest017)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"({"width": "-10vp", "height": "20vp"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest018
 * @tc.desc: Verify ParseBackgroundImageSize rejects matchParent dimension.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest018)
{
    StyleBackgroundImageSize size;
    auto adapter = JsonAdapter::Parse(R"({"width": "matchParent", "height": "20vp"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseBackgroundImageSize(adapter->GetRoot(), size));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest019
 * @tc.desc: Verify ParseFlexShrink rejects out-of-range values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest019)
{
    float flexShrink = 0.0F;
    auto adapter = JsonAdapter::Parse(R"(2.0)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseFlexShrink(adapter->GetRoot(), flexShrink));

    auto negAdapter = JsonAdapter::Parse(R"(-0.5)");
    ASSERT_NE(negAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseFlexShrink(negAdapter->GetRoot(), flexShrink));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest020
 * @tc.desc: Verify ParseFlexShrink rejects non-finite values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest020)
{
    float flexShrink = 0.0F;
    auto adapter = JsonAdapter::Parse(R"("notANumber")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseFlexShrink(adapter->GetRoot(), flexShrink));
}

TEST(StyleApplyUtilsEffectsTest, should_parse_linear_gradient_with_plain_color_array_and_auto_stops)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({"colors":["#000000","#FFFFFF"]})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.colors.size(), 2U);
    ASSERT_EQ(gradient.stops.size(), 2U);
    EXPECT_EQ(gradient.colors[0], 0xFF000000U);
    EXPECT_EQ(gradient.colors[1], 0xFFFFFFFFU);
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.0F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 1.0F);
}

TEST(StyleApplyUtilsEffectsTest, should_parse_linear_gradient_with_plain_color_array_and_stops_override)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({"colors":["#000000","#FFFFFF"],"stops":[0.25,2.0]})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.stops.size(), 2U);
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.25F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 1.0F);
}

TEST(StyleApplyUtilsEffectsTest, should_parse_linear_gradient_with_empty_stops_using_even_distribution)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({"colors":["#111111","#222222","#333333"],"stops":[]})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.stops.size(), 3U);
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.0F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 0.5F);
    EXPECT_FLOAT_EQ(gradient.stops[2], 1.0F);
}

TEST(StyleApplyUtilsEffectsTest, should_skip_invalid_plain_gradient_color_entries)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({"colors":["not-a-color","#FFFFFF"]})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.colors.size(), 1U);
    ASSERT_EQ(gradient.stops.size(), 1U);
    EXPECT_EQ(gradient.colors[0], 0xFFFFFFFFU);
    EXPECT_FLOAT_EQ(gradient.stops[0], 1.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest021
 * @tc.desc: Verify ParseBackgroundImage extracts trimmed string.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest021)
{
    std::string result;
    auto adapter = JsonAdapter::Parse(R"( " path/to/image.png " )");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseBackgroundImage(adapter->GetRoot(), result));
    EXPECT_EQ(result, "path/to/image.png");

    auto invalidAdapter = JsonAdapter::Parse(R"(123)");
    ASSERT_NE(invalidAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseBackgroundImage(invalidAdapter->GetRoot(), result));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest022
 * @tc.desc: Verify ParseClip accepts boolean values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest022)
{
    bool clip = false;
    auto adapter = JsonAdapter::Parse(R"(true)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseClip(adapter->GetRoot(), clip));
    EXPECT_TRUE(clip);

    auto invalidAdapter = JsonAdapter::Parse(R"("true")");
    ASSERT_NE(invalidAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseClip(invalidAdapter->GetRoot(), clip));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest023
 * @tc.desc: Verify ParseLinearGradient with angle in degrees.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest023)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "90deg",
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 90.0F);
    EXPECT_EQ(gradient.direction, ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest024
 * @tc.desc: Verify ParseLinearGradient with angle in radians.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest024)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "1.5708rad",
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_NEAR(gradient.angle, 90.0F, 0.01F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest025
 * @tc.desc: Verify ParseLinearGradient with angle in turns.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest025)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "0.25turn",
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 90.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest026
 * @tc.desc: Verify ParseLinearGradient with angle in degrees.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest026)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "90deg",
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 90.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest027
 * @tc.desc: Verify ParseLinearGradient with numeric angle.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest027)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": 45,
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 45.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest028
 * @tc.desc: Verify ParseLinearGradient with repeating flag.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest028)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "0deg",
        "repeating": true,
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_TRUE(gradient.repeating);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest029
 * @tc.desc: Verify ParseLinearGradient rejects invalid repeating flag.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest029)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "0deg",
        "repeating": "yes",
        "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest030
 * @tc.desc: Verify ParseLinearGradient rejects object with path binding.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest030)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "path": "/gradient",
        "colors": [["#FF0000", 0.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest031
 * @tc.desc: Verify ParseLinearGradient with object color stop format.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest031)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "direction": "left",
        "colors": [
            {"color": "#FF0000", "stop": 0.0},
            {"color": "#0000FF", "stop": 1.0}
        ]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.colors.size(), 2u);
    EXPECT_EQ(gradient.colors[0], 0xFFFF0000u);
    EXPECT_EQ(gradient.colors[1], 0xFF0000FFu);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest032
 * @tc.desc: Verify ParseLinearGradient clamps stop values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest032)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "direction": "left",
        "colors": [["#FF0000", -0.5], ["#0000FF", 2.0]]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    ASSERT_EQ(gradient.stops.size(), 2u);
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.0F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 1.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest033
 * @tc.desc: Verify ParseLinearGradient with color stop using position key.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest033)
{
    StyleLinearGradient gradient;
    auto adapter = JsonAdapter::Parse(R"({
        "direction": "right",
        "colors": [
            {"color": "#FF0000", "position": 0.3},
            {"color": "#0000FF", "position": 0.7}
        ]
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.3F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 0.7F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest034
 * @tc.desc: Verify ParseShadow rejects boolean and null types.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest034)
{
    StyleShadow shadow;
    auto boolAdapter = JsonAdapter::Parse(R"(true)");
    ASSERT_NE(boolAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(boolAdapter->GetRoot(), shadow));

    auto nullAdapter = JsonAdapter::Parse(R"(null)");
    ASSERT_NE(nullAdapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(nullAdapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest035
 * @tc.desc: Verify ParseShadow from object with color only (no radius).
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest035)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 0,
        "color": "#FF000000"
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
    EXPECT_EQ(shadow.kind, StyleShadowKind::CUSTOM);
    EXPECT_FLOAT_EQ(shadow.radius, 0.0F);
}

/**
 * @tc.name: StyleApplyUtilsEffectsTest036
 * @tc.desc: Verify ParseShadow from object rejects invalid color.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsEffectsTest, StyleApplyUtilsEffectsTest036)
{
    StyleShadow shadow;
    auto adapter = JsonAdapter::Parse(R"({
        "radius": 10,
        "color": "notAColor"
    })");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseShadow(adapter->GetRoot(), shadow));
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest001
 * @tc.desc: Verify ParseEdge with object format using individual sides.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest001)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"({"top": 10, "right": 20, "bottom": 30, "left": 40})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_FLOAT_EQ(edge.top.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 30.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 40.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest002
 * @tc.desc: Verify ParseEdge with object format using all/vertical/horizontal.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest002)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"({"all": 15})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_FLOAT_EQ(edge.top.value, 15.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 15.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 15.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 15.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest003
 * @tc.desc: Verify ParseEdge rejects object with path binding.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest003)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"({"path": "/edge"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest004
 * @tc.desc: Verify ParseEdge with string shorthand (2 values).
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest004)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"("10 20")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_FLOAT_EQ(edge.top.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 20.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest005
 * @tc.desc: Verify ParseEdge with string shorthand (3 values).
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest005)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"("10 20 30")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_FLOAT_EQ(edge.top.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 30.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest006
 * @tc.desc: Verify ParseRadius with object format using individual corners.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest006)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"({"topLeft": 5, "topRight": 10, "bottomRight": 15, "bottomLeft": 20})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
    EXPECT_FLOAT_EQ(radius.topLeft.value, 5.0F);
    EXPECT_FLOAT_EQ(radius.topRight.value, 10.0F);
    EXPECT_FLOAT_EQ(radius.bottomRight.value, 15.0F);
    EXPECT_FLOAT_EQ(radius.bottomLeft.value, 20.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest007
 * @tc.desc: Verify ParseRadius with string shorthand.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest007)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"("5 10 15 20")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
    EXPECT_FLOAT_EQ(radius.topLeft.value, 5.0F);
    EXPECT_FLOAT_EQ(radius.topRight.value, 10.0F);
    EXPECT_FLOAT_EQ(radius.bottomRight.value, 15.0F);
    EXPECT_FLOAT_EQ(radius.bottomLeft.value, 20.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest008
 * @tc.desc: Verify ParseRadius rejects object with path binding.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest008)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"({"path": "/radius"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
}

TEST(StyleApplyUtilsLayoutTest, should_parse_empty_edge_object_as_zero_values)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_EQ(edge.top.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(edge.right.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(edge.bottom.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(edge.left.unit, StyleDimensionUnit::VP);
    EXPECT_FLOAT_EQ(edge.top.value, 0.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 0.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 0.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 0.0F);
}

TEST(StyleApplyUtilsLayoutTest, should_parse_empty_radius_object_as_zero_values)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);

    EXPECT_TRUE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
    EXPECT_EQ(radius.topLeft.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(radius.topRight.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(radius.bottomRight.unit, StyleDimensionUnit::VP);
    EXPECT_EQ(radius.bottomLeft.unit, StyleDimensionUnit::VP);
    EXPECT_FLOAT_EQ(radius.topLeft.value, 0.0F);
    EXPECT_FLOAT_EQ(radius.topRight.value, 0.0F);
    EXPECT_FLOAT_EQ(radius.bottomRight.value, 0.0F);
    EXPECT_FLOAT_EQ(radius.bottomLeft.value, 0.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest009
 * @tc.desc: Verify ParseRadius with all keyword.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest009)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"({"all": 12})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
    EXPECT_FLOAT_EQ(radius.topLeft.value, 12.0F);
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest010
 * @tc.desc: Verify ParseEdge with invalid value returns false.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest010)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"(null)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest011
 * @tc.desc: Verify ParseRadius with invalid value returns false.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest011)
{
    StyleRadius radius;
    auto adapter = JsonAdapter::Parse(R"(null)");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseRadius(adapter->GetRoot(), radius));
}

/**
 * @tc.name: StyleApplyUtilsLayoutTest012
 * @tc.desc: Verify ParseEdge with object using vertical/horizontal.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsLayoutTest, StyleApplyUtilsLayoutTest012)
{
    StyleEdge edge;
    auto adapter = JsonAdapter::Parse(R"({"vertical": 10, "horizontal": 20})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseEdge(adapter->GetRoot(), edge));
    EXPECT_FLOAT_EQ(edge.top.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 20.0F);
}

/**
 * @tc.name: StyleParserTest002
 * @tc.desc: Verify StyleParser::ToPropertyName maps all style names correctly.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest002)
{
    EXPECT_EQ(StyleParser::ToPropertyName("width"), StylePropertyName::WIDTH);
    EXPECT_EQ(StyleParser::ToPropertyName("height"), StylePropertyName::HEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("padding"), StylePropertyName::PADDING);
    EXPECT_EQ(StyleParser::ToPropertyName("paddingTop"), StylePropertyName::PADDING);
    EXPECT_EQ(StyleParser::ToPropertyName("margin"), StylePropertyName::MARGIN);
    EXPECT_EQ(StyleParser::ToPropertyName("marginTop"), StylePropertyName::MARGIN);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundColor"), StylePropertyName::BACKGROUND_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("borderRadius"), StylePropertyName::BORDER_RADIUS);
    EXPECT_EQ(StyleParser::ToPropertyName("borderWidth"), StylePropertyName::BORDER_WIDTH);
    EXPECT_EQ(StyleParser::ToPropertyName("borderColor"), StylePropertyName::BORDER_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("fontColor"), StylePropertyName::FONT_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("fontSize"), StylePropertyName::FONT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("fontWeight"), StylePropertyName::FONT_WEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("textAlign"), StylePropertyName::TEXT_ALIGN);
    EXPECT_EQ(StyleParser::ToPropertyName("maxLines"), StylePropertyName::MAX_LINES);
    EXPECT_EQ(StyleParser::ToPropertyName("textOverflow"), StylePropertyName::TEXT_OVERFLOW);
    EXPECT_EQ(StyleParser::ToPropertyName("decoration"), StylePropertyName::DECORATION);
    EXPECT_EQ(StyleParser::ToPropertyName("flexShrink"), StylePropertyName::FLEX_SHRINK);
    EXPECT_EQ(StyleParser::ToPropertyName("clip"), StylePropertyName::CLIP);
    EXPECT_EQ(StyleParser::ToPropertyName("linearGradient"), StylePropertyName::LINEAR_GRADIENT);
    EXPECT_EQ(StyleParser::ToPropertyName("shadow"), StylePropertyName::SHADOW);
    EXPECT_EQ(StyleParser::ToPropertyName("visibility"), StylePropertyName::VISIBILITY);
    EXPECT_EQ(StyleParser::ToPropertyName("opacity"), StylePropertyName::OPACITY);
    EXPECT_EQ(StyleParser::ToPropertyName("layoutWeight"), StylePropertyName::LAYOUT_WEIGHT);
    EXPECT_EQ(StyleParser::ToPropertyName("constraintSize"), StylePropertyName::CONSTRAINT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("placeholderColor"), StylePropertyName::PLACEHOLDER_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("caretColor"), StylePropertyName::CARET_COLOR);
    EXPECT_EQ(StyleParser::ToPropertyName("showUnderline"), StylePropertyName::SHOW_UNDERLINE);
    EXPECT_EQ(StyleParser::ToPropertyName("unknownProp"), StylePropertyName::UNKNOWN);
}

/**
 * @tc.name: StyleParserTest003
 * @tc.desc: Verify StyleParser::Parse handles various value kinds correctly.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest003)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": 100,
        "height": "50%",
        "fontColor": {"path": "/theme/color"},
        "maxLines": {"call": "getMaxLines"},
        "padding": {"top": 8, "right": 4}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.properties.size(), 5u);
}

/**
 * @tc.name: StyleParserTest004
 * @tc.desc: Verify StyleParser::Parse with all mapped property names.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest004)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": 100,
        "height": 50,
        "padding": 10,
        "paddingTop": 5,
        "paddingRight": 5,
        "paddingBottom": 5,
        "paddingLeft": 5,
        "margin": 8,
        "marginTop": 4,
        "marginRight": 4,
        "marginBottom": 4,
        "marginLeft": 4,
        "backgroundColor": "#FF0000",
        "borderRadius": 10,
        "borderWidth": 2,
        "borderColor": "#000000",
        "fontColor": "#333333",
        "fontSize": 14,
        "fontWeight": "bold",
        "textAlign": "center",
        "maxLines": 3,
        "minFontSize": 10,
        "maxFontSize": 20,
        "textOverflow": "ellipsis",
        "wordBreak": "breakAll",
        "decoration": {"type": "underline"},
        "placeholderColor": "#999999",
        "caretColor": "#0066FF",
        "showUnderline": true,
        "visibility": "visible",
        "opacity": 0.8,
        "shadow": {},
        "flexShrink": 1,
        "backgroundImage": "test.png",
        "backgroundimage": "test2.png",
        "backgroundImageSizeWithStyle": 1,
        "backgroundimageSizeWithStyle": 1,
        "clip": true,
        "layoutWeight": 2,
        "constraintSize": {"minWidth": 10}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.properties.empty());
}

/**
 * @tc.name: StyleParserTest005
 * @tc.desc: Verify StyleParser::Parse with non-object input returns error.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest005)
{
    auto adapter = JsonAdapter::Parse(R"([1, 2, 3])");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @tc.name: StyleParserTest006
 * @tc.desc: Verify StyleParser::Parse with null/invalid input returns success with empty properties.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest006)
{
    auto adapter = JsonAdapter::Parse("null");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.properties.empty());
}

/**
 * @tc.name: StyleParserTest007
 * @tc.desc: Verify StyleParser::ToPropertyName with special properties.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest007)
{
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSize"), StylePropertyName::BACKGROUND_IMAGE_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundimageSize"), StylePropertyName::BACKGROUND_IMAGE_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("linearGradient"), StylePropertyName::LINEAR_GRADIENT);
    EXPECT_EQ(StyleParser::ToPropertyName("unknownProperty"), StylePropertyName::UNKNOWN);
}

/**
 * @tc.name: StyleParserTest008
 * @tc.desc: Verify StyleParser::IsCompositeProperty for all property types.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest008)
{
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::PADDING));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::MARGIN));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::BORDER_RADIUS));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::SHADOW));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::BACKGROUND_IMAGE_SIZE));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::LINEAR_GRADIENT));
    EXPECT_TRUE(StyleParser::IsCompositeProperty(StylePropertyName::DECORATION));
    EXPECT_FALSE(StyleParser::IsCompositeProperty(StylePropertyName::WIDTH));
    EXPECT_FALSE(StyleParser::IsCompositeProperty(StylePropertyName::UNKNOWN));
}

/**
 * @tc.name: StyleParserTest009
 * @tc.desc: Verify StyleParser::Parse with path binding value.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest009)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": {"path": "/data/value"}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 1u);
    EXPECT_EQ(result.properties[0].kind, StyleValueKind::PATH_BINDING);
}

/**
 * @tc.name: StyleParserTest010
 * @tc.desc: Verify StyleParser::Parse with function call value.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest010)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": {"call": "myFunc"}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 1u);
    EXPECT_EQ(result.properties[0].kind, StyleValueKind::FUNCTION_CALL);
}

/**
 * @tc.name: StyleParserTest011
 * @tc.desc: Verify StyleParser::Parse with expression string.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest011)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": "{{some.expression}}"
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 1u);
    EXPECT_EQ(result.properties[0].kind, StyleValueKind::EXPRESSION);
}

/**
 * @tc.name: StyleParserTest012
 * @tc.desc: Verify StyleParser::Parse with composite object on padding.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest012)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {"top": 10, "right": 20}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 1u);
    EXPECT_EQ(result.properties[0].kind, StyleValueKind::COMPOSITE_OBJECT);
}

/**
 * @tc.name: StyleParserTest013
 * @tc.desc: Verify StyleParser::Parse with unknown property object is STATIC_VALUE.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest013)
{
    auto adapter = JsonAdapter::Parse(R"({
        "unknownProp": {"top": 10, "right": 20}
    })");
    ASSERT_NE(adapter, nullptr);
    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 1u);
    EXPECT_EQ(result.properties[0].name, StylePropertyName::UNKNOWN);
}

/**
 * @tc.name: StyleParserTest014
 * @tc.desc: Verify StyleParser correctly maps minFontSize/maxFontSize to text font size properties.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest014)
{
    EXPECT_EQ(StyleParser::ToPropertyName("minFontSize"), StylePropertyName::TEXT_MIN_FONT_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("maxFontSize"), StylePropertyName::TEXT_MAX_FONT_SIZE);
}

/**
 * @tc.name: ExtendedStyleResolverTest035
 * @tc.desc: Verify ConvertDimensionToFloat with FP unit.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest035)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"styles": {"width": "10fp"}})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest036
 * @tc.desc: Verify ConvertDimensionToFloat with WRAP_CONTENT unit.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest036)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"width": "wrap_content"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest037
 * @tc.desc: Verify ParseEdgeStyle with individual edge keys (top/right/bottom/left).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest037)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "paddingTop": "5vp",
        "paddingRight": "10vp",
        "paddingBottom": "15vp",
        "paddingLeft": "20vp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest038
 * @tc.desc: Verify ParseEdgeStyle with individual margin keys.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest038)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "marginTop": "5vp",
        "marginRight": "10vp",
        "marginBottom": "15vp",
        "marginLeft": "20vp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest039
 * @tc.desc: Verify style resolution with invalid but present values triggers warning paths.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest039)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "fontWeight": -999,
        "maxLines": -1,
        "textOverflow": "invalidOverflow",
        "textAlign": "invalidAlign",
        "wordBreak": "invalidBreak"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

TEST(StyleApplyUtilsTextTest, L0_should_reject_removed_text_align_alias_values)
{
    const std::array<const char*, 6> removedAliases = { "left", "leftToRight", "ltr", "right", "rightToLeft", "rtl" };
    for (const char* alias : removedAliases) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(std::string("\"") + alias + "\"");
        ASSERT_NE(adapter, nullptr);

        int32_t textAlign = -1;
        EXPECT_FALSE(StyleApplyUtils::ParseTextAlign(adapter->GetRoot(), textAlign)) << alias;
        EXPECT_EQ(textAlign, -1) << alias;
    }
}

/**
 * @tc.name: ExtendedStyleResolverTest040
 * @tc.desc: Verify ApplyDimension with invalid dimension value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest040)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"width": "invalid_value"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    EXPECT_FALSE(applier.HasWidthPercent());
}

/**
 * @tc.name: ExtendedStyleResolverTest041
 * @tc.desc: Verify ApplyTextComponentStyles with minFontSize=0 (invalid).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest041)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "minFontSize": 0,
        "maxFontSize": 0
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest042
 * @tc.desc: Verify borderColor style resolution.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest042)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"borderColor": "#FF5733"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest043
 * @tc.desc: Verify placeholderColor and caretColor resolution.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest043)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "placeholderColor": "#CCCCCC",
        "caretColor": "#0000FF",
        "showUnderline": true
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest044
 * @tc.desc: Verify clip style resolution.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest044)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"clip": true})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest045
 * @tc.desc: Verify backgroundImage style resolution.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest045)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"backgroundImage": "test.png"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest046
 * @tc.desc: Verify decoration style resolution.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest046)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"decoration": {"type": "underline", "color": "#FF0000"}})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest047
 * @tc.desc: Verify Resolve with null styles.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest047)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse("null");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest048
 * @tc.desc: Verify backgroundColor with invalid value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest048)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"backgroundColor": "not_a_color"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest049
 * @tc.desc: Verify fontColor with invalid value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest049)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"fontColor": "not_a_color"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest050
 * @tc.desc: Verify width/height with match_parent route to the ArkUI layout-policy API (MATCH_PARENT).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest050)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"width": "match_parent", "height": "match_parent"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    ASSERT_TRUE(applier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));
    ASSERT_TRUE(applier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));
}

/**
 * @tc.name: ExtendedStyleResolverTest051
 * @tc.desc: Verify constraintSize with all four fields.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest051)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10vp",
            "maxWidth": "200vp",
            "minHeight": "20vp",
            "maxHeight": "300vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    EXPECT_TRUE(applier.HasConstraintSize());
}

/**
 * @tc.name: ExtendedStyleResolverTest052
 * @tc.desc: Verify borderRadius with object having all corners.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest052)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {
            "topLeft": "10vp",
            "topRight": "20vp",
            "bottomRight": "30vp",
            "bottomLeft": "40vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest053
 * @tc.desc: Verify borderWidth with object having all edges.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest053)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": {
            "top": "1vp",
            "right": "2vp",
            "bottom": "3vp",
            "left": "4vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest054
 * @tc.desc: Verify opacity with invalid value triggers warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest054)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"opacity": "not_a_number"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest055
 * @tc.desc: Verify visibility with invalid value triggers warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest055)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({"visibility": "invalid"})");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest056
 * @tc.desc: Verify shadow with custom type.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest056)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "shadow": {
            "kind": "custom",
            "color": "#80000000",
            "offsetX": 5,
            "offsetY": 5,
            "radius": 10,
            "isFilled": true
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest057
 * @tc.desc: Verify fp unit dimension via ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest057)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "width": "10fp",
        "height": "20fp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest058
 * @tc.desc: Verify non-object input via ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest058)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"("not_an_object")");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest059
 * @tc.desc: Verify ApplyTextComponentStyles with many properties.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest059)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "fontWeight": "medium",
        "maxLines": 5,
        "minFontSize": "8fp",
        "maxFontSize": "24fp",
        "textOverflow": "clip",
        "textAlign": "end",
        "wordBreak": "breakAll",
        "decoration": {"type": "linethrough"}
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(adapter->GetRoot(), applier);
    ASSERT_TRUE(applier.HasFontWeight());
    EXPECT_EQ(applier.GetFontWeight(), ARKUI_FONT_WEIGHT_MEDIUM);
}

/**
 * @tc.name: ExtendedStyleResolverTest060
 * @tc.desc: Verify ResolveAndApply with comprehensive styles.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest060)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "visibility": "hidden",
        "opacity": 0.5,
        "clip": false,
        "layoutWeight": 3,
        "flexShrink": 0.5,
        "constraintSize": {
            "minWidth": "5vp",
            "maxWidth": "100vp",
            "minHeight": "10vp",
            "maxHeight": "200vp"
        },
        "backgroundImage": "img.png",
        "backgroundImageSizeWithStyle": 2,
        "linearGradient": {
            "direction": "topleft",
            "colors": [["#FF0000", 0.0], ["#0000FF", 1.0]]
        },
        "fontWeight": "bold"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    ASSERT_TRUE(applier.HasFontWeight());
    EXPECT_EQ(applier.GetFontWeight(), ARKUI_FONT_WEIGHT_BOLD);
}

/**
 * @tc.name: ExtendedStyleResolverTest061
 * @tc.desc: Verify constraintSize with missing fields.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest061)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest062
 * @tc.desc: Verify constraintSize with non-object value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest062)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": "invalid"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest063
 * @tc.desc: Verify borderRadius with percent value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest063)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": "50%"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest064
 * @tc.desc: Verify Reset with unknown property.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest064)
{
    RecordingCommonStyleApplier applier;
    StyleResetProperty prop;
    prop.name = StylePropertyName::UNKNOWN;
    prop.rawName = "unknownProp";
    ExtendedStyleResolver::Reset(prop, applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest065
 * @tc.desc: Verify backgroundimageSizeWithStyle via ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest065)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundimageSizeWithStyle": 1
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest066
 * @tc.desc: Verify linearGradient with repeating.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest066)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "linearGradient": {
            "direction": "right",
            "colors": [["#FF0000", 0.0], ["#00FF00", 0.5], ["#0000FF", 1.0]],
            "repeating": true
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest067
 * @tc.desc: Verify shadow with STYLE kind.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest067)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "shadow": {
            "kind": "style",
            "color": "#FF000000",
            "offsetX": 1,
            "offsetY": 1,
            "radius": 3,
            "isFilled": true
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest068
 * @tc.desc: Verify decoration with color, style, and thicknessScale.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest068)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "underline",
            "color": "#FF0000",
            "style": 1,
            "thicknessScale": 2.0
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest069
 * @tc.desc: Verify backgroundImageSize via ResolveAndApply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest069)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundImageSize": {"width": 100, "height": 200}
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest070
 * @tc.desc: Verify ResolveAndApply with backgroundimage (lowercase i).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest070)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundimage": "test.png"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest071
 * @tc.desc: Verify ParseColor public API.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest071)
{
    auto adapter = JsonAdapter::Parse(R"("#FF5733")");
    ASSERT_NE(adapter, nullptr);
    uint32_t color = 0;
    EXPECT_TRUE(ExtendedStyleResolver::ParseColor(adapter->GetRoot(), color));
    EXPECT_NE(color, 0u);
}

/**
 * @tc.name: ExtendedStyleResolverTest072
 * @tc.desc: Verify ResolveAndApply with fontSize and fontWeight.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest072)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "fontSize": "16fp",
        "fontWeight": "bold"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest073
 * @tc.desc: Verify ResolveAndApply with decoration and shadow.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest073)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "decoration": {"type": "overline", "color": "#FF0000"},
        "shadow": {
            "kind": "style",
            "color": "#80000000",
            "offsetX": 2,
            "offsetY": 2,
            "radius": 4,
            "isFilled": false
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest074
 * @tc.desc: Verify ResolveAndApply with all edge styles as VP.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest074)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "10vp",
        "margin": "5vp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest075
 * @tc.desc: Verify ResolveAndApply with color styles.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest075)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundColor": "#FF0000",
        "fontColor": "#333333",
        "borderColor": "#000000",
        "placeholderColor": "#CCCCCC"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest076
 * @tc.desc: Verify ResolveAndApply with borderWidth object and borderRadius object.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest076)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": {"top": "1vp", "right": "2vp", "bottom": "3vp", "left": "4vp"},
        "borderRadius": {"topLeft": "5vp", "topRight": "6vp", "bottomRight": "7vp", "bottomLeft": "8vp"}
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest077
 * @tc.desc: Verify ResolveAndApply with padding/margin individual keys + all.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest077)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "10vp",
        "paddingTop": "5vp",
        "margin": "8vp",
        "marginTop": "4vp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest078
 * @tc.desc: Verify ResolveAndApply with null node handle.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest078)
{
    RecordingCommonStyleApplier applier;
    applier.SetReturnNullRootNode(true);
    auto adapter = JsonAdapter::Parse(R"({
        "width": "100vp",
        "backgroundColor": "#FF0000",
        "fontWeight": "bold",
        "opacity": 0.5,
        "visibility": "visible",
        "clip": true,
        "layoutWeight": 1,
        "flexShrink": 0.5,
        "borderRadius": "10vp",
        "borderWidth": "2vp",
        "padding": "5vp",
        "margin": "3vp"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest079
 * @tc.desc: Verify ResolveAndApply with backgroundImageSize object.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest079)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundImageSize": {"width": 100, "height": 200}
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest080
 * @tc.desc: Verify ApplyTextComponentStyles with null root node.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest080)
{
    RecordingCommonStyleApplier applier;
    applier.SetReturnNullRootNode(true);
    auto adapter = JsonAdapter::Parse(R"({
        "fontWeight": "bold",
        "maxLines": 3
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest081
 * @tc.desc: Verify ApplyTextComponentStyles with non-object input.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest081)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"("not_object")");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest082
 * @tc.desc: Verify Reset with all property types.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest082)
{
    RecordingCommonStyleApplier applier;
    StyleResetProperty prop;
    prop.rawName = "width";
    for (int i = 0; i <= 18; ++i) {
        prop.name = static_cast<StylePropertyName>(i);
        ExtendedStyleResolver::Reset(prop, applier);
    }
}

/**
 * @tc.name: ExtendedStyleResolverTest083
 * @tc.desc: Verify ResolveAndApply with fontSize invalid.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest083)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "fontSize": "invalid"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest084
 * @tc.desc: Verify ResolveAndApply with invalid collection values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest084)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundColor": [],
        "borderRadius": [],
        "borderWidth": [],
        "shadow": "not_object"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest085
 * @tc.desc: Verify null root with many properties covers LOG paths.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest085)
{
    RecordingCommonStyleApplier applier;
    applier.SetReturnNullRootNode(true);
    auto adapter = JsonAdapter::Parse(R"({
        "width": "100vp",
        "height": "50vp",
        "backgroundColor": "#FF0000",
        "fontSize": "16fp",
        "opacity": 0.5,
        "visibility": "hidden",
        "clip": true,
        "layoutWeight": 1,
        "flexShrink": 0,
        "borderRadius": "10vp",
        "borderWidth": "2vp",
        "borderColor": "#000000",
        "padding": "5vp",
        "margin": "3vp",
        "shadow": {"kind": "style", "color": "#000000", "offsetX": 1, "offsetY": 1, "radius": 2},
        "decoration": {"type": "underline"},
        "linearGradient": {"direction": "bottom", "colors": [["#FF0000", 0.0]]},
        "backgroundImage": "test.png",
        "backgroundImageSizeWithStyle": 1,
        "constraintSize": {"minWidth": "10vp", "maxWidth": "100vp", "minHeight": "5vp", "maxHeight": "50vp"}
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest086
 * @tc.desc: Verify padding with mixed zero/non-zero dimensions triggers continue path.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest086)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {
            "top": "10vp",
            "right": "0vp",
            "bottom": "5vp",
            "left": "0vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest087
 * @tc.desc: Verify margin with mixed zero/non-zero dimensions.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest087)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "margin": {
            "top": "0vp",
            "right": "10vp",
            "bottom": "0vp",
            "left": "5vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest088
 * @tc.desc: Verify borderWidth with all zero values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest088)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": {
            "top": "0vp",
            "right": "1vp",
            "bottom": "0vp",
            "left": "2vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest089
 * @tc.desc: Verify borderRadius with all zero values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest089)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {
            "topLeft": "0vp",
            "topRight": "5vp",
            "bottomRight": "0vp",
            "bottomLeft": "3vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest090
 * @tc.desc: Verify borderWidth as percent string.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest090)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "50%"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest091
 * @tc.desc: Verify borderRadius with wrap_content hits ConvertDimensionToFloat WRAP_CONTENT path.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest091)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": "wrap_content"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest092
 * @tc.desc: Verify borderRadius with fixAtIdealSize.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest092)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": "fixAtIdealSize"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest093
 * @tc.desc: Verify borderWidth with wrap_content.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest093)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "wrap_content"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest094
 * @tc.desc: Verify constraintSize with non-VP/FP dimension type triggers error path.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest094)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10%",
            "maxWidth": "100%",
            "minHeight": "20%",
            "maxHeight": "200%"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest095
 * @tc.desc: Verify constraintSize with mix of valid and invalid dimension types.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest095)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "constraintSize": {
            "minWidth": "10vp",
            "maxWidth": "100vp",
            "minHeight": "wrap_content",
            "maxHeight": "200vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest096
 * @tc.desc: Verify border radius with match_parent value.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest096)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": "match_parent"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest097
 * @tc.desc: Verify padding with unparseable value triggers LOG warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest097)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "invalid_padding"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest098
 * @tc.desc: Verify margin with unparseable value triggers LOG warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest098)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "margin": true
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest099
 * @tc.desc: Verify padding with mixed percent and absolute triggers LOG warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest099)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {
            "top": "10%",
            "right": "5vp",
            "bottom": "10%",
            "left": "5vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest100
 * @tc.desc: Verify margin with mixed percent and absolute triggers LOG warning.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest100)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "margin": {
            "top": "5vp",
            "right": "10%",
            "bottom": "5vp",
            "left": "10%"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest101
 * @tc.desc: Verify borderWidth with match_parent triggers invalid unit LOG path.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest101)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderWidth": "match_parent"
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest102
 * @tc.desc: Verify borderRadius with all same VP values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest102)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {
            "topLeft": "10vp",
            "topRight": "10vp",
            "bottomRight": "10vp",
            "bottomLeft": "10vp"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

/**
 * @tc.name: ExtendedStyleResolverTest103
 * @tc.desc: Verify borderRadius with all same percent values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest103)
{
    RecordingCommonStyleApplier applier;
    auto adapter = JsonAdapter::Parse(R"({
        "borderRadius": {
            "topLeft": "25%",
            "topRight": "25%",
            "bottomRight": "25%",
            "bottomLeft": "25%"
        }
    })");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

TEST(StyleApplyUtilsTextTest, StyleApplyUtilsTextTest001)
{
    int32_t maxLines = 0;
    auto zeroValue = JsonAdapter::Parse("0");
    ASSERT_NE(zeroValue, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseMaxLines(zeroValue->GetRoot(), maxLines));
    EXPECT_EQ(maxLines, 0);

    auto fractionalValue = JsonAdapter::Parse("0.5");
    ASSERT_NE(fractionalValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseMaxLines(fractionalValue->GetRoot(), maxLines));

    auto boolValue = JsonAdapter::Parse("true");
    ASSERT_NE(boolValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseMaxLines(boolValue->GetRoot(), maxLines));

    auto nanValue = JsonAdapter::CreateObject();
    ASSERT_NE(nanValue, nullptr);
    JsonValue nanRoot = nanValue->GetRoot();
    ASSERT_TRUE(nanRoot.PutNumber("value", std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(StyleApplyUtils::ParseMaxLines(nanRoot.GetItem("value"), maxLines));

    auto negativeValue = JsonAdapter::Parse("-1");
    ASSERT_NE(negativeValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseMaxLines(negativeValue->GetRoot(), maxLines));

    auto validValue = JsonAdapter::Parse("2");
    ASSERT_NE(validValue, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseMaxLines(validValue->GetRoot(), maxLines));
    EXPECT_EQ(maxLines, 2);

    auto invalidStringValue = JsonAdapter::Parse(R"("bad")");
    ASSERT_NE(invalidStringValue, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseMaxLines(invalidStringValue->GetRoot(), maxLines));

    auto validStringValue = JsonAdapter::Parse(R"("3")");
    ASSERT_NE(validStringValue, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseMaxLines(validStringValue->GetRoot(), maxLines));
    EXPECT_EQ(maxLines, 3);
}

TEST(StyleApplyUtilsTextTest, StyleApplyUtilsTextTest002)
{
    StyleTextDecoration decoration;

    auto missingStyleAndThickness = JsonAdapter::Parse(R"({
        "type": "underline"
    })");
    ASSERT_NE(missingStyleAndThickness, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseTextDecoration(missingStyleAndThickness->GetRoot(), decoration));
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);

    auto invalidStyleAndThickness = JsonAdapter::Parse(R"({
        "type": "underline",
        "style": "bad",
        "thicknessScale": "bad"
    })");
    ASSERT_NE(invalidStyleAndThickness, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseTextDecoration(invalidStyleAndThickness->GetRoot(), decoration));
    EXPECT_FALSE(decoration.hasStyle);
    EXPECT_FALSE(decoration.hasThicknessScale);

    auto invalidType = JsonAdapter::Parse(R"({
        "type": 1
    })");
    ASSERT_NE(invalidType, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseTextDecoration(invalidType->GetRoot(), decoration));

    auto invalidToken = JsonAdapter::Parse(R"({
        "type": "unsupported"
    })");
    ASSERT_NE(invalidToken, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseTextDecoration(invalidToken->GetRoot(), decoration));

    auto missingType = JsonAdapter::Parse(R"({
        "style": "dashed",
        "thicknessScale": "2"
    })");
    ASSERT_NE(missingType, nullptr);
    EXPECT_TRUE(StyleApplyUtils::ParseTextDecoration(missingType->GetRoot(), decoration));
    EXPECT_FALSE(decoration.hasColor);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 3);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 2.0F);
}
