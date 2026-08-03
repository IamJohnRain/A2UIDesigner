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

#ifndef GTEST_EXT_H
#define GTEST_EXT_H

#include <gtest/gtest.h>

namespace testing {
namespace ext {

enum TestSize { Level0 = 0, Level1 = 1, Level2 = 2, Level3 = 3, Level4 = 4 };

} // namespace ext
} // namespace testing

#undef TEST_F
#define TEST_F(test_fixture, test_name, ...) \
    GTEST_TEST_(test_fixture, test_name, test_fixture, ::testing::internal::GetTypeId<test_fixture>())

#endif // GTEST_EXT_H
