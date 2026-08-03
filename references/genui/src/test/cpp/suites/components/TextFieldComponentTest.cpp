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

#include "components/A2UI/textfield/TextFieldComponent.h"

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

int32_t CountAttribute(const MockArkUINativeProvider* provider, int32_t attribute)
{
    if (provider == nullptr) {
        return 0;
    }
    int32_t count = 0;
    for (const auto& record : provider->setAttributeRecords_) {
        if (record.attribute == attribute) {
            ++count;
        }
    }
    return count;
}

bool FindLastAttributeValues(
    const MockArkUINativeProvider* provider, int32_t attribute, std::vector<ArkUI_NumberValue>& values)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->attribute != attribute || it->values.empty()) {
            continue;
        }
        values = it->values;
        return true;
    }
    return false;
}

class TextFieldComponentProbe : public TextFieldComponent {
public:
    void InvokeOnDataUpdate(const std::string& property, const JsonValue& value)
    {
        OnDataUpdate(property, value);
    }

    void InvokeOnConfigChange(const ThemeContext& context)
    {
        OnConfigChange(context);
    }
};

} // namespace

class TextFieldComponentTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        renderId_ = nextRenderId_++;
        surfaceId_ = "textfield_test_surface_" + std::to_string(renderId_);

        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId_);
        std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
        ASSERT_NE(surfaceManager, nullptr);
        surfaceManager->CreateSurface(surfaceId_);
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(renderId_);
        A2UITest::TearDown();
    }

    void BindThemeContext(const std::shared_ptr<Component>& component) const
    {
        component->SetRenderId(renderId_);
        component->SetSurfaceId(surfaceId_);
    }

    static int32_t nextRenderId_;
    int32_t renderId_ = -1;
    std::string surfaceId_;
};

int32_t TextFieldComponentTest::nextRenderId_ = 5000;

/**
 * @tc.name: TextFieldComponentTest001
 * @tc.desc: Verify constructor applies themed internal node attributes.
 * @tc.type: FUNC
 */
TEST_F(TextFieldComponentTest, TextFieldComponentTest001)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ASSERT_NE(textField, nullptr);

    EXPECT_EQ(textField->GetType(), "TextField");
    EXPECT_GE(CountAttribute(mockArkUIPtr_, NODE_FONT_SIZE), 2);
    EXPECT_GE(CountAttribute(mockArkUIPtr_, NODE_PADDING), 2);

    std::vector<ArkUI_NumberValue> fontWeightValues;
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_FONT_WEIGHT, fontWeightValues));
    ASSERT_FALSE(fontWeightValues.empty());
    EXPECT_EQ(fontWeightValues[0].i32, ARKUI_FONT_WEIGHT_W500);

    std::vector<ArkUI_NumberValue> paddingValues;
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_PADDING, paddingValues));
    ASSERT_EQ(paddingValues.size(), 4U);
    EXPECT_FLOAT_EQ(paddingValues[0].f32, 8.0F);
    EXPECT_FLOAT_EQ(paddingValues[1].f32, 16.0F);
    EXPECT_FLOAT_EQ(paddingValues[2].f32, 0.0F);
    EXPECT_FLOAT_EQ(paddingValues[3].f32, 16.0F);
}

/**
 * @tc.name: TextFieldComponentTest002
 * @tc.desc: Verify variant routes to number/obscured/longText/default branches.
 * @tc.type: FUNC
 */
TEST_F(TextFieldComponentTest, TextFieldComponentTest002)
{
    auto textField = std::make_shared<TextFieldComponentProbe>();
    ASSERT_NE(textField, nullptr);
    auto baseDescriptor = ParseJson(R"({"id":"tf","component":"TextField","value":"123"})");
    ASSERT_NE(baseDescriptor, nullptr);
    textField->ApplyDescriptor(baseDescriptor->GetRoot());

    auto numberDescriptor = ParseJson(R"({"id":"tf","component":"TextField","variant":"number","value":"123"})");
    ASSERT_NE(numberDescriptor, nullptr);
    textField->ApplyDescriptor(numberDescriptor->GetRoot());

    std::vector<ArkUI_NumberValue> inputTypeValues;
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_TEXT_INPUT_TYPE, inputTypeValues));
    ASSERT_FALSE(inputTypeValues.empty());
    EXPECT_EQ(inputTypeValues[0].i32, ARKUI_TEXTINPUT_TYPE_NUMBER);

    auto obscuredDescriptor = ParseJson(R"({"id":"tf","component":"TextField","variant":"obscured","value":"123"})");
    ASSERT_NE(obscuredDescriptor, nullptr);
    textField->ApplyDescriptor(obscuredDescriptor->GetRoot());
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_TEXT_INPUT_TYPE, inputTypeValues));
    EXPECT_EQ(inputTypeValues[0].i32, ARKUI_TEXTINPUT_TYPE_PASSWORD);

    auto defaultDescriptor = ParseJson(R"({"id":"tf","component":"TextField","variant":"unexpected","value":"123"})");
    ASSERT_NE(defaultDescriptor, nullptr);
    textField->ApplyDescriptor(defaultDescriptor->GetRoot());
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_TEXT_INPUT_TYPE, inputTypeValues));
    EXPECT_EQ(inputTypeValues[0].i32, ARKUI_TEXTINPUT_TYPE_NORMAL);

    auto longTextDescriptor = ParseJson(R"({"id":"tf","component":"TextField","variant":"longText","value":"123"})");
    ASSERT_NE(longTextDescriptor, nullptr);
    textField->ApplyDescriptor(longTextDescriptor->GetRoot());

    std::vector<ArkUI_NumberValue> visibilityValues;
    ASSERT_TRUE(FindLastAttributeValues(mockArkUIPtr_, NODE_VISIBILITY, visibilityValues));
    ASSERT_FALSE(visibilityValues.empty());
    EXPECT_EQ(visibilityValues[0].i32, ARKUI_VISIBILITY_VISIBLE);
}

/**
 * @tc.name: TextFieldComponentTest003
 * @tc.desc: Verify theme-based font color application in descriptor/config-change paths.
 * @tc.type: FUNC
 */
TEST_F(TextFieldComponentTest, TextFieldComponentTest003)
{
    auto textField = std::make_shared<TextFieldComponentProbe>();
    ASSERT_NE(textField, nullptr);
    BindThemeContext(textField);

    auto descriptor = ParseJson(R"({"id":"tf","component":"TextField","label":"L","value":"V"})");
    ASSERT_NE(descriptor, nullptr);

    int32_t beforeApply = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    textField->ApplyDescriptor(descriptor->GetRoot());
    int32_t afterApply = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    EXPECT_GE(afterApply, beforeApply + 2);

    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    int32_t beforeConfigChange = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    textField->InvokeOnConfigChange(context);
    int32_t afterConfigChange = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    EXPECT_GE(afterConfigChange, beforeConfigChange + 2);
}
