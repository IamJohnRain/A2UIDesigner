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
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "components/extended/ExtendedStackComponent.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/JsonAdapter.h"

#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

void RegisterDispatchCallbacks(MockNapiProvider* mockNapi)
{
    ASSERT_NE(mockNapi, nullptr);
    napi_env env = reinterpret_cast<napi_env>(0x1601);
    napi_value warningCallback = nullptr;
    mockNapi->CreateFunction(env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(env, warningCallback);
    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionRecv_ = nullptr;
    mockNapi->lastCallFunctionFunc_ = nullptr;
    mockNapi->lastCallFunctionArgs_.clear();
    mockNapi->callFunctionArgsHistory_.clear();
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
    return propertyIt == objectIt->second.end() ? nullptr : propertyIt->second;
}

std::string GetStringValue(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto it = mockNapi->stringValues_.find(value);
    return it == mockNapi->stringValues_.end() ? "" : it->second;
}

size_t CountWarningRequests(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    if (mockNapi == nullptr) {
        return 0U;
    }

    size_t count = 0U;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }
        napi_value request = args.front();
        std::string warningCode = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "code"));
        std::string warningPath = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "path"));
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeForNode(
    const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto it = provider.setAttributeRecords_.rbegin(); it != provider.setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle == node && it->attribute == attribute) {
            return &(*it);
        }
    }
    return nullptr;
}

ArkUINodeApiAdapter CreateNodeApiAdapter(ExtendedStackComponent& component)
{
    return ArkUINodeApiAdapter([&component]() { return component.GetNativeView(); },
        [&component]() { return component.GetComponentId(); },
        [&component](
            float top, float right, float bottom, float left) { component.SetMargin(top, right, bottom, left); },
        [&component]() { component.ResetCommonMargin(); },
        [&component](const std::function<void()>& onClick) { component.RegisterOnClick(onClick); });
}

class TestableExtendedStackComponent : public ExtendedStackComponent {
public:
    using ExtendedComponent::SetApplyingStyleDeltaUpdateForTest;

    void CallApplyPrivateAttributes(const JsonValue& descriptor)
    {
        ApplyPrivateAttributes(descriptor);
    }

    void CallValidateComponentDescriptorSchema(const JsonValue& descriptor)
    {
        ValidateComponentDescriptorSchema(descriptor);
    }

    void CallValidateComponentSpecificStylesSchema(const JsonValue& styles)
    {
        ValidateComponentSpecificStylesSchema(styles);
    }

    void CallValidateStyleEnumProperty(
        const JsonValue& styles, const std::string& styleName, std::initializer_list<const char*> allowedValues)
    {
        ValidateStyleEnumProperty(styles, styleName, allowedValues);
    }

    void CallValidateStyleStringProperty(const JsonValue& styles, const std::string& styleName)
    {
        ValidateStyleStringProperty(styles, styleName);
    }

    void CallValidateStyleNumberProperty(const JsonValue& styles, const std::string& styleName, double minimumValue)
    {
        ValidateStyleNumberProperty(styles, styleName, minimumValue);
    }

    void CallReportStyleTypeMismatch(const std::string& propertyPath, const std::string& expectedType)
    {
        ReportStyleTypeMismatch(propertyPath, expectedType);
    }

    void CallReportStyleInvalidValue(const std::string& propertyPath)
    {
        ReportStyleInvalidValue(propertyPath);
    }

    bool CallIsDynamicValueDescriptor(const JsonValue& value) const
    {
        return IsDynamicValueDescriptor(value);
    }

    void CallApplyComponentSpecificStyles(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    PropertyDeclaration CallGetPrivatePropertyDeclaration(const std::string& propertyName)
    {
        return GetPrivatePropertyDeclaration(propertyName);
    }

    bool CallExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildren(childList, surfaceSlot, childIds);
    }
};

class ExtendedStackComponentTddTest : public A2UITest {};

TEST_F(ExtendedStackComponentTddTest, should_return_stack_type_and_apply_default_center_alignment)
{
    TestableExtendedStackComponent component;
    EXPECT_EQ(component.GetType(), "Stack");

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> emptyDescriptor = ParseJson(R"({})");
    ASSERT_NE(emptyDescriptor, nullptr);
    component.CallApplyPrivateAttributes(emptyDescriptor->GetRoot());

    const auto* alignRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_STACK_ALIGN_CONTENT);
    ASSERT_NE(alignRecord, nullptr);
    ASSERT_FALSE(alignRecord->values.empty());
    EXPECT_EQ(alignRecord->values[0].i32, ARKUI_ALIGNMENT_CENTER);
}

TEST_F(ExtendedStackComponentTddTest, should_report_stack_descriptor_and_style_schema_warnings)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    TestableExtendedStackComponent component;
    component.SetRenderId(1);
    component.SetSurfaceId("surface_stack");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidChildrenDescriptor = ParseJson(R"({
        "children": ["first", "", 3]
    })");
    ASSERT_NE(invalidChildrenDescriptor, nullptr);
    component.CallValidateComponentDescriptorSchema(invalidChildrenDescriptor->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "root.children[1]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.children[2]"), 1U);

    std::unique_ptr<JsonAdapter> typeMismatchStyles = ParseJson(R"({
        "alignContent": 1
    })");
    ASSERT_NE(typeMismatchStyles, nullptr);
    component.CallValidateComponentSpecificStylesSchema(typeMismatchStyles->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.alignContent"), 1U);

    std::unique_ptr<JsonAdapter> invalidValueStyles = ParseJson(R"({
        "alignContent": ""
    })");
    ASSERT_NE(invalidValueStyles, nullptr);
    component.CallValidateComponentSpecificStylesSchema(invalidValueStyles->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.alignContent"), 1U);
}

TEST_F(ExtendedStackComponentTddTest, should_map_stack_align_content_values_and_fallback_to_center)
{
    const std::vector<std::pair<const char*, int32_t>> cases = {
        { "topStart", ARKUI_ALIGNMENT_TOP_START },
        { "top", ARKUI_ALIGNMENT_TOP },
        { "topEnd", ARKUI_ALIGNMENT_TOP_END },
        { "start", ARKUI_ALIGNMENT_START },
        { "center", ARKUI_ALIGNMENT_CENTER },
        { "end", ARKUI_ALIGNMENT_END },
        { "bottomStart", ARKUI_ALIGNMENT_BOTTOM_START },
        { "bottom", ARKUI_ALIGNMENT_BOTTOM },
        { "bottomEnd", ARKUI_ALIGNMENT_BOTTOM_END },
        { "unexpected", ARKUI_ALIGNMENT_CENTER },
    };

    TestableExtendedStackComponent component;
    for (const auto& [alignContent, expectedAlignment] : cases) {
        std::unique_ptr<JsonAdapter> styles = ParseJson(std::string(R"({"alignContent":")") + alignContent + R"("})");
        ASSERT_NE(styles, nullptr);

        mockArkUIPtr_->setAttributeRecords_.clear();
        component.CallApplyComponentSpecificStyles(styles->GetRoot());

        const auto* alignRecord =
            FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_STACK_ALIGN_CONTENT);
        ASSERT_NE(alignRecord, nullptr);
        ASSERT_FALSE(alignRecord->values.empty());
        EXPECT_EQ(alignRecord->values[0].i32, expectedAlignment);
    }
}

TEST_F(ExtendedStackComponentTddTest, should_skip_reset_during_delta_update_when_align_content_is_missing)
{
    TestableExtendedStackComponent component;

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::CreateString("center");
    ASSERT_NE(nonObjectStyles, nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    component.CallApplyComponentSpecificStyles(nonObjectStyles->GetRoot());
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    std::unique_ptr<JsonAdapter> emptyStyles = ParseJson(R"({})");
    ASSERT_NE(emptyStyles, nullptr);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetApplyingStyleDeltaUpdateForTest(false);
    component.CallApplyComponentSpecificStyles(emptyStyles->GetRoot());
    const auto* fullUpdateRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_STACK_ALIGN_CONTENT);
    ASSERT_NE(fullUpdateRecord, nullptr);
    ASSERT_FALSE(fullUpdateRecord->values.empty());
    EXPECT_EQ(fullUpdateRecord->values[0].i32, ARKUI_ALIGNMENT_CENTER);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.CallApplyComponentSpecificStyles(emptyStyles->GetRoot());
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_STACK_ALIGN_CONTENT), nullptr);
}

TEST_F(ExtendedStackComponentTddTest, should_delegate_private_property_lookup_and_template_expansion)
{
    TestableExtendedStackComponent component;
    PropertyDeclaration declaration = component.CallGetPrivatePropertyDeclaration("unknownProperty");
    EXPECT_TRUE(declaration.name.empty());

    ChildListDescriptor childList;
    childList.type = ChildListType::TEMPLATE_PATH;
    childList.templateComponentId = "missingTemplate";
    childList.templatePath = "/items";

    SurfaceSlot surfaceSlot;
    surfaceSlot.SetSurfaceId("surface_stack");

    std::list<std::string> childIds = { "stale" };
    EXPECT_FALSE(component.CallExpandTemplateChildren(childList, surfaceSlot, childIds));
    EXPECT_TRUE(childIds.empty());
}

/**
 * @tc.name: 基类样式字符串与枚举校验
 * @tc.desc: 覆盖ExtendedComponent通用style helper的动态值、类型错误、空值和合法值分支。
 * @tc.type: FUNC
 */
TEST_F(ExtendedStackComponentTddTest, should_cover_base_style_string_and_enum_validation_helpers)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    TestableExtendedStackComponent component;
    component.SetRenderId(2);
    component.SetSurfaceId("surface_stack");
    component.SetComponentId("root");

    component.CallReportStyleTypeMismatch("", "string");
    component.CallReportStyleInvalidValue("");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);

    std::unique_ptr<JsonAdapter> dynamicEnumStyles = ParseJson(R"({"alignContent":{"path":"/align"}})");
    std::unique_ptr<JsonAdapter> enumTypeMismatchStyles = ParseJson(R"({"alignContent":1})");
    std::unique_ptr<JsonAdapter> enumEmptyStyles = ParseJson(R"({"alignContent":""})");
    std::unique_ptr<JsonAdapter> enumInvalidStyles = ParseJson(R"({"alignContent":"diagonal"})");
    std::unique_ptr<JsonAdapter> enumValidStyles = ParseJson(R"({"alignContent":"center"})");
    std::unique_ptr<JsonAdapter> stringTypeMismatchStyles = ParseJson(R"({"title":1})");
    std::unique_ptr<JsonAdapter> stringEmptyStyles = ParseJson(R"({"title":""})");
    std::unique_ptr<JsonAdapter> stringDynamicStyles = ParseJson(R"({"title":{"call":"resolveTitle"}})");
    std::unique_ptr<JsonAdapter> stringExpressionStyles = ParseJson(R"({"title":"{{ 'Header' }}"})");
    std::unique_ptr<JsonAdapter> stringValidStyles = ParseJson(R"({"title":"Header"})");
    std::unique_ptr<JsonAdapter> enumExpressionStyles = ParseJson(R"({"alignContent":"{{ 'center' }}"})");
    std::unique_ptr<JsonAdapter> numberExpressionStyles = ParseJson(R"({"gap":"{{ 4 }}"})");
    ASSERT_NE(dynamicEnumStyles, nullptr);
    ASSERT_NE(enumTypeMismatchStyles, nullptr);
    ASSERT_NE(enumEmptyStyles, nullptr);
    ASSERT_NE(enumInvalidStyles, nullptr);
    ASSERT_NE(enumValidStyles, nullptr);
    ASSERT_NE(stringTypeMismatchStyles, nullptr);
    ASSERT_NE(stringEmptyStyles, nullptr);
    ASSERT_NE(stringDynamicStyles, nullptr);
    ASSERT_NE(stringExpressionStyles, nullptr);
    ASSERT_NE(stringValidStyles, nullptr);
    ASSERT_NE(enumExpressionStyles, nullptr);
    ASSERT_NE(numberExpressionStyles, nullptr);

    component.CallValidateStyleEnumProperty(dynamicEnumStyles->GetRoot(), "alignContent", { "center", "topStart" });
    component.CallValidateStyleEnumProperty(enumExpressionStyles->GetRoot(), "alignContent", { "center", "topStart" });
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);

    component.CallValidateStyleEnumProperty(
        enumTypeMismatchStyles->GetRoot(), "alignContent", { "center", "topStart" });
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.alignContent"), 1U);

    component.CallValidateStyleEnumProperty(enumEmptyStyles->GetRoot(), "alignContent", { "center", "topStart" });
    component.CallValidateStyleEnumProperty(enumInvalidStyles->GetRoot(), "alignContent", { "center", "topStart" });
    component.CallValidateStyleEnumProperty(enumValidStyles->GetRoot(), "alignContent", { "center", "topStart" });
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.alignContent"), 2U);

    component.CallValidateStyleStringProperty(stringTypeMismatchStyles->GetRoot(), "title");
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.title"), 1U);

    component.CallValidateStyleStringProperty(stringEmptyStyles->GetRoot(), "title");
    component.CallValidateStyleStringProperty(stringDynamicStyles->GetRoot(), "title");
    component.CallValidateStyleStringProperty(stringExpressionStyles->GetRoot(), "title");
    component.CallValidateStyleStringProperty(stringValidStyles->GetRoot(), "title");
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.title"), 1U);

    component.CallValidateStyleNumberProperty(numberExpressionStyles->GetRoot(), "gap", 0.0);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.gap"), 0U);
}

/**
 * @tc.name: 基类样式数字与动态描述符校验
 * @tc.desc: 覆盖ExtendedComponent通用number helper的早退、动态描述符、类型错误和非法值分支。
 * @tc.type: FUNC
 */
TEST_F(ExtendedStackComponentTddTest, should_cover_base_style_number_and_dynamic_descriptor_helpers)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    TestableExtendedStackComponent component;
    component.SetRenderId(3);
    component.SetSurfaceId("surface_stack");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> pathDescriptor = ParseJson(R"({"path":"/gap"})");
    std::unique_ptr<JsonAdapter> callDescriptor = ParseJson(R"({"call":"resolveGap"})");
    std::unique_ptr<JsonAdapter> plainObjectDescriptor = ParseJson(R"({"value":1})");
    std::unique_ptr<JsonAdapter> dynamicNumberStyles = ParseJson(R"({"gap":{"call":"resolveGap"}})");
    std::unique_ptr<JsonAdapter> numberTypeMismatchStyles = ParseJson(R"({"gap":"wide"})");
    std::unique_ptr<JsonAdapter> numberInvalidStyles = ParseJson(R"({"gap":-1})");
    std::unique_ptr<JsonAdapter> numberValidStyles = ParseJson(R"({"gap":3})");
    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::CreateString("center");
    ASSERT_NE(pathDescriptor, nullptr);
    ASSERT_NE(callDescriptor, nullptr);
    ASSERT_NE(plainObjectDescriptor, nullptr);
    ASSERT_NE(dynamicNumberStyles, nullptr);
    ASSERT_NE(numberTypeMismatchStyles, nullptr);
    ASSERT_NE(numberInvalidStyles, nullptr);
    ASSERT_NE(numberValidStyles, nullptr);
    ASSERT_NE(nonObjectStyles, nullptr);

    EXPECT_TRUE(component.CallIsDynamicValueDescriptor(pathDescriptor->GetRoot()));
    EXPECT_TRUE(component.CallIsDynamicValueDescriptor(callDescriptor->GetRoot()));
    EXPECT_FALSE(component.CallIsDynamicValueDescriptor(plainObjectDescriptor->GetRoot()));
    EXPECT_FALSE(component.CallIsDynamicValueDescriptor(nonObjectStyles->GetRoot()));

    component.CallValidateStyleNumberProperty(nonObjectStyles->GetRoot(), "gap", 0.0);
    component.CallValidateStyleNumberProperty(numberValidStyles->GetRoot(), "", 0.0);
    component.CallValidateStyleNumberProperty(dynamicNumberStyles->GetRoot(), "gap", 0.0);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);

    component.CallValidateStyleNumberProperty(numberTypeMismatchStyles->GetRoot(), "gap", 0.0);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.gap"), 1U);

    component.CallValidateStyleNumberProperty(numberInvalidStyles->GetRoot(), "gap", 0.0);
    component.CallValidateStyleNumberProperty(numberValidStyles->GetRoot(), "gap", 0.0);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.gap"), 1U);
}

TEST_F(ExtendedStackComponentTddTest, should_cover_local_helper_guard_branches)
{
    EXPECT_EQ(GetRequestProperty(mockNapiPtr_, nullptr, "code"), nullptr);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, nullptr), "");
    EXPECT_EQ(CountWarningRequests(nullptr, "ERROR_CODE_INVALID_VALUE", "root.styles.gap"), 0U);

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    mockNapiPtr_->callFunctionArgsHistory_.push_back({});
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "root.styles.gap"), 0U);

    TestableExtendedStackComponent component;
    auto adapter = CreateNodeApiAdapter(component);
    (void)adapter;
}

} // namespace
