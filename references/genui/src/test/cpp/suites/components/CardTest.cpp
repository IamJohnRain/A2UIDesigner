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
#include <gtest/gtest.h>

#include "components/A2UI/card/CardComponent.h"
#include "components/A2UI/card/CardTheme.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"
#include "arkui/native_node.h"

using namespace NativeModule;

class CardTest : public A2UITest {
protected:
    std::unique_ptr<CardComponent> card_;

    void SetUp() override
    {
        A2UITest::SetUp();
        card_ = std::make_unique<CardComponent>();
    }

    void TearDown() override
    {
        card_.reset();
        A2UITest::TearDown();
    }
};

/**
 * @tc.name: CardTest001
 * @tc.desc: Verify the following CardComponent behavior: return card type.
 * @tc.type: FUNC
 */
TEST_F(CardTest, CardTest001)
{
    /**
     * @tc.steps: step1. Query the component type from CardComponent.
     * @tc.expected: The component type is Card.
     */

    EXPECT_EQ(card_->GetType(), "Card");
}

/**
 * @tc.name: CardTest002
 * @tc.desc: Verify the following CardTheme behavior: expose default style metrics.
 * @tc.type: FUNC
 */
TEST_F(CardTest, CardTest002)
{
    /**
     * @tc.steps: step1. Create CardTheme and query the default style metrics.
     * @tc.expected: The theme exposes the expected border radius, padding, border, background, and shadow style.
     */

    CardTheme theme(ThemeContext {});
    const CardTheme::StyleMetrics& styleMetrics = theme.GetStyleMetrics();
    const CardTheme::ValueMetrics& valueMetrics = theme.GetValueMetrics();
    const CardTheme::AppearanceMetrics& appearanceMetrics = theme.GetAppearanceMetrics();

    EXPECT_FLOAT_EQ(styleMetrics.borderRadius, 8.0F);
    EXPECT_EQ(styleMetrics.shadowStyle, ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    EXPECT_FLOAT_EQ(styleMetrics.padding, 16.0F);
    EXPECT_FLOAT_EQ(styleMetrics.borderWidth, 1.0F);
    EXPECT_EQ(styleMetrics.borderColor, 0xFFE0E0E0);
    EXPECT_EQ(styleMetrics.backgroundColor, 0xFFFFFFFF);

    EXPECT_FLOAT_EQ(valueMetrics.borderRadius, 8.0F);
    EXPECT_FLOAT_EQ(valueMetrics.padding, 16.0F);
    EXPECT_FLOAT_EQ(valueMetrics.borderWidth, 1.0F);
    EXPECT_EQ(appearanceMetrics.shadowStyle, ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    EXPECT_EQ(appearanceMetrics.borderColor, 0xFFE0E0E0);
    EXPECT_EQ(appearanceMetrics.backgroundColor, 0xFFFFFFFF);
}

/**
 * @tc.name: CardTest003
 * @tc.desc: Verify the following CardTheme behavior: expose dark mode style metrics.
 * @tc.type: FUNC
 */
TEST_F(CardTest, CardTest003)
{
    /**
     * @tc.steps: step1. Create CardTheme with dark mode and query the default style metrics.
     * @tc.expected: The theme exposes dark-mode-aware border and background colors while preserving shadow style.
     */

    ThemeContext darkContext;
    darkContext.colorMode = ThemeMode::DARK;

    CardTheme theme(darkContext);
    const CardTheme::StyleMetrics& styleMetrics = theme.GetStyleMetrics();
    const CardTheme::AppearanceMetrics& appearanceMetrics = theme.GetAppearanceMetrics();

    EXPECT_FLOAT_EQ(styleMetrics.borderRadius, 8.0F);
    EXPECT_EQ(styleMetrics.shadowStyle, ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    EXPECT_FLOAT_EQ(styleMetrics.padding, 16.0F);
    EXPECT_FLOAT_EQ(styleMetrics.borderWidth, 1.0F);
    EXPECT_EQ(styleMetrics.borderColor, 0xFF333333);
    EXPECT_EQ(styleMetrics.backgroundColor, 0xFF1A1A1A);

    EXPECT_EQ(appearanceMetrics.shadowStyle, ARKUI_SHADOW_STYLE_OUTER_DEFAULT_LG);
    EXPECT_EQ(appearanceMetrics.borderColor, 0xFF333333);
    EXPECT_EQ(appearanceMetrics.backgroundColor, 0xFF1A1A1A);
}

/**
 * @tc.name: CardTest004
 * @tc.desc: Verify the following CardComponent behavior: do not silently apply fallback theme defaults when theme is
 * unavailable.
 * @tc.type: FUNC
 */
TEST_F(CardTest, CardTest004)
{
    /**
     * @tc.steps: step1. Apply an empty descriptor to a detached CardComponent without render/surface/theme context.
     * @tc.expected: The component does not push fallback theme attributes to ArkUI, exposing the missing theme as an
     * error path.
     */

    auto descriptor = JsonAdapter::Parse(R"({})");
    ASSERT_NE(descriptor, nullptr);

    mockArkUIPtr_->ResetAllMocks();
    card_->ApplyDescriptor(descriptor->GetRoot());

    EXPECT_EQ(std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(),
                  NODE_BORDER_RADIUS),
        0);
    EXPECT_EQ(std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(),
                  NODE_BACKGROUND_COLOR),
        0);
    EXPECT_EQ(
        std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(), NODE_SHADOW), 0);
    EXPECT_EQ(
        std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(), NODE_PADDING),
        0);
    EXPECT_EQ(std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(),
                  NODE_BORDER_WIDTH),
        0);
    EXPECT_EQ(std::count(mockArkUIPtr_->setAttributeTypes_.begin(), mockArkUIPtr_->setAttributeTypes_.end(),
                  NODE_BORDER_COLOR),
        0);
}

/**
 * @tc.name: CardTest005
 * @tc.desc: Verify the following CardTheme behavior: expose value metrics through a breakpoint-oriented interface.
 * @tc.type: FUNC
 */
TEST_F(CardTest, CardTest005)
{
    /**
     * @tc.steps: step1. Create CardTheme with a wide-screen breakpoint and query value metrics.
     * @tc.expected: The numeric metrics are provided through the dedicated value interface and currently keep the
     * default fixed values.
     */

    ThemeContext wideContext;
    wideContext.breakpoint = Breakpoint::XL;

    CardTheme theme(wideContext);
    const CardTheme::ValueMetrics& valueMetrics = theme.GetValueMetrics();

    EXPECT_FLOAT_EQ(valueMetrics.borderRadius, 8.0F);
    EXPECT_FLOAT_EQ(valueMetrics.padding, 16.0F);
    EXPECT_FLOAT_EQ(valueMetrics.borderWidth, 1.0F);
}
