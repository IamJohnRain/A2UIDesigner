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
#include <map>
#include <memory>
#include <string>

#define private public
#include "components/A2UI/button/ButtonComponent.h"
#undef private
#include "components/A2UI/text/TextComponent.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

class IconProbeComponent : public Component {
public:
    explicit IconProbeComponent(ArkUI_NodeHandle handle) : Component(handle, false) {}
    ~IconProbeComponent() override = default;

    std::string GetType() const override
    {
        return "Icon";
    }

    void OnDataUpdate(const std::string& property, const JsonValue& value) override
    {
        if (property == "color") {
            lastColor_ = value.GetUint32Value(lastColor_);
        }
        if (property == "size") {
            lastSize_ = static_cast<float>(value.GetNumberValue(lastSize_));
        }
        Component::OnDataUpdate(property, value);
    }

    uint32_t GetLastColor() const
    {
        return lastColor_;
    }

    float GetLastSize() const
    {
        return lastSize_;
    }

private:
    uint32_t lastColor_ = 0;
    float lastSize_ = -1.0F;
};

class ButtonComponentProbe : public ButtonComponent {
public:
    void InvokeOnDataUpdate(const std::string& property, const JsonValue& value)
    {
        OnDataUpdate(property, value);
    }
};

std::unique_ptr<JsonAdapter> ParseJson(const std::string& content)
{
    return JsonAdapter::Parse(content);
}

struct ThemeBindingContext {
    int32_t renderId = -1;
    std::string surfaceId;
};

ThemeBindingContext BindComponentThemeContext(const std::shared_ptr<Component>& component)
{
    static int32_t nextRenderId = 3000;
    ThemeBindingContext context;
    context.renderId = nextRenderId++;
    context.surfaceId = "button_test_surface_" + std::to_string(context.renderId);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(context.renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    if (surfaceManager != nullptr) {
        surfaceManager->CreateSurface(context.surfaceId);
    }
    component->SetRenderId(context.renderId);
    component->SetSurfaceId(context.surfaceId);
    return context;
}

void UnbindComponentThemeContext(const ThemeBindingContext& context)
{
    if (context.renderId >= 0) {
        RenderManager::GetInstance().RemoveRenderSlot(context.renderId);
    }
}

bool FindLastAttribute(
    const MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, int32_t attribute, ArkUI_NumberValue& value)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle != nodeHandle || it->attribute != attribute || it->values.empty()) {
            continue;
        }
        value = it->values[0];
        return true;
    }
    return false;
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

} // namespace

class ButtonTest : public A2UITest {
protected:
    std::unique_ptr<ButtonComponent> button_;

    void SetUp() override
    {
        A2UITest::SetUp();
        button_ = std::make_unique<ButtonComponent>();
    }

    void TearDown() override
    {
        button_.reset();
        A2UITest::TearDown();
    }
};

/**
 * @tc.name: ButtonTest001
 * @tc.desc: Verify the following ButtonComponent behavior: return button type.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest001)
{
    /**
     * @tc.steps: step1. Invoke the target ButtonComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    EXPECT_EQ(button_->GetType(), "Button");
}

/**
 * @tc.name: ButtonTest002
 * @tc.desc: Verify the following ButtonComponent behavior: not crash on creation.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest002)
{
    /**
     * @tc.steps: step1. Invoke the target ButtonComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    ASSERT_NE(button_, nullptr);
}

/**
 * @tc.name: ButtonTest003
 * @tc.desc: Verify the following ButtonComponent behavior: handle on data update.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest003)
{
    /**
     * @tc.steps: step1. Invoke the target ButtonComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("someValue");
    ASSERT_NE(value, nullptr);
    Component* component = static_cast<Component*>(button_.get());
    ASSERT_NE(component, nullptr);
    EXPECT_NO_THROW(component->OnDataUpdate("someProperty", value->GetRoot()));
}

/**
 * @tc.name: ButtonTest004
 * @tc.desc: Verify the following ButtonComponent behavior: handle empty property.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest004)
{
    /**
     * @tc.steps: step1. Invoke the target ButtonComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("value");
    ASSERT_NE(value, nullptr);
    Component* component = static_cast<Component*>(button_.get());
    ASSERT_NE(component, nullptr);
    EXPECT_NO_THROW(component->OnDataUpdate("", value->GetRoot()));
}

/**
 * @tc.name: ButtonTest005
 * @tc.desc: Verify the following ButtonComponent behavior: handle empty value.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest005)
{
    /**
     * @tc.steps: step1. Invoke the target ButtonComponent interface with the current test input.
     * @tc.expected: The returned value or exception behavior matches the expectation.
     */

    auto value = JsonAdapter::CreateString("");
    ASSERT_NE(value, nullptr);
    Component* component = static_cast<Component*>(button_.get());
    ASSERT_NE(component, nullptr);
    EXPECT_NO_THROW(component->OnDataUpdate("property", value->GetRoot()));
}

/**
 * @tc.name: ButtonTest006
 * @tc.desc: Verify icon child uses icon-mode size and variant color.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest006)
{
    auto button = std::make_shared<ButtonComponent>();
    auto descriptorDefault = ParseJson(R"({"id":"btn","component":"Button","variant":"default"})");
    ASSERT_NE(descriptorDefault, nullptr);
    button->ApplyDescriptor(descriptorDefault->GetRoot());

    auto iconChild = std::make_shared<IconProbeComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x601));
    button->AddChild(iconChild);

    ArkUI_NumberValue widthValue = {};
    ArkUI_NumberValue heightValue = {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, button->GetNativeView(), NODE_WIDTH, widthValue));
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, button->GetNativeView(), NODE_HEIGHT, heightValue));
    EXPECT_FLOAT_EQ(widthValue.f32, 48.0F);
    EXPECT_FLOAT_EQ(heightValue.f32, 48.0F);
    EXPECT_FLOAT_EQ(iconChild->GetLastSize(), 24.0F);
}

/**
 * @tc.name: ButtonTest007
 * @tc.desc: Verify text child keeps text-mode height.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest007)
{
    auto button = std::make_shared<ButtonComponent>();
    auto descriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"default"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    auto textChild = std::make_shared<TextComponent>();
    auto textDescriptor = ParseJson(R"({"id":"txt","component":"Text","text":"Button Text","variant":"body"})");
    ASSERT_NE(textDescriptor, nullptr);
    textChild->ApplyDescriptor(textDescriptor->GetRoot());
    button->AddChild(textChild);

    ArkUI_NumberValue heightValue = {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, button->GetNativeView(), NODE_HEIGHT, heightValue));
    EXPECT_FLOAT_EQ(heightValue.f32, 40.0F);
}

/**
 * @tc.name: ButtonTest008
 * @tc.desc: Verify icon child color follows variant switch.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest008)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    ThemeBindingContext themeBinding = BindComponentThemeContext(button);
    auto descriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"default"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    auto iconChild = std::make_shared<IconProbeComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x611));
    button->AddChild(iconChild);
    EXPECT_EQ(iconChild->GetLastColor(), 0xE5000000U);

    auto primaryDescriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"primary"})");
    ASSERT_NE(primaryDescriptor, nullptr);
    button->ApplyDescriptor(primaryDescriptor->GetRoot());
    EXPECT_EQ(iconChild->GetLastColor(), 0xFFFFFFFFU);

    auto backgroundValue = ArkUI_NumberValue {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR, backgroundValue));
    EXPECT_EQ(backgroundValue.u32, 0xFF0A59F7U);
    UnbindComponentThemeContext(themeBinding);
}

/**
 * @tc.name: ButtonTest009
 * @tc.desc: Verify clear children path covers non-current and current tracked children.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest009)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    auto descriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"default"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    auto textChild = std::make_shared<TextComponent>();
    auto textDescriptor = ParseJson(R"({"id":"txt","component":"Text","text":"A","variant":"body"})");
    ASSERT_NE(textDescriptor, nullptr);
    textChild->ApplyDescriptor(textDescriptor->GetRoot());

    auto iconChild = std::make_shared<IconProbeComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x612));
    button->AddChild(textChild);
    button->AddChild(iconChild);
    ASSERT_EQ(button->GetChildren().size(), 2U);

    int32_t beforeColorCount = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    button->ClearChildren();
    EXPECT_EQ(button->GetChildren().size(), 0U);

    auto primaryDescriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"primary"})");
    ASSERT_NE(primaryDescriptor, nullptr);
    button->ApplyDescriptor(primaryDescriptor->GetRoot());
    int32_t afterColorCount = CountAttribute(mockArkUIPtr_, NODE_FONT_COLOR);
    EXPECT_EQ(afterColorCount, beforeColorCount);
}

/**
 * @tc.name: ButtonTest010
 * @tc.desc: Verify unknown variant falls back to default background style.
 * @tc.type: FUNC
 */
TEST_F(ButtonTest, ButtonTest010)
{
    auto button = std::make_shared<ButtonComponentProbe>();
    ThemeBindingContext themeBinding = BindComponentThemeContext(button);
    auto descriptor = ParseJson(R"({"id":"btn","component":"Button","variant":"unexpected"})");
    ASSERT_NE(descriptor, nullptr);
    button->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NumberValue backgroundValue = {};
    ASSERT_TRUE(FindLastAttribute(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR, backgroundValue));
    EXPECT_EQ(backgroundValue.u32, 0x0C000000U);
    UnbindComponentThemeContext(themeBinding);
}
