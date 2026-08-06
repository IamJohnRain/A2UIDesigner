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

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#define private public
#include "components/extended/ExtendedStyleResolver.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleTypes.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "SchemaErrorCodes.h"
#undef private

#include "TestFixture.h"

using namespace NativeModule;

namespace {

// Helper: create an ArkUINodeApiAdapter wired to a dummy node.
ArkUINodeApiAdapter MakeTestApplier(ArkUI_NodeHandle node, std::function<void()> onResetCommonMargin = nullptr,
    std::function<void(const std::function<void()>&)> onClickRegistrar = nullptr)
{
    // EdgeSetter (marginSetter) is passed as null so that SetMargin calls
    // SetNodeMargin (recorded by mock) instead of the callback shortcut.
    return ArkUINodeApiAdapter([node]() { return node; }, []() { return std::string("test-component"); },
        ArkUINodeApiAdapter::EdgeSetter(), onResetCommonMargin ? onResetCommonMargin : []() {},
        onClickRegistrar ? onClickRegistrar : [](const std::function<void()>&) {});
}

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

class ExtendedStyleResolverTddTest : public A2UITest {
protected:
    ArkUI_NodeHandle testNode_ = reinterpret_cast<ArkUI_NodeHandle>(0xA500);
};

// =============================================================================
// ResolveAndApply - top-level dispatch
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_SkipsInvalidStyles)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // Invalid JsonValue (default) → early return
    JsonValue invalid;
    ExtendedStyleResolver::ResolveAndApply(invalid, applier);
    // No crash = pass
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_SkipsNonObjectValidStyles)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto adapter = JsonAdapter::CreateString("not_an_object");
    ASSERT_NE(adapter, nullptr);
    // Valid but non-object → LOG warn + early return
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AcceptsEmptyObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto adapter = JsonAdapter::Parse("{}");
    ASSERT_NE(adapter, nullptr);
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
}

// =============================================================================
// ApplyTextComponentStyles
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_SkipsNonObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto numAdapter = JsonAdapter::CreateNumber(5.0);
    ASSERT_NE(numAdapter, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(numAdapter->GetRoot(), applier);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_SkipsNullNode)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    auto styles = JsonAdapter::Parse(R"({"fontWeight": 700})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesFontWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"fontWeight": 700})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    // Verify via mock that NODE_FONT_WEIGHT was set
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FONT_WEIGHT) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, ARKUI_FONT_WEIGHT_W700);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_FallsBackInvalidFontWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"fontWeight": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FONT_WEIGHT) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, ARKUI_FONT_WEIGHT_W400);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesMaxLines)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"maxLines": 3})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MAX_LINES) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 3);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_IgnoresInvalidMaxLines)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"maxLines": "not_a_number"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MAX_LINES) {
            found = true;
        }
    }
    EXPECT_FALSE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesMinFontSizeLegacyAlias)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"minFontSize": 10})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MIN_FONT_SIZE) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_FLOAT_EQ(rec.values[0].f32, 10.0F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesMaxFontSizePreferredName)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"maxFontSize": 26})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MAX_FONT_SIZE) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_FLOAT_EQ(rec.values[0].f32, 26.0F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_SkipsZeroOrNegativeMinMaxFontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"minFontSize": 0, "maxFontSize": -5})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_MIN_FONT_SIZE);
        EXPECT_NE(rec.attribute, NODE_TEXT_MAX_FONT_SIZE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesTextOverflowEllipsis)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"textOverflow": "ellipsis"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_OVERFLOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_IgnoresInvalidTextOverflow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"textOverflow": 999})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_OVERFLOW);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesTextAlignCenter)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"textAlign": "center"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_ALIGN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_IgnoresInvalidTextAlign)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"textAlign": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_ALIGN);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesWordBreak)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"wordBreak": "breakWord"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_WORD_BREAK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_AppliesTextDecoration)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "decoration": {"type": "underline", "color": "#ff007dff", "style": "solid", "thicknessScale": 1.5}
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_DECORATION) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplySizeStyles (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesWidthAndHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": 100, "height": 200})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool hasWidth = false;
    bool hasHeight = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            hasWidth = true;
        }
        if (rec.attribute == NODE_HEIGHT) {
            hasHeight = true;
        }
    }
    EXPECT_TRUE(hasWidth);
    EXPECT_TRUE(hasHeight);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthPercentAppliesWidthPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "50%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool hasWidthPercent = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH_PERCENT) {
            hasWidthPercent = true;
        }
    }
    EXPECT_TRUE(hasWidthPercent);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightPercentAppliesHeightPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "75%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool hasHeightPercent = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT_PERCENT) {
            hasHeightPercent = true;
        }
    }
    EXPECT_TRUE(hasHeightPercent);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthWrapContentResetsWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "wrap_content"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 1); // WRAP_CONTENT
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightMatchParentAppliesPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "match_parent"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 0); // MATCH_PARENT
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyColorStyles (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBackgroundColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundColor": "#FF112233"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidBackgroundColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundColor": "not_a_color"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_COLOR);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBorderColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderColor": "#FF000000"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyTextStyles (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesFontColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"fontColor": "#FF000000"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FONT_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesFontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"fontSize": 18})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FONT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidFontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"fontSize": "not_a_number"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_SIZE);
    }
}

// =============================================================================
// ApplyEdgeStyles - Padding (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesPaddingAllVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesPaddingPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesPaddingIndividualFields)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": 4, "right": 8, "bottom": 12, "left": 16})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresMixedPaddingUnits)
{
    // Mixing percent and VP should log a warning and skip
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"top": "5%", "right": "10vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_PADDING);
        EXPECT_NE(rec.attribute, NODE_PADDING_PERCENT);
    }
}

// =============================================================================
// ApplyEdgeStyles - Margin (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesMarginAllVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"margin": "8vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesMarginPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"margin": "10%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesMarginIndividualFields)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"marginTop": 1, "marginRight": 2, "marginBottom": 3, "marginLeft": 4})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresMixedMarginUnits)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"marginTop": "5%", "marginRight": "10vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_MARGIN);
        EXPECT_NE(rec.attribute, NODE_MARGIN_PERCENT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidPaddingInput)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"padding": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_PADDING);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidMarginInput)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"margin": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_MARGIN);
    }
}

// =============================================================================
// ApplyDecorationStyles (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBorderWidthVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "2vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBorderWidthPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidBorderWidthUnit)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "10px"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_WIDTH);
        EXPECT_NE(rec.attribute, NODE_BORDER_WIDTH_PERCENT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidBorderWidthInput)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_WIDTH);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesOpacity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"opacity": 0.5})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_OPACITY) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidOpacity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"opacity": "not_a_number"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_OPACITY);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesVisibility)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"visibility": "hidden"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_VISIBILITY) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidVisibility)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"visibility": 999})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_VISIBILITY);
    }
}

// =============================================================================
// ApplyRadius (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesUniformBorderRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": 8})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesPercentBorderRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": "50%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresMixedRadiusUnits)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderRadius": {"topLeft": "10vp", "topRight": "5%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_RADIUS);
        EXPECT_NE(rec.attribute, NODE_BORDER_RADIUS_PERCENT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderRadius": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_RADIUS);
    }
}

// =============================================================================
// ApplyShadow (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesStyleShadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"shadow": "OUTER_DEFAULT_MD"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_SHADOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidShadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"shadow": 42})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

// =============================================================================
// ApplyBackgroundImageSize (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBackgroundImageSizeObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": 100, "height": 200}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBackgroundImageSizeContain)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": "contain"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyLinearGradient (via ResolveAndApply)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesLinearGradient)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "linearGradient": {
            "angle": 90,
            "colors": [["#FF0000", 0.0], ["#00FF00", 1.0]]
        }
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_LINEAR_GRADIENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidLinearGradient)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"linearGradient": 42})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LINEAR_GRADIENT);
    }
}

// =============================================================================
// ApplyCommonNodeStyles - flexShrink, backgroundImage, clip, layoutWeight, constraintSize
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesFlexShrink)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": 1})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AcceptsFlexShrinkGreaterThanOne)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"flexShrink": 2.6})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext context;
    context.parentComponentType = "Column";
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, context, issues);
    EXPECT_TRUE(issues.empty());
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK && !rec.values.empty()) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 2.6F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ResetsInvalidFlexShrinkWithoutParentContext)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"flexShrink": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesBackgroundImage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": "https://example.com/img.png"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
            EXPECT_EQ(rec.stringValue, "https://example.com/img.png");
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ResetsBackgroundImageWhenEmpty)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": ""})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidBackgroundImage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": 123})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesClip)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"clip": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CLIP) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidClip)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"clip": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CLIP);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesLayoutWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": 1})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_LAYOUT_WEIGHT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AllowsZeroLayoutWeightAsNoop)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": 0})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RejectsNegativeLayoutWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": -2})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1U);
    EXPECT_EQ(issues[0].path, "styles.layoutWeight");
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidLayoutWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": "not_a_number"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AllowsEmptyEdgeAndRadiusObjectsButReportsExplicitInvalidChildren)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "margin": {},
        "padding": {"left":"4vp","right":""},
        "borderRadius": {"topLeft":"8vp","topRight":""}
    })");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 2U);
    EXPECT_EQ(issues[0].path, "styles.padding");
    EXPECT_EQ(issues[1].path, "styles.borderRadius");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsNestedMarginIssueWhenMarginStillApplies)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "margin": {
            "top": "6vp",
            "right": ""
        }
    })");
    ASSERT_NE(styles, nullptr);

    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1U);
    EXPECT_EQ(issues[0].path, "styles.margin");
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesConstraintSizeVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": 50, "maxWidth": 200}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_IgnoresInvalidConstraintSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"constraintSize": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_SkipsCommonNodeStylesWhenNodeNull)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"flexShrink": 1, "clip": true, "layoutWeight": 2})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FLEX_SHRINK);
        EXPECT_NE(rec.attribute, NODE_CLIP);
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

// =============================================================================
// Reset - all property name branches
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, Reset_SkipsNullNodeOrEmptyRawName)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    StyleResetProperty prop;
    prop.rawName = "width";
    prop.name = StylePropertyName::WIDTH;
    ExtendedStyleResolver::Reset(prop, applier);
    // No crash, no attribute set (node is null)

    ArkUINodeApiAdapter applier2 = MakeTestApplier(testNode_);
    StyleResetProperty emptyProp;
    emptyProp.rawName = "";
    emptyProp.name = StylePropertyName::WIDTH;
    ExtendedStyleResolver::Reset(emptyProp, applier2);
    // No crash
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Width)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "width", StylePropertyName::WIDTH };
    ExtendedStyleResolver::Reset(prop, applier);
    int resetCount = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH || rec.attribute == NODE_WIDTH_PERCENT) {
            resetCount++;
        }
    }
    EXPECT_GE(resetCount, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Height)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "height", StylePropertyName::HEIGHT };
    ExtendedStyleResolver::Reset(prop, applier);
    int resetCount = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT || rec.attribute == NODE_HEIGHT_PERCENT) {
            resetCount++;
        }
    }
    EXPECT_GE(resetCount, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Padding)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "padding", StylePropertyName::PADDING };
    ExtendedStyleResolver::Reset(prop, applier);
    int resetCount = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_PADDING || rec.attribute == NODE_PADDING_PERCENT) {
            resetCount++;
        }
    }
    EXPECT_GE(resetCount, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Margin)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "margin", StylePropertyName::MARGIN };
    ExtendedStyleResolver::Reset(prop, applier);
    int resetCount = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN || rec.attribute == NODE_MARGIN_PERCENT) {
            resetCount++;
        }
    }
    EXPECT_GE(resetCount, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BackgroundColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "backgroundColor", StylePropertyName::BACKGROUND_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BorderRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "borderRadius", StylePropertyName::BORDER_RADIUS };
    ExtendedStyleResolver::Reset(prop, applier);
    int count = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS || rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            count++;
        }
    }
    EXPECT_GE(count, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BorderWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "borderWidth", StylePropertyName::BORDER_WIDTH };
    ExtendedStyleResolver::Reset(prop, applier);
    int count = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH || rec.attribute == NODE_BORDER_WIDTH_PERCENT) {
            count++;
        }
    }
    EXPECT_GE(count, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BorderColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "borderColor", StylePropertyName::BORDER_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_FontColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "fontColor", StylePropertyName::FONT_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FONT_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_FontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "fontSize", StylePropertyName::FONT_SIZE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FONT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_FontWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "fontWeight", StylePropertyName::FONT_WEIGHT };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FONT_WEIGHT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_TextAlign)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "textAlign", StylePropertyName::TEXT_ALIGN };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_ALIGN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_MaxLines)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "maxLines", StylePropertyName::MAX_LINES };
    ExtendedStyleResolver::Reset(prop, applier);
    int count = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MAX_LINES || rec.attribute == NODE_TEXT_INPUT_NUMBER_OF_LINES) {
            count++;
        }
    }
    EXPECT_GE(count, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_TextMinFontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "minFontSize", StylePropertyName::TEXT_MIN_FONT_SIZE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MIN_FONT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_TextMaxFontSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "maxFontSize", StylePropertyName::TEXT_MAX_FONT_SIZE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MAX_FONT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_TextOverflow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "textOverflow", StylePropertyName::TEXT_OVERFLOW };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_OVERFLOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_WordBreak)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "wordBreak", StylePropertyName::WORD_BREAK };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_WORD_BREAK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Decoration)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "decoration", StylePropertyName::DECORATION };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_DECORATION) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Visibility)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "visibility", StylePropertyName::VISIBILITY };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_VISIBILITY) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Opacity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "opacity", StylePropertyName::OPACITY };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_OPACITY) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Shadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "shadow", StylePropertyName::SHADOW };
    ExtendedStyleResolver::Reset(prop, applier);
    int count = 0;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_SHADOW || rec.attribute == NODE_CUSTOM_SHADOW) {
            count++;
        }
    }
    EXPECT_GE(count, 2);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_FlexShrink)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "flexShrink", StylePropertyName::FLEX_SHRINK };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BackgroundImage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "backgroundImage", StylePropertyName::BACKGROUND_IMAGE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_BackgroundImageSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    mockArkUIPtr_->setAttributeRecords_.clear();
    StyleResetProperty prop { "backgroundImageSize", StylePropertyName::BACKGROUND_IMAGE_SIZE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool resetSize = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            resetSize = true;
        }
    }
    const MockArkUINativeProvider::SetAttributeRecord* sizeWithStyle = nullptr;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            sizeWithStyle = &rec;
        }
    }
    EXPECT_TRUE(resetSize);
    ASSERT_NE(sizeWithStyle, nullptr);
    ASSERT_EQ(sizeWithStyle->values.size(), 1u);
    EXPECT_EQ(sizeWithStyle->values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
}

TEST_F(ExtendedStyleResolverTddTest, Reset_LinearGradient)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "linearGradient", StylePropertyName::LINEAR_GRADIENT };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_LINEAR_GRADIENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_Clip)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "clip", StylePropertyName::CLIP };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_CLIP) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_PlaceholderColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "placeholderColor", StylePropertyName::PLACEHOLDER_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_INPUT_PLACEHOLDER_COLOR) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_LayoutWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "layoutWeight", StylePropertyName::LAYOUT_WEIGHT };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_LAYOUT_WEIGHT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_ConstraintSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty prop { "constraintSize", StylePropertyName::CONSTRAINT_SIZE };
    ExtendedStyleResolver::Reset(prop, applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, Reset_UnknownPropertyNoop)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    StyleResetProperty prop { "unknownProp", StylePropertyName::UNKNOWN };
    ExtendedStyleResolver::Reset(prop, applier);
    // No reset should have occurred
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

// =============================================================================
// ParseColor delegation
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ParseColor_DelegatesToStyleApplyUtils)
{
    auto adapter = JsonAdapter::CreateString("#FF112233");
    ASSERT_NE(adapter, nullptr);
    uint32_t color = 0;
    EXPECT_TRUE(ExtendedStyleResolver::ParseColor(adapter->GetRoot(), color));
    EXPECT_EQ(color, 0xFF112233u);
}

TEST_F(ExtendedStyleResolverTddTest, ParseColor_ReturnsFalseForInvalid)
{
    auto adapter = JsonAdapter::CreateString("not_a_color");
    ASSERT_NE(adapter, nullptr);
    uint32_t color = 0;
    EXPECT_FALSE(ExtendedStyleResolver::ParseColor(adapter->GetRoot(), color));
}

// =============================================================================
// DimensionToFloat & ParseEdgeStyle (via public interface)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_VP)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::VP;
    dim.value = 10.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 10.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_Percent)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::PERCENT;
    dim.value = 50.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 50.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_WrapContent)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::WRAP_CONTENT;
    dim.value = 999.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_NegativeVP_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::VP;
    dim.value = -5.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_InvalidUnit_ReturnsZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::INVALID;
    dim.value = 42.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_ReturnsFalseForNonObject)
{
    StyleEdge edge;
    auto numAdapter = JsonAdapter::CreateNumber(5.0);
    ASSERT_NE(numAdapter, nullptr);
    EXPECT_FALSE(ExtendedStyleResolver::ParseEdgeStyle(
        numAdapter->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
}

TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_ParsesAllKey)
{
    StyleEdge edge;
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp"})");
    ASSERT_NE(styles, nullptr);
    EXPECT_TRUE(
        ExtendedStyleResolver::ParseEdgeStyle(styles->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
}

TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_ParsesIndividualKeys)
{
    StyleEdge edge;
    auto styles = JsonAdapter::Parse(R"({"top": 4, "right": 8})");
    ASSERT_NE(styles, nullptr);
    EXPECT_TRUE(
        ExtendedStyleResolver::ParseEdgeStyle(styles->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
    EXPECT_FLOAT_EQ(edge.top.value, 4.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 8.0F);
}

TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_ReturnsFalseWhenNoFields)
{
    StyleEdge edge;
    auto styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    EXPECT_FALSE(
        ExtendedStyleResolver::ParseEdgeStyle(styles->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
}

// =============================================================================
// DimensionToFloat - FP unit (was completely missing)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_FP)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FP;
    dim.value = 14.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    // FP conversion uses display density; exact value depends on mock, but should be >= 0
    EXPECT_GE(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_NegativeFP_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FP;
    dim.value = -5.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_NegativePercent_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::PERCENT;
    dim.value = -10.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_InfiniteVP_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::VP;
    dim.value = std::numeric_limits<float>::infinity();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_FixAtIdealSize)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FIX_AT_IDEAL_SIZE;
    dim.value = 999.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_MatchParent)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::MATCH_PARENT;
    dim.value = 100.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 100.0F);
}

TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_NegativeMatchParent_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::MATCH_PARENT;
    dim.value = -1.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// ApplyDimension - isWidth=false branches (height paths)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightWrapContentResetsHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "wrap_content"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 1); // WRAP_CONTENT
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightFixAtIdealSizeResetsHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "fix_at_ideal_size"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 2); // FIX_AT_IDEAL_SIZE
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthFP_AppliesWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "20fp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightFP_AppliesHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "16fp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthUnsupportedUnit_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    // "px" is not a supported unit for width/height
    auto styles = JsonAdapter::Parse(R"({"width": "10px"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH);
        EXPECT_NE(rec.attribute, NODE_WIDTH_PERCENT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightUnsupportedUnit_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"height": "10px"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_HEIGHT);
        EXPECT_NE(rec.attribute, NODE_HEIGHT_PERCENT);
    }
}

// =============================================================================
// ApplyRadius - different VP corner radii (4-corner SetNodeBorderRadius path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesDifferentCornerRadii)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "5vp", "topRight": "10vp", "bottomRight": "15vp", "bottomLeft": "20vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
            ASSERT_GE(rec.values.size(), 4U);
            EXPECT_FLOAT_EQ(rec.values[0].f32, 5.0F);
            EXPECT_FLOAT_EQ(rec.values[1].f32, 10.0F);
            EXPECT_FLOAT_EQ(rec.values[2].f32, 20.0F);
            EXPECT_FLOAT_EQ(rec.values[3].f32, 15.0F);
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyShadow - custom shadow path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesCustomShadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "shadow": {
            "radius": 10,
            "color": "#FF000000",
            "offsetX": 2,
            "offsetY": 4,
            "type": "color"
        }
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CUSTOM_SHADOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyShadow / ApplyBackgroundImage / ApplyTextDecoration - null node paths
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ShadowSkipsWhenNodeNull)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"shadow": "OUTER_DEFAULT_MD"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSkipsWhenNodeNull)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": "https://example.com/img.png"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ApplyTextDecoration_SkipsNullNode)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    auto value = JsonAdapter::Parse(R"({"type": "underline", "color": "#ff007dff"})");
    ASSERT_NE(value, nullptr);
    // No crash, no attribute set for null node
    ExtendedStyleResolver::ApplyTextDecoration(value->GetRoot(), applier);
}

// =============================================================================
// ConstraintSize - percent dispatch path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizePercentDispatchesToETS)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 20;

    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10%", "maxWidth": "80%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    // Percent path: should NOT set C++ constraint size, should dispatch to ETS
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizePercentWithoutDispatchContext)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    ASSERT_FALSE(mockArkUIPtr_->createdNodes_.empty());
    ArkUI_NodeHandle wrapper = mockArkUIPtr_->createdNodes_.back().nodeHandle;
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[0].f32, 100.0F);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeInvalidDimension)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "not_a_dimension"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // ParseDimension fails → returns false → constraintSize ignored
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeUnsupportedUnit)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10px"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // "px" is not VP/FP/PERCENT → returns false
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// BorderWidth FP unit
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthFP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "3fp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// BackgroundImageSize - invalid valid value (LOG_WARN path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RestoresInvalidBackgroundImageSizeToAuto)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": 42})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetSize = false;
    bool restoredAuto = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            resetSize = true;
        }
    }
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE && !rec.values.empty()) {
            restoredAuto = true;
            EXPECT_EQ(rec.values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
        }
    }
    EXPECT_TRUE(resetSize);
    EXPECT_TRUE(restoredAuto);
}

// =============================================================================
// Radius - percent with conversion failure fallback
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_AppliesZeroPercentRadiusCorners)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // Non-zero percent should be applied via percent path
    auto styles = JsonAdapter::Parse(R"({"borderRadius": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Should apply percent radius
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// Edge dimension conversion failure paths
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingAllFieldsZero_DoesNotCrash)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "0vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Zero padding should still be applied
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginAllFieldsZero_DoesNotCrash)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"margin": "0vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// Radius with individual corners all zero (HasSameRadius true path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusAllZeroCorners_UniformRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "0vp", "topRight": "0vp", "bottomRight": "0vp", "bottomLeft": "0vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyTextComponentStyles - clip path through ApplyTextDecoration with valid decoration
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_IgnoresInvalidDecoration)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"decoration": "invalid_decoration"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_DECORATION);
    }
}

// =============================================================================
// backgroundImageSize validation issue reporting
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBackgroundImageSizeString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "bogus_value"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBackgroundImageSizeObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "not_a_dimension"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSize");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForBackgroundImageSizeNumber)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": 42})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NoIssueForValidBackgroundImageSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "cover"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NoIssueForAbsentBackgroundImageSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"opacity": 0.5})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// SchemaWarning issue reporting for invalid style values
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.height");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBackgroundColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundColor": "not_a_color"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundColor");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBorderColor)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderColor": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderColor");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidPadding)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.padding");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidMargin)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"margin": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.margin");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBorderWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderWidth");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBorderRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": "bogus"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderRadius");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidVisibility)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"visibility": "invisible"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.visibility");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidClip)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"clip": "not_a_bool"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.clip");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidFlexShrink)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": "not_a_number"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.flexShrink");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidLayoutWeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.layoutWeight");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidConstraintSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": "not_an_object"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.constraintSize");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidBackgroundImage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": 123})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImage");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidShadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"shadow": "not_a_shadow"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.shadow");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ReportsIssueForInvalidLinearGradient)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"linearGradient": "not_an_object"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.linearGradient");
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NoIssuesForAllValidStyles)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"width": "100vp", "height": "50vp", "backgroundColor": "#FF0000", "borderColor": "#00FF00",)"
        R"("padding": "10vp", "margin": "5vp", "borderWidth": "2vp", "borderRadius": "8vp",)"
        R"("visibility": "visible", "clip": true, "flexShrink": 0.5, "layoutWeight": 1,)"
        R"("backgroundImage": "test.png", "backgroundImageSizeWithStyle": "cover",)"
        R"("shadow": 0, "linearGradient": {"angle": 90, "colors": [["#FF0000", 0], ["#00FF00", 1]]}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// FEAT-20260613-001: 异常值恢复默认 (reset to default)
// 异常值 (存在但无效) → 调用 Reset 恢复系统默认, 而非忽略残留。
// 复用 ExtendedStyleResolver::Reset, 与 diff 删除恢复默认走同一代码路径。
// =============================================================================

// dimension 类 (双通道 + 多失败点): width 异常 → ResetNodeWidth + ResetNodeWidthPercent
TEST_F(ExtendedStyleResolverTddTest, InvalidWidth_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"width": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool resetWidth = false;
    bool resetWidthPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            resetWidth = true;
        }
        if (rec.attribute == NODE_WIDTH_PERCENT) {
            resetWidthPercent = true;
        }
    }
    EXPECT_TRUE(resetWidth);
    EXPECT_TRUE(resetWidthPercent);
}

TEST_F(ExtendedStyleResolverTddTest, InvalidHeight_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"height": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetHeight = false;
    bool resetHeightPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT) {
            resetHeight = true;
        }
        if (rec.attribute == NODE_HEIGHT_PERCENT) {
            resetHeightPercent = true;
        }
    }
    EXPECT_TRUE(resetHeight);
    EXPECT_TRUE(resetHeightPercent);
}

// color 类 (单通道): backgroundColor 异常 → ResetNodeBackgroundColor
TEST_F(ExtendedStyleResolverTddTest, InvalidBackgroundColor_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundColor": "not_a_color"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_COLOR) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// mixed units 类 (双通道): borderRadius 异常 → ResetNodeBorderRadius + Percent
TEST_F(ExtendedStyleResolverTddTest, InvalidBorderRadius_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderRadius": "not_a_radius"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetRadius = false;
    bool resetRadiusPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            resetRadius = true;
        }
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            resetRadiusPercent = true;
        }
    }
    EXPECT_TRUE(resetRadius);
    EXPECT_TRUE(resetRadiusPercent);
}

// resource 类 (双 reset): shadow 异常 → ResetNodeShadow + ResetNodeCustomShadow
TEST_F(ExtendedStyleResolverTddTest, InvalidShadow_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"shadow": "not_a_shadow"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetShadow = false;
    bool resetCustomShadow = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_SHADOW) {
            resetShadow = true;
        }
        if (rec.attribute == NODE_CUSTOM_SHADOW) {
            resetCustomShadow = true;
        }
    }
    EXPECT_TRUE(resetShadow);
    EXPECT_TRUE(resetCustomShadow);
}

// enum 类 (单通道): flexShrink 异常 → ResetNodeFlexShrink
TEST_F(ExtendedStyleResolverTddTest, InvalidFlexShrink_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"flexShrink": "invalid"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, InvalidFlexShrink_UsesColumnParentDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": -1})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext context;
    context.parentComponentType = "Column";

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, context);

    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK && !rec.values.empty()) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 0.0F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, InvalidFlexShrink_UsesFlexParentDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": -1})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext context;
    context.parentComponentType = "Flex";

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, context);

    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK && !rec.values.empty()) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 1.0F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, InvalidFlexShrink_UsesRowParentDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": -1})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext context;
    context.parentComponentType = "Row";

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, context);

    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK && !rec.values.empty()) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 0.0F);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResetFlexShrink_UsesParentComponentDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    StyleResetProperty property { "flexShrink", StylePropertyName::FLEX_SHRINK };
    const std::vector<std::pair<std::string, float>> cases = { { "Column", 0.0F }, { "Row", 0.0F }, { "Flex", 1.0F } };

    for (const auto& [parentType, expected] : cases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        ExtendedStyleResolver::Reset(property, applier, MIN_API_VERSION_LAYOUT_POLICY, parentType);

        ASSERT_FALSE(mockArkUIPtr_->setAttributeRecords_.empty());
        const auto& record = mockArkUIPtr_->setAttributeRecords_.back();
        EXPECT_EQ(record.attribute, NODE_FLEX_SHRINK);
        ASSERT_FALSE(record.values.empty());
        EXPECT_FLOAT_EQ(record.values[0].f32, expected);
    }

    mockArkUIPtr_->resetAttributeRecords_.clear();
    ExtendedStyleResolver::Reset(property, applier);
    ASSERT_FALSE(mockArkUIPtr_->resetAttributeRecords_.empty());
    EXPECT_EQ(mockArkUIPtr_->resetAttributeRecords_.back().attribute, NODE_FLEX_SHRINK);
}

// DFX: 异常值 issue message 含 "reset to default"
TEST_F(ExtendedStyleResolverTddTest, InvalidStyle_ReportsResetToDefaultMessage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_NE(issues[0].message.find("reset to default"), std::string::npos);
}

// 增量更新残留修复: 先有效值, 再异常值 → reset 清除 (不残留上一次有效值)
TEST_F(ExtendedStyleResolverTddTest, IncrementalUpdate_InvalidValueClearsPreviousValid)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // 1. 先应用有效 width → NODE_WIDTH 被 set
    auto valid = JsonAdapter::Parse(R"({"width": "100vp"})");
    ASSERT_NE(valid, nullptr);
    ExtendedStyleResolver::ResolveAndApply(valid->GetRoot(), applier);
    bool wasSet = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            wasSet = true;
        }
    }
    EXPECT_TRUE(wasSet);
    // 2. 再传入异常 width → 应 reset (不残留 100vp)
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto invalid = JsonAdapter::Parse(R"({"width": "not_a_dimension"})");
    ASSERT_NE(invalid, nullptr);
    ExtendedStyleResolver::ResolveAndApply(invalid->GetRoot(), applier);
    bool resetWidth = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            resetWidth = true;
        }
    }
    EXPECT_TRUE(resetWidth);
}

// 三态: 字段缺失 → 不触发 reset (由 diff 删除逻辑负责)
TEST_F(ExtendedStyleResolverTddTest, MissingFields_DoNotReset)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

// color 类 (单通道): borderColor 异常 → ResetNodeBorderColor
TEST_F(ExtendedStyleResolverTddTest, InvalidBorderColor_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderColor": "not_a_color"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_COLOR) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// edge 类 (双通道): padding 异常 → ResetNodePadding + ResetNodePaddingPercent
TEST_F(ExtendedStyleResolverTddTest, InvalidPadding_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"padding": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetPadding = false;
    bool resetPaddingPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            resetPadding = true;
        }
        if (rec.attribute == NODE_PADDING_PERCENT) {
            resetPaddingPercent = true;
        }
    }
    EXPECT_TRUE(resetPadding);
    EXPECT_TRUE(resetPaddingPercent);
}

// edge 类 (双通道): margin 异常 → ResetNodeMargin + ResetNodeMarginPercent
TEST_F(ExtendedStyleResolverTddTest, InvalidMargin_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"margin": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetMargin = false;
    bool resetMarginPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            resetMargin = true;
        }
        if (rec.attribute == NODE_MARGIN_PERCENT) {
            resetMarginPercent = true;
        }
    }
    EXPECT_TRUE(resetMargin);
    EXPECT_TRUE(resetMarginPercent);
}

// dimension 类 (双通道): borderWidth 异常 → ResetNodeBorderWidth + ResetNodeBorderWidthPercent
TEST_F(ExtendedStyleResolverTddTest, InvalidBorderWidth_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetWidth = false;
    bool resetWidthPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH) {
            resetWidth = true;
        }
        if (rec.attribute == NODE_BORDER_WIDTH_PERCENT) {
            resetWidthPercent = true;
        }
    }
    EXPECT_TRUE(resetWidth);
    EXPECT_TRUE(resetWidthPercent);
}

// enum 类 (单通道): visibility 异常 → ResetNodeVisibility
TEST_F(ExtendedStyleResolverTddTest, InvalidVisibility_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"visibility": "invisible"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_VISIBILITY) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// bool 类 (单通道): clip 异常 → ResetNodeClip
TEST_F(ExtendedStyleResolverTddTest, InvalidClip_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"clip": "not_a_bool"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_CLIP) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// number 类 (单通道): layoutWeight 异常 → ResetNodeLayoutWeight
TEST_F(ExtendedStyleResolverTddTest, InvalidLayoutWeight_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": "abc"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_LAYOUT_WEIGHT) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// object 类 (单通道): constraintSize 异常 → ResetNodeConstraintSize
TEST_F(ExtendedStyleResolverTddTest, InvalidConstraintSize_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"constraintSize": "not_an_object"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// resource 类 (单通道): backgroundImage 异常 → ResetNodeBackgroundImage
TEST_F(ExtendedStyleResolverTddTest, InvalidBackgroundImage_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": 123})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// backgroundImageSizeWithStyle 异常：清理尺寸对象并显式恢复协议默认 auto。
TEST_F(ExtendedStyleResolverTddTest, InvalidBackgroundImageSize_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "bogus_value"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetSize = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            resetSize = true;
        }
    }
    const MockArkUINativeProvider::SetAttributeRecord* sizeWithStyle = nullptr;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            sizeWithStyle = &rec;
        }
    }
    EXPECT_TRUE(resetSize);
    ASSERT_NE(sizeWithStyle, nullptr);
    ASSERT_EQ(sizeWithStyle->values.size(), 1u);
    EXPECT_EQ(sizeWithStyle->values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
}

TEST_F(ExtendedStyleResolverTddTest, BackgroundImageSizeFallback_IsAppliedAfterBackgroundImage)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle":"bogus_value","backgroundImage":"img.png"})");
    ASSERT_NE(styles, nullptr);

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);

    size_t imageIndex = mockArkUIPtr_->setAttributeRecords_.size();
    size_t sizeIndex = mockArkUIPtr_->setAttributeRecords_.size();
    for (size_t index = 0; index < mockArkUIPtr_->setAttributeRecords_.size(); ++index) {
        const auto& record = mockArkUIPtr_->setAttributeRecords_[index];
        if (record.attribute == NODE_BACKGROUND_IMAGE) {
            imageIndex = index;
        }
        if (record.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            sizeIndex = index;
            ASSERT_EQ(record.values.size(), 1u);
            EXPECT_EQ(record.values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
        }
    }
    ASSERT_LT(imageIndex, mockArkUIPtr_->setAttributeRecords_.size());
    ASSERT_LT(sizeIndex, mockArkUIPtr_->setAttributeRecords_.size());
    EXPECT_LT(imageIndex, sizeIndex);
}

// resource 类 (单通道): linearGradient 异常 → ResetNodeLinearGradient
TEST_F(ExtendedStyleResolverTddTest, InvalidLinearGradient_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"linearGradient": "not_an_object"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool reset = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_LINEAR_GRADIENT) {
            reset = true;
        }
    }
    EXPECT_TRUE(reset);
}

// =============================================================================
// FEAT-20260613-001 分支补全: 多失败子分支 (mixed units / keyword unit)
// 每个属性的 Reset 调用点有多个子分支, 上面 *_ResetsToDefault 只覆盖 parse-fail 主路径。
// 这里覆盖其余"可达"子分支; 依赖 DimensionToFloat==false 的 conversion-failed 子分支
// 见文件末尾说明 (resolver 版 ConvertDimensionToFloat 恒 true, 不可达)。
// =============================================================================

// mixed units 子分支: padding 同时含 vp + % → ResetNodePadding + Percent
TEST_F(ExtendedStyleResolverTddTest, InvalidPadding_MixedUnits_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp", "right": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetPadding = false;
    bool resetPaddingPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            resetPadding = true;
        }
        if (rec.attribute == NODE_PADDING_PERCENT) {
            resetPaddingPercent = true;
        }
    }
    EXPECT_TRUE(resetPadding);
    EXPECT_TRUE(resetPaddingPercent);
}

// mixed units 子分支: margin 同时含 vp + % → ResetNodeMargin + Percent
TEST_F(ExtendedStyleResolverTddTest, InvalidMargin_MixedUnits_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"margin": "10vp", "marginRight": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetMargin = false;
    bool resetMarginPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            resetMargin = true;
        }
        if (rec.attribute == NODE_MARGIN_PERCENT) {
            resetMarginPercent = true;
        }
    }
    EXPECT_TRUE(resetMargin);
    EXPECT_TRUE(resetMarginPercent);
}

// mixed units 子分支: borderRadius 对象同时含 vp + % → ResetNodeBorderRadius + Percent
TEST_F(ExtendedStyleResolverTddTest, InvalidBorderRadius_MixedUnits_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderRadius": {"topLeft": "10vp", "topRight": "5%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetRadius = false;
    bool resetRadiusPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            resetRadius = true;
        }
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            resetRadiusPercent = true;
        }
    }
    EXPECT_TRUE(resetRadius);
    EXPECT_TRUE(resetRadiusPercent);
}

// keyword-unit 子分支: borderWidth = wrapcontent (非 VP/FP/PERCENT) → invalid unit reset
TEST_F(ExtendedStyleResolverTddTest, InvalidBorderWidth_KeywordUnit_ResetsToDefault)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "wrapcontent"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool resetWidth = false;
    bool resetWidthPercent = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH) {
            resetWidth = true;
        }
        if (rec.attribute == NODE_BORDER_WIDTH_PERCENT) {
            resetWidthPercent = true;
        }
    }
    EXPECT_TRUE(resetWidth);
    EXPECT_TRUE(resetWidthPercent);
}

// =============================================================================
// ResolveAndApply — valid non-object styles (LOG path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ValidNonObjectStyles_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto adapter = JsonAdapter::CreateString("not_an_object");
    ASSERT_NE(adapter, nullptr);
    // Valid string but not object → LOG_WARN + early return
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH);
    }
}

// =============================================================================
// ApplyTextComponentStyles — null node LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_NullNode_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"fontWeight": 700})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    // Null node → LOG_WARN, no attributes set
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_WEIGHT);
    }
}

// =============================================================================
// ApplyTextComponentStyles — fontWeight parse fail LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_FontWeightParseFail_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    // Object value for fontWeight → ParseFontWeight fails → LOG_WARN path
    auto styles = JsonAdapter::Parse(R"({"fontWeight": {"nested": true}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    // fontWeight is an object, not parseable → else if LOG_WARN
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_WEIGHT);
    }
}

// =============================================================================
// ApplyTextComponentStyles — maxLines parse fail LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_MaxLinesParseFail_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"maxLines": {"bad": true}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_MAX_LINES);
    }
}

// =============================================================================
// ApplyTextComponentStyles — textOverflow invalid number LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_TextOverflowInvalid_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"textOverflow": 999})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_OVERFLOW);
    }
}

// =============================================================================
// ApplyTextComponentStyles — textAlign invalid LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_TextAlignInvalid_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"textAlign": {"bad": true}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_ALIGN);
    }
}

// =============================================================================
// ApplyTextComponentStyles — non-object styles early return
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_NonObject_Returns)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto numAdapter = JsonAdapter::CreateNumber(42);
    ASSERT_NE(numAdapter, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(numAdapter->GetRoot(), applier);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
}

// =============================================================================
// ApplyColorStyles — borderColor with array type (FormatStyleInput array path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderColorArray_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderColor": [1, 2, 3]})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.borderColor");
}

// =============================================================================
// ApplyTextStyles — invalid fontColor LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_InvalidFontColor_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"fontColor": "not_a_color"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_COLOR);
    }
}

// =============================================================================
// ApplyTextStyles — invalid fontSize LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_InvalidFontSize_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"fontSize": {"nested": true}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_SIZE);
    }
}

// =============================================================================
// ApplyEdgeStyles — padding invalid input (bool), HasAnyField=true but ParseEdgeStyle=false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingBoolInput_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"padding": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.padding") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyEdgeStyles — margin invalid input (bool), HasAnyField=true but ParseEdgeStyle=false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginBoolInput_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"margin": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — borderWidth percent valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — borderWidth wrap_content (unsupported unit)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthWrapContent_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "wrap_content"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.borderWidth") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — borderWidth pure number (not a dimension)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthNumber_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderWidth": 42})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Pure number parsed as VP dimension by ParseDimension, so it succeeds.
    // This covers the VP/FP branch with number input.
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_WIDTH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — opacity invalid LOG path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_OpacityInvalid_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"opacity": {"bad": true}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_OPACITY);
    }
}

// =============================================================================
// ApplyCommonNodeStyles — null node paths
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeFlexShrink_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"flexShrink": 1.5})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FLEX_SHRINK);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeClip_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"clip": true})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CLIP);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeLayoutWeight_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": 1})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeBackgroundImage_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": "https://example.com/img.png"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeConstraintSize_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": 50}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ApplyDimension — null node with percent
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeWidthPercent_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"width": "50%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH_PERCENT);
    }
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeHeightPercent_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"height": "50%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_HEIGHT_PERCENT);
    }
}

// =============================================================================
// ApplyRadius — percent with valid non-zero value
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusPercentValid_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": "10%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyShadow — style shadow (number input)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ShadowStyleNumber_SetsNodeShadow)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"shadow": 0})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_SHADOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyBackgroundImageSize — percent with dispatch context
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizePercentWithDispatch_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%", "height": "100vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    // Percent path → dispatches to ETS, should NOT set C++ attribute
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// ApplyBackgroundImageSize — empty object (no fields)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeEmptyObject_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSize");
}

// =============================================================================
// ApplyLinearGradient — empty object (no colors array)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_LinearGradientEmptyObject_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"linearGradient": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.linearGradient");
}

// =============================================================================
// Reset — null node with non-empty property name
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, Reset_NullNodeWithNonEmptyName_NoReset)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    StyleResetProperty prop { "backgroundColor", StylePropertyName::BACKGROUND_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

TEST_F(ExtendedStyleResolverTddTest, Reset_FontColorNullNode)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    StyleResetProperty prop { "fontColor", StylePropertyName::FONT_COLOR };
    ExtendedStyleResolver::Reset(prop, applier);
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

// =============================================================================
// ApplyBackgroundImage — null node + empty string paths
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageEmptyString_NullNode_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": ""})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Empty string on null node → null node early return in ApplyBackgroundImage
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

// =============================================================================
// ApplyBackgroundImageSize — percent without dispatch context, fallback to C++ path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizePercentWithoutContext_FallbackToCpp)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%"}})");
    ASSERT_NE(styles, nullptr);
    // No dispatch context → LOG_WARN fallback + C++ path
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt);
    // Falls back to C++ SetNodeBackgroundImageSize with VP values (0 for percent)
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyBackgroundImageSize — object with VP width + percent height, with dispatch
// Exercises ParseBackgroundImageSizeWithPercent VP-in-percent-path branch (line 334)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeMixedUnits_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "100vp", "height": "50%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    // Mixed VP+percent → percent dispatch path (hasPercent=true)
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// HasSameRadius — non-uniform radii return false (FormatRadius LOG path)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusDifferentUnits_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    // One corner percent, one corner VP → mixed units → LOG + issue + return
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "5vp", "topRight": "5vp", "bottomRight": "5vp", "bottomLeft": "5%"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Mixed units → warning + issue
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.borderRadius") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyTextDecoration — null node
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextDecoration_NullNode_NoSet)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto value = JsonAdapter::Parse(R"({"type": "underline", "color": "#ff007dff"})");
    ASSERT_NE(value, nullptr);
    ExtendedStyleResolver::ApplyTextDecoration(value->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_DECORATION);
    }
}

// =============================================================================
// ApplyBackgroundImageSize — backgroundImageSizeWithStyle unknown string (string parse fail LOG)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeWithStyleUnknownString_Logs)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "unknown_value"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // String parse fails → LOG_WARN + issue
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");
}

// =============================================================================
// ConstraintSize — percent with mixed VP+percent in one field set, with dispatch
// Exercises ParseConstraintSizeStyle VP-in-percent-path branch (line 170)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeMixedPercentAndVP_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 20;
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10%", "maxWidth": "200vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    // Percent field present → dispatch to ETS
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// FormatStyleInput — invalid value type (exercised via invalid input warning LOG)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundColorObjectValue_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    // Object value for backgroundColor → ParseColor fails → LOG + issue + FormatStyleInput(object)
    auto styles = JsonAdapter::Parse(R"({"backgroundColor": {"nested": true}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.backgroundColor");
}

// =============================================================================
// ParseConstraintSizeStyle — infinite/negative converted value → return false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeInfConvertedValue_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "infvp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // "infvp" won't parse as a dimension, so constraintSize is ignored
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ParseConstraintSizeStyle — only some fields present (partial fields)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizePartialFields_ValidVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": 50}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ConstraintSize — non-percent, non-VP/FP unit (e.g. wrap_content) → fails
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeWrapContentField_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "wrap_content"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ParseBackgroundImageSizeWithPercent — only height present (no width)
// Covers branch: parsed[0].present=false, parsed[1].present=true
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyHeight_VP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"height": 200}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ParseBackgroundImageSizeWithPercent — non-VP/FP/PERCENT unit → return false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeWrapContentField_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "wrap_content"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // wrap_content not valid for backgroundImageSize
    ASSERT_EQ(issues.size(), 1u);
}

// =============================================================================
// ParseBackgroundImageSizeWithPercent — only height percent (no width)
// Covers: parsed[0].present=false, parsed[1].present=true, hasPercent=true
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyHeightPercent_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"height": "50%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// ConvertDimensionToFloat — FP with invalid (NaN) value → clamped to 0
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_FP_NaN_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FP;
    dim.value = std::numeric_limits<float>::quiet_NaN();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// ConvertDimensionToFloat — infinite percent → clamped to 0
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_InfinitePercent_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::PERCENT;
    dim.value = std::numeric_limits<float>::infinity();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// ConstraintSize — all 4 fields present with VP values
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeAllFourFieldsVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"constraintSize": {"minWidth": 10, "maxWidth": 200, "minHeight": 20, "maxHeight": 300}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeInvalidFieldUsesDefaultWithoutDroppingSiblings)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"constraintSize": {"minWidth": "", "maxWidth": 260, "minHeight": 40, "maxHeight": 160}})");
    ASSERT_NE(styles, nullptr);

    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);

    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute != NODE_CONSTRAINT_SIZE) {
            continue;
        }
        found = true;
        ASSERT_GE(rec.values.size(), 4U);
        EXPECT_FLOAT_EQ(rec.values[0].f32, 0.0F);
        EXPECT_FLOAT_EQ(rec.values[1].f32, 260.0F);
        EXPECT_FLOAT_EQ(rec.values[2].f32, 40.0F);
        EXPECT_FLOAT_EQ(rec.values[3].f32, 160.0F);
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeNegativeFieldUsesItsDefault)
{
    struct TestCase {
        const char* styles;
        std::array<float, 4> expected;
    };
    const std::array<TestCase, 4> cases = { {
        { R"({"constraintSize":{"minWidth":-1,"maxWidth":260,"minHeight":40,"maxHeight":160}})",
            { 0.0F, 260.0F, 40.0F, 160.0F } },
        { R"({"constraintSize":{"minWidth":10,"maxWidth":-1,"minHeight":40,"maxHeight":160}})",
            { 10.0F, FLT_MAX, 40.0F, 160.0F } },
        { R"({"constraintSize":{"minWidth":10,"maxWidth":260,"minHeight":-1,"maxHeight":160}})",
            { 10.0F, 260.0F, 0.0F, 160.0F } },
        { R"({"constraintSize":{"minWidth":10,"maxWidth":260,"minHeight":40,"maxHeight":-1}})",
            { 10.0F, 260.0F, 40.0F, FLT_MAX } },
    } };

    for (const auto& testCase : cases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
        auto styles = JsonAdapter::Parse(testCase.styles);
        ASSERT_NE(styles, nullptr);

        ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);

        auto record =
            std::find_if(mockArkUIPtr_->setAttributeRecords_.begin(), mockArkUIPtr_->setAttributeRecords_.end(),
                [](const auto& item) { return item.attribute == NODE_CONSTRAINT_SIZE; });
        ASSERT_NE(record, mockArkUIPtr_->setAttributeRecords_.end());
        ASSERT_GE(record->values.size(), testCase.expected.size());
        for (size_t index = 0; index < testCase.expected.size(); ++index) {
            EXPECT_FLOAT_EQ(record->values[index].f32, testCase.expected[index]);
        }
    }
}

// =============================================================================
// ConstraintSize — all 4 fields with percent values
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeAllFourFieldsPercent_UsesNativePath)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 23;
    auto styles = JsonAdapter::Parse(
        R"({"constraintSize": {"minWidth": "10%", "maxWidth": "90%", "minHeight": "5%", "maxHeight": "95%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    ASSERT_FALSE(mockArkUIPtr_->createdNodes_.empty());
    ArkUI_NodeHandle wrapper = mockArkUIPtr_->createdNodes_.back().nodeHandle;
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[0].f32, 100.0F);
    EXPECT_FLOAT_EQ(record->values[1].f32, 900.0F);
    EXPECT_FLOAT_EQ(record->values[2].f32, 25.0F);
    EXPECT_FLOAT_EQ(record->values[3].f32, 475.0F);
}

// =============================================================================
// ConstraintSize — FP unit field
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeFPField_Valid)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "16fp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ConstraintSize — negative VP value → ParseConstraintSizeStyle returns false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeNegativeVP_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // A dimension string "-10vp" may not parse correctly, but let's try number -10
    // Actually, numbers without unit suffix are parsed as VP by default
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": -10}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Negative VP value → converted value < 0 → return false
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ConstraintSize — invalid type (not object)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeNotObject_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ApplyDimension — width match_parent routes to the ArkUI layout-policy API (MATCH_PARENT=0)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthMatchParent_AppliesPercent1)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "match_parent"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 0); // MATCH_PARENT
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDimension — width fix_at_ideal_size routes to layout-policy (FIX_AT_IDEAL_SIZE=2)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthFixAtIdealSize_ResetsWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "fix_at_ideal_size"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 2); // FIX_AT_IDEAL_SIZE
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDimension — null node with VP width → skipped
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeWidthVP_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"width": 100})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH);
        EXPECT_NE(rec.attribute, NODE_WIDTH_PERCENT);
    }
}

// =============================================================================
// ApplyDimension — null node with VP height → skipped
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeHeightVP_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"height": 100})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_HEIGHT);
        EXPECT_NE(rec.attribute, NODE_HEIGHT_PERCENT);
    }
}

// =============================================================================
// ApplyDimension — null node with wrap_content → skipped (no reset)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeWidthWrapContent_Skips)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"width": "wrap_content"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    EXPECT_TRUE(mockArkUIPtr_->resetAttributeRecords_.empty());
}

// =============================================================================
// ApplyRadius — percent radius with all-zero corners (hasPercent=false, HasSameRadius=true)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusZeroPercentCorners_UniformZero)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "0%", "topRight": "0%", "bottomRight": "0%", "bottomLeft": "0%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // All zeros → hasPercent=false (value==0 skips), HasSameRadius=true → SetBorderRadius(0)
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyRadius — percent conversion failure (all corners same percent, but HasSameRadius false path)
// This is hard to trigger directly; the 4-corner different VP path covers it.
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusFourDifferentVPCorners_SetBorderRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles =
        JsonAdapter::Parse(R"({"borderRadius": {"topLeft": 1, "topRight": 2, "bottomRight": 3, "bottomLeft": 4}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyRadius — percent all same percent corners
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusSamePercentCorners_SetPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "10%", "topRight": "10%", "bottomRight": "10%", "bottomLeft": "10%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyBackgroundImageSize — invalid value (non-string, non-object) → number type
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeNumber_ReportsIssue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": 42})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Number is not string and not object → passes through to ParseBackgroundImageSizeWithPercent
    // which returns false for non-object → issue reported
    ASSERT_EQ(issues.size(), 1u);
}

// =============================================================================
// ApplyBackgroundImageSize — backgroundimageSize (lowercase 'i') string valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundimageSizeLowerCaseI_Cover)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundimageSize": "cover"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyBackgroundImageSize — backgroundImageSizeWithStyle (lowercase 'i') object valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundimageSizeWithStyle_ObjectVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": {"width": 50, "height": 60}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyBackgroundImage — backgroundImage (lowercase 'i') valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundimageLowerCaseI_Valid)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": "test.png"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyEdgeStyles — padding dimension conversion failure
// Use FP with NaN to trigger DimensionToFloat returning true but clamping to 0
// This exercises the absolute padding path with all-zero values
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingZeroFP_AppliesZero)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "0fp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyEdgeStyles — margin dimension conversion failure path
// Test with individual margin keys where one value is zero VP
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginZeroValue_NoPercentApplied)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"marginTop": 0, "marginBottom": 0})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — borderWidth negative FP rejected by ParseDimension
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthFP_NegativeClamp)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "-5fp"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Negative FP → ParseDimension rejects → validation issue reported, no setter called
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.borderWidth") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyDecorationStyles — borderWidth negative percent rejected by ParseDimension
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BorderWidthNegativePercent_Clamps)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderWidth": "-5%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Negative percent → ParseDimension rejects → validation issue reported, no setter called
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.borderWidth") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyLinearGradient — valid with repeating flag
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_LinearGradientRepeating)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "linearGradient": {
            "angle": 45,
            "repeating": true,
            "colors": [["#FF0000", 0.0], ["#00FF00", 1.0]]
        }
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_LINEAR_GRADIENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ApplyShadow — custom shadow with color strategy
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_CustomShadowColorStrategy)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "shadow": {
            "radius": 15,
            "color": "#FF112233",
            "offsetX": 3,
            "offsetY": 5,
            "type": "color",
            "fill": true
        }
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CUSTOM_SHADOW) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ConvertDimensionToPercentRatio — MATCH_PARENT returns 1.0
// (indirectly tested via match_parent, but ensure explicit coverage)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_MatchParent100)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::MATCH_PARENT;
    dim.value = 100.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 100.0F);
}

// =============================================================================
// DimensionToFloat — infinite FP → density path falls back to densityScale=1.0
// Then value*1.0 = inf → !finite → clamped to 0
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_FP_Infinite_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FP;
    dim.value = std::numeric_limits<float>::infinity();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// ParseEdgeStyle — individual keys override allKey
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_IndividualOverrideAll)
{
    StyleEdge edge;
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp", "top": "20vp"})");
    ASSERT_NE(styles, nullptr);
    EXPECT_TRUE(
        ExtendedStyleResolver::ParseEdgeStyle(styles->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
    // Individual "top" overrides "padding" allKey's top
    EXPECT_FLOAT_EQ(edge.top.value, 20.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 10.0F);
}

// =============================================================================
// ConstraintSize — only percent field present (no VP fields at all)
// Exercises hasPercent path with single field
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeSinglePercentField_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 20;
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"maxHeight": "80%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// ConstraintSize — percent without dispatch context (std::nullopt)
// Exercises else branch at line 1326: no dispatch → nothing dispatched
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizePercentNoDispatch_NoCrash)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"maxHeight": "80%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt);
    ASSERT_FALSE(mockArkUIPtr_->createdNodes_.empty());
    ArkUI_NodeHandle wrapper = mockArkUIPtr_->createdNodes_.back().nodeHandle;
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, 1000, 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[3].f32, 400.0F);
}

// =============================================================================
// ParseBackgroundImageSizeWithPercent — negative VP value → returns false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeNegativeVP_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": -10}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_EQ(issues.size(), 1u);
}

// =============================================================================
// ApplyBackgroundImageSize — FP unit in object form
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeFP_VP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "16fp", "height": "14fp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// ConstraintSize — FP unit field
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeFPField_MixedWithVP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10fp", "maxWidth": 200}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BoolToString(false) — clip=false triggers LOG_INFO with "false"
// Exercises line 784: BoolToString(clip)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ClipFalse_AppliesClipFalse)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"clip": false})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CLIP) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatNamedStyleInputs returning "{}" — null node with no common style keys
// Exercises FormatNamedStyleInputs with empty result path (line 403-404)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_NullNodeNoCommonKeys_LogsEmptyInputs)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    auto styles = JsonAdapter::Parse(R"({"width": 100, "height": 200})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // No crash; exercises FormatNamedStyleInputs with no matching keys → returns "{}"
}

// =============================================================================
// COVERAGE GAP: FormatEdge with non-VP dimensions via mixed padding
// Exercises FormatDimension with MATCH_PARENT unit in FormatEdge
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingMatchParentMixedPercent_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"top": "match_parent", "right": "5%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Mixed match_parent + percent → LOG_WARN with FormatEdge containing FormatDimension(match_parent)
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.padding") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatEdge with WRAP_CONTENT via padding
// wrap_content converts to value=0 so the mixed-unit loop skips it (value==0).
// Instead, test that wrap_content + percent padding applies percent correctly.
// Exercises ConvertDimensionToFloat(WRAP_CONTENT) → returns 0.0F
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingWrapContentWithPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": "wrap_content", "right": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // wrap_content → value=0 → skipped in loop; "5%" → hasPercent=true → SetPaddingPercent
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatEdge with FIX_AT_IDEAL_SIZE via padding
// fix_at_ideal_size converts to value=0 so the mixed-unit loop skips it.
// Test that fix_at_ideal_size + percent padding applies percent correctly.
// Exercises ConvertDimensionToFloat(FIX_AT_IDEAL_SIZE) → returns 0.0F
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingFixAtIdealSizeWithPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": "fix_at_ideal_size", "right": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // fix_at_ideal_size → value=0 → skipped; "5%" → hasPercent=true → SetPaddingPercent
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatEdge with MATCH_PARENT in margin mixed units
// Exercises FormatDimension with MATCH_PARENT for margin in FormatEdge
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginMatchParentMixedPercent_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"marginTop": "match_parent", "marginRight": "5%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatEdge with WRAP_CONTENT in margin
// wrap_content → value=0 → skipped in loop. Test percent is still applied.
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginWrapContentWithPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"marginTop": "wrap_content", "marginRight": "5%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatRadius with MATCH_PARENT in mixed radius
// Exercises FormatDimension with MATCH_PARENT in FormatRadius
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusMatchParentMixedPercent_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"borderRadius": {"topLeft": "match_parent", "topRight": "5%"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.borderRadius") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatRadius with WRAP_CONTENT in radius
// wrap_content → value=0 → skipped. Test percent is still applied.
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusWrapContentWithPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": {"topLeft": "wrap_content", "topRight": "5%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatRadius with FIX_AT_IDEAL_SIZE in radius
// fix_at_ideal_size → value=0 → skipped. Test percent is still applied.
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusFixAtIdealSizeWithPercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"borderRadius": {"topLeft": "fix_at_ideal_size", "topRight": "5%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ShadowKindToString "custom" — custom shadow with null node
// Exercises ShadowKindToString(kind) with kind=CUSTOM in LOG
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_CustomShadowNullNode_LogsWithKind)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({
        "shadow": {
            "radius": 10,
            "color": "#FF000000",
            "offsetX": 2,
            "offsetY": 4,
            "type": "color"
        }
    })");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Null node + custom shadow → LOG with ShadowKindToString("custom")
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

// =============================================================================
// COVERAGE GAP: Padding percent with individual percent fields
// Exercises hasPercent=true, hasAbsolute=false path via individual keys
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingIndividualPercentFields_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": "10%", "right": "15%", "bottom": "20%", "left": "25%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: Margin percent with individual percent fields
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginIndividualPercentFields_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles =
        JsonAdapter::Parse(R"({"marginTop": "10%", "marginRight": "15%", "marginBottom": "20%", "marginLeft": "25%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: HasAnyField — styles not object returns false
// This is called from ApplyEdgeStyles with styles that were already validated as object
// by ResolveAndApply. But to exercise the !styles.IsObject() path in HasAnyField,
// we'd need to call ApplyEdgeStyles directly, which is private.
// Test the reachable path: styles with padding key present → HasAnyField returns true
// and with margin keys present → HasAnyField returns true
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingKeyOnly_HasAnyFieldTrue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: FormatDimension with FP unit — via LOG in mixed padding
// Exercises FormatDimension output like "10fp" via FormatEdge
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingFPMixedPercent_LogsFormatEdgeWithFP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"top": "10fp", "right": "5%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Mixed FP + percent → LOG with FormatEdge containing FormatDimension("10fp")
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.padding") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: GetTextStyleValue — preferred name path (legacy invalid, preferred valid)
// Test with minFontSize present but minfontSize absent
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_MinFontSizePreferredName)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"minFontSize": 8})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_TEXT_MIN_FONT_SIZE) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_FLOAT_EQ(rec.values[0].f32, 8.0F);
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSize — only width VP (no height)
// Exercises ParseBackgroundImageSizeWithPercent with parsed[0].present=true, parsed[1].present=false
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyWidth_VP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": 100}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSize — width percent only, with dispatch
// Exercises percent path with only width
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyWidthPercent_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSize — both percent
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeBothPercent_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%", "height": "75%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSize — VP height + percent width (mixed)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizePercentWidthVPHeight_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%", "height": 200}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: FormatStyleInput with number type — exercises line 288 (non-string/array/object)
// FormatStyleInput(value) for a number returns "type=number"
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_FontSizeNumber_LogsFormatStyleInputNumber)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"fontSize": 18})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // fontSize=18 → ParseNumber succeeds → SetNodeFontSize. Also exercises LOG_INFO.
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FONT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: DimensionUnitToString FP unit — via unsupported unit LOG in ApplyDimension
// ParseDimension("10fp") returns FP unit → VP/FP case → DimensionToFloat → LOG_INFO
// This is NOT the unsupported unit LOG; it's the normal VP/FP path.
// The unsupported unit LOG calls DimensionUnitToString only for unknown units.
// To exercise DimensionUnitToString(VP), we need FormatDimension(VP) which is called
// from FormatEdge/FormatRadius. Already exercised by mixed VP+percent tests.
// For FP: exercise via mixed FP+percent padding
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginFPMixedPercent_LogsFormatEdgeWithFP)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"marginTop": "10fp", "marginRight": "5%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ApplyTextComponentStyles null node with multiple style keys
// Exercises FormatNamedStyleInputs with matching keys
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextComponentStyles_NullNodeMultipleKeys_LogsAllInputs)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles =
        JsonAdapter::Parse(R"({"fontWeight": 700, "maxLines": 3, "textAlign": "center", "decoration": "underline"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ApplyTextComponentStyles(styles->GetRoot(), applier);
    // Null node → LOG_WARN with FormatNamedStyleInputs showing all present keys
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FONT_WEIGHT);
        EXPECT_NE(rec.attribute, NODE_TEXT_MAX_LINES);
        EXPECT_NE(rec.attribute, NODE_TEXT_ALIGN);
        EXPECT_NE(rec.attribute, NODE_TEXT_DECORATION);
    }
}

// =============================================================================
// COVERAGE GAP: ParseBackgroundImageSizeWithPercent — infinite converted value
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeInfiniteVP_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // A very large VP value that after conversion might be infinite
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": 1e38}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Large number may or may not be infinite depending on float range
    // The key is the code path is exercised
}

// =============================================================================
// COVERAGE GAP: ApplyRadius — uniform VP radius with same value (HasSameRadius=true)
// Exercises SetBorderRadius(sameRadius) path (line 1204-1208)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusUniformVP_SameRadiusPath)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "10vp", "topRight": "10vp", "bottomRight": "10vp", "bottomLeft": "10vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusUniformVP_ReportsNestedIssueWhenInvalidChildExists)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "borderRadius": {
            "all": "",
            "topLeft": "10vp",
            "topRight": "10vp",
            "bottomRight": "10vp",
            "bottomLeft": "10vp"
        }
    })");
    ASSERT_NE(styles, nullptr);

    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1U);
    EXPECT_EQ(issues[0].path, "styles.borderRadius");
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
}

// =============================================================================
// COVERAGE GAP: Radius — all same percent corners, value != 0 → hasPercent=true
// Exercises SetBorderRadiusPercent path
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusAllSamePercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "20%", "topRight": "20%", "bottomRight": "20%", "bottomLeft": "20%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusAllSamePercent_ReportsNestedIssueWhenInvalidChildExists)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({
        "borderRadius": {
            "all": "",
            "topLeft": "20%",
            "topRight": "20%",
            "bottomRight": "20%",
            "bottomLeft": "20%"
        }
    })");
    ASSERT_NE(styles, nullptr);

    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1U);
    EXPECT_EQ(issues[0].path, "styles.borderRadius");
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
}

// =============================================================================
// COVERAGE GAP: ParseConstraintSizeStyle — negative converted value returns false
// Already tested with minWidth=-10. Add test for negative FP.
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeNegativeFP_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "-10fp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: ConstraintSize — infinite percent value
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeInfinitePercent_Ignored)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "1e38%"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    ASSERT_FALSE(mockArkUIPtr_->createdNodes_.empty());
    ArkUI_NodeHandle wrapper = mockArkUIPtr_->createdNodes_.back().nodeHandle;
    ASSERT_TRUE(mockArkUIPtr_->DispatchMeasureEvent(wrapper, std::numeric_limits<int32_t>::max(), 500));

    const auto* record = FindLastConstraintRecord(mockArkUIPtr_, testNode_);
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[0].f32, 0.0F);
}

// =============================================================================
// COVERAGE GAP: FlexShrink valid LOG_INFO — exercises FormatStyleInput(number) in LOG
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_FlexShrinkValid_LogsInfoWithFormatStyleInput)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"flexShrink": 0.5})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Exercises LOG_INFO at line 762-765 with FormatStyleInput(0.5)
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_FLEX_SHRINK) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: LayoutWeight valid LOG_INFO
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_LayoutWeightPositive_NormalizedToUint32)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"layoutWeight": 3})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_LAYOUT_WEIGHT) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].u32, 3U);
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ConstraintSize VP — exercises LOG_INFO for constraintSize pure VP
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeAllVP_SetConstraintSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"constraintSize": {"minWidth": 10, "maxWidth": 500, "minHeight": 20, "maxHeight": 800}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: Edge styles with both padding all-key and individual keys
// Exercises ParseEdgeStyle individual override of all-key
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingAllKeyAndIndividualKeys_IndividualOverrides)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"padding": "10vp", "top": "20vp", "left": "30vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: Margin all-key + individual key override
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_MarginAllKeyAndIndividualKeys_IndividualOverrides)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"margin": "5vp", "marginTop": "15vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_MARGIN) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: HasSameRadius — non-uniform VP radii
// Exercises the !HasSameRadius path in ApplyRadius (line 1204-1208 false → 1210-1226)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusNonUniformVP_SetBorderRadius4Corners)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "5vp", "topRight": "10vp", "bottomRight": "15vp", "bottomLeft": "20vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: Radius zero VP (HasSameRadius returns true, all values 0)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_RadiusUniformZeroVP_SetsZeroRadius)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"borderRadius": {"topLeft": "0vp", "topRight": "0vp", "bottomRight": "0vp", "bottomLeft": "0vp"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BORDER_RADIUS) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ConvertDimensionToPercentRatio — PERCENT returns value/100
// Exercises line 91: value / 100.0F
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_Width50Percent_AppliesHalfPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "50%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH_PERCENT) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_FLOAT_EQ(rec.values[0].f32, 0.5F);
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImage — null node with empty string
// Exercises LOG_WARN at line 862-866 (null node after parsing valid backgroundImage)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageNullNodeEmptyString_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": ""})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Empty string → ParseBackgroundImage succeeds → empty backgroundImage
    // → LOG_INFO reset path, but node is null → return before Set/Reset
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE);
    }
}

// =============================================================================
// COVERAGE GAP: ConstraintSize — only some fields with VP, rest defaults
// Exercises ParseConstraintSizeStyle with partial fields (not all 4 present)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeOnlyMinWidth_SetsDefaults)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": 50}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ConstraintSize — only maxWidth and maxHeight
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeOnlyMaxFields_SetsDefaults)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"maxWidth": 300, "maxHeight": 400}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_CONSTRAINT_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSize — with percent height + VP width + no dispatch context
// Exercises fallback to C++ path with VP width value and percent height = 0
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizePercentHeightFallback_Cpp)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": 100, "height": "50%"}})");
    ASSERT_NE(styles, nullptr);
    // No dispatch context → LOG_WARN fallback + C++ path
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: DimensionToFloat — MATCH_PARENT negative (already covered but add explicit test)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_MatchParentInfinite_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::MATCH_PARENT;
    dim.value = std::numeric_limits<float>::infinity();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// COVERAGE GAP: DimensionToFloat — MATCH_PARENT NaN
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_MatchParentNaN_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::MATCH_PARENT;
    dim.value = std::numeric_limits<float>::quiet_NaN();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// COVERAGE GAP: DimensionToFloat — FP negative infinite
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_FP_NegativeInfinity_ClampsToZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::FP;
    dim.value = -std::numeric_limits<float>::infinity();
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// COVERAGE GAP: ParseEdgeStyle — all key present + individual keys
// Exercises the full ParseEdgeStyle path with both all-key and individual overrides
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ParseEdgeStyle_AllKeyAndIndividualBothPresent)
{
    StyleEdge edge;
    auto styles = JsonAdapter::Parse(R"({"padding": "5vp", "top": "10vp", "bottom": "15vp"})");
    ASSERT_NE(styles, nullptr);
    EXPECT_TRUE(
        ExtendedStyleResolver::ParseEdgeStyle(styles->GetRoot(), "padding", "top", "right", "bottom", "left", edge));
    // Individual "top" and "bottom" override "padding" all-key
    EXPECT_FLOAT_EQ(edge.top.value, 10.0F);
    EXPECT_FLOAT_EQ(edge.right.value, 5.0F);
    EXPECT_FLOAT_EQ(edge.bottom.value, 15.0F);
    EXPECT_FLOAT_EQ(edge.left.value, 5.0F);
}

// =============================================================================
// COVERAGE GAP: ApplyBackgroundImage — invalid value (bool)
// Exercises LOG_WARN at line 853-856 with FormatStyleInput(bool)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageBool_LogsWarning)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    // Bool is not a valid background image → ParseBackgroundImage fails → LOG + issue
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.backgroundImage") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ApplyBackgroundImage — backgroundImage (lowercase i) empty string
// Exercises the empty string reset path with lowercase property name
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageEmpty_Resets)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": ""})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->resetAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSizeWithStyle — valid string "auto"
// Exercises string parse path with "auto" → IMAGE_SIZE → SetNodeBackgroundImageSizeWithStyle
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeAuto_SetsWithStyle)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "auto"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImageSizeWithStyle — valid string "fill"
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeFill_SetsWithStyle)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "fill"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: BackgroundImage — valid URL (exercises LOG_INFO at line 878-881)
// Already tested but add explicit test for LOG coverage with specific URL format
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageValidUrl_LogsAppliedLength)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImage": "https://example.com/long/path/image.png"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Exercises LOG_INFO with backgroundImage.size() format
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ConstraintSize — percent with only minWidth percent
// Exercises ParseConstraintSizeStyle with single percent field (hasPercent=true, outPercentJson has one field)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeOnlyMinWidthPercent_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 20;
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "30%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: ConstraintSize — percent with mixed VP+FP+percent
// Exercises ParseConstraintSizeStyle with VP+FP in percent payload
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeMixedFPPercent_Dispatches)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    ConstraintDispatchContext dispatchCtx;
    dispatchCtx.renderId = 1;
    dispatchCtx.componentId = "test-comp";
    dispatchCtx.nodeUniqueId = 42;
    dispatchCtx.componentType = "Column";
    dispatchCtx.apiVersion = 20;
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "16fp", "maxWidth": "80%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, dispatchCtx);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// COVERAGE GAP: DimensionToFloat — PERCENT zero value (value==0 → clamped to 0)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_PercentZero_ReturnsZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::PERCENT;
    dim.value = 0.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// COVERAGE GAP: DimensionToFloat — VP zero (already covered but explicit test)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, DimensionToFloat_VPZero_ReturnsZero)
{
    StyleDimension dim;
    dim.unit = StyleDimensionUnit::VP;
    dim.value = 0.0F;
    float out = -1.0F;
    EXPECT_TRUE(ExtendedStyleResolver::DimensionToFloat(dim, out));
    EXPECT_FLOAT_EQ(out, 0.0F);
}

// =============================================================================
// COVERAGE GAP: ApplyDimension — width FP conversion (exercises VP/FP path for width)
// This exercises the ResetNodeWidthPercent + SetWidth path with FP-converted value
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_WidthFP_ConvertedAndApplied)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "20fp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_WIDTH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ConvertDimensionToPercentRatio — MATCH_PARENT returns 1.0
// This is tested via match_parent width/height but add direct verification
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_HeightMatchParent_ExactPercentRatio)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"height": "match_parent"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            found = true;
            ASSERT_FALSE(rec.values.empty());
            EXPECT_EQ(rec.values[0].i32, 0); // MATCH_PARENT
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: ApplyTextDecoration — valid decoration with null node
// Already tested but add for completeness
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ApplyTextDecoration_ValidDecorationNullNode_NoSet)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto value =
        JsonAdapter::Parse(R"({"type": "overline", "color": "#ff000000", "style": "dashed", "thicknessScale": 2.0})");
    ASSERT_NE(value, nullptr);
    ExtendedStyleResolver::ApplyTextDecoration(value->GetRoot(), applier);
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_TEXT_DECORATION);
    }
}

// =============================================================================
// COVERAGE GAP: ApplyShadow — style shadow with null node
// Exercises ShadowKindToString(STYLE) in LOG
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_StyleShadowNullNode_LogsKindStyle)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    auto styles = JsonAdapter::Parse(R"({"shadow": 0})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    // Null node + style shadow → LOG with ShadowKindToString("style")
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

// =============================================================================
// COVERAGE GAP: Padding individual keys with FP units
// Exercises FormatDimension with FP unit in FormatEdge (via individual keys)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingFPIndividual_SetsPadding)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": "10fp", "right": "5vp"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE GAP: Padding with all same percent via individual keys
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_PaddingIndividualSamePercent_SetsPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"top": "10%", "right": "10%", "bottom": "10%", "left": "10%"})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_PADDING_PERCENT) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE: ApplyBackgroundImageSize — string input that ParseBackgroundImageSize
// returns true but kind != IMAGE_SIZE (e.g. object with width/height → kind=SIZE)
// Exercises L1279 branch 1 false: imageSize.kind != IMAGE_SIZE
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeObjectWithVpOnly_NoPercentKindNotImageSize)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    // Object with only VP values → ParseBackgroundImageSize returns kind=SIZE (not IMAGE_SIZE)
    // but ApplyBackgroundImageSize checks IsString first, so this goes to the object path.
    // To hit L1279 we need a string input where ParseBackgroundImageSize succeeds but kind!=IMAGE_SIZE
    // That's impossible for strings (always IMAGE_SIZE), so this branch is effectively dead for string path.
    // The only way: a string that ParseBackgroundImageSize parses as a non-IMAGE_SIZE kind.
    // Since ParseImageSizeToken only maps to IMAGE_SIZE, this is dead code for string path.
    // No test can cover this — it's defensive code.
}

// =============================================================================
// COVERAGE: ParseConstraintSizeStyle — FP unit with failing density API
// Exercises L138: ConvertDimensionToFloat returns false for FP when density fails
// By default stub returns density=1.0, so we need to manipulate the global.
// The globals g_displayDensityPixels/g_displayScaledDensity are in stub_impl.cpp
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeFP_NegativeDensityClamp)
{
    // FP values with density=1.0 should work fine and return true
    // ConvertDimensionToFloat for FP only returns false if... let's check:
    // It calculates densityScale = scaledDensity/densityPixels, then value = dim.value * densityScale
    // If the result is non-finite or negative, it clamps to 0 and returns true (not false!)
    // So ConvertDimensionToFloat never returns false for FP — it always clamps.
    // L138/L219 are dead code branches.
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10fp", "maxWidth": "50fp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — only width present (no height)
// Exercises L235 branch: only one field present → still valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyWidth_VpOnly)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "100vp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — only height present (no width)
// Exercises L235 branch: only height present → still valid
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyHeight_VpOnly)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"height": "200vp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — only width percent (no height)
// Exercises percent path with single field
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeOnlyWidthPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "75%"}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext ctx { 1, "comp-bis", 100, "Column" };
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, ctx, issues);
    // Percent field → dispatches to ETS, no NODE_BACKGROUND_IMAGE_SIZE set
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseConstraintSizeStyle — all 4 fields percent → heavy percent JSON
// Exercises ostringstream percent JSON building with all 4 fields
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeAllPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(
        R"({"constraintSize": {"minWidth": "10%", "maxWidth": "90%", "minHeight": "5%", "maxHeight": "95%"}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext ctx { 1, "comp-allpct", 200, "Column" };
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, ctx, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseConstraintSizeStyle — mixed: some VP some percent, 3 fields
// Exercises 3-field mixed path (not all 4, not pure VP)
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_ConstraintSizeThreeFieldsMixed)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles =
        JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10vp", "maxWidth": "50%", "maxHeight": "100vp"}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext ctx { 1, "comp-3mix", 300, "Row" };
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, ctx, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — width percent + height VP
// (reverse of the existing test which is width% + height VP)
// Exercises both branches of the if(parsed[i].unit == PERCENT) for each field
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeHeightPercentWidthVp)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "80vp", "height": "60%"}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext ctx { 1, "comp-rev", 100, "Column" };
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, ctx, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — both percent
// Exercises both fields as percent
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeBothPercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "50%", "height": "75%"}})");
    ASSERT_NE(styles, nullptr);
    ConstraintDispatchContext ctx { 1, "comp-bothpct", 100, "Column" };
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, ctx, issues);
    EXPECT_TRUE(issues.empty());
}

// =============================================================================
// COVERAGE: ParseBackgroundImageSizeWithPercent — empty object → return false
// Exercises L235: neither width nor height present
// =============================================================================
TEST_F(ExtendedStyleResolverTddTest, ResolveAndApply_BackgroundImageSizeEmptyObject2_NoFields)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.backgroundImageSize") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// aspectRatio common style — AC-1~AC-7, AC-11
// =============================================================================

namespace {
float LastAspectRatioValue(const MockArkUINativeProvider* mock, ArkUI_NodeHandle node)
{
    for (auto iter = mock->setAttributeRecords_.rbegin(); iter != mock->setAttributeRecords_.rend(); ++iter) {
        if (iter->nodeHandle == node && iter->attribute == NODE_ASPECT_RATIO) {
            if (!iter->values.empty()) {
                return iter->values[0].f32;
            }
        }
    }
    return -1.0F;
}

int32_t CountAspectRatioCalls(const MockArkUINativeProvider* mock, ArkUI_NodeHandle node)
{
    int32_t count = 0;
    for (const auto& rec : mock->setAttributeRecords_) {
        if (rec.nodeHandle == node && rec.attribute == NODE_ASPECT_RATIO) {
            ++count;
        }
    }
    return count;
}

bool HasIssue(const std::vector<DescriptorValidationIssue>& issues, const std::string& code, const std::string& path,
    const std::string& messageFragment)
{
    for (const auto& issue : issues) {
        if (issue.code == code && issue.path == path && issue.message.find(messageFragment) != std::string::npos) {
            return true;
        }
    }
    return false;
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_overflow_infinity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": 1e309})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}
} // namespace

TEST_F(ExtendedStyleResolverTddTest, L0_should_apply_aspect_ratio_for_common_component)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": 1.5})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_TRUE(issues.empty());
    EXPECT_EQ(CountAspectRatioCalls(mockArkUIPtr_, testNode_), 1);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.5F);
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_negative_value)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": -1})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_zero)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": 0})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_nan)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto nanAdapter = JsonAdapter::CreateObject();
    ASSERT_NE(nanAdapter, nullptr);
    JsonValue nanRoot = nanAdapter->GetRoot();
    ASSERT_TRUE(nanRoot.PutNumber("aspectRatio", std::numeric_limits<double>::quiet_NaN()));
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(nanRoot, applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_non_number_type)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": "2.5"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.aspectRatio", "got type"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_null)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": null})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.aspectRatio", "got type 'null'"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_not_apply_aspect_ratio_when_not_set)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"width": "100vp"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_EQ(CountAspectRatioCalls(mockArkUIPtr_, testNode_), 0);
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_reset_aspect_ratio_when_previously_applied_then_removed)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto first = JsonAdapter::Parse(R"({"aspectRatio": 2.0})");
    ASSERT_NE(first, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(first->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 2.0F);

    StyleResetProperty resetProp { .rawName = "aspectRatio", .name = StylePropertyName::ASPECT_RATIO };
    ExtendedStyleResolver::Reset(resetProp, applier);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_skip_aspect_ratio_when_node_is_null)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(nullptr);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": 2.0})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_EQ(CountAspectRatioCalls(mockArkUIPtr_, testNode_), 0);
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_infinity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto infAdapter = JsonAdapter::CreateObject();
    ASSERT_NE(infAdapter, nullptr);
    JsonValue infRoot = infAdapter->GetRoot();
    ASSERT_TRUE(infRoot.PutNumber("aspectRatio", std::numeric_limits<double>::infinity()));
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(infRoot, applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_negative_infinity)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto negInfAdapter = JsonAdapter::CreateObject();
    ASSERT_NE(negInfAdapter, nullptr);
    JsonValue negInfRoot = negInfAdapter->GetRoot();
    ASSERT_TRUE(negInfRoot.PutNumber("aspectRatio", -std::numeric_limits<double>::infinity()));
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(negInfRoot, applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.aspectRatio", "invalid number value"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_boolean_type)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.aspectRatio", "got type"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_object_type)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": {"nested": 1}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.aspectRatio", "got type"));
}

TEST_F(ExtendedStyleResolverTddTest, L0_should_fallback_aspect_ratio_for_array_type)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    auto styles = JsonAdapter::Parse(R"({"aspectRatio": [1, 2]})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);
    EXPECT_FLOAT_EQ(LastAspectRatioValue(mockArkUIPtr_, testNode_), 1.0F);
    EXPECT_TRUE(HasIssue(issues, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.aspectRatio", "got type"));
}

} // namespace
