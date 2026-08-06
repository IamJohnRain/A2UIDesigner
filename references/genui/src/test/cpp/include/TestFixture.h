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

#ifndef A2UI_TEST_FIXTURE_H
#define A2UI_TEST_FIXTURE_H

#include <gtest/gtest.h>

#define private public
#include "utils/SystemProperties.h"
#undef private

#include "ArkUINativeAPI.h"
#include "NapiBridge.h"
#include "include/mock_arkui_native_provider.h"
#include "include/mock_napi_provider.h"

namespace NativeModule {

inline void ResetApiVersionForTdd()
{
    // SystemProperties intentionally ignores a second non-zero assignment in production.
    // TDD must force-reset the process-wide singleton between test cases to avoid order coupling.
    SystemProperties::GetInstance().apiVersion_ = 0;
}

class A2UITest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto mockArkUI = MockArkUINativeProvider::Create();
        mockArkUIPtr_ = mockArkUI.get();
        ArkUINativeAPI::SetProvider(std::move(mockArkUI));

        auto mockNapi = MockNapiProvider::Create();
        mockNapiPtr_ = mockNapi.get();
        NapiBridge::SetProvider(std::move(mockNapi));
        ResetApiVersionForTdd();
    }

    void TearDown() override
    {
        ResetApiVersionForTdd();
        if (mockArkUIPtr_ != nullptr) {
            mockArkUIPtr_->ResetAllMocks();
        }
        if (mockNapiPtr_ != nullptr) {
            mockNapiPtr_->Reset();
        }
        ArkUINativeAPI::SetProvider(nullptr);
        NapiBridge::SetProvider(nullptr);
        mockArkUIPtr_ = nullptr;
        mockNapiPtr_ = nullptr;
    }

    MockArkUINativeProvider* mockArkUIPtr_ = nullptr;
    MockNapiProvider* mockNapiPtr_ = nullptr;
};

} // namespace NativeModule

#endif // A2UI_TEST_FIXTURE_H
