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

#include "utils/ThemeColorUtils.h"

#include <gtest/gtest.h>

using namespace NativeModule;

/**
 * @tc.name: ThemeColorUtilsTest001
 * @tc.desc: Verify the following theme color parsing behavior: parse #RRGGBB with default alpha.
 * @tc.type: FUNC
 */
TEST(ThemeColorUtilsTest, ThemeColorUtilsTest001)
{
    uint32_t argb = 0;
    bool success = ThemeColorUtils::TryParseArgb("#FF6A00", argb);
    EXPECT_TRUE(success);
    EXPECT_EQ(argb, 0xFFFF6A00u);
}

/**
 * @tc.name: ThemeColorUtilsTest002
 * @tc.desc: Verify the following theme color parsing behavior: parse #AARRGGBB with explicit alpha.
 * @tc.type: FUNC
 */
TEST(ThemeColorUtilsTest, ThemeColorUtilsTest002)
{
    uint32_t argb = 0;
    bool success = ThemeColorUtils::TryParseArgb("#80112233", argb);
    EXPECT_TRUE(success);
    EXPECT_EQ(argb, 0x80112233u);
}

/**
 * @tc.name: ThemeColorUtilsTest003
 * @tc.desc: Verify the following theme color parsing behavior: reject invalid prefix or length.
 * @tc.type: FUNC
 */
TEST(ThemeColorUtilsTest, ThemeColorUtilsTest003)
{
    uint32_t argb = 0;
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("FF6A00", argb));
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("#12345", argb));
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("#123456789", argb));
}

/**
 * @tc.name: ThemeColorUtilsTest004
 * @tc.desc: Verify the following theme color parsing behavior: reject invalid hex characters.
 * @tc.type: FUNC
 */
TEST(ThemeColorUtilsTest, ThemeColorUtilsTest004)
{
    uint32_t argb = 0;
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("#GG6A00", argb));
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("#FF0X0Y", argb));
    EXPECT_FALSE(ThemeColorUtils::TryParseArgb("#AA11ZZ33", argb));
}

/**
 * @tc.name: ThemeColorUtilsTest005
 * @tc.desc: Verify the following theme color utility behavior: invert RGB and keep alpha.
 * @tc.type: FUNC
 */
TEST(ThemeColorUtilsTest, ThemeColorUtilsTest005)
{
    EXPECT_EQ(ThemeColorUtils::InvertRgbKeepAlpha(0xFF123456u), 0xFFEDCBA9u);
    EXPECT_EQ(ThemeColorUtils::InvertRgbKeepAlpha(0x80112233u), 0x80EEDDCCu);
}
