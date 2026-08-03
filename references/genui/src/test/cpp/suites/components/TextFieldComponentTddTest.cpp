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

#include "A2UIComponentTddTestHelper.h"

#define private public
#include "components/A2UI/textfield/TextFieldComponent.h"
#undef private
#include "functions/WarningDispatchBridge.h"

using namespace NativeModule;

class TextFieldComponentTddTest : public A2UIComponentTddTest {};

namespace {

constexpr char SCHEMA_ERROR_CODE_INVALID_VALUE[] = "ERROR_CODE_INVALID_VALUE";

struct DispatchCallbacks {
    napi_env env = nullptr;
    napi_value warningCallback = nullptr;
};

DispatchCallbacks RegisterDispatchCallbacks(MockNapiProvider* mockNapi)
{
    DispatchCallbacks callbacks;
    if (mockNapi == nullptr) {
        return callbacks;
    }

    callbacks.env = reinterpret_cast<napi_env>(0x1301);
    mockNapi->CreateFunction(
        callbacks.env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &callbacks.warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(callbacks.env, callbacks.warningCallback);

    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionRecv_ = nullptr;
    mockNapi->lastCallFunctionFunc_ = nullptr;
    mockNapi->lastCallFunctionArgs_.clear();
    return callbacks;
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
    auto propIt = objectIt->second.find(key);
    if (propIt == objectIt->second.end()) {
        return nullptr;
    }
    return propIt->second;
}

std::string GetStringValue(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto it = mockNapi->stringValues_.find(value);
    return it == mockNapi->stringValues_.end() ? "" : it->second;
}

int32_t GetInt32Value(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return 0;
    }
    auto it = mockNapi->numberValues_.find(value);
    return it == mockNapi->numberValues_.end() ? 0 : static_cast<int32_t>(it->second);
}

class TextFieldComponentProbe : public TextFieldComponent {
public:
    using TextFieldComponent::OnDataUpdate;

    void InvokeOnAttachToParent()
    {
        OnAttachToParent();
    }
};

std::vector<ArkUI_NodeHandle> g_textFieldCreateNodeResults;
size_t g_textFieldCreateNodeIndex = 0;

ArkUI_NodeHandle CreateTextFieldNodeFromSequence(ArkUI_NodeType type)
{
    if (g_textFieldCreateNodeIndex < g_textFieldCreateNodeResults.size()) {
        return g_textFieldCreateNodeResults[g_textFieldCreateNodeIndex++];
    }
    ++g_textFieldCreateNodeIndex;
    return TrackCreateNode(type);
}

void UseTextFieldCreateNodeSequence(ArkUI_NativeNodeAPI_1* api, const std::vector<ArkUI_NodeHandle>& results)
{
    g_textFieldCreateNodeResults = results;
    g_textFieldCreateNodeIndex = 0;
    api->createNode = CreateTextFieldNodeFromSequence;
}

int32_t CountTextFieldAddChildCalls(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.addChildCalls) {
        if (call.first == parent && call.second == child) {
            ++count;
        }
    }
    return count;
}

int32_t CountTextFieldInsertChildAtCalls(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.insertChildAtCalls) {
        if (std::get<0>(call) == parent && std::get<1>(call) == child && std::get<2>(call) == index) {
            ++count;
        }
    }
    return count;
}

} // namespace

TEST_F(TextFieldComponentTddTest, L0_textfield_should_create_column_label_and_error_nodes)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ASSERT_NE(textField, nullptr);
    ArkUI_NodeHandle columnNode = textField->GetNativeView();
    ArkUI_NodeHandle labelNode = FindCreatedNode(ARKUI_NODE_TEXT, 0);
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);

    EXPECT_EQ(textField->GetType(), "TextField");
    EXPECT_EQ(columnNode, FindCreatedNode(ARKUI_NODE_COLUMN));
    ASSERT_NE(labelNode, nullptr);
    ASSERT_NE(errorNode, nullptr);
    EXPECT_FALSE(HasAddChildCall(columnNode, labelNode));
    EXPECT_FALSE(HasAddChildCall(columnNode, errorNode));

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    EXPECT_TRUE(HasAddChildCall(columnNode, labelNode));
    EXPECT_TRUE(HasAddChildCall(columnNode, errorNode));
    ExpectI32Attribute(columnNode, NODE_COLUMN_ALIGN_ITEMS, ARKUI_HORIZONTAL_ALIGNMENT_START);
    ExpectI32Attribute(columnNode, NODE_COLUMN_JUSTIFY_CONTENT, ARKUI_FLEX_ALIGNMENT_START);
    ExpectF32Attribute(labelNode, NODE_FONT_SIZE, 14.0F);
    ExpectF32Attribute(errorNode, NODE_FONT_SIZE, 12.0F);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_not_attach_internal_nodes_twice)
{
    auto textField = std::make_shared<TextFieldComponentProbe>();
    ArkUI_NodeHandle columnNode = textField->GetNativeView();
    ArkUI_NodeHandle labelNode = FindCreatedNode(ARKUI_NODE_TEXT, 0);
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);
    ASSERT_NE(columnNode, nullptr);
    ASSERT_NE(labelNode, nullptr);
    ASSERT_NE(errorNode, nullptr);

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);
    ASSERT_EQ(CountTextFieldAddChildCalls(columnNode, labelNode), 1);
    ASSERT_EQ(CountTextFieldAddChildCalls(columnNode, errorNode), 1);

    textField->InvokeOnAttachToParent();

    EXPECT_EQ(CountTextFieldAddChildCalls(columnNode, labelNode), 1);
    EXPECT_EQ(CountTextFieldAddChildCalls(columnNode, errorNode), 1);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_insert_input_node_when_descriptor_applied_after_attach)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ArkUI_NodeHandle columnNode = textField->GetNativeView();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    auto descriptor =
        ParseJson(R"({"id":"late","component":"TextField","label":"Late","variant":"number","value":"7"})");
    ASSERT_NE(descriptor, nullptr);
    textField->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle inputNode = FindCreatedNode(ARKUI_NODE_TEXT_INPUT);
    ASSERT_NE(inputNode, nullptr);
    EXPECT_EQ(CountTextFieldInsertChildAtCalls(columnNode, inputNode, 1), 1);

    auto updateDescriptor =
        ParseJson(R"({"id":"late","component":"TextField","label":"Late","variant":"obscured","value":"8"})");
    ASSERT_NE(updateDescriptor, nullptr);
    textField->ApplyDescriptor(updateDescriptor->GetRoot());

    EXPECT_EQ(CountTextFieldInsertChildAtCalls(columnNode, inputNode, 1), 1);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_skip_internal_init_and_attach_when_native_view_is_missing)
{
    UseTextFieldCreateNodeSequence(api_, { nullptr });

    auto textField = std::make_shared<TextFieldComponent>();
    EXPECT_EQ(textField->GetNativeView(), nullptr);
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    EXPECT_TRUE(g_tracker.addChildCalls.empty());
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_skip_attach_when_label_node_is_missing)
{
    ArkUI_NodeHandle columnNode = reinterpret_cast<ArkUI_NodeHandle>(0x830010);
    ArkUI_NodeHandle errorNode = reinterpret_cast<ArkUI_NodeHandle>(0x830011);
    UseTextFieldCreateNodeSequence(api_, { columnNode, nullptr, errorNode });

    auto textField = std::make_shared<TextFieldComponent>();
    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    EXPECT_FALSE(HasAddChildCall(columnNode, errorNode));
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_apply_number_variant_label_value_and_validation)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ArkUI_NodeHandle labelNode = FindCreatedNode(ARKUI_NODE_TEXT, 0);
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);
    auto descriptor = ParseJson(R"({"id":"age","component":"TextField","label":"Age","variant":"number",)"
                                R"("validationRegexp":"^[0-9]+$","value":"42"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle inputNode = FindCreatedNode(ARKUI_NODE_TEXT_INPUT);
    ASSERT_NE(inputNode, nullptr);
    ExpectStringAttribute(labelNode, NODE_TEXT_CONTENT, "Age");
    ExpectI32Attribute(inputNode, NODE_TEXT_INPUT_TYPE, ARKUI_TEXTINPUT_TYPE_NUMBER);
    ExpectStringAttribute(inputNode, NODE_TEXT_INPUT_TEXT, "42");
    ExpectStringAttribute(errorNode, NODE_TEXT_CONTENT, "");
    EXPECT_FALSE(HasInsertChildAtCall(textField->GetNativeView(), inputNode, 1));

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    EXPECT_TRUE(HasAddChildCall(textField->GetNativeView(), labelNode));
    EXPECT_TRUE(HasAddChildCall(textField->GetNativeView(), inputNode));
    EXPECT_TRUE(HasAddChildCall(textField->GetNativeView(), errorNode));
    EXPECT_TRUE(HasRegisterNodeEventCall(inputNode, NODE_TEXT_INPUT_ON_CHANGE));
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_apply_obscured_variant_as_password_input)
{
    auto textField = std::make_shared<TextFieldComponent>();
    auto descriptor = ParseJson(
        R"({"id":"password","component":"TextField","label":"Password","variant":"obscured","value":"secret"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle inputNode = FindCreatedNode(ARKUI_NODE_TEXT_INPUT);
    ASSERT_NE(inputNode, nullptr);
    ExpectI32Attribute(inputNode, NODE_TEXT_INPUT_TYPE, ARKUI_TEXTINPUT_TYPE_PASSWORD);
    ExpectStringAttribute(inputNode, NODE_TEXT_INPUT_TEXT, "secret");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_apply_long_text_variant_with_text_area)
{
    auto textField = std::make_shared<TextFieldComponent>();
    auto descriptor =
        ParseJson(R"({"id":"bio","component":"TextField","label":"Bio","variant":"longText","value":"Long value"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle textAreaNode = FindCreatedNode(ARKUI_NODE_TEXT_AREA);
    ASSERT_NE(textAreaNode, nullptr);
    ExpectStringAttribute(textAreaNode, NODE_TEXT_AREA_TEXT, "Long value");

    auto parent = std::make_shared<BasicLeafComponent>(api_->createNode(ARKUI_NODE_COLUMN));
    parent->AddChild(textField);

    EXPECT_TRUE(HasAddChildCall(textField->GetNativeView(), textAreaNode));
    EXPECT_TRUE(HasRegisterNodeEventCall(textAreaNode, NODE_TEXT_AREA_ON_CHANGE));
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_fallback_invalid_variant_to_normal_text_input)
{
    auto textField = std::make_shared<TextFieldComponent>();
    auto descriptor =
        ParseJson(R"({"id":"name","component":"TextField","label":"Name","variant":"unknown","value":"Alice"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ArkUI_NodeHandle inputNode = FindCreatedNode(ARKUI_NODE_TEXT_INPUT);
    ASSERT_NE(inputNode, nullptr);
    ExpectI32Attribute(inputNode, NODE_TEXT_INPUT_TYPE, ARKUI_TEXTINPUT_TYPE_NORMAL);
    ExpectStringAttribute(inputNode, NODE_TEXT_INPUT_TEXT, "Alice");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_show_error_when_validation_regexp_fails)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);
    auto descriptor = ParseJson(R"({"id":"age","component":"TextField","variant":"number",)"
                                R"("validationRegexp":"^[0-9]+$","value":"abc"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(errorNode, NODE_TEXT_CONTENT, "Input does not match validationRegexp");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_show_error_when_validation_regexp_is_invalid)
{
    auto textField = std::make_shared<TextFieldComponent>();
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);
    auto descriptor = ParseJson(R"({"id":"age","component":"TextField","validationRegexp":"[","value":"abc"})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());

    ExpectStringAttribute(errorNode, NODE_TEXT_CONTENT, "Input does not match validationRegexp");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_dispatch_schema_warning_when_validation_regexp_is_invalid)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    auto textField = std::make_shared<TextFieldComponent>();
    textField->SetRenderId(COMPONENT_TDD_RENDER_ID);
    textField->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    textField->SetComponentId("age");

    auto descriptor =
        ParseJson(R"({"id":"age","component":"TextField","label":"Age","validationRegexp":"[","value":"abc"})");
    ASSERT_NE(descriptor, nullptr);
    textField->ApplyDescriptor(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "age");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "age.validationRegexp");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "TextField");
    std::string message = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(message.find("validationRegexp"), std::string::npos);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_dispatch_schema_warning_when_validation_regexp_has_syntax_error)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    auto textField = std::make_shared<TextFieldComponent>();
    textField->SetRenderId(COMPONENT_TDD_RENDER_ID);
    textField->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    textField->SetComponentId("age");

    auto descriptor =
        ParseJson(R"({"id":"age","component":"TextField","label":"Age","validationRegexp":"(abc","value":"abc"})");
    ASSERT_NE(descriptor, nullptr);
    textField->ApplyDescriptor(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "age");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "age.validationRegexp");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "TextField");
    std::string message = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(message.find("validationRegexp"), std::string::npos);
}

TEST_F(
    TextFieldComponentTddTest, L0_textfield_should_dispatch_schema_warning_with_default_path_when_component_id_is_empty)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    auto textField = std::make_shared<TextFieldComponent>();
    textField->SetRenderId(COMPONENT_TDD_RENDER_ID);
    textField->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    textField->SetValidationRegexp("[");
    textField->SetValueText("abc");

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "validationRegexp");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "TextField");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_dispatch_invalid_regex_warning_only_once_until_regexp_changes)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    auto textField = std::make_shared<TextFieldComponent>();
    textField->SetRenderId(COMPONENT_TDD_RENDER_ID);
    textField->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    textField->SetComponentId("age");
    textField->SetValidationRegexp("[");
    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    textField->SetValueText("abc");
    textField->SetValueText("abcd");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    textField->SetValidationRegexp("(abc");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 2U);
}

TEST_F(TextFieldComponentTddTest,
    L0_textfield_should_not_dispatch_schema_warning_when_invalid_regexp_and_render_id_is_negative)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    auto textField = std::make_shared<TextFieldComponent>();
    textField->SetRenderId(-1);
    textField->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    textField->SetComponentId("age");

    textField->SetValidationRegexp("[");
    textField->SetValueText("abc");

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_parse_checks_and_refresh_error_on_check_update)
{
    auto textField = std::make_shared<TextFieldComponentProbe>();
    ArkUI_NodeHandle errorNode = FindCreatedNode(ARKUI_NODE_TEXT, 1);
    auto descriptor =
        ParseJson(R"({"id":"guardedField","component":"TextField","value":"",)"
                  R"("checks":[{"condition":{"call":"required","args":{"value":""}},"message":"required"}]})");
    ASSERT_NE(descriptor, nullptr);

    textField->ApplyDescriptor(descriptor->GetRoot());
    ExpectStringAttribute(errorNode, NODE_TEXT_CONTENT, "required");

    auto updateValue = JsonAdapter::CreateString("refresh");
    ASSERT_NE(updateValue, nullptr);
    textField->OnDataUpdate("__checks_dep_1", updateValue->GetRoot());

    ExpectStringAttribute(errorNode, NODE_TEXT_CONTENT, "required");
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_unregister_and_dispose_internal_nodes_on_destroy)
{
    ArkUI_NodeHandle textAreaNode = nullptr;
    ArkUI_NodeHandle inputNode = nullptr;
    {
        auto textField = std::make_shared<TextFieldComponent>();
        auto descriptor = ParseJson(R"({"id":"cleanup","component":"TextField","variant":"longText","value":"draft"})");
        ASSERT_NE(descriptor, nullptr);
        textField->ApplyDescriptor(descriptor->GetRoot());
        textAreaNode = FindCreatedNode(ARKUI_NODE_TEXT_AREA);
        ASSERT_NE(textAreaNode, nullptr);
        auto inputDescriptor =
            ParseJson(R"({"id":"cleanup","component":"TextField","variant":"shortText","value":"draft"})");
        ASSERT_NE(inputDescriptor, nullptr);
        textField->ApplyDescriptor(inputDescriptor->GetRoot());
        inputNode = FindCreatedNode(ARKUI_NODE_TEXT_INPUT);
        ASSERT_NE(inputNode, nullptr);
    }

    EXPECT_TRUE(HasUnregisterNodeEventCall(textAreaNode, NODE_TEXT_AREA_ON_CHANGE));
    EXPECT_TRUE(HasUnregisterNodeEventCall(inputNode, NODE_TEXT_INPUT_ON_CHANGE));
    EXPECT_TRUE(HasRemoveNodeEventReceiverCall(textAreaNode));
    EXPECT_TRUE(HasRemoveNodeEventReceiverCall(inputNode));
    EXPECT_TRUE(HasDisposedNode(textAreaNode));
    EXPECT_TRUE(HasDisposedNode(inputNode));
    EXPECT_GE(g_tracker.unregisterNodeEventCount, 2);
    EXPECT_GE(g_tracker.removeNodeEventReceiverCount, 3);
    EXPECT_GE(g_tracker.disposeNodeCount, 4);
}

TEST_F(TextFieldComponentTddTest, L0_textfield_should_return_theme_when_surface_context_is_available)
{
    auto textField = std::make_shared<TextFieldComponent>();
    PrepareThemeContext(*textField);

    EXPECT_NE(textField->GetTheme(), nullptr);
}
