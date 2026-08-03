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
#include <initializer_list>
#include <memory>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "components/extended/ExtendedTextInputComponent.h"
#include "components/extended/RenderContext.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog(std::initializer_list<const char*> componentNames = {})
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

std::string GetRequestStringProperty(const MockNapiProvider* mockNapi, napi_value object, const char* key)
{
    auto objectIt = mockNapi->objectProperties_.find(object);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return "";
    }
    auto valueIt = objectIt->second.find(key);
    if (valueIt == objectIt->second.end()) {
        return "";
    }
    auto stringIt = mockNapi->stringValues_.find(valueIt->second);
    return stringIt != mockNapi->stringValues_.end() ? stringIt->second : "";
}

struct CrossLanguageAttributeBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref callbackRef_ = nullptr;
};

void ResetCrossLanguageAttributeBridge()
{
    auto* bridge = reinterpret_cast<CrossLanguageAttributeBridgeMirror*>(&CrossLanguageAttributeBridge::GetInstance());
    bridge->napiEnv_ = nullptr;
    bridge->callbackRef_ = nullptr;
}

} // namespace

class FontScaleModeButtonTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-1");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

class ButtonStyleModeBridgeTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        ResetCrossLanguageAttributeBridge();
        slot_.SetSurfaceId("surface-button-style-mode");
        slot_.SetRenderId(1);
        slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    }

    void TearDown() override
    {
        ResetCrossLanguageAttributeBridge();
        A2UITest::TearDown();
    }

    void RegisterBridgeCallback()
    {
        napi_value callback = nullptr;
        ASSERT_EQ(mockNapiPtr_->CreateFunction(env_, "bridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
        CrossLanguageAttributeBridge::GetInstance().RegisterCrossLanguageCallback(env_, callback);
    }

    bool WasAttributeDispatched(const std::string& attributeName) const
    {
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            if (GetRequestStringProperty(mockNapiPtr_, args.front(), "attributeName") == attributeName) {
                return true;
            }
        }
        return false;
    }

    napi_env env_ = reinterpret_cast<napi_env>(0x102);
    SurfaceSlot slot_;
};

class FontScaleModeTextTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-1");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

class FontScaleModeTextInputTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-1");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

TEST_F(ButtonStyleModeBridgeTest, should_not_dispatch_button_style_when_border_radius_is_not_declared)
{
    RegisterBridgeCallback();

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "backgroundColor": "#00AAFF" }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());
}

TEST_F(ButtonStyleModeBridgeTest, should_not_dispatch_button_style_when_border_radius_is_declared)
{
    RegisterBridgeCallback();

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "borderRadius": 24, "backgroundColor": "#00AAFF" }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 1U);
    EXPECT_TRUE(WasAttributeDispatched("borderRadius"));
    EXPECT_FALSE(WasAttributeDispatched("styleMode"));
}

TEST_F(FontScaleModeButtonTest, should_default_to_followSystem_when_no_fontScaleMode)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 16.0F);
}

TEST_F(FontScaleModeButtonTest, should_store_custom_mode_when_fontScaleMode_is_custom)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 20
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 20.0F);
}

TEST_F(FontScaleModeButtonTest, should_apply_minFontSize_and_maxFontSize_when_styles_set)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontSize": 12,
                "maxFontSize": 24
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontSizeForTest(), 12.0F);
    EXPECT_FLOAT_EQ(button->GetMaxFontSizeForTest(), 24.0F);
}

TEST_F(FontScaleModeButtonTest, should_store_followSystem_mode_when_fontScaleMode_is_followSystem)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "followSystem",
                "fontSize": 18
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 18.0F);
}

TEST_F(FontScaleModeButtonTest, should_update_mode_when_fontScaleMode_changes_on_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "followSystem"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "followSystem");

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "styles": {
                "fontScaleMode": "custom"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "custom");
}

TEST_F(FontScaleModeButtonTest, should_preserve_fontSize_when_mode_changes_with_scale)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetFontSizeScale(1.5F);

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 21
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 21.0F);
}

TEST_F(FontScaleModeButtonTest, should_reset_only_invalid_delta_style_and_preserve_missing_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontSize": 24,
                "fontWeight": "bold",
                "minFontSize": 12,
                "maxFontSize": 30,
                "fontScaleMode": "custom",
                "minFontScale": 0.6,
                "maxFontScale": 1.8,
                "fontColor": "#FF112233"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 24.0F);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "custom");
    EXPECT_EQ(button->GetFontColorForTest(), 0xFF112233u);

    std::unique_ptr<JsonAdapter> invalidFontSize = JsonAdapter::Parse(R"("invalid")");
    ASSERT_NE(invalidFontSize, nullptr);
    std::static_pointer_cast<Component>(button)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("fontSize"), invalidFontSize->GetRoot());

    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(button->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_FLOAT_EQ(button->GetMinFontSizeForTest(), 12.0F);
    EXPECT_FLOAT_EQ(button->GetMaxFontSizeForTest(), 30.0F);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.6F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 1.8F);
    EXPECT_EQ(button->GetFontColorForTest(), 0xFF112233u);
    EXPECT_TRUE(button->HasFontColorForTest());
}

TEST_F(FontScaleModeTextTest, should_default_to_followSystem_when_no_fontScaleMode)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextTest, should_store_custom_mode_when_fontScaleMode_is_custom)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 20
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 20.0F);
}

TEST_F(FontScaleModeTextTest, should_store_followSystem_mode_when_fontScaleMode_is_followSystem)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "followSystem",
                "fontSize": 18
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 18.0F);
}

TEST_F(FontScaleModeTextTest, should_update_mode_when_fontScaleMode_changes_on_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "followSystem"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "followSystem");

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "custom");
}

TEST_F(FontScaleModeTextTest, should_fallback_to_followSystem_when_fontScaleMode_is_invalid)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "unsupported"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextTest, should_clamp_out_of_range_min_and_max_font_scale_values)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "minFontScale": 1.8,
                "maxFontScale": 0.5
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetMinFontScaleForTest(), 1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontScaleForTest(), 1.0F);
}

TEST_F(FontScaleModeTextInputTest, should_default_to_followSystem_when_no_fontScaleMode)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextInputTest, should_store_custom_mode_when_fontScaleMode_is_custom)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "custom"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "custom");
}

TEST_F(FontScaleModeTextInputTest, should_apply_minFontSize_and_maxFontSize_when_styles_set)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "minFontSize": 10,
                "maxFontSize": 28
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_FLOAT_EQ(input->GetMinFontSizeForTest(), 10.0F);
    EXPECT_FLOAT_EQ(input->GetMaxFontSizeForTest(), 28.0F);
}

TEST_F(FontScaleModeTextInputTest, should_store_followSystem_mode_when_fontScaleMode_is_followSystem)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "followSystem"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextInputTest, should_update_mode_when_fontScaleMode_changes_on_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "followSystem"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "followSystem");

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "custom"
            }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "custom");
}

TEST_F(FontScaleModeTextInputTest, should_reset_only_invalid_delta_style_and_preserve_missing_styles)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "caretColor": "#FF112233",
                "selectedBackgroundColor": "#44112233",
                "cancelButton": {
                    "style": "constant",
                    "fontSize": 18,
                    "fontColor": "#FF445566",
                    "icon": { "src": "legacy.png" }
                },
                "fontWeight": "bold",
                "textAlign": "center",
                "minFontSize": 10,
                "maxFontSize": 28,
                "fontScaleMode": "custom",
                "minFontScale": 0.6,
                "maxFontScale": 1.8,
                "showUnderline": true,
                "underlineColor": {
                    "typing": "#FF010203",
                    "normal": "#FF040506",
                    "error": "#FF070809",
                    "disable": "#FF0A0B0C"
                },
                "wordBreak": "breakWord"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetCaretColorForTest(), 0xFF112233u);
    EXPECT_TRUE(input->HasCancelButtonForTest());
    EXPECT_TRUE(input->HasCancelButtonIconSizeForTest());
    EXPECT_FLOAT_EQ(input->GetCancelButtonIconSizeForTest(), 18.0F);
    EXPECT_TRUE(input->HasCancelButtonIconColorForTest());
    EXPECT_EQ(input->GetCancelButtonIconColorForTest(), 0xFF445566u);
    EXPECT_FALSE(input->HasCancelButtonIconSrcForTest());
    EXPECT_EQ(input->GetFontScaleModeForTest(), "custom");

    std::unique_ptr<JsonAdapter> invalidCaretColor = JsonAdapter::Parse("true");
    ASSERT_NE(invalidCaretColor, nullptr);
    std::static_pointer_cast<Component>(input)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("caretColor"), invalidCaretColor->GetRoot());

    EXPECT_EQ(input->GetCaretColorForTest(), 0xFF007DFFu);
    EXPECT_EQ(input->GetSelectedBackgroundColorForTest(), 0x44112233u);
    EXPECT_TRUE(input->HasCancelButtonForTest());
    EXPECT_EQ(input->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_EQ(input->GetTextAlignForTest(), 1);
    EXPECT_FLOAT_EQ(input->GetMinFontSizeForTest(), 10.0F);
    EXPECT_FLOAT_EQ(input->GetMaxFontSizeForTest(), 28.0F);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(input->GetMinFontScaleForTest(), 0.6F);
    EXPECT_FLOAT_EQ(input->GetMaxFontScaleForTest(), 1.8F);
    EXPECT_TRUE(input->GetShowUnderlineForTest());
    EXPECT_EQ(input->GetWordBreakForTest(), 2);
    EXPECT_TRUE(input->HasUnderlineColorForTest());
}

class FontScaleModeComputeTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-1");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

TEST_F(FontScaleModeComputeTest, should_return_baseFontSize_when_followSystem_mode)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetFontSizeScale(1.5F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "followSystem",
                "fontSize": 21
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 21.0F);
    EXPECT_FLOAT_EQ(button->ComputeEffectiveFontSizeForTest(21.0F), 21.0F);
}

TEST_F(FontScaleModeComputeTest, should_return_vpFontSize_times_scale_when_custom_mode_button)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetFontSizeScale(1.5F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 21
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 21.0F);
    EXPECT_FLOAT_EQ(button->ComputeEffectiveFontSizeForTest(21.0F), 21.0F * 1.5F);
}

TEST_F(FontScaleModeComputeTest, should_return_vpFontSize_times_scale_when_custom_mode_text)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    slot_.SetFontSizeScale(2.0F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 32
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 32.0F);
    EXPECT_FLOAT_EQ(text->ComputeEffectiveFontSizeForTest(32.0F), 32.0F * 2.0F);
}

TEST_F(FontScaleModeComputeTest, should_clamp_to_minFontScale_when_custom_mode_text)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    slot_.SetFontSizeScale(0.5F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 32,
                "minFontScale": 0.9
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 32.0F);
    EXPECT_FLOAT_EQ(text->ComputeEffectiveFontSizeForTest(32.0F), 32.0F * 0.9F);
}

TEST_F(FontScaleModeComputeTest, should_clamp_to_maxFontScale_when_custom_mode_text)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    slot_.SetFontSizeScale(2.2F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Text",
            "content": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 32,
                "maxFontScale": 1.15
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 32.0F);
    EXPECT_FLOAT_EQ(text->ComputeEffectiveFontSizeForTest(32.0F), 32.0F * 1.15F);
}

TEST_F(FontScaleModeComputeTest, should_return_vpFontSize_times_scale_when_custom_mode_textinput)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    slot_.SetFontSizeScale(1.5F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "custom",
                "fontSize": 30
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "custom");
    EXPECT_FLOAT_EQ(input->ComputeEffectiveFontSizeForTest(30.0F), 30.0F * 1.5F);
}

TEST_F(FontScaleModeComputeTest, should_return_base_when_no_fontScaleMode_regardless_of_scale)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetFontSizeScale(2.0F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "fontSize": 20 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 20.0F);
    EXPECT_FLOAT_EQ(button->ComputeEffectiveFontSizeForTest(20.0F), 20.0F);
}

TEST_F(FontScaleModeButtonTest, should_reset_minFontSize_to_zero_when_invalid)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontSize": "invalid"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontSizeForTest(), 0.0F);
}

TEST_F(FontScaleModeButtonTest, should_reset_maxFontSize_to_zero_when_invalid)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "maxFontSize": false
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMaxFontSizeForTest(), 0.0F);
}

TEST_F(FontScaleModeButtonTest, should_reset_minFontScale_when_non_number)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontScale": "auto"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.0F);
}

TEST_F(FontScaleModeButtonTest, should_reset_maxFontScale_when_non_number)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "maxFontScale": false
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 0.0F);
}

TEST_F(FontScaleModeButtonTest, should_use_default_fontScaleMode_when_non_string)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": 42
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextInputTest, should_reset_minFontScale_when_non_number_2)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "minFontScale": "auto"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_FLOAT_EQ(input->GetMinFontScaleForTest(), 0.0F);
}

TEST_F(FontScaleModeTextInputTest, should_reset_maxFontScale_when_non_number_2)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "maxFontScale": false
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_FLOAT_EQ(input->GetMaxFontScaleForTest(), 0.0F);
}

TEST_F(FontScaleModeTextInputTest, should_use_default_fontScaleMode_when_non_string_2)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": 42
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeTextInputTest, should_use_default_fontScaleMode_when_invalid_string)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "autoScale"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetFontScaleModeForTest(), "followSystem");
}

TEST_F(FontScaleModeComputeTest, should_use_scale_1_when_fontSizeScale_is_zero_in_custom_mode)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetFontSizeScale(0.0F);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontSize": 20,
                "fontScaleMode": "custom"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->ComputeEffectiveFontSizeForTest(20.0F), 20.0F);
}
