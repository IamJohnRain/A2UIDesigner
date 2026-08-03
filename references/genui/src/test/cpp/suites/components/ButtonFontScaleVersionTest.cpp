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
#include "components/extended/RenderContext.h"
#include "functions/CrossLanguageAttributeBridge.h"
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

double GetRequestNumberProperty(const MockNapiProvider* mockNapi, napi_value object, const char* key)
{
    auto objectIt = mockNapi->objectProperties_.find(object);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return 0.0;
    }
    auto valueIt = objectIt->second.find(key);
    if (valueIt == objectIt->second.end()) {
        return 0.0;
    }
    auto numberIt = mockNapi->numberValues_.find(valueIt->second);
    return numberIt != mockNapi->numberValues_.end() ? numberIt->second : 0.0;
}

bool GetRequestBoolProperty(const MockNapiProvider* mockNapi, napi_value object, const char* key)
{
    auto objectIt = mockNapi->objectProperties_.find(object);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return false;
    }
    auto valueIt = objectIt->second.find(key);
    if (valueIt == objectIt->second.end()) {
        return false;
    }
    auto boolIt = mockNapi->boolValues_.find(valueIt->second);
    return boolIt != mockNapi->boolValues_.end() ? boolIt->second : false;
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

class ButtonFontScaleVersionTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-1");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

class TextInputCrossLanguageBridgeTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-cross-language");
        slot_.SetRenderId(1);
        slot_.SetApiVersion(18);
        slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    }

    void RegisterBridgeCallback()
    {
        napi_value callback = nullptr;
        ASSERT_EQ(mockNapiPtr_->CreateFunction(env_, "bridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
        CrossLanguageAttributeBridge::GetInstance().RegisterCrossLanguageCallback(env_, callback);
    }

    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    SurfaceSlot slot_;
};

class ButtonCrossLanguageBridgeTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-button-cross-language");
        slot_.SetRenderId(1);
        slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    }

    void RegisterBridgeCallback()
    {
        napi_value callback = nullptr;
        ASSERT_EQ(mockNapiPtr_->CreateFunction(env_, "bridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
        CrossLanguageAttributeBridge::GetInstance().RegisterCrossLanguageCallback(env_, callback);
    }

    napi_env env_ = reinterpret_cast<napi_env>(0x102);
    SurfaceSlot slot_;
};

class CrossLanguageAttributeBridgeCoverageTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        ResetCrossLanguageAttributeBridge();
    }

    void TearDown() override
    {
        ResetCrossLanguageAttributeBridge();
        A2UITest::TearDown();
    }

    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        EXPECT_EQ(mockNapiPtr_->CreateFunction(env_, "bridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
        return callback;
    }

    napi_env env_ = reinterpret_cast<napi_env>(0x220);
};

TEST_F(ButtonFontScaleVersionTest, should_store_minFontScale_when_api_version_ge_18)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(18);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.5, "maxFontScale": 2.0 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.5F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 2.0F);
}

TEST_F(ButtonFontScaleVersionTest, should_store_minFontScale_when_api_version_gt_18)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(20);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.8, "maxFontScale": 1.5 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.8F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 1.5F);
}

TEST_F(ButtonFontScaleVersionTest, should_store_zero_when_api_version_lt_18)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(17);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.5, "maxFontScale": 2.0 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.5F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 2.0F);
}

TEST_F(ButtonFontScaleVersionTest, should_store_zero_when_no_api_version_set)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.5, "maxFontScale": 2.0 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.5F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 2.0F);
}

TEST_F(ButtonFontScaleVersionTest, should_clamp_out_of_range_font_scale_values)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(18);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 1.8, "maxFontScale": 0.5 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 1.0F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 1.0F);
}

TEST_F(ButtonFontScaleVersionTest, should_reset_to_zero_when_minFontScale_removed)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(18);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.5, "maxFontScale": 2.0 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.5F);

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok"
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(button->GetMaxFontScaleForTest(), 0.0F);
}

TEST_F(ButtonFontScaleVersionTest, should_update_minFontScale_on_delta_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    slot_.SetApiVersion(18);
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": "ok",
            "styles": { "minFontScale": 0.5 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.5F);

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "styles": { "minFontScale": 0.8 }
        }]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetMinFontScaleForTest(), 0.8F);
}

TEST_F(TextInputCrossLanguageBridgeTest, should_dispatch_render_scoped_request_when_textinput_font_scale_is_applied)
{
    RegisterBridgeCallback();

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": { "minFontScale": 0.8 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    ASSERT_GE(mockNapiPtr_->callFunctionArgsHistory_.size(), 1u);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[0].empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_[0][0];
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, request, "renderId"), 1.0);
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, request, "componentId"), "root");
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, request, "nodeUniqueId"), 1.0);
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, request, "componentType"), "TextInput");
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, request, "attributeName"), "minFontScale");
    EXPECT_NEAR(GetRequestNumberProperty(mockNapiPtr_, request, "floatValue"), 0.8, 1e-6);
    EXPECT_FALSE(GetRequestBoolProperty(mockNapiPtr_, request, "reset"));
}

TEST_F(TextInputCrossLanguageBridgeTest, should_dispatch_reset_request_when_textinput_font_scale_style_is_removed)
{
    RegisterBridgeCallback();

    std::unique_ptr<JsonAdapter> initial = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": { "minFontScale": 0.8, "maxFontScale": 1.4 }
        }]
    })");
    ASSERT_NE(initial, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initial->GetRoot()));

    std::unique_ptr<JsonAdapter> reset = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": {}
        }]
    })");
    ASSERT_NE(reset, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(reset->GetRoot()));

    ASSERT_GE(mockNapiPtr_->callFunctionArgsHistory_.size(), 4u);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[2].empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[3].empty());

    napi_value minResetRequest = mockNapiPtr_->callFunctionArgsHistory_[2][0];
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, minResetRequest, "attributeName"), "minFontScale");
    EXPECT_DOUBLE_EQ(GetRequestNumberProperty(mockNapiPtr_, minResetRequest, "floatValue"), 0.0);
    EXPECT_TRUE(GetRequestBoolProperty(mockNapiPtr_, minResetRequest, "reset"));

    napi_value maxResetRequest = mockNapiPtr_->callFunctionArgsHistory_[3][0];
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, maxResetRequest, "attributeName"), "maxFontScale");
    EXPECT_DOUBLE_EQ(GetRequestNumberProperty(mockNapiPtr_, maxResetRequest, "floatValue"), 0.0);
    EXPECT_TRUE(GetRequestBoolProperty(mockNapiPtr_, maxResetRequest, "reset"));
}

TEST_F(TextInputCrossLanguageBridgeTest, should_clamp_out_of_range_textinput_font_scale_values_before_dispatch)
{
    RegisterBridgeCallback();

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "TextInput",
            "text": "hello",
            "styles": { "minFontScale": 1.8, "maxFontScale": 0.5 }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    ASSERT_GE(mockNapiPtr_->callFunctionArgsHistory_.size(), 2u);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[0].empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[1].empty());

    napi_value minRequest = mockNapiPtr_->callFunctionArgsHistory_[0][0];
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, minRequest, "attributeName"), "minFontScale");
    EXPECT_DOUBLE_EQ(GetRequestNumberProperty(mockNapiPtr_, minRequest, "floatValue"), 1.0);
    EXPECT_FALSE(GetRequestBoolProperty(mockNapiPtr_, minRequest, "reset"));

    napi_value maxRequest = mockNapiPtr_->callFunctionArgsHistory_[1][0];
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, maxRequest, "attributeName"), "maxFontScale");
    EXPECT_DOUBLE_EQ(GetRequestNumberProperty(mockNapiPtr_, maxRequest, "floatValue"), 1.0);
    EXPECT_FALSE(GetRequestBoolProperty(mockNapiPtr_, maxRequest, "reset"));
}

TEST_F(CrossLanguageAttributeBridgeCoverageTest, should_cover_registration_and_dispatch_branches)
{
    auto& bridge = CrossLanguageAttributeBridge::GetInstance();
    CrossLanguageAttributeRequest request = { .renderId = 7,
        .componentId = "root",
        .nodeUniqueId = 42,
        .componentType = "TextInput",
        .attributeName = "minFontScale",
        .floatValue = 1.25F,
        .reset = true };

    bridge.RegisterCrossLanguageCallback(nullptr, nullptr);
    bridge.RegisterCrossLanguageCallback(env_, nullptr);
    EXPECT_FALSE(bridge.Dispatch(request));

    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    bridge.RegisterCrossLanguageCallback(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_FALSE(bridge.Dispatch(request));

    bridge.RegisterCrossLanguageCallback(env_, CreateCallback());
    ASSERT_FALSE(mockNapiPtr_->refToValue_.empty());
    napi_ref oldRef = mockNapiPtr_->refToValue_.begin()->first;
    bridge.RegisterCrossLanguageCallback(env_, CreateCallback());
    EXPECT_EQ(mockNapiPtr_->refToValue_.count(oldRef), 0u);

    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(request));
    mockNapiPtr_->ResetGetReferenceValueStatus();

    mockNapiPtr_->refToValue_.clear();
    EXPECT_FALSE(bridge.Dispatch(request));

    bridge.RegisterCrossLanguageCallback(env_, CreateCallback());
    mockNapiPtr_->SetCreateObjectStatus(napi_generic_failure);
    EXPECT_FALSE(bridge.Dispatch(request));
    mockNapiPtr_->ResetCreateObjectStatus();

    mockNapiPtr_->nextValueId_ = 0;
    EXPECT_FALSE(bridge.Dispatch(request));
    mockNapiPtr_->nextValueId_ = 100;

    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(request));
    mockNapiPtr_->ResetCallFunctionStatus();

    EXPECT_TRUE(bridge.Dispatch(request));
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    napi_value dispatchedRequest = mockNapiPtr_->callFunctionArgsHistory_.back()[0];
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, dispatchedRequest, "renderId"), 7.0);
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, dispatchedRequest, "componentId"), "root");
    EXPECT_EQ(GetRequestNumberProperty(mockNapiPtr_, dispatchedRequest, "nodeUniqueId"), 42.0);
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, dispatchedRequest, "componentType"), "TextInput");
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, dispatchedRequest, "attributeName"), "minFontScale");
    EXPECT_NEAR(GetRequestNumberProperty(mockNapiPtr_, dispatchedRequest, "floatValue"), 1.25, 1e-6);
    EXPECT_TRUE(GetRequestBoolProperty(mockNapiPtr_, dispatchedRequest, "reset"));
}
