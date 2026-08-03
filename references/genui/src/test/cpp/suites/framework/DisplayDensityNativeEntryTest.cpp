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

#include "utils/DisplayDensityUtils.h"

#include "NativeEntry.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

constexpr int32_t DISPLAY_DENSITY_TEST_RENDER_ID = 830199;

class DisplayDensityNativeEntryTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);

    napi_value CreateInt32Arg(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    napi_value CreateDoubleArg(double value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateDouble(env_, value, &result);
        return result;
    }

    void TearDown() override
    {
        mockNapiPtr_->ResetGetValueInt32Status();
        mockNapiPtr_->ResetGetValueDoubleStatus();
        mockNapiPtr_->ResetGetValueDoubleFailOnCall();
        DisplayDensityUtils::GetInstance().ClearDisplayDensity(DISPLAY_DENSITY_TEST_RENDER_ID);
        A2UITest::TearDown();
    }
};

TEST_F(DisplayDensityNativeEntryTest, should_set_display_density_with_two_args_via_napi)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5) });

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 40.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_set_fp_to_vp_scale_with_three_args_via_napi)
{
    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5), CreateDoubleArg(0.75) });

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float fp = DisplayDensityUtils::GetInstance().ConvertFpToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 8.0F);
    EXPECT_FLOAT_EQ(fp, 6.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_with_insufficient_args)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID) });
    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_when_render_id_arg_is_null)
{
    mockNapiPtr_->SetCallbackArgs({ nullptr, CreateDoubleArg(2.5) });

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 100.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_when_density_arg_is_null)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), nullptr });

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 100.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_when_render_id_parse_fails)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5) });
    mockNapiPtr_->SetGetValueInt32Status(napi_invalid_arg);

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 100.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_when_density_parse_fails)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5) });
    mockNapiPtr_->SetGetValueDoubleStatus(napi_invalid_arg);

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 100.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_reject_set_display_density_when_fp_to_vp_scale_parse_fails)
{
    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5), CreateDoubleArg(0.75) });
    mockNapiPtr_->SetGetValueDoubleFailOnCall(2, napi_invalid_arg);

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    float fp = DisplayDensityUtils::GetInstance().ConvertFpToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 8.0F);
    EXPECT_FLOAT_EQ(vp, 100.0F);
    EXPECT_FLOAT_EQ(fp, 8.0F);
}

TEST_F(DisplayDensityNativeEntryTest, should_accept_missing_optional_fp_to_vp_scale_when_third_arg_is_null)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5), nullptr });

    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    float fp = DisplayDensityUtils::GetInstance().ConvertFpToVp(DISPLAY_DENSITY_TEST_RENDER_ID, 8.0F);
    EXPECT_FLOAT_EQ(vp, 40.0F);
    EXPECT_FLOAT_EQ(fp, 8.0F);
}

} // namespace
