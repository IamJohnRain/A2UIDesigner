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

#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/ExtendedDescriptorNormalizer.h"
#include "components/extended/ExtendedDividerComponent.h"
#include "components/extended/ExtendedProgressComponent.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedRowComponent.h"
#include "components/extended/ExtendedStyleResolver.h"
#include "components/extended/ExtendedTextComponent.h"
#include "components/extended/ExtendedTextInputComponent.h"
#include "components/extended/RenderContext.h"
#include "data/BindingEngine.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedCatalog(std::initializer_list<const char*> componentNames = {})
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* componentName : componentNames) {
        if (componentName == nullptr || componentName[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::string WrapComponents(const std::string& innerJson)
{
    return R"({"components": [)" + innerJson + R"(]})";
}

template<typename TComponent>
ArkUINodeApiAdapter CreateNodeApiAdapter(TComponent& component)
{
    return ArkUINodeApiAdapter([&component]() { return component.GetNativeView(); },
        [&component]() { return component.GetComponentId(); },
        [&component](
            float top, float right, float bottom, float left) { component.SetMargin(top, right, bottom, left); },
        [&component]() { component.ResetCommonMargin(); },
        [&component](const std::function<void()>& onClick) { component.RegisterOnClick(onClick); });
}

void ExpectCustomComponentFallback(
    const SurfaceSlot& slot, const std::string& componentId, const std::string& componentType)
{
    auto component = slot.FindComponentById(componentId);
    ASSERT_NE(component, nullptr);
    EXPECT_EQ(component->GetType(), componentType);
    EXPECT_NE(std::dynamic_pointer_cast<CustomComponent>(component), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<ExtendedComponent>(component), nullptr);
}

} // namespace

class ExtendedCoverageTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-cov");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

// ========== ExtendedDescriptorNormalizer tests (64.71%) ==========

TEST_F(ExtendedCoverageTest, Normalize_InvalidDescriptor)
{
    auto result = ExtendedDescriptorNormalizer::Normalize(JsonValue());
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(result.adapter, nullptr);
}

TEST_F(ExtendedCoverageTest, Normalize_ValidDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"({"id":"root","component":"Text","content":"hello"})");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.descriptor.IsObject());
}

TEST_F(ExtendedCoverageTest, Normalize_DescriptorWithStyles)
{
    auto adapter = JsonAdapter::Parse(
        R"({"id":"root","component":"Text","content":"hello","styles":{"fontSize":16,"fontWeight":"bold"}})");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.styles.IsObject());
}

// ========== ExtendedComponent common method tests (36.59%) ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_InitAndUpdate)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"hello"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Text");

    std::string update = WrapComponents(R"({"id":"root","component":"Text","content":"world"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_WithListeners)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Button","label":"test","listeners":{"click":{"type":"function","call":"navigateTo"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_NonObjectListeners)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","listeners":"invalid"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_InitWithNonExistentType)
{
    slot_.SetCatalog(BuildExtendedCatalog({}));
    std::string msg = WrapComponents(R"({"id":"root","component":"NonExistent"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedRowComponent tests (0%) ==========

TEST_F(ExtendedCoverageTest, Row_DefaultAlignment)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Row");
}

TEST_F(ExtendedCoverageTest, Row_AlignItems)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Row","alignItems":"center","justifyContent":"spaceBetween"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_AlignItemsStart)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Row","alignItems":"start","justifyContent":"start"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_AlignItemsEnd)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","alignItems":"end","justifyContent":"end"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_JustifySpaceAround)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","justifyContent":"spaceAround"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_JustifySpaceEvenly)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","justifyContent":"spaceEvenly"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_InvalidAlignItems)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","alignItems":"invalidVal"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_InvalidJustifyContent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","justifyContent":"invalid"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_Wrap)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","styles":{"wrap":"wrap"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Row_InvalidWrap)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Row" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Row","styles":{"wrap":"invalidWrap"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedButtonComponent tests (0%) ==========

TEST_F(ExtendedCoverageTest, Button_DefaultProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Button");
}

TEST_F(ExtendedCoverageTest, Button_LabelAndEnabled)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","label":"Click Me","enabled":false})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto btn = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(btn->GetLabelForTest(), "Click Me");
    EXPECT_FALSE(btn->GetEnabledForTest());
}

TEST_F(ExtendedCoverageTest, Button_Styles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Button","label":"Test","styles":{"fontSize":20,"fontWeight":"bold","fontColor":"#FF0000"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto btn = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(btn, nullptr);
    EXPECT_FLOAT_EQ(btn->GetFontSizeForTest(), 20.0F);
    EXPECT_EQ(btn->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_TRUE(btn->HasFontColorForTest());
    EXPECT_EQ(btn->GetFontColorForTest(), 0xFFFF0000u);
}

TEST_F(ExtendedCoverageTest, Button_UpdateLabel)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","label":"First"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Button","label":"Updated","enabled":true})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto btn = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(btn, nullptr);
    EXPECT_EQ(btn->GetLabelForTest(), "Updated");
    EXPECT_TRUE(btn->GetEnabledForTest());
}

TEST_F(ExtendedCoverageTest, Button_MaxFontSizeStyle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","styles":{"maxFontSize":24}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto btn = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(btn, nullptr);
    EXPECT_FLOAT_EQ(btn->GetMaxFontSizeForTest(), 24.0F);
}

TEST_F(ExtendedCoverageTest, Button_EmptyLabelWithoutListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto btn = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->HasFontColorForTest());
}

// ========== ExtendedRadioComponent tests (0%) ==========

TEST_F(ExtendedCoverageTest, Radio_DefaultState)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Radio");
}

TEST_F(ExtendedCoverageTest, Radio_Properties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Radio","value":"opt1","checked":true,"group":"group1"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_TRUE(radio->GetCheckedForTest());
    EXPECT_EQ(radio->GetValueForTest(), "opt1");
    EXPECT_EQ(radio->GetGroupForTest(), "group1");
}

TEST_F(ExtendedCoverageTest, Radio_Styles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Radio","styles":{"checkedBackgroundColor":"#FF00FF","uncheckedBackgroundColor":"#00FF00","indicatorColor":"#FFFFFF"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetCheckedBackgroundColorForTest(), 0xFFFF00FFu);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFF00FF00u);
    EXPECT_EQ(radio->GetIndicatorColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExtendedCoverageTest, Radio_UpdateChecked)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"opt1","checked":false})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Radio","checked":true})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_TRUE(radio->GetCheckedForTest());
}

TEST_F(ExtendedCoverageTest, Radio_StyleWithRenamedField)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Radio","styles":{"unCheckedBorderColor":"#AABBCC"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFFAABBCCu);
}

TEST_F(ExtendedCoverageTest, Radio_ValueAndGroup)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"opt2","group":"g2"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetValueForTest(), "opt2");
    EXPECT_EQ(radio->GetGroupForTest(), "g2");
}

// ========== Unsupported extended component fallback tests ==========

TEST_F(ExtendedCoverageTest, Toggle_DefaultState)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_Properties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","isOn":true,"enabled":false})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_Styles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Toggle","styles":{"selectedColor":"#FF0000","unSelectedColor":"#00FF00","switchPointColor":"#0000FF"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_UpdateIsOn)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","isOn":false})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Toggle","isOn":true,"enabled":true})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_StyleAlias)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","styles":{"unselectedColor":"#112233"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

// ========== ExtendedTextInputComponent tests (0%) ==========

TEST_F(ExtendedCoverageTest, TextInput_DefaultState)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "TextInput");
}

TEST_F(ExtendedCoverageTest, TextInput_Properties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"TextInput","text":"hello","placeholder":"enter text","enabled":false,"maxLength":10,"type":"password"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetTextForTest(), "hello");
    EXPECT_EQ(ti->GetPlaceholderForTest(), "enter text");
    EXPECT_FALSE(ti->GetEnabledForTest());
    EXPECT_EQ(ti->GetMaxLengthForTest(), 10);
    EXPECT_EQ(ti->GetInputTypeForTest(), ARKUI_TEXTINPUT_TYPE_PASSWORD);
}

TEST_F(ExtendedCoverageTest, TextInput_TypeEnumEmail)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","type":"email"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetInputTypeForTest(), static_cast<ArkUI_TextInputType>(5));
}

TEST_F(ExtendedCoverageTest, TextInput_TypeNumber)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","type":"number"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetInputTypeForTest(), ARKUI_TEXTINPUT_TYPE_NUMBER);
}

TEST_F(ExtendedCoverageTest, TextInput_TypePhoneNumber)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","type":"phoneNumber"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
}

TEST_F(ExtendedCoverageTest, TextInput_TypeNormal)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","type":"normal"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetInputTypeForTest(), ARKUI_TEXTINPUT_TYPE_NORMAL);
}

TEST_F(ExtendedCoverageTest, TextInput_CancelButton)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"cancelButton":{"style":"input"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_TRUE(ti->HasCancelButtonForTest());
}

TEST_F(ExtendedCoverageTest, TextInput_CancelButtonInvisible)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"cancelButton":{"style":"invisible"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_TRUE(ti->HasCancelButtonForTest());
    EXPECT_EQ(ti->GetCancelButtonStyleForTest(), 1);
}

TEST_F(ExtendedCoverageTest, TextInput_CancelButtonWithIcon)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"TextInput","styles":{"cancelButton":{"style":"constant","fontSize":"20vp","fontColor":"#FF0000","icon":{"src":"icon.png"}}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetCancelButtonStyleForTest(), 0);
    EXPECT_TRUE(ti->HasCancelButtonIconSizeForTest());
    EXPECT_FLOAT_EQ(ti->GetCancelButtonIconSizeForTest(), 20.0F);
    EXPECT_TRUE(ti->HasCancelButtonIconColorForTest());
    EXPECT_EQ(ti->GetCancelButtonIconColorForTest(), 0xFFFF0000u);
    EXPECT_FALSE(ti->HasCancelButtonIconSrcForTest());
}

TEST_F(ExtendedCoverageTest, TextInput_CancelButtonNumberStyle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","styles":{"cancelButton":{"style":0}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetCancelButtonStyleForTest(), 0);
}

TEST_F(ExtendedCoverageTest, TextInput_CancelButtonInvalid)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","styles":{"cancelButton":"invalid"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_FALSE(ti->HasCancelButtonForTest());
}

TEST_F(ExtendedCoverageTest, TextInput_UnderlineColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"TextInput","styles":{"underlineColor":{"typing":"#FF0000","normal":"#00FF00","error":"#0000FF","disable":"#FFFF00"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_TRUE(ti->HasUnderlineColorForTest());
    EXPECT_EQ(ti->GetUnderlineColorTypingForTest(), 0xFFFF0000u);
    EXPECT_EQ(ti->GetUnderlineColorNormalForTest(), 0xFF00FF00u);
    EXPECT_EQ(ti->GetUnderlineColorErrorForTest(), 0xFF0000FFu);
    EXPECT_EQ(ti->GetUnderlineColorDisableForTest(), 0xFFFFFF00u);
}

TEST_F(ExtendedCoverageTest, TextInput_UnderlineColorSingle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","styles":{"underlineColor":"#AABBCC"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_TRUE(ti->HasUnderlineColorForTest());
    EXPECT_EQ(ti->GetUnderlineColorNormalForTest(), 0xFFAABBCCu);
}

TEST_F(ExtendedCoverageTest, TextInput_ShowUnderline)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","styles":{"showUnderline":true}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_TRUE(ti->GetShowUnderlineForTest());
}

TEST_F(ExtendedCoverageTest, TextInput_WordBreak)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","styles":{"wordBreak":"breakAll"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetWordBreakForTest(), 1);
}

TEST_F(ExtendedCoverageTest, TextInput_InlineStyleOnlyForValidMaxLinesOrNonDefaultWordBreak)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string message =
        WrapComponents(R"({"id":"valid-max-lines","component":"TextInput","styles":{"maxLines":1}},)"
                       R"({"id":"non-default-word-break","component":"TextInput","styles":{"wordBreak":"breakAll"}},)"
                       R"({"id":"normal-word-break","component":"TextInput","styles":{"wordBreak":"normal"}},)"
                       R"({"id":"invalid-word-break","component":"TextInput","styles":{"wordBreak":"invalid"}},)"
                       R"({"id":"zero-max-lines","component":"TextInput","styles":{"maxLines":0}},)"
                       R"({"id":"string-max-lines","component":"TextInput","styles":{"maxLines":"2"}},)"
                       R"({"id":"fractional-max-lines","component":"TextInput","styles":{"maxLines":1.5}},)"
                       R"({"id":"invalid-max-lines","component":"TextInput","styles":{"maxLines":-1}},)"
                       R"({"id":"default-style","component":"TextInput","styles":{"fontSize":16}}))");
    auto adapter = JsonAdapter::Parse(message);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    for (const auto& expected : { std::pair<const char*, int32_t> { "valid-max-lines", ARKUI_TEXTINPUT_STYLE_INLINE },
             { "non-default-word-break", ARKUI_TEXTINPUT_STYLE_INLINE },
             { "normal-word-break", ARKUI_TEXTINPUT_STYLE_DEFAULT },
             { "invalid-word-break", ARKUI_TEXTINPUT_STYLE_DEFAULT },
             { "zero-max-lines", ARKUI_TEXTINPUT_STYLE_DEFAULT }, { "string-max-lines", ARKUI_TEXTINPUT_STYLE_DEFAULT },
             { "fractional-max-lines", ARKUI_TEXTINPUT_STYLE_DEFAULT },
             { "invalid-max-lines", ARKUI_TEXTINPUT_STYLE_DEFAULT },
             { "default-style", ARKUI_TEXTINPUT_STYLE_DEFAULT } }) {
        auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById(expected.first));
        ASSERT_NE(input, nullptr);
        int32_t style = -1;
        for (auto iter = mockArkUIPtr_->setAttributeRecords_.rbegin();
             iter != mockArkUIPtr_->setAttributeRecords_.rend(); ++iter) {
            if (iter->nodeHandle == input->GetNativeView() && iter->attribute == NODE_TEXT_INPUT_STYLE) {
                ASSERT_FALSE(iter->values.empty());
                style = iter->values[0].i32;
                break;
            }
        }
        EXPECT_EQ(style, expected.second);
    }
}

TEST_F(ExtendedCoverageTest, TextInput_InlineStylePreservedWhenUnrelatedStyleUpdates)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"maxLines":2}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    auto fontSize = JsonAdapter::Parse("16");
    ASSERT_NE(fontSize, nullptr);
    std::static_pointer_cast<Component>(input)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("fontSize"), fontSize->GetRoot());

    int32_t style = -1;
    for (auto iter = mockArkUIPtr_->setAttributeRecords_.rbegin(); iter != mockArkUIPtr_->setAttributeRecords_.rend();
         ++iter) {
        if (iter->nodeHandle == input->GetNativeView() && iter->attribute == NODE_TEXT_INPUT_STYLE) {
            ASSERT_FALSE(iter->values.empty());
            style = iter->values[0].i32;
            break;
        }
    }
    EXPECT_EQ(style, ARKUI_TEXTINPUT_STYLE_INLINE);
}

TEST_F(ExtendedCoverageTest, TextInput_Styles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"TextInput","styles":{"fontColor":"#FF0000","placeholderColor":"#00FF00","caretColor":"#0000FF","fontWeight":"bold","fontSize":18,"maxFontSize":24,"textAlign":"center"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_EQ(ti->GetTextAlignForTest(), 1);
    EXPECT_FLOAT_EQ(ti->GetMaxFontSizeForTest(), 24.0F);
}

TEST_F(ExtendedCoverageTest, TextInput_UpdateText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","text":"first"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update =
        WrapComponents(R"({"id":"root","component":"TextInput","text":"second","placeholder":"hint"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetTextForTest(), "second");
    EXPECT_EQ(ti->GetPlaceholderForTest(), "hint");
}

TEST_F(ExtendedCoverageTest, TextInput_MaxLengthInvalid)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","maxLength":-5})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetMaxLengthForTest(), std::numeric_limits<int32_t>::max());
}

TEST_F(ExtendedCoverageTest, TextInput_MaxLengthZero)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"TextInput","maxLength":0})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetMaxLengthForTest(), 0);
}

TEST_F(ExtendedCoverageTest, TextInput_SelectedBackgroundColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"selectedBackgroundColor":"#123456"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetSelectedBackgroundColorForTest(), 0xFF123456u);
}

// ========== ExtendedStyleResolver tests (40.68%) ==========

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidFontColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"fontColor":"invalid"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_WidthHeightVP)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"width":100,"height":200}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_WidthHeightPercent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"width":"50%","height":"30%"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_WidthHeightWrapContent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"width":"wrap_content","height":"wrap_content"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidDimensions)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"width":"bad","height":[1,2,3]}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingAll)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"padding":10}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingIndividual)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"top":5,"right":10,"bottom":15,"left":20}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingPercent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"padding":"5%"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingMixedUnits)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"padding":"5%","top":10}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_Margin)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"margin":5}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_MarginIndividual)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"marginTop":1,"marginRight":2,"marginBottom":3,"marginLeft":4}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BorderRadius)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"borderRadius":8}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BorderWidthAndColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"borderWidth":2,"borderColor":"#FF0000"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_VisibilityNone)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"visibility":"none"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_VisibilityHidden)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"visibility":"hidden"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_Opacity)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"opacity":0.5}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ConstraintSize)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"constraintSize":{"minWidth":10,"maxWidth":100,"minHeight":10,"maxHeight":100}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ShadowStyle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"shadow":{"style":1}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_CustomShadow)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"shadow":{"radius":5,"offsetX":2,"offsetY":3,"color":"#FF0000","fill":true,"type":0}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidShadow)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"shadow":"invalid"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImage)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImage":"https://example.com/bg.png"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageEmpty)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImage":""}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageSize)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImageSize":{"width":100,"height":200}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageSizeCover)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImageSize":"cover"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_LinearGradient)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"linearGradient":{"angle":45,"colors":["#FF0000","#00FF00"],"stops":[0,1],"repeating":false,"direction":0}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidLinearGradient)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"linearGradient":"invalid"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidOpacity)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"opacity":"bad"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidVisibility)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"visibility":123}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidBorderWidth)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"borderWidth":"bad"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_LayoutWeight)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"layoutWeight":1}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidLayoutWeight)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"layoutWeight":"bad"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidConstraintSize)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"constraintSize":"bad"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidBorderRadius)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"borderRadius":"bad"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_FlexShrink)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"flexShrink":1.5}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageLowercase)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImage":"https://example.com/bg.png"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageSizeLowercase)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"backgroundimageSize":"cover"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AsyncFontColorBindingUpdate)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"fontColor":{"path":"/textColor"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedDescriptorNormalizer edge cases ==========

TEST_F(ExtendedCoverageTest, Normalize_InvalidJsonValue)
{
    JsonValue invalid;
    auto result = ExtendedDescriptorNormalizer::Normalize(invalid);
    EXPECT_FALSE(result.IsValid());
}

TEST_F(ExtendedCoverageTest, Normalize_NonObjectDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"("stringDescriptor")");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(result.adapter, nullptr);
}

TEST_F(ExtendedCoverageTest, Normalize_ArrayDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(result.adapter, nullptr);
}

TEST_F(ExtendedCoverageTest, Normalize_NumberDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"(42)");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_FALSE(result.IsValid());
    EXPECT_EQ(result.adapter, nullptr);
}

TEST_F(ExtendedCoverageTest, Normalize_EmptyObjectDescriptor)
{
    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);
    auto result = ExtendedDescriptorNormalizer::Normalize(adapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_FALSE(result.styles.IsValid());
}

// ========== ExtendedComponentFactory tests ==========

TEST_F(ExtendedCoverageTest, ComponentFactory_Create_DefaultBranch)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    std::string unknownType = "UnknownWidget99";
    EXPECT_EQ(factory.GetShortName(unknownType), unknownType);
    EXPECT_FALSE(factory.IsExtendedComponent(unknownType));
    EXPECT_EQ(factory.CreateComponent(unknownType), nullptr);
}

TEST_F(ExtendedCoverageTest, ComponentFactory_GetShortName_Empty)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.GetShortName(""), "");
}

TEST_F(ExtendedCoverageTest, ComponentFactory_Register_InvalidInput)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    factory.RegisterComponent("", nullptr);
    EXPECT_FALSE(factory.IsExtendedComponent(""));
}

TEST_F(ExtendedCoverageTest, ComponentFactory_GetShortName_NoSeparator)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.GetShortName("Text"), "Text");
}

TEST_F(ExtendedCoverageTest, ComponentFactory_GetShortName_WithPrefix)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.GetShortName("Extended.Text"), "Text");
}

TEST_F(ExtendedCoverageTest, ComponentFactory_GetShortName_TrailingDot)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.GetShortName("Extended."), "Extended.");
}

TEST_F(ExtendedCoverageTest, ComponentFactory_IsExtended_Unknown)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_FALSE(factory.IsExtendedComponent("NonExistent"));
}

TEST_F(ExtendedCoverageTest, ComponentFactory_IsExtended_Known)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Button"));
}

TEST_F(ExtendedCoverageTest, ComponentFactory_Create_Unknown)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.CreateComponent("NonExistent"), nullptr);
}

TEST_F(ExtendedCoverageTest, ComponentFactory_Create_Empty)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.CreateComponent(""), nullptr);
}

TEST_F(ExtendedCoverageTest, ComponentFactory_Create_Known)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    auto comp = factory.CreateComponent("Button");
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->GetType(), "Button");
}

TEST_F(ExtendedCoverageTest, ComponentFactory_Create_WithPrefix)
{
    auto& factory = ExtendedComponentFactory::GetInstance();
    auto comp = factory.CreateComponent("Extended.Text");
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->GetType(), "Text");
}

// ========== ArkUINodeApiAdapter tests (37.10%) ==========

TEST_F(ExtendedCoverageTest, NodeApplier_GetRootNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    EXPECT_NE(applier.GetRootNode(), nullptr);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetWidthHeight)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetWidth(100.0F);
    applier.SetHeight(200.0F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetWidthHeightPercent)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetWidthPercent(0.5F);
    applier.SetHeightPercent(0.3F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetBackgroundColor)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetBackgroundColor(0xFFFF0000);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetBorderRadius)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetBorderRadius(8.0F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetPadding)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetPadding(1, 2, 3, 4);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetPaddingPercent)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetPaddingPercent(0.1F, 0.2F, 0.3F, 0.4F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetMargin)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetMargin(5, 10, 15, 20);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeFloat)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeFontSize(applier.GetRootNode(), 16.0F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeInt32)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeFontWeight(applier.GetRootNode(), ARKUI_FONT_WEIGHT_W700);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeUint32)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeFontColor(applier.GetRootNode(), 0xFFFF0000);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeBool)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeClip(applier.GetRootNode(), true);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeString)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeButtonLabel(applier.GetRootNode(), "test");
}

TEST_F(ExtendedCoverageTest, NodeApplier_ResetNodeAttribute)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.ResetNodeFontSize(applier.GetRootNode());
}

TEST_F(ExtendedCoverageTest, NodeApplier_RegisterOnClick)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    int callCount = 0;
    applier.RegisterOnClick([&callCount]() { callCount++; });
}

// ========== RenderContext tests ==========

TEST_F(ExtendedCoverageTest, RenderContext_IsValid_Invalid)
{
    RenderContext ctx;
    EXPECT_FALSE(ctx.IsValid());
}

TEST_F(ExtendedCoverageTest, RenderContext_IsValid_Valid)
{
    RenderContext ctx;
    ctx.renderId = 0;
    ctx.surfaceId = "test";
    EXPECT_TRUE(ctx.IsValid());
}

TEST_F(ExtendedCoverageTest, RenderContext_CreateWithBindings)
{
    auto catalog = BuildExtendedCatalog({ "Text" });
    auto bindingEngine = BindingEngine::Create();
    auto ctx = RenderContext::Create(0, "surface-test", bindingEngine, catalog);
    EXPECT_TRUE(ctx.IsValid());
    EXPECT_NE(ctx.dataModel, nullptr);
}

// ========== ExtendedComponent style update delta tests ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_StyleDeltaUpdate)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"initial","styles":{"fontSize":16,"fontColor":"#FF0000"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update =
        WrapComponents(R"({"id":"root","component":"Text","content":"updated","styles":{"fontSize":20}})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_StyleResetViaUpdate)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"fontSize":24,"backgroundColor":"#FF0000"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Text","content":"test"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_UpdateDataModel)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":{"path":"/textContent"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto dataModelAdapter = JsonAdapter::Parse(R"({"value":{"textContent":"bound value"}})");
    ASSERT_NE(dataModelAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModelAdapter->GetRoot()));
}

// ========== More style resolver edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_ObjectFitContain)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"objectFit":"contain"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_HitTestBehaviorWithInvalidString)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"hitTestBehavior":"invalidValue"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_FontSizeInFP)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"fontSize":"16fp"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_MultipleBorders)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"border":{"width":2,"color":"#FF0000","radius":4}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== NodeApplier more mutators ==========

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeInt32WithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeVisibility(nullptr, 1);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeUint32WithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeBackgroundColor(nullptr, 0xFFFF0000);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeBoolWithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeEnabled(nullptr, true);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeStringWithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeButtonLabel(nullptr, "test");
}

// ========== TextInput keyboard type edge cases ==========

TEST_F(ExtendedCoverageTest, TextInput_TypeNumberPassword)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"numberPassword"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(ti, nullptr);
}

TEST_F(ExtendedCoverageTest, TextInput_TypeScreenLockPassword)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"screenLockPassword"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_TypeUserName)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"userName"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_TypeNewPassword)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"newPassword"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_TypeNumberDecimal)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"numberDecimal"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_TypeOneTimeCode)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","type":"oneTimeCode"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== TextInput word break edge cases ==========

TEST_F(ExtendedCoverageTest, TextInput_WordBreakBreakWord)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"wordBreak":"breakWord"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_WordBreakHyphenation)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"wordBreak":"hyphenation"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_WordBreakNormal)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"wordBreak":"normal"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_WordBreakNonString)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"wordBreak":123}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== TextInput enter key type ==========

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeSend)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"send"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeSearch)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"search"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeGo)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"go"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeDone)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"done"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeNext)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"next"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver more edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImagePosition)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImagePosition":{"x":0.5,"y":0.5}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImagePositionCenter)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"backgroundImagePosition":"center"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_FlexGrow)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"flexGrow":1}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_FlexBasis)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"flexBasis":"50%"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AlignSelf)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"alignSelf":"center"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AlignSelfEnd)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"alignSelf":"end"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AlignSelfStretch)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"alignSelf":"stretch"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AlignSelfBaseline)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"alignSelf":"baseline"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_AlignSelfAuto)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"alignSelf":"auto"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ObjectPosition)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"objectFit":"cover"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ObjectFitFill)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"objectFit":"fill"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ObjectFitNone)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"objectFit":"none"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ObjectFitScaleDown)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"objectFit":"scaleDown"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_NoStylesAtAll)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test"})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ArkUINodeApiAdapter more coverage ==========

TEST_F(ExtendedCoverageTest, NodeApplier_SetOpacityViaSetter)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeOpacity(applier.GetRootNode(), 0.5F);
    applier.SetNodeOpacity(applier.GetRootNode(), -0.1F);
    applier.SetNodeOpacity(applier.GetRootNode(), 1.5F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetPaddingZero)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetPadding(0, 0, 0, 0);
}

TEST_F(ExtendedCoverageTest, NodeApplier_MultipleResets)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.ResetNodeFontSize(applier.GetRootNode());
    applier.ResetNodeBackgroundColor(applier.GetRootNode());
    applier.ResetNodeWidth(applier.GetRootNode());
}

// ========== Style binding update triggering OnDataUpdate ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_StyleBindingUpdate)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test","styles":{"fontColor":{"path":"/textColor"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_MultipleStyleBindings)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"fontColor":{"path":"/textColor"},"fontSize":{"path":"/textSize"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Radio edge cases ==========

TEST_F(ExtendedCoverageTest, Radio_WithChangeListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"opt1","checked":true,)"
                                     R"("listeners":{"change":{"type":"function","call":"handleChange"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Radio_NoChecked)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"opt1"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Toggle edge cases ==========

TEST_F(ExtendedCoverageTest, Toggle_WithChangeListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","isOn":true,)"
                                     R"("listeners":{"change":{"type":"function","call":"handleToggle"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_WithSelectedColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","styles":{"selectedColor":"#FF00FF"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

// ========== Button edge cases ==========

TEST_F(ExtendedCoverageTest, Button_WithClickFunctionListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Button","label":"click me",)"
                       R"("listeners":{"click":{"type":"function","call":"handleClick","args":{"id":"btn1"}}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Button_WithClickEventListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","label":"click me",)"
                                     R"("listeners":{"click":{"type":"event","name":"buttonClicked"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Column edge cases ==========

TEST_F(ExtendedCoverageTest, Column_DefaultProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Column" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Column"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Column");
}

TEST_F(ExtendedCoverageTest, Column_AlignItems)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Column" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Column","alignItems":"center"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Column_JustifyContent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Column" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Column","justifyContent":"spaceBetween"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Divider edge cases ==========

TEST_F(ExtendedCoverageTest, Divider_DefaultProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Divider" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Divider"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Divider");
}

TEST_F(ExtendedCoverageTest, Divider_Styles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Divider" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Divider","styles":{"strokeWidth":2,"lineCap":"round","color":"#FF0000"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Progress edge cases ==========

TEST_F(ExtendedCoverageTest, Progress_DefaultProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Progress" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Progress"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    auto root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Progress");
}

TEST_F(ExtendedCoverageTest, Progress_Properties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Progress" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Progress","value":50,"max":100,)"
                                     R"("styles":{"color":"#FF0000","backgroundColor":"#00FF00"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== StyleResolver more edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_MultipleGradients)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test",)"
        R"("styles":{"linearGradient":{"angle":90,"colors":["#FF0000","#00FF00","#0000FF"],"stops":[0,0.5,1],"repeating":true,"direction":1}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ThreeColorGradient)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"linearGradient":{"colors":["#FF0000","#00FF00","#0000FF"],"direction":2}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ShadowOuterDefault)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"shadow":{"style":0}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ShadowStyle2)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"shadow":{"style":2}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ShadowStyle5)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"shadow":{"style":5}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundColor)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"backgroundColor":"#123456"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ZIndex)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"zIndex":10}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_HitTestBehavior)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"hitTestBehavior":"none"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_HitTestBlock)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"hitTestBehavior":"block"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_HitTestDefault)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"hitTestBehavior":"default"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedComponent data model sync ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_UpdateWithStyleBinding)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"fontSize":{"path":"/size"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto dataAdapter = JsonAdapter::Parse(R"({"value":{"size":24}})");
    ASSERT_NE(dataAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataAdapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_UpdateWithMultipleBindings)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                     R"("styles":{"fontColor":{"path":"/color"},"fontWeight":{"path":"/weight"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto dataAdapter = JsonAdapter::Parse(R"({"value":{"color":"#FF0000","weight":700}})");
    ASSERT_NE(dataAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataAdapter->GetRoot()));
}

// ========== StyleResolver margin edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_MarginEmpty)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"margin":0}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingZero)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"padding":0}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedComponent with nested listeners ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_WithActionListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Button","label":"test",)"
        R"("listeners":{"click":{"action":{"functionCall":{"call":"navigateTo","args":{"page":"home"}}}}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_WithEventAndContextListener)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Button","label":"test",)"
                       R"("listeners":{"click":{"type":"event","name":"btnClicked","context":{"pageId":"main"}}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedComponent probe for testing protected internals ==========

class ExtendedComponentProbe : public ExtendedComponent {
public:
    ExtendedComponentProbe() : ExtendedComponent(nullptr, true) {}
    explicit ExtendedComponentProbe(ArkUI_NodeHandle handle) : ExtendedComponent(handle, false) {}

    bool InvokeCreateArkUINode()
    {
        return CreateArkUINode();
    }
    bool InvokeHasEventHandler(const std::string& name) const
    {
        return HasEventHandler(name);
    }
    void InvokeApplyDeclaredPropertyOrFallback(const JsonValue& desc, const std::string& name)
    {
        ApplyDeclaredPropertyOrFallback(desc, name);
    }

    std::string GetType() const override
    {
        return "Probe";
    }
    void ApplyPrivateAttributes(const JsonValue& descriptor) override
    {
        ApplyDeclaredPropertyOrFallback(descriptor, "");
    }
};

// ========== ExtendedComponent listener parsing edge cases ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_ProbeEmptyPropertyName)
{
    ExtendedComponentProbe probe;
    auto adapter = JsonAdapter::Parse(R"({"someKey":"someValue"})");
    ASSERT_NE(adapter, nullptr);
    // Call with empty property name to exercise early return path
    probe.InvokeApplyDeclaredPropertyOrFallback(adapter->GetRoot(), "");
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_ProbeEmptyListenerName)
{
    ExtendedComponentProbe probe;
    EXPECT_FALSE(probe.InvokeHasEventHandler(""));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_ProbeNullNodeApplier)
{
    ExtendedComponentProbe probe;
    // CreateArkUINode should return false since nativeView_ is null
    EXPECT_FALSE(probe.InvokeCreateArkUINode());
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_ParseNonObjectListeners)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test","listeners":["not","an","object"]})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_ApplyDeclaredPropertyEmptyName)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":"test"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, ExtendedComponent_OnDataUpdateRouteToParent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Text","content":{"path":"/textContent"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto dataAdapter = JsonAdapter::Parse(R"({"value":{"textContent":"updated"}})");
    ASSERT_NE(dataAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataAdapter->GetRoot()));
}

// ========== ExtendedStyleResolver ApplyTextComponentStyles coverage ==========

TEST_F(ExtendedCoverageTest, StyleResolver_MinMaxFontSizeOnText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"minFontSize":10,"maxFontSize":20,"textOverflow":"ellipsis"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_TextDecorationOnText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"decoration":{"type":"underline","color":"#FF0000","style":"solid"}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_TextDecorationOnTextInvalid)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"decoration":"invalid"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_FontWeightOnText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"fontWeight":"bold"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_TextAlignOnText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"textAlign":"center"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver ApplyDimension edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_DimensionMatchParent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"width":"match_parent","height":"match_parent"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_DimensionsWrapContentBoth)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"width":"wrap_content","height":"wrap_content"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_DimensionNegativeValue)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"width":-10}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver ApplyRadius individual corners ==========

TEST_F(ExtendedCoverageTest, StyleResolver_RadiusIndividualCorners)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"borderRadius":{"topLeft":5,"topRight":10,"bottomRight":15,"bottomLeft":20}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_RadiusStringValue)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"borderRadius":"8"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver ApplyShadow custom shadow ==========

TEST_F(ExtendedCoverageTest, StyleResolver_CustomShadowFull)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(
        R"({"id":"root","component":"Text","content":"test",)"
        R"("styles":{"shadow":{"radius":10,"offsetX":2,"offsetY":4,"color":"#FF0000","fill":true,"type":0}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver BackgroundImageSize with style ==========

TEST_F(ExtendedCoverageTest, StyleResolver_BgImageSizeCoverWithStyle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"backgroundImageSize":"cover"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_BackgroundImageSizeContain)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"backgroundimageSize":"contain"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ExtendedStyleResolver common node style edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_ClipEnabled)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"clip":true}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_InvalidInputWarnings)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"flexShrink":"bad","layoutWeight":"bad","clip":"bad","opacity":"bad"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== ArkUINodeApiAdapter additional coverage ==========

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeFloatWithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.SetNodeFontSize(nullptr, 16.0F);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetNodeNumberArrayEmpty)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    std::vector<ArkUI_NumberValue> empty;
    applier.SetNodeShadow(applier.GetRootNode(), empty);
}

TEST_F(ExtendedCoverageTest, NodeApplier_ResetNodeAttributeWithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    applier.ResetNodeBackgroundColor(nullptr);
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetLinearGradientWithNullNode)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    StyleLinearGradient gradient;
    gradient.colors.push_back(0xFFFF0000);
    gradient.stops.push_back(0.0F);
    applier.SetNodeLinearGradient(nullptr, gradient.angle, gradient.direction, gradient.repeating,
        gradient.colors.data(), static_cast<int32_t>(gradient.colors.size()), gradient.stops.data(),
        static_cast<int32_t>(gradient.stops.size()));
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetLinearGradientColorsStopsMismatch)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    StyleLinearGradient gradient;
    gradient.colors.push_back(0xFFFF0000U);
    gradient.colors.push_back(0xFF00FF00U);
    gradient.stops.push_back(0.0F);
    applier.SetNodeLinearGradient(applier.GetRootNode(), gradient.angle, gradient.direction, gradient.repeating,
        gradient.colors.data(), static_cast<int32_t>(gradient.colors.size()), gradient.stops.data(),
        static_cast<int32_t>(gradient.stops.size()));
}

TEST_F(ExtendedCoverageTest, NodeApplier_SetLinearGradientValid)
{
    auto comp = std::make_shared<ExtendedTextComponent>();
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*comp);
    StyleLinearGradient gradient;
    gradient.angle = 45.0F;
    gradient.direction = 0;
    gradient.repeating = false;
    gradient.colors.push_back(0xFFFF0000U);
    gradient.colors.push_back(0xFF00FF00U);
    gradient.stops.push_back(0.0F);
    gradient.stops.push_back(1.0F);
    applier.SetNodeLinearGradient(applier.GetRootNode(), gradient.angle, gradient.direction, gradient.repeating,
        gradient.colors.data(), static_cast<int32_t>(gradient.colors.size()), gradient.stops.data(),
        static_cast<int32_t>(gradient.stops.size()));
}

// ========== ExtendedComponent DispatchEvent unsupported ==========

TEST_F(ExtendedCoverageTest, ExtendedComponent_DispatchEventEventWithContext)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","label":"test"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Radio component extra edge cases ==========

TEST_F(ExtendedCoverageTest, Radio_UpdateChangeEventRegistration)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"opt1","checked":false,)"
                                     R"("listeners":{"change":{"type":"function","call":"onChange"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Radio_StyleColorRenamedField)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Radio","styles":{"unCheckedBorderColor":"#AABBCC"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Radio_ApplyStyleWithNonObjectStyles)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Radio","styles":{"checkedBackgroundColor":"#FF00FF"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Toggle component extra edge cases ==========

TEST_F(ExtendedCoverageTest, Toggle_UpdateChangeEventRegistration)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","isOn":true,)"
                                     R"("listeners":{"change":{"type":"function","call":"onToggle"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_ColorAlias)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","styles":{"unselectedColor":"#112233"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_StyleColorWithInvalidInput)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","styles":{"selectedColor":"invalid"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

TEST_F(ExtendedCoverageTest, Toggle_EnabledTrue)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle","isOn":false,"enabled":true})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
    ExpectCustomComponentFallback(slot_, "root", "Toggle");
}

// ========== StyleConstraintSize edge cases ==========

TEST_F(ExtendedCoverageTest, StyleResolver_ConstraintSizePartial)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"constraintSize":{"minWidth":10,"maxWidth":100}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_ConstraintSizePercent)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                          R"("styles":{"constraintSize":{"minWidth":"10%","maxWidth":"100%"}}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== Padding Mixed Units validation coverage ==========

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingMixedUnitsValidation)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                                                     R"("styles":{"padding":"5%","top":10}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, StyleResolver_PaddingPercentAll)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"Text","content":"test","styles":{"padding":"10%"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// ========== TextInput enter key type and extra edge cases ==========

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeEdit)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter = JsonAdapter::Parse(
        WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"editor"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, TextInput_EnterKeyTypeSign)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto adapter =
        JsonAdapter::Parse(WrapComponents(R"({"id":"root","component":"TextInput","styles":{"enterKeyType":"sign"}})"));
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));
}

// Test all Reset paths for style properties
TEST_F(ExtendedCoverageTest, StyleResolver_ResetProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"fontSize":24,"backgroundColor":"#FF0000","borderWidth":2,"borderColor":"#00FF00",)"
                       R"("width":100,"height":200,"padding":10,"margin":5,"borderRadius":8,)"
                       R"("fontWeight":"bold","textAlign":"center","maxLines":3,"textOverflow":"ellipsis",)"
                       R"("visibility":"visible","opacity":0.8,"layoutWeight":1,"flexShrink":0.5,)"
                       R"("constraintSize":{"minWidth":10,"maxWidth":200,"minHeight":10,"maxHeight":200},)"
                       R"("fontColor":"#FF0000","fontSize":16}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Text","content":"test"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
}

// Test additional style property resets that hit different branches
TEST_F(ExtendedCoverageTest, StyleResolver_ResetFontProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Text","content":"test",)"
        R"("styles":{"minFontSize":10,"maxFontSize":20,"wordBreak":"breakAll","decoration":{"type":"underline"}}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Text","content":"test"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
}

// Test reset for shadow, background image, linear gradient, clip
TEST_F(ExtendedCoverageTest, StyleResolver_ResetVisualProperties)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Text","content":"test",)"
                       R"("styles":{"shadow":{"style":1},"backgroundImage":"https://example.com/bg.png",)"
                       R"("linearGradient":{"colors":["#FF0000","#00FF00"],"stops":[0,1]},"clip":true}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    std::string update = WrapComponents(R"({"id":"root","component":"Text","content":"test"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));
}

TEST_F(ExtendedCoverageTest, Button_OnPropertyRemoved_Label)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(
        R"({"id":"root","component":"Button","label":"hello","enabled":false,"action":{"call":"testFn"}})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto update = WrapComponents(R"({"id":"root","component":"Button"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetLabelForTest(), "");
    EXPECT_TRUE(button->GetEnabledForTest());
}

TEST_F(ExtendedCoverageTest, Button_ApplyPrivateAttributes_TextAliasForLabel)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","text":"from_text"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetLabelForTest(), "from_text");
}

TEST_F(ExtendedCoverageTest, Button_ApplyPrivateAttributes_LabelOverridesText)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Button","text":"from_text","label":"from_label"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetLabelForTest(), "from_label");
}

TEST_F(ExtendedCoverageTest, Toggle_OnPropertyRemoved)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"Toggle","label":"switch","isOn":true,"enabled":false})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto update = WrapComponents(R"({"id":"root","component":"Toggle"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(toggle, nullptr);
    EXPECT_FALSE(toggle->GetIsOnForTest());
    EXPECT_TRUE(toggle->GetEnabledForTest());
    EXPECT_EQ(toggle->GetLabelForTest(), "");
}

TEST_F(ExtendedCoverageTest, Radio_OnPropertyRemoved)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Radio","value":"v1","group":"g1","checked":true})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_TRUE(radio->GetCheckedForTest());

    auto update = WrapComponents(R"({"id":"root","component":"Radio"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    EXPECT_FALSE(radio->GetCheckedForTest());
    EXPECT_EQ(radio->GetValueForTest(), "");
    EXPECT_EQ(radio->GetGroupForTest(), "");
}

TEST_F(ExtendedCoverageTest, TextInput_OnPropertyRemoved)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    std::string msg =
        WrapComponents(R"({"id":"root","component":"TextInput","text":"hello","placeholder":"hint","enabled":false,)"
                       R"("maxLength":10,"type":"password"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto update = WrapComponents(R"({"id":"root","component":"TextInput"})");
    auto updateAdapter = JsonAdapter::Parse(update);
    ASSERT_NE(updateAdapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(updateAdapter->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetTextForTest(), "");
    EXPECT_EQ(input->GetPlaceholderForTest(), "");
    EXPECT_TRUE(input->GetEnabledForTest());
    EXPECT_EQ(input->GetMaxLengthForTest(), std::numeric_limits<int32_t>::max());
}

TEST_F(ExtendedCoverageTest, Toggle_DefaultColorsWhenNoStyle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    std::string msg = WrapComponents(R"({"id":"root","component":"Toggle"})");
    auto adapter = JsonAdapter::Parse(msg);
    ASSERT_NE(adapter, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(adapter->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(toggle, nullptr);
    EXPECT_EQ(toggle->GetSelectedColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(toggle->GetUnSelectedColorForTest(), 0x19000000u);
    EXPECT_EQ(toggle->GetSwitchPointColorForTest(), 0xFFFFFFFFu);
}
