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
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedGridTheme.h"
#include "components/extended/ExtendedListTheme.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedTextInputComponent.h"
#include "components/extended/ExtendedToggleComponent.h"
#include "functions/FunctionBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "styles/StyleResolver.h"
#include "theme/ThemeBase.h"
#include "theme/ThemeFactory.h"
#include "theme/ThemeManager.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

constexpr uint32_t RADIO_DARK_CHECKED_BACKGROUND_COLOR = 0xFF317AF7u;
constexpr uint32_t RADIO_LIGHT_CHECKED_BACKGROUND_COLOR = 0xFF0A59F7u;
constexpr uint32_t RADIO_DEFAULT_UNCHECKED_BORDER_COLOR = 0x33FFFFFFu;
constexpr uint32_t RADIO_DEFAULT_INDICATOR_COLOR = 0xFFFFFFFFu;
constexpr uint32_t TOGGLE_LIGHT_SELECTED_COLOR = 0xFF007DFFu;
constexpr uint32_t TOGGLE_LIGHT_UNSELECTED_COLOR = 0x19000000u;
constexpr uint32_t TOGGLE_LIGHT_SWITCH_POINT_COLOR = 0xFFFFFFFFu;
constexpr uint32_t TOGGLE_DARK_SELECTED_COLOR = 0xFF006CDEu;
constexpr uint32_t TOGGLE_DARK_UNSELECTED_COLOR = 0x19FFFFFFu;
constexpr uint32_t TOGGLE_DARK_SWITCH_POINT_COLOR = 0xFFE5E5E5u;
constexpr float TOGGLE_LABEL_SPACING = 12.0F;
constexpr uint32_t TEXT_INPUT_DARK_FONT_COLOR = 0xE5FFFFFFu;
constexpr uint32_t TEXT_INPUT_DARK_PLACEHOLDER_COLOR = 0x99FFFFFFu;
constexpr uint32_t TEXT_INPUT_DARK_CARET_COLOR = 0xFF5291FFu;
constexpr uint32_t TEXT_INPUT_DARK_SELECTED_BACKGROUND_COLOR = 0x33006CDEu;
constexpr uint32_t TEXT_INPUT_DARK_UNDERLINE_COLOR = 0x33FFFFFFu;
constexpr uint32_t BUTTON_LIGHT_NORMAL_FONT_COLOR = 0xFF0A59F7u;
constexpr uint32_t BUTTON_DARK_NORMAL_FONT_COLOR = 0xFF5291FFu;
constexpr uint32_t BUTTON_LIGHT_NORMAL_BACKGROUND_COLOR = 0x0C000000u;
constexpr uint32_t BUTTON_DARK_NORMAL_BACKGROUND_COLOR = 0x19FFFFFFu;
intptr_t g_uniqueNodeHandleSeed = 0;

ArkUI_NodeHandle CreateUniqueTestNodeHandle(ArkUI_NodeType)
{
    ++g_uniqueNodeHandleSeed;
    return reinterpret_cast<ArkUI_NodeHandle>(g_uniqueNodeHandleSeed);
}

class ScopedCreateNodeOverride {
public:
    explicit ScopedCreateNodeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi)
        : nativeNodeApi_(nativeNodeApi),
          originalCreateNode_(nativeNodeApi != nullptr ? nativeNodeApi->createNode : nullptr)
    {}

    ~ScopedCreateNodeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->createNode = originalCreateNode_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    ArkUI_NodeHandle (*originalCreateNode_)(ArkUI_NodeType) = nullptr;
};

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog(
    std::initializer_list<const char*> componentNames = {}, std::initializer_list<const char*> functionNames = {})
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
    for (const char* functionName : functionNames) {
        if (functionName == nullptr || functionName[0] == '\0') {
            continue;
        }
        catalog->AddFunction(std::make_shared<CatalogItem>(functionName, CatalogItemType::LOCAL_FUNCTION));
    }
    return catalog;
}

ThemeContext CreateThemeContext(ThemeMode mode)
{
    ThemeContext context;
    context.colorMode = mode;
    return context;
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastSetAttributeRecord(
    const MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, int32_t attribute)
{
    if (provider == nullptr) {
        return nullptr;
    }
    for (auto iter = provider->setAttributeRecords_.rbegin(); iter != provider->setAttributeRecords_.rend(); ++iter) {
        if (iter->nodeHandle == nodeHandle && iter->attribute == attribute) {
            return &(*iter);
        }
    }
    return nullptr;
}

struct DispatchCallbacks {
    napi_env env = nullptr;
    napi_value warningCallback = nullptr;
};

DispatchCallbacks RegisterWarningDispatchCallback(MockNapiProvider* mockNapi)
{
    DispatchCallbacks callbacks;
    if (mockNapi == nullptr) {
        return callbacks;
    }
    callbacks.env = reinterpret_cast<napi_env>(0x1400);
    mockNapi->CreateFunction(
        callbacks.env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &callbacks.warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(callbacks.env, callbacks.warningCallback);
    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionArgs_.clear();
    return callbacks;
}

napi_value RawNapiValue(intptr_t id)
{
    return reinterpret_cast<napi_value>(id);
}

napi_value CreateManualBool(MockNapiProvider* mockNapi, intptr_t id, bool value)
{
    napi_value napiValue = RawNapiValue(id);
    if (mockNapi != nullptr) {
        mockNapi->valueTypes_[napiValue] = napi_boolean;
        mockNapi->boolValues_[napiValue] = value;
    }
    return napiValue;
}

napi_value CreateManualString(MockNapiProvider* mockNapi, intptr_t id, const std::string& value)
{
    napi_value napiValue = RawNapiValue(id);
    if (mockNapi != nullptr) {
        mockNapi->valueTypes_[napiValue] = napi_string;
        mockNapi->stringValues_[napiValue] = value;
    }
    return napiValue;
}

void PrepareNextLocalFunctionResult(MockNapiProvider* mockNapi, int32_t nextValueId, napi_value returnValue)
{
    if (mockNapi == nullptr) {
        return;
    }
    mockNapi->nextValueId_ = nextValueId;
    napi_value resultObject = RawNapiValue(static_cast<intptr_t>(nextValueId + 8));
    mockNapi->valueTypes_[resultObject] = napi_object;
    mockNapi->objectProperties_[resultObject] = {};
    mockNapi->objectProperties_[resultObject]["success"] =
        CreateManualBool(mockNapi, static_cast<intptr_t>(nextValueId + 100), true);
    mockNapi->objectProperties_[resultObject]["value"] = returnValue;
}

napi_value RegisterLocalFunctionCallback(MockNapiProvider* mockNapi, napi_env env)
{
    napi_value callback = nullptr;
    if (mockNapi != nullptr) {
        mockNapi->CreateFunction(env, "invokeLocalFunction", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env, callback);
    }
    return callback;
}

napi_value GetRequestProperty(const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    if (mockNapi == nullptr || request == nullptr) {
        return nullptr;
    }
    auto objectIt = mockNapi->objectProperties_.find(request);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return nullptr;
    }
    auto propertyIt = objectIt->second.find(key);
    if (propertyIt == objectIt->second.end()) {
        return nullptr;
    }
    return propertyIt->second;
}

std::string GetRequestStringProperty(const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    napi_value value = GetRequestProperty(mockNapi, request, key);
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto stringIt = mockNapi->stringValues_.find(value);
    return stringIt == mockNapi->stringValues_.end() ? "" : stringIt->second;
}

size_t CountWarningRequests(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    if (mockNapi == nullptr) {
        return 0;
    }
    size_t count = 0;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }
        napi_value request = args[0];
        std::string warningCode = GetRequestStringProperty(mockNapi, request, "code");
        std::string warningPath = GetRequestStringProperty(mockNapi, request, "path");
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

size_t CountWarningRequestsExactPath(const MockNapiProvider* mockNapi, const std::string& code, const std::string& path)
{
    if (mockNapi == nullptr) {
        return 0;
    }
    size_t count = 0;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }
        napi_value request = args[0];
        std::string warningCode = GetRequestStringProperty(mockNapi, request, "code");
        std::string warningPath = GetRequestStringProperty(mockNapi, request, "path");
        if (warningCode == code && warningPath == path) {
            ++count;
        }
    }
    return count;
}

std::string GetFirstWarningMessageExactPath(
    const MockNapiProvider* mockNapi, const std::string& code, const std::string& path)
{
    if (mockNapi == nullptr) {
        return "";
    }
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }
        napi_value request = args[0];
        std::string warningCode = GetRequestStringProperty(mockNapi, request, "code");
        std::string warningPath = GetRequestStringProperty(mockNapi, request, "path");
        if (warningCode == code && warningPath == path) {
            return GetRequestStringProperty(mockNapi, request, "message");
        }
    }
    return "";
}

} // namespace

class ExtendedThemeDefaultsTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-theme-defaults");
        slot_.SetRenderId(1);
    }

    void InitializeTheme(ThemeMode mode)
    {
        slot_.InitializeThemeManager(CreateThemeContext(mode));
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedThemeDefaultsTest, L0_should_apply_renamed_radio_unchecked_border_color_field)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Radio",
            "styles": {
                "unCheckedBorderColor": "#AABBCC"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFFAABBCCu);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_ignore_legacy_radio_unchecked_border_color_field)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Radio",
            "styles": {
                "uncheckedBorderColor": "#AABBCC"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), RADIO_DEFAULT_UNCHECKED_BORDER_COLOR);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_indicator_type_as_undefined_field_for_radio)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1",
            "indicatorType": "dot"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(radio, nullptr);
    ASSERT_GE(mockNapiPtr_->callFunctionArgsHistory_.size(), 1u);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_[0].empty());
    napi_value warningRequest = mockNapiPtr_->callFunctionArgsHistory_[0][0];
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, warningRequest, "code"), "ERROR_CODE_UNDEFINED_FIELD");
    EXPECT_EQ(GetRequestStringProperty(mockNapiPtr_, warningRequest, "path"), "root.indicatorType");
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_static_invalid_private_styles_for_button_and_textinput)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button", "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontWeight": "ultraHeavy",
                    "minFontSize": -1,
                    "fontScaleMode": "invalid"
                }
            },
            {
                "id": "buttonMissingLabel",
                "component": "Button",
                "enabled": true
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": "input",
                "styles": {
                    "cancelButton": "invalid",
                    "underlineColor": {
                        "typing": false
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_REQUIRED_MISS", "buttonMissingLabel.label"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.styles.fontWeight"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.styles.minFontSize"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.styles.fontScaleMode"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "button.styles.fontScaleMode"), 0U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.cancelButton"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.typing"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "input.styles.cancelButton"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "input.styles.underlineColor"), 0U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_classify_button_and_textinput_schema_warning_boundaries)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button", "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "buttonWeightType",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontWeight": false
                }
            },
            {
                "id": "buttonWeightObject",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontWeight": { "invalid": "bad-weight" }
                }
            },
            {
                "id": "inputAlignType",
                "component": "TextInput",
                "text": "align type",
                "styles": {
                    "textAlign": 2026
                }
            },
            {
                "id": "inputAlignObject",
                "component": "TextInput",
                "text": "align object",
                "styles": {
                    "textAlign": { "invalid": "center" }
                }
            },
            {
                "id": "inputCancelSizeInvalid",
                "component": "TextInput",
                "text": "cancel invalid",
                "styles": {
                    "cancelButton": {
                        "fontSize": "bad-size"
                    }
                }
            },
            {
                "id": "inputCancelSizeType",
                "component": "TextInput",
                "text": "cancel type",
                "styles": {
                    "cancelButton": {
                        "fontSize": false
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "buttonWeightType.styles.fontWeight"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "buttonWeightObject.styles.fontWeight"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "inputAlignType.styles.textAlign"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "inputAlignObject.styles.textAlign"), 1U);
    EXPECT_GE(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "inputCancelSizeInvalid.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "inputCancelSizeType.styles.cancelButton.fontSize"),
        1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_allow_valid_text_image_divider_and_progress_private_style_keys)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text", "Image", "Divider", "Progress" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "text",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "fontScaleMode": "custom",
                    "minFontScale": 0.5,
                    "maxFontScale": 1.5
                }
            },
            {
                "id": "image",
                "component": "Image",
                "src": "https://example.com/photo.png",
                "styles": {
                    "aspectRatio": 1.2,
                    "objectFit": "contain"
                }
            },
            {
                "id": "divider",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "1px",
                    "vertical": false,
                    "color": "#33000000"
                }
            },
            {
                "id": "progress",
                "component": "Progress",
                "value": 50,
                "total": 100,
                "styles": {
                    "color": "#FF0A59F7",
                    "type": "linear"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "text.styles.fontScaleMode"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "text.styles.minFontScale"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "text.styles.maxFontScale"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "image.styles.aspectRatio"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "image.styles.objectFit"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "divider.styles.strokeWidth"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "divider.styles.vertical"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "divider.styles.color"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "progress.styles.color"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "progress.styles.type"), 0U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_static_invalid_extended_private_attributes_once)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(
        BuildExtendedProtocolCatalog({ "Button", "TextInput", "Toggle", "Radio", "Checkbox", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": false,
                "styles": {
                    "fontSize": false
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "enabled": "bad"
            },
            {
                "id": "toggle",
                "component": "Toggle",
                "isOn": "bad"
            },
            {
                "id": "radio",
                "component": "Radio",
                "checked": "bad"
            },
            {
                "id": "checkbox",
                "component": "Checkbox",
                "select": "bad"
            },
            {
                "id": "checkboxGroup",
                "component": "CheckboxGroup",
                "selectAll": "bad"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.label"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.enabled"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.isOn"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.checked"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.select"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkboxGroup.selectAll"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontSize"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_legacy_private_style_names_as_undefined_without_alias)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "radio",
                "component": "Radio",
                "styles": {
                    "uncheckedBorderColor": "#AABBCC"
                }
            },
            {
                "id": "group",
                "component": "CheckboxGroup",
                "styles": {
                    "unselectedColor": "#112233",
                    "shape": "rounded_square"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "radio.styles.uncheckedBorderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "group.styles.unselectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "group.styles.shape"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_static_private_color_type_mismatch_for_non_string_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button", "TextInput", "Radio", "Checkbox", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontColor": 2026
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": "input",
                "styles": {
                    "fontColor": 2026,
                    "placeholderColor": 2026,
                    "caretColor": 2026,
                    "selectedBackgroundColor": 2026,
                    "cancelButton": {
                        "fontColor": 2026
                    },
                    "underlineColor": {
                        "typing": 2026,
                        "normal": 2026,
                        "error": 2026,
                        "disable": 2026
                    }
                }
            },
            {
                "id": "radio",
                "component": "Radio",
                "styles": {
                    "checkedBackgroundColor": 2026,
                    "unCheckedBorderColor": 2026,
                    "indicatorColor": 2026
                }
            },
            {
                "id": "checkbox",
                "component": "Checkbox",
                "label": "checkbox",
                "styles": {
                    "selectedColor": 2026,
                    "unselectedColor": 2026,
                    "mark": {
                        "strokeColor": 2026
                    }
                }
            },
            {
                "id": "group",
                "component": "CheckboxGroup",
                "styles": {
                    "selectedColor": 2026,
                    "unSelectedColor": 2026,
                    "mark": {
                        "strokeColor": 2026
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.placeholderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.caretColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.selectedBackgroundColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.cancelButton.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.typing"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.normal"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.error"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.disable"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.checkedBackgroundColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.unCheckedBorderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.indicatorColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.unselectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.mark.strokeColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_static_private_color_invalid_value_for_bad_strings)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(
        BuildExtendedProtocolCatalog({ "Button", "TextInput", "Toggle", "Radio", "Checkbox", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontColor": "bad-color"
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": "input",
                "styles": {
                    "fontColor": "bad-color",
                    "placeholderColor": "bad-color",
                    "caretColor": "bad-color",
                    "selectedBackgroundColor": "bad-color",
                    "cancelButton": {
                        "fontColor": "bad-color"
                    },
                    "underlineColor": {
                        "typing": "bad-color",
                        "normal": "bad-color",
                        "error": "bad-color",
                        "disable": "bad-color"
                    }
                }
            },
            {
                "id": "toggle",
                "component": "Toggle",
                "styles": {
                    "selectedColor": "bad-color",
                    "unSelectedColor": "bad-color",
                    "switchPointColor": "bad-color"
                }
            },
            {
                "id": "radio",
                "component": "Radio",
                "styles": {
                    "checkedBackgroundColor": "bad-color",
                    "unCheckedBorderColor": "bad-color",
                    "indicatorColor": "bad-color"
                }
            },
            {
                "id": "checkbox",
                "component": "Checkbox",
                "label": "checkbox",
                "styles": {
                    "selectedColor": "bad-color",
                    "unselectedColor": "bad-color",
                    "mark": {
                        "strokeColor": "bad-color"
                    }
                }
            },
            {
                "id": "group",
                "component": "CheckboxGroup",
                "styles": {
                    "selectedColor": "bad-color",
                    "unSelectedColor": "bad-color",
                    "mark": {
                        "strokeColor": "bad-color"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.placeholderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.caretColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.selectedBackgroundColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.underlineColor.typing"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.underlineColor.normal"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.underlineColor.error"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.underlineColor.disable"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "toggle.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "toggle.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "toggle.styles.switchPointColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "radio.styles.checkedBackgroundColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "radio.styles.unCheckedBorderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "radio.styles.indicatorColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.unselectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "group.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "group.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "group.styles.mark.strokeColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_dynamic_private_color_type_mismatch_for_non_string_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button", "TextInput", "Radio", "Checkbox", "CheckboxGroup" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "buttonFontColor": 2026,
            "inputFontColor": 2026,
            "inputPlaceholderColor": 2026,
            "inputCaretColor": 2026,
            "inputSelectedBackgroundColor": 2026,
            "inputCancelButton": {
                "fontColor": 2026
            },
            "inputUnderlineColor": {
                "typing": 2026,
                "normal": 2026,
                "error": 2026,
                "disable": 2026
            },
            "radioCheckedBackgroundColor": 2026,
            "radioUnCheckedBorderColor": 2026,
            "radioIndicatorColor": 2026,
            "checkboxSelectedColor": 2026,
            "checkboxUnselectedColor": 2026,
            "checkboxMark": {
                "strokeColor": 2026
            },
            "groupSelectedColor": 2026,
            "groupUnSelectedColor": 2026,
            "groupMark": {
                "strokeColor": 2026
            }
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontColor": { "path": "/buttonFontColor" }
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": "input",
                "styles": {
                    "fontColor": { "path": "/inputFontColor" },
                    "placeholderColor": { "path": "/inputPlaceholderColor" },
                    "caretColor": { "path": "/inputCaretColor" },
                    "selectedBackgroundColor": { "path": "/inputSelectedBackgroundColor" },
                    "cancelButton": { "path": "/inputCancelButton" },
                    "underlineColor": { "path": "/inputUnderlineColor" }
                }
            },
            {
                "id": "radio",
                "component": "Radio",
                "styles": {
                    "checkedBackgroundColor": { "path": "/radioCheckedBackgroundColor" },
                    "unCheckedBorderColor": { "path": "/radioUnCheckedBorderColor" },
                    "indicatorColor": { "path": "/radioIndicatorColor" }
                }
            },
            {
                "id": "checkbox",
                "component": "Checkbox",
                "label": "checkbox",
                "styles": {
                    "selectedColor": { "path": "/checkboxSelectedColor" },
                    "unselectedColor": { "path": "/checkboxUnselectedColor" },
                    "mark": { "path": "/checkboxMark" }
                }
            },
            {
                "id": "group",
                "component": "CheckboxGroup",
                "styles": {
                    "selectedColor": { "path": "/groupSelectedColor" },
                    "unSelectedColor": { "path": "/groupUnSelectedColor" },
                    "mark": { "path": "/groupMark" }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.placeholderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.caretColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.selectedBackgroundColor"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.cancelButton.fontColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.typing"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.normal"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.error"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor.disable"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.checkedBackgroundColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.unCheckedBorderColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.indicatorColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.unselectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.mark.strokeColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_invalid_checkbox_private_style_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "checkbox",
                "component": "Checkbox",
                "label": "checkbox",
                "styles": {
                    "shape": "invalidShape",
                    "mark": {
                        "strokeColor": false,
                        "size": -1
                    }
                }
            },
            {
                "id": "group",
                "component": "CheckboxGroup",
                "styles": {
                    "checkboxShape": "invalidShape",
                    "mark": "invalid"
                }
            },
            {
                "id": "checkboxEmpty",
                "component": "Checkbox",
                "label": "empty mark",
                "styles": {
                    "mark": {}
                }
            },
            {
                "id": "checkboxUnknown",
                "component": "Checkbox",
                "label": "unknown mark",
                "styles": {
                    "mark": {
                        "invalid": "bad-mark"
                    }
                }
            },
            {
                "id": "groupEmpty",
                "component": "CheckboxGroup",
                "styles": {
                    "mark": {}
                }
            },
            {
                "id": "groupUnknown",
                "component": "CheckboxGroup",
                "styles": {
                    "mark": {
                        "invalid": "bad-mark"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.shape"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.mark.size"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "group.styles.checkboxShape"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "group.styles.mark"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkboxEmpty.styles.mark"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "checkboxEmpty.styles.mark"), 0U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "checkboxUnknown.styles.mark.invalid"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "groupEmpty.styles.mark"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "groupEmpty.styles.mark"), 0U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "groupUnknown.styles.mark.invalid"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_mark_nested_dynamic_member_warnings)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox", "CheckboxGroup" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "checkboxStrokeColor": 2026,
            "checkboxSize": -1,
            "checkboxStrokeWidth": "bad-number",
            "groupStrokeColor": "bad-color",
            "groupSize": "bad-number",
            "groupStrokeWidth": 0
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "checkboxNested",
                "component": "Checkbox",
                "label": "nested mark",
                "styles": {
                    "mark": {
                        "strokeColor": { "path": "/checkboxStrokeColor" },
                        "size": { "path": "/checkboxSize" },
                        "strokeWidth": { "path": "/checkboxStrokeWidth" }
                    }
                }
            },
            {
                "id": "groupNested",
                "component": "CheckboxGroup",
                "styles": {
                    "mark": {
                        "strokeColor": { "path": "/groupStrokeColor" },
                        "size": { "path": "/groupSize" },
                        "strokeWidth": { "path": "/groupStrokeWidth" }
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkboxNested.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkboxNested.styles.mark.size"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkboxNested.styles.mark.strokeWidth"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "groupNested.styles.mark.strokeColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "groupNested.styles.mark.size"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "groupNested.styles.mark.strokeWidth"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_allow_dynamic_private_attributes_and_styles_except_button_action)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(
        BuildExtendedProtocolCatalog({ "Button", "TextInput", "Toggle", "Radio", "Checkbox", "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": { "path": "/buttonLabel" },
                "enabled": { "path": "/buttonEnabled" },
                "action": { "path": "/buttonAction" },
                "styles": {
                    "fontSize": { "path": "/buttonFontSize" },
                    "fontColor": { "path": "/buttonFontColor" },
                    "fontScaleMode": { "path": "/buttonFontScaleMode" }
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": { "path": "/inputText" },
                "placeholder": { "path": "/inputPlaceholder" },
                "enabled": { "path": "/inputEnabled" },
                "maxLength": { "path": "/inputMaxLength" },
                "type": { "path": "/inputType" },
                "styles": {
                    "placeholderColor": { "path": "/inputPlaceholderColor" },
                    "cancelButton": { "path": "/inputCancelButton" },
                    "underlineColor": { "path": "/inputUnderlineColor" },
                    "wordBreak": { "path": "/inputWordBreak" }
                }
            },
            {
                "id": "toggle",
                "component": "Toggle",
                "label": { "path": "/toggleLabel" },
                "isOn": { "path": "/toggleIsOn" },
                "enabled": { "path": "/toggleEnabled" },
                "styles": {
                    "selectedColor": { "path": "/toggleSelectedColor" },
                    "unSelectedColor": { "path": "/toggleUnSelectedColor" },
                    "switchPointColor": { "path": "/toggleSwitchPointColor" }
                }
            },
            {
                "id": "radio",
                "component": "Radio",
                "value": { "path": "/radioValue" },
                "checked": { "path": "/radioChecked" },
                "group": { "path": "/radioGroup" },
                "styles": {
                    "checkedBackgroundColor": { "path": "/radioCheckedBackgroundColor" },
                    "unCheckedBorderColor": { "path": "/radioUnCheckedBorderColor" },
                    "indicatorColor": { "path": "/radioIndicatorColor" }
                }
            },
            {
                "id": "checkbox",
                "component": "Checkbox",
                "label": { "path": "/checkboxLabel" },
                "select": { "path": "/checkboxSelect" },
                "value": { "path": "/checkboxValue" },
                "group": { "path": "/checkboxGroup" },
                "styles": {
                    "selectedColor": { "path": "/checkboxSelectedColor" },
                    "unselectedColor": { "path": "/checkboxUnselectedColor" },
                    "mark": { "path": "/checkboxMark" },
                    "shape": { "path": "/checkboxShape" }
                }
            },
            {
                "id": "checkboxGroup",
                "component": "CheckboxGroup",
                "group": { "path": "/checkboxGroupName" },
                "selectAll": { "path": "/checkboxGroupSelectAll" },
                "styles": {
                    "selectedColor": { "path": "/checkboxGroupSelectedColor" },
                    "unSelectedColor": { "path": "/checkboxGroupUnSelectedColor" },
                    "mark": { "path": "/checkboxGroupMark" },
                    "checkboxShape": { "path": "/checkboxGroupShape" }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.action"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "button.styles.fontScaleMode"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.cancelButton"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "input.styles.cancelButton"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "input.styles.underlineColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.selectedColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "toggle.styles.selectedColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "radio.styles.indicatorColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "radio.styles.indicatorColor"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkbox.styles.mark"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "checkbox.styles.mark"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "checkboxGroup.styles.mark"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "checkboxGroup.styles.mark"), 0U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_toggle_color_style_type_mismatch_for_non_string_static_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "styles": {
                "selectedColor": 2026,
                "unSelectedColor": false,
                "switchPointColor": 2026
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.switchPointColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_toggle_color_style_type_mismatch_for_non_string_dynamic_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "selectedColor": 2026,
            "unSelectedColor": false,
            "switchPointColor": 2026
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "styles": {
                "selectedColor": { "path": "/selectedColor" },
                "unSelectedColor": { "path": "/unSelectedColor" },
                "switchPointColor": { "path": "/switchPointColor" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.selectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.unSelectedColor"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "toggle.styles.switchPointColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_dynamic_style_type_mismatch_and_keep_compatible_number)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "buttonFontSize": "18"
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "button",
            "component": "Button",
            "label": "button",
            "styles": {
                "fontSize": { "path": "/buttonFontSize" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("button"));
    ASSERT_NE(button, nullptr);
    EXPECT_FLOAT_EQ(button->GetFontSizeForTest(), 18.0F);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontSize"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_binding_schema_warnings_once_on_initial_build)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    constexpr int32_t renderId = 9201;
    const std::string surfaceId = "surface-button-binding-warning-once";
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(surfaceId, nullptr);
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));

    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "buttonLabel": 2026,
            "buttonEnabled": "not-boolean",
            "buttonFontSize": "bad-number",
            "buttonMinFontSize": "bad-number",
            "buttonMaxFontSize": "bad-number",
            "buttonFontWeight": false,
            "buttonFontScaleMode": 2026,
            "buttonMinFontScale": "bad-number",
            "buttonMaxFontScale": "bad-number"
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Button",
            "label": { "path": "/buttonLabel" },
            "enabled": { "path": "/buttonEnabled" },
            "styles": {
                "fontSize": { "path": "/buttonFontSize" },
                "minFontSize": { "path": "/buttonMinFontSize" },
                "maxFontSize": { "path": "/buttonMaxFontSize" },
                "fontWeight": { "path": "/buttonFontWeight" },
                "fontScaleMode": { "path": "/buttonFontScaleMode" },
                "minFontScale": { "path": "/buttonMinFontScale" },
                "maxFontScale": { "path": "/buttonMaxFontScale" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.label"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.enabled"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.fontSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.minFontSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.maxFontSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.fontWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.fontScaleMode"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.minFontScale"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "root.styles.maxFontScale"), 1U);
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_dynamic_font_weight_invalid_enum_for_button_and_textinput)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button", "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "buttonFontWeight": "ultraHeavy",
            "inputFontWeight": "ultraHeavy"
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "button",
                "component": "Button",
                "label": "button",
                "styles": {
                    "fontWeight": { "path": "/buttonFontWeight" }
                }
            },
            {
                "id": "input",
                "component": "TextInput",
                "text": "input",
                "styles": {
                    "fontWeight": { "path": "/inputFontWeight" }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "button.styles.fontWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "button.styles.fontWeight"), 0U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.fontWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.fontWeight"), 0U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_dynamic_number_range_error_for_textinput_max_length)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "inputMaxLength": -5
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "input",
            "maxLength": { "path": "/inputMaxLength" }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("input"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetMaxLengthForTest(), std::numeric_limits<int32_t>::max());
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.maxLength"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_schema_dfx_matrix_invalid_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "static invalid",
            "placeholder": "placeholder",
            "enabled": true,
            "maxLength": -1,
            "type": "unknownType",
            "styles": {
                "width": 260,
                "height": 44,
                "backgroundColor": "#FFF7ED",
                "borderRadius": 12,
                "cancelButton": {
                    "style": "badStyle",
                    "fontSize": "bad-size",
                    "fontColor": "bad-color"
                },
                "fontWeight": "ultraHeavy",
                "maxLines": 0,
                "minFontScale": 1.5,
                "maxFontScale": 0.5
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.maxLength"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.type"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.fontWeight"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.maxLines"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.minFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.maxFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.style"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.fontSize"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.fontColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_dfx_matrix_invalid_values)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "inputText": "binding invalid",
            "inputMaxLength": -1,
            "inputType": "unknownType",
            "cancelButton": {
                "style": "badStyle",
                "fontSize": "bad-size",
                "fontColor": "bad-color"
            },
            "fontWeight": "ultraHeavy",
            "maxLines": 0,
            "minFontScale": 1.5,
            "maxFontScale": 0.5
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": { "path": "/inputText" },
            "placeholder": "placeholder",
            "enabled": true,
            "maxLength": { "path": "/inputMaxLength" },
            "type": { "path": "/inputType" },
            "styles": {
                "width": 260,
                "height": 44,
                "backgroundColor": "#FFF7ED",
                "borderRadius": 12,
                "cancelButton": { "path": "/cancelButton" },
                "fontWeight": { "path": "/fontWeight" },
                "maxLines": { "path": "/maxLines" },
                "minFontScale": { "path": "/minFontScale" },
                "maxFontScale": { "path": "/maxFontScale" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.maxLength"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.type"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.fontWeight"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.maxLines"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.minFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.maxFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.style"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.fontSize"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.cancelButton.fontColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_cancel_button_static_member_warnings_by_category)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "typeMismatch",
                "component": "TextInput",
                "text": "type mismatch",
                "styles": {
                    "cancelButton": {
                        "style": 2026,
                        "fontSize": false,
                        "fontColor": 2026
                    }
                }
            },
            {
                "id": "invalidValue",
                "component": "TextInput",
                "text": "invalid value",
                "styles": {
                    "cancelButton": {
                        "style": "badStyle",
                        "fontSize": "bad-size",
                        "fontColor": "bad-color"
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontColor"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontColor"),
        1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_cancel_button_binding_member_warnings_by_category)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "typeMismatch": {
                "style": 2026,
                "fontSize": false,
                "fontColor": 2026
            },
            "invalidValue": {
                "style": "badStyle",
                "fontSize": "bad-size",
                "fontColor": "bad-color"
            }
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "typeMismatch",
                "component": "TextInput",
                "text": "type mismatch",
                "styles": {
                    "cancelButton": { "path": "/typeMismatch" }
                }
            },
            {
                "id": "invalidValue",
                "component": "TextInput",
                "text": "invalid value",
                "styles": {
                    "cancelButton": { "path": "/invalidValue" }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontColor"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontColor"),
        1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_cancel_button_nested_dynamic_member_warnings)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "typeStyle": 2026,
            "typeFontSize": false,
            "typeFontColor": 2026,
            "invalidStyle": "badStyle",
            "invalidFontSize": "bad-size",
            "invalidFontColor": "bad-color"
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "typeMismatch",
                "component": "TextInput",
                "text": "nested type mismatch",
                "styles": {
                    "cancelButton": {
                        "style": { "path": "/typeStyle" },
                        "fontSize": { "path": "/typeFontSize" },
                        "fontColor": { "path": "/typeFontColor" }
                    }
                }
            },
            {
                "id": "invalidValue",
                "component": "TextInput",
                "text": "nested invalid value",
                "styles": {
                    "cancelButton": {
                        "style": { "path": "/invalidStyle" },
                        "fontSize": { "path": "/invalidFontSize" },
                        "fontColor": { "path": "/invalidFontColor" }
                    }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeMismatch.styles.cancelButton.fontColor"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.style"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontSize"),
        1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidValue.styles.cancelButton.fontColor"),
        1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_underline_color_nested_function_member_warnings)
{
    DispatchCallbacks callbacks = RegisterWarningDispatchCallback(mockNapiPtr_);
    RegisterLocalFunctionCallback(mockNapiPtr_, callbacks.env);
    constexpr int32_t renderId = 97001;
    const std::string surfaceId = "surface_textinput_underline_function";
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& managedSlot = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    managedSlot.InitializeThemeManager(CreateThemeContext(ThemeMode::LIGHT));
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }, { "resolveUnderline" }));

    auto updateUnderlineColor = [this, &managedSlot](const std::string& componentId, const std::string& colorKey,
                                    napi_value returnValue, int32_t nextValueId) {
        PrepareNextLocalFunctionResult(mockNapiPtr_, nextValueId, returnValue);
        std::string messageJson = R"({"components":[{"id":")" + componentId +
                                  R"(","component":"TextInput","text":"nested function",)"
                                  R"("styles":{"underlineColor":{")" +
                                  colorKey + R"(":{"call":"resolveUnderline","returnType":"string"}}}}]})";
        auto message = JsonAdapter::Parse(messageJson);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));
    };

    updateUnderlineColor("typeTyping", "typing", CreateManualBool(mockNapiPtr_, 10101, true), 10000);
    updateUnderlineColor("typeNormal", "normal", CreateManualBool(mockNapiPtr_, 11101, false), 11000);
    updateUnderlineColor("invalidError", "error", CreateManualString(mockNapiPtr_, 12101, "not_a_color"), 12000);
    updateUnderlineColor("invalidDisable", "disable", CreateManualString(mockNapiPtr_, 13101, "not_a_color"), 13000);

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeTyping.styles.underlineColor.typing"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "typeNormal.styles.underlineColor.normal"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidError.styles.underlineColor.error"), 1U);
    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "invalidDisable.styles.underlineColor.disable"),
        1U);
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_allow_empty_textinput_private_style_objects_and_report_unknown_members)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "cancelUnknown",
                "component": "TextInput",
                "text": "cancel unknown",
                "styles": {
                    "cancelButton": { "invalid": "bad-cancel" }
                }
            },
            {
                "id": "cancelEmpty",
                "component": "TextInput",
                "text": "cancel empty",
                "styles": {
                    "cancelButton": {}
                }
            },
            {
                "id": "underlineUnknown",
                "component": "TextInput",
                "text": "underline unknown",
                "styles": {
                    "underlineColor": { "invalid": "bad-underline" }
                }
            },
            {
                "id": "underlineEmpty",
                "component": "TextInput",
                "text": "underline empty",
                "styles": {
                    "underlineColor": {}
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "cancelUnknown.styles.cancelButton.invalid"),
        1U);
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "cancelEmpty.styles.cancelButton"), 0U)
        << GetFirstWarningMessageExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "cancelEmpty.styles.cancelButton");
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "cancelEmpty.styles.cancelButton"),
        0U);
    EXPECT_GE(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "underlineUnknown.styles.underlineColor.invalid"),
        1U);
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "underlineEmpty.styles.underlineColor"),
        0U)
        << GetFirstWarningMessageExactPath(
               mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "underlineEmpty.styles.underlineColor");
    EXPECT_EQ(CountWarningRequestsExactPath(
                  mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "underlineEmpty.styles.underlineColor"),
        0U);
}

TEST_F(
    ExtendedThemeDefaultsTest, L0_should_allow_dynamic_empty_textinput_private_style_objects_and_report_unknown_members)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    constexpr int32_t renderId = 97002;
    const std::string surfaceId = "surface_textinput_empty_object_binding";
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& managedSlot = surfaceManager->CreateSurface(surfaceId, nullptr);
    managedSlot.InitializeThemeManager(CreateThemeContext(ThemeMode::LIGHT));
    managedSlot.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "cancelUnknown": { "invalid": "bad-cancel" },
            "cancelEmpty": {},
            "underlineUnknown": { "invalid": "bad-underline" },
            "underlineEmpty": {}
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(managedSlot.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "cancelUnknown",
                "component": "TextInput",
                "text": "cancel unknown",
                "styles": {
                    "cancelButton": { "path": "/cancelUnknown" }
                }
            },
            {
                "id": "cancelEmpty",
                "component": "TextInput",
                "text": "cancel empty",
                "styles": {
                    "cancelButton": { "path": "/cancelEmpty" }
                }
            },
            {
                "id": "underlineUnknown",
                "component": "TextInput",
                "text": "underline unknown",
                "styles": {
                    "underlineColor": { "path": "/underlineUnknown" }
                }
            },
            {
                "id": "underlineEmpty",
                "component": "TextInput",
                "text": "underline empty",
                "styles": {
                    "underlineColor": { "path": "/underlineEmpty" }
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(managedSlot.UpdateComponents(message->GetRoot()));

    EXPECT_GE(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "cancelUnknown.styles.cancelButton.invalid"),
        1U);
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "cancelEmpty.styles.cancelButton"), 0U)
        << GetFirstWarningMessageExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "cancelEmpty.styles.cancelButton");
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "cancelEmpty.styles.cancelButton"),
        0U);
    EXPECT_GE(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "underlineUnknown.styles.underlineColor.invalid"),
        1U);
    EXPECT_EQ(
        CountWarningRequestsExactPath(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "underlineEmpty.styles.underlineColor"),
        0U)
        << GetFirstWarningMessageExactPath(
               mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "underlineEmpty.styles.underlineColor");
    EXPECT_EQ(CountWarningRequestsExactPath(
                  mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "underlineEmpty.styles.underlineColor"),
        0U);
    RenderManager::GetInstance().RemoveRenderSlot(renderId);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_apply_zero_textinput_max_length)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "input",
            "maxLength": 0
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("input"));
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->GetMaxLengthForTest(), 0);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_dynamic_enum_range_error_for_checkbox_shape)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "checkboxShape": "roundedSquare"
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "checkbox",
            "component": "Checkbox",
            "label": "checkbox",
            "styles": {
                "shape": { "path": "/checkboxShape" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "checkbox.styles.shape"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_apply_toggle_spacing_between_label_and_switch)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "label": "demo"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);
    const auto* marginCall = FindLastSetAttributeRecord(mockArkUIPtr_, toggle->GetToggleNodeForTest(), NODE_MARGIN);
    ASSERT_NE(marginCall, nullptr);
    ASSERT_EQ(marginCall->values.size(), 4u);
    EXPECT_FLOAT_EQ(marginCall->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(marginCall->values[1].f32, 0.0F);
    EXPECT_FLOAT_EQ(marginCall->values[2].f32, 0.0F);
    EXPECT_FLOAT_EQ(marginCall->values[3].f32, TOGGLE_LABEL_SPACING);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_handle_toggle_change_event_from_toggle_node)
{
    InitializeTheme(ThemeMode::LIGHT);
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedCreateNodeOverride createNodeOverride(nativeNodeApi);
    g_uniqueNodeHandleSeed = 0;
    nativeNodeApi->createNode = CreateUniqueTestNodeHandle;

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "isOn": false,
            "onChange": [{"call": "dispatchEvent", "args": {"eventName": "toggleChanged"}}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);
    ASSERT_NE(toggle->GetToggleNodeForTest(), nullptr);
    EXPECT_FALSE(toggle->GetIsOnForTest());

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;

    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, toggle->GetToggleNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TOGGLE_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(toggle->GetToggleNodeForTest(), &fakeEvent));
    EXPECT_TRUE(toggle->GetIsOnForTest());
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_track_toggle_state_without_change_listener)
{
    InitializeTheme(ThemeMode::LIGHT);
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedCreateNodeOverride createNodeOverride(nativeNodeApi);
    g_uniqueNodeHandleSeed = 0;
    nativeNodeApi->createNode = CreateUniqueTestNodeHandle;

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "label": "消息提醒",
            "isOn": false
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);
    ASSERT_NE(toggle->GetToggleNodeForTest(), nullptr);
    EXPECT_FALSE(toggle->GetIsOnForTest());

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;

    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, toggle->GetToggleNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TOGGLE_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(toggle->GetToggleNodeForTest(), &fakeEvent));
    EXPECT_TRUE(toggle->GetIsOnForTest());
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_reset_only_invalid_toggle_delta_style_and_preserve_missing_styles)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "styles": {
                "selectedColor": "#112233",
                "unSelectedColor": "#445566",
                "switchPointColor": "#778899"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);
    EXPECT_EQ(toggle->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(toggle->GetUnSelectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(toggle->GetSwitchPointColorForTest(), 0xFF778899u);

    auto invalidColor = JsonAdapter::Parse("true");
    ASSERT_NE(invalidColor, nullptr);
    std::static_pointer_cast<Component>(toggle)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("selectedColor"), invalidColor->GetRoot());

    EXPECT_EQ(toggle->GetSelectedColorForTest(), TOGGLE_LIGHT_SELECTED_COLOR);
    EXPECT_EQ(toggle->GetUnSelectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(toggle->GetSwitchPointColorForTest(), 0xFF778899u);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_reset_only_invalid_radio_delta_style_and_preserve_missing_styles)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "radio",
            "component": "Radio",
            "styles": {
                "checkedBackgroundColor": "#112233",
                "unCheckedBorderColor": "#445566",
                "indicatorColor": "#778899"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("radio"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetCheckedBackgroundColorForTest(), 0xFF112233u);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFF445566u);
    EXPECT_EQ(radio->GetIndicatorColorForTest(), 0xFF778899u);

    auto invalidColor = JsonAdapter::Parse("true");
    ASSERT_NE(invalidColor, nullptr);
    std::static_pointer_cast<Component>(radio)->OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("checkedBackgroundColor"), invalidColor->GetRoot());

    EXPECT_EQ(radio->GetCheckedBackgroundColorForTest(), RADIO_LIGHT_CHECKED_BACKGROUND_COLOR);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFF445566u);
    EXPECT_EQ(radio->GetIndicatorColorForTest(), 0xFF778899u);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_apply_dark_theme_defaults_for_radio_toggle_textinput_and_button)
{
    InitializeTheme(ThemeMode::DARK);

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio", "Toggle", "TextInput", "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [
            {"id": "radio", "component": "Radio"},
            {"id": "toggle", "component": "Toggle"},
            {"id": "input", "component": "TextInput"},
            {"id": "button", "component": "Button", "label": "ok"}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("radio"));
    ASSERT_NE(radio, nullptr);
    EXPECT_EQ(radio->GetCheckedBackgroundColorForTest(), RADIO_DARK_CHECKED_BACKGROUND_COLOR);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), RADIO_DEFAULT_UNCHECKED_BORDER_COLOR);
    EXPECT_EQ(radio->GetIndicatorColorForTest(), RADIO_DEFAULT_INDICATOR_COLOR);

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);
    EXPECT_EQ(toggle->GetSelectedColorForTest(), TOGGLE_DARK_SELECTED_COLOR);
    EXPECT_EQ(toggle->GetUnSelectedColorForTest(), TOGGLE_DARK_UNSELECTED_COLOR);
    EXPECT_EQ(toggle->GetSwitchPointColorForTest(), TOGGLE_DARK_SWITCH_POINT_COLOR);

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("input"));
    ASSERT_NE(input, nullptr);
    ArkUI_NodeHandle inputNode = input->GetNativeView();
    ASSERT_NE(inputNode, nullptr);
    const auto* fontColorCall = FindLastSetAttributeRecord(mockArkUIPtr_, inputNode, NODE_FONT_COLOR);
    const auto* placeholderColorCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, inputNode, NODE_TEXT_INPUT_PLACEHOLDER_COLOR);
    const auto* caretColorCall = FindLastSetAttributeRecord(mockArkUIPtr_, inputNode, NODE_TEXT_INPUT_CARET_COLOR);
    const auto* selectedBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, inputNode, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR);
    const auto* underlineColorCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, inputNode, NODE_TEXT_INPUT_UNDERLINE_COLOR);
    ASSERT_NE(fontColorCall, nullptr);
    ASSERT_NE(placeholderColorCall, nullptr);
    ASSERT_NE(caretColorCall, nullptr);
    ASSERT_NE(selectedBackgroundCall, nullptr);
    ASSERT_NE(underlineColorCall, nullptr);
    ASSERT_FALSE(fontColorCall->values.empty());
    ASSERT_FALSE(placeholderColorCall->values.empty());
    ASSERT_FALSE(caretColorCall->values.empty());
    ASSERT_FALSE(selectedBackgroundCall->values.empty());
    ASSERT_EQ(underlineColorCall->values.size(), 4u);
    EXPECT_EQ(fontColorCall->values[0].u32, TEXT_INPUT_DARK_FONT_COLOR);
    EXPECT_EQ(placeholderColorCall->values[0].u32, TEXT_INPUT_DARK_PLACEHOLDER_COLOR);
    EXPECT_EQ(caretColorCall->values[0].u32, TEXT_INPUT_DARK_CARET_COLOR);
    EXPECT_EQ(selectedBackgroundCall->values[0].u32, TEXT_INPUT_DARK_SELECTED_BACKGROUND_COLOR);
    EXPECT_EQ(underlineColorCall->values[0].u32, TEXT_INPUT_DARK_UNDERLINE_COLOR);
    EXPECT_EQ(underlineColorCall->values[1].u32, TEXT_INPUT_DARK_UNDERLINE_COLOR);
    EXPECT_EQ(underlineColorCall->values[2].u32, TEXT_INPUT_DARK_UNDERLINE_COLOR);
    EXPECT_EQ(underlineColorCall->values[3].u32, TEXT_INPUT_DARK_UNDERLINE_COLOR);

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("button"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontColorForTest(), BUTTON_DARK_NORMAL_FONT_COLOR);
    EXPECT_FALSE(button->HasFontColorForTest());
    const auto* buttonBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(buttonBackgroundCall, nullptr);
    ASSERT_FALSE(buttonBackgroundCall->values.empty());
    EXPECT_EQ(buttonBackgroundCall->values[0].u32, BUTTON_DARK_NORMAL_BACKGROUND_COLOR);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_apply_light_theme_defaults_for_button_font_and_background)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "button",
            "component": "Button",
            "label": "ok"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("button"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontColorForTest(), BUTTON_LIGHT_NORMAL_FONT_COLOR);
    EXPECT_FALSE(button->HasFontColorForTest());
    const auto* buttonBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(buttonBackgroundCall, nullptr);
    ASSERT_FALSE(buttonBackgroundCall->values.empty());
    EXPECT_EQ(buttonBackgroundCall->values[0].u32, BUTTON_LIGHT_NORMAL_BACKGROUND_COLOR);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_resolve_grid_columns_template_by_breakpoint)
{
    struct Case {
        Breakpoint breakpoint;
        std::string expected;
    };

    const std::vector<Case> cases = {
        { Breakpoint::XS, "1fr 1fr" },
        { Breakpoint::SM, "1fr 1fr" },
        { Breakpoint::MD, "1fr 1fr 1fr" },
        { Breakpoint::LG, "1fr 1fr 1fr 1fr 1fr" },
        { Breakpoint::XL, "1fr 1fr 1fr 1fr 1fr" },
    };

    for (const auto& item : cases) {
        ThemeContext context;
        context.breakpoint = item.breakpoint;
        ExtendedGridTheme theme(context);
        EXPECT_EQ(theme.GetColumnsTemplate(), item.expected);
    }
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_resolve_list_lanes_by_breakpoint)
{
    struct Case {
        Breakpoint breakpoint;
        int32_t expected;
    };

    const std::vector<Case> cases = {
        { Breakpoint::XS, 1 },
        { Breakpoint::SM, 1 },
        { Breakpoint::MD, 2 },
        { Breakpoint::LG, 3 },
        { Breakpoint::XL, 3 },
    };

    for (const auto& item : cases) {
        ThemeContext context;
        context.breakpoint = item.breakpoint;
        ExtendedListTheme theme(context);
        EXPECT_EQ(theme.GetLanes(), item.expected);
    }
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_create_extended_grid_theme_from_theme_factory)
{
    ThemeContext context;
    context.breakpoint = Breakpoint::MD;

    auto theme = std::dynamic_pointer_cast<ExtendedGridTheme>(ThemeFactory::CreateTheme("Grid", context));

    ASSERT_NE(theme, nullptr);
    EXPECT_EQ(theme->GetColumnsTemplate(), "1fr 1fr 1fr");
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_fallback_to_default_grid_template_on_unknown_breakpoint)
{
    ThemeContext context;
    context.breakpoint = static_cast<Breakpoint>(-1);
    ExtendedGridTheme theme(context);
    EXPECT_EQ(theme.GetColumnsTemplate(), "1fr 1fr");

    ThemeContext updateContext;
    updateContext.breakpoint = Breakpoint::LG;
    theme.OnConfigChange(updateContext);
    EXPECT_EQ(theme.GetColumnsTemplate(), "1fr 1fr 1fr 1fr 1fr");
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_fallback_to_single_lane_on_unknown_breakpoint)
{
    ThemeContext context;
    context.breakpoint = static_cast<Breakpoint>(-1);
    ExtendedListTheme theme(context);
    EXPECT_EQ(theme.GetLanes(), 1);

    ThemeContext updateContext;
    updateContext.breakpoint = Breakpoint::MD;
    theme.OnConfigChange(updateContext);
    EXPECT_EQ(theme.GetLanes(), 2);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_validate_schema_type_mismatch_for_fontSize)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontSize": "not_a_number"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "btn.styles.fontSize"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_validate_schema_type_mismatch_for_minFontScale)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontScale": "not_number",
                "maxFontScale": false
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "btn.styles.minFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "btn.styles.maxFontScale"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_validate_schema_invalid_font_size_and_scale_ranges_on_update)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto initialMessage = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontSize": 12,
                "minFontScale": 0.5,
                "maxFontScale": 1.5
            }
        }]
    })");
    ASSERT_NE(initialMessage, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(initialMessage->GetRoot()));

    auto invalidUpdate = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontSize": 0,
                "minFontScale": 1.5,
                "maxFontScale": 0.5
            }
        }]
    })");
    ASSERT_NE(invalidUpdate, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(invalidUpdate->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.minFontSize"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.minFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.maxFontScale"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_dfx_invalid_dynamic_font_size_and_scale_ranges)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto dataModel = JsonAdapter::Parse(R"({
        "value": {
            "buttonMinFontSize": 0,
            "buttonMinFontScale": 1.5,
            "buttonMaxFontScale": 0.5
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "minFontSize": { "path": "/buttonMinFontSize" },
                "minFontScale": { "path": "/buttonMinFontScale" },
                "maxFontScale": { "path": "/buttonMaxFontScale" }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.minFontSize"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.minFontScale"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "btn.styles.maxFontScale"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_button_validate_schema_type_mismatch_for_fontScaleMode_non_string)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontScaleMode": 123
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "btn.styles.fontScaleMode"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_validate_schema_invalid_fontScaleMode_value)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "fontScaleMode": "autoScale"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "input.styles.fontScaleMode"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_validate_schema_underline_color_type_mismatch)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "underlineColor": "not_a_color"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.underlineColor"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_textinput_validate_schema_word_break_type_mismatch)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "wordBreak": 123
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "input.styles.wordBreak"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_validate_schema_non_object_mark)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb",
            "component": "Checkbox",
            "label": "test",
            "styles": {
                "mark": "not_an_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "cb.styles.mark"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_mark_non_number_size_and_strokeWidth)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb",
            "component": "Checkbox",
            "label": "test",
            "styles": {
                "mark": {
                    "size": "big",
                    "strokeWidth": false
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "cb.styles.mark.size"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "cb.styles.mark.strokeWidth"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_shape_type_mismatch)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb",
            "component": "Checkbox",
            "label": "test",
            "styles": {
                "shape": 42
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "cb.styles.shape"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_group_invalid_checkboxShape)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "grp",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "checkboxShape": "hexagon"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "grp.styles.checkboxShape"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_group_non_string_checkboxShape)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "grp",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "checkboxShape": 42
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "grp.styles.checkboxShape"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_report_checkbox_group_mark_non_object)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "grp",
            "component": "CheckboxGroup",
            "group": "g1",
            "styles": {
                "mark": "not_object"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "grp.styles.mark"), 1U);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_preserve_button_font_color_on_config_change_when_user_overridden)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok",
            "styles": {
                "fontColor": "#FF0000",
                "backgroundColor": "#00FF00"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("btn"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetFontColorForTest(), 0xFFFF0000u);
    EXPECT_TRUE(button->HasFontColorForTest());
    const auto* initialBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(initialBackgroundCall, nullptr);
    ASSERT_FALSE(initialBackgroundCall->values.empty());
    EXPECT_EQ(initialBackgroundCall->values[0].u32, 0xFF00FF00u);

    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::DARK);
    button->OnConfigChange(themeManager->GetContext());

    EXPECT_EQ(button->GetFontColorForTest(), 0xFFFF0000u);
    const auto* updatedBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(updatedBackgroundCall, nullptr);
    ASSERT_FALSE(updatedBackgroundCall->values.empty());
    EXPECT_EQ(updatedBackgroundCall->values[0].u32, 0xFF00FF00u);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_update_button_default_background_on_config_change_when_not_overridden)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn",
            "component": "Button",
            "label": "ok"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto button = std::dynamic_pointer_cast<ExtendedButtonComponent>(slot_.FindComponentById("btn"));
    ASSERT_NE(button, nullptr);
    const auto* initialBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(initialBackgroundCall, nullptr);
    ASSERT_FALSE(initialBackgroundCall->values.empty());
    EXPECT_EQ(initialBackgroundCall->values[0].u32, BUTTON_LIGHT_NORMAL_BACKGROUND_COLOR);

    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::DARK);
    button->OnConfigChange(themeManager->GetContext());

    const auto* updatedBackgroundCall =
        FindLastSetAttributeRecord(mockArkUIPtr_, button->GetNativeView(), NODE_BACKGROUND_COLOR);
    ASSERT_NE(updatedBackgroundCall, nullptr);
    ASSERT_FALSE(updatedBackgroundCall->values.empty());
    EXPECT_EQ(updatedBackgroundCall->values[0].u32, BUTTON_DARK_NORMAL_BACKGROUND_COLOR);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_preserve_toggle_colors_on_config_change_when_overridden)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "toggle",
            "component": "Toggle",
            "styles": {
                "selectedColor": "#112233",
                "unSelectedColor": "#445566",
                "switchPointColor": "#778899"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto toggle = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("toggle"));
    ASSERT_NE(toggle, nullptr);

    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(toggle->GetSelectedColorForTest(), 0xFF112233u);
    EXPECT_EQ(toggle->GetUnSelectedColorForTest(), 0xFF445566u);
    EXPECT_EQ(toggle->GetSwitchPointColorForTest(), 0xFF778899u);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_preserve_radio_colors_on_config_change_when_overridden)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "radio",
            "component": "Radio",
            "styles": {
                "checkedBackgroundColor": "#112233",
                "unCheckedBorderColor": "#445566",
                "indicatorColor": "#778899"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("radio"));
    ASSERT_NE(radio, nullptr);

    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(radio->GetCheckedBackgroundColorForTest(), 0xFF112233u);
    EXPECT_EQ(radio->GetUncheckedBackgroundColorForTest(), 0xFF445566u);
    EXPECT_EQ(radio->GetIndicatorColorForTest(), 0xFF778899u);
}

TEST_F(ExtendedThemeDefaultsTest, L0_should_preserve_textinput_colors_on_config_change_when_overridden)
{
    InitializeTheme(ThemeMode::LIGHT);
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "input",
            "component": "TextInput",
            "text": "hello",
            "styles": {
                "caretColor": "#778899",
                "selectedBackgroundColor": "#AABBCC"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto input = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("input"));
    ASSERT_NE(input, nullptr);

    auto themeManager = slot_.GetThemeManager();
    ASSERT_NE(themeManager, nullptr);
    themeManager->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(input->GetCaretColorForTest(), 0xFF778899u);
    EXPECT_EQ(input->GetSelectedBackgroundColorForTest(), 0xFFAABBCCu);
}
