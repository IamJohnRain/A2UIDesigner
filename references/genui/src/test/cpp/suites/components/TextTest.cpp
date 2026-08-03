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

#include "components/A2UI/text/TextComponent.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

class TextTest : public A2UITest {
protected:
    std::unique_ptr<TextComponent> text_;

    void SetUp() override
    {
        A2UITest::SetUp();
        text_ = std::make_unique<TextComponent>();
    }

    void TearDown() override
    {
        text_.reset();
        A2UITest::TearDown();
    }
};

/**
 * @tc.name: TextTest001
 * @tc.desc: Verify the following TextComponent behavior: return text type.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest001)
{
    /**
     * @tc.steps: step1. Invoke the target TextComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    EXPECT_EQ(text_->GetType(), "Text");
}

/**
 * @tc.name: TextTest001_001
 * @tc.desc: Verify component base stores A2UI protocol version metadata.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest001_001)
{
    SurfaceContext context;
    context.a2UIProtocolVersion = "v0.9";

    text_->SetSurfaceContext(context);

    EXPECT_EQ(text_->GetSurfaceContext().a2UIProtocolVersion, "v0.9");
}

/**
 * @tc.name: TextTest002
 * @tc.desc: Verify the following TextComponent behavior: not crash on creation.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest002)
{
    /**
     * @tc.steps: step1. Invoke the target TextComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    ASSERT_NE(text_, nullptr);
}

/**
 * @tc.name: TextTest003
 * @tc.desc: Verify the following TextComponent behavior: handle on data update.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest003)
{
    /**
     * @tc.steps: step1. Invoke the target TextComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("hello");
    ASSERT_NE(value, nullptr);
    EXPECT_NO_THROW(text_->OnDataUpdate("text", value->GetRoot()));
}

/**
 * @tc.name: TextTest004
 * @tc.desc: Verify the following TextComponent behavior: handle empty property.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest004)
{
    /**
     * @tc.steps: step1. Invoke the target TextComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("value");
    ASSERT_NE(value, nullptr);
    EXPECT_NO_THROW(text_->OnDataUpdate("", value->GetRoot()));
}

/**
 * @tc.name: TextTest005
 * @tc.desc: Verify the following TextComponent behavior: handle empty value.
 * @tc.type: FUNC
 */
TEST_F(TextTest, TextTest005)
{
    /**
     * @tc.steps: step1. Invoke the target TextComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("");
    ASSERT_NE(value, nullptr);
    EXPECT_NO_THROW(text_->OnDataUpdate("text", value->GetRoot()));
}
