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

#include <cmath>
#include <gtest/gtest.h>
#include <limits>

#define private public
#include "utils/DisplayDensityUtils.h"
#undef private

using namespace NativeModule;

class DisplayDensityUtilsTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        DisplayDensityUtils::GetInstance().ClearDisplayDensity(kValidRenderId);
        DisplayDensityUtils::GetInstance().ClearDisplayDensity(kOtherRenderId);
    }

    static constexpr int32_t kValidRenderId = 1;
    static constexpr int32_t kOtherRenderId = 99;
};

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_ValidStores)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F);
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 50.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_NegativeRenderIdIgnored)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(-1, 3.0F);
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(-1, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_NanDensityIgnored)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, std::nanf(""));
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_InfDensityIgnored)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, std::numeric_limits<float>::infinity());
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_ZeroDensityIgnored)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 0.0F);
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_NegativeDensityIgnored)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, -2.5F);
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, ClearDisplayDensity_RemovesEntry)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 3.0F);
    float before = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 90.0F);
    EXPECT_FLOAT_EQ(before, 30.0F);
    DisplayDensityUtils::GetInstance().ClearDisplayDensity(kValidRenderId);
    float after = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 90.0F);
    EXPECT_FLOAT_EQ(after, 90.0F);
}

TEST_F(DisplayDensityUtilsTest, ClearDisplayDensity_NonExistentNoop)
{
    DisplayDensityUtils::GetInstance().ClearDisplayDensity(9999);
}

TEST_F(DisplayDensityUtilsTest, ConvertPxToVp_NotFoundReturnsOriginal)
{
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(9999, 50.0F);
    EXPECT_FLOAT_EQ(result, 50.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertPxToVp_InvalidStoredDensityReturnsOriginal)
{
    DisplayDensityUtils::GetInstance().densityByRenderId_[kValidRenderId] = { 0.0F, 1.0F };
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 50.0F);
    EXPECT_FLOAT_EQ(result, 50.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertPxToVp_DifferentRenderIds)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F);
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kOtherRenderId, 4.0F);
    float result1 = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    float result2 = DisplayDensityUtils::GetInstance().ConvertPxToVp(kOtherRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result1, 50.0F);
    EXPECT_FLOAT_EQ(result2, 25.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertFpToVp_WithValidScale)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F, 1.5F);
    float result = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 150.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertFpToVp_NoScaleReturnsOriginal)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F);
    float result = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(result, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertFpToVp_NoDensityReturnsOriginal)
{
    float result = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, 50.0F);
    EXPECT_FLOAT_EQ(result, 50.0F);
}

TEST_F(DisplayDensityUtilsTest, SetDisplayDensity_InvalidNonZeroScaleFallsBackToZero)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F, -1.0F);
    float pxResult = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, 100.0F);
    float fpResult = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, 100.0F);
    EXPECT_FLOAT_EQ(pxResult, 50.0F);
    EXPECT_FLOAT_EQ(fpResult, 100.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertPxToVp_NegativeResolvedValueReturnsOriginal)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F);
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, -100.0F);
    EXPECT_FLOAT_EQ(result, -100.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertPxToVp_OverflowResolvedValueReturnsOriginal)
{
    float originalPx = std::numeric_limits<float>::max();
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, std::numeric_limits<float>::min());
    float result = DisplayDensityUtils::GetInstance().ConvertPxToVp(kValidRenderId, originalPx);
    EXPECT_FLOAT_EQ(result, originalPx);
}

TEST_F(DisplayDensityUtilsTest, ConvertFpToVp_NegativeResolvedValueReturnsOriginal)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F, 1.5F);
    float result = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, -8.0F);
    EXPECT_FLOAT_EQ(result, -8.0F);
}

TEST_F(DisplayDensityUtilsTest, ConvertFpToVp_OverflowResolvedValueReturnsOriginal)
{
    DisplayDensityUtils::GetInstance().SetDisplayDensity(kValidRenderId, 2.0F, std::numeric_limits<float>::max());
    float result = DisplayDensityUtils::GetInstance().ConvertFpToVp(kValidRenderId, 2.0F);
    EXPECT_FLOAT_EQ(result, 2.0F);
}
