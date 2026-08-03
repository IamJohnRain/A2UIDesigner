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
#include <memory>
#include <string>

#include "components/extended/ExtendedDividerComponent.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "SchemaWarningTestHelper.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

class TestableExtendedDividerCoverageComponent : public ExtendedDividerComponent {
public:
    using ExtendedDividerComponent::ApplyComponentSpecificStyles;
    using ExtendedDividerComponent::ApplyPrivateAttributes;
    using ExtendedDividerComponent::GetPrivatePropertyDeclaration;
    using ExtendedDividerComponent::OnDataUpdate;
};

bool HasSetAttributeValue(ArkUI_NodeHandle node, int32_t attribute, float expected)
{
    for (const auto& call : g_tracker.attributeCalls) {
        if (call.node != node || call.attribute != attribute || call.values.empty()) {
            continue;
        }
        if (call.values[0].f32 == expected) {
            return true;
        }
    }
    return false;
}

size_t CountWarningMessages(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment,
    const std::string& messageFragment)
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
        std::string warningCode = TestHelpers::GetRequestStringProperty(mockNapi, request, "code");
        std::string warningPath = TestHelpers::GetRequestStringProperty(mockNapi, request, "path");
        std::string warningMessage = TestHelpers::GetRequestStringProperty(mockNapi, request, "message");
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos &&
            warningMessage.find(messageFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

void ApplyDividerStrokeAndVertical(
    TestableExtendedDividerCoverageComponent& component, const std::string& strokeWidth, bool vertical)
{
    PropertyDeclaration strokeWidthDeclaration = component.GetPrivatePropertyDeclaration("strokeWidth");
    PropertyDeclaration verticalDeclaration = component.GetPrivatePropertyDeclaration("vertical");
    ASSERT_TRUE(static_cast<bool>(strokeWidthDeclaration.applyValue));
    ASSERT_TRUE(static_cast<bool>(verticalDeclaration.applyValue));

    std::unique_ptr<JsonAdapter> strokeWidthValue = JsonAdapter::Parse(strokeWidth);
    std::unique_ptr<JsonAdapter> verticalValue = JsonAdapter::Parse(vertical ? "true" : "false");
    ASSERT_NE(strokeWidthValue, nullptr);
    ASSERT_NE(verticalValue, nullptr);
    strokeWidthDeclaration.applyValue(strokeWidthValue->GetRoot());
    verticalDeclaration.applyValue(verticalValue->GetRoot());
}

class ExtendedDividerComponentGeometryTest : public A2UIComponentTddTest {};

class ExtendedDividerComponentSchemaWarningTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        callbacks_ = TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    }

    TestHelpers::DispatchCallbacks callbacks_;
};

TEST(ExtendedDividerComponentCoverageTest,
    L0_should_fallback_divider_private_attributes_for_non_object_and_object_payloads)
{
    TestableExtendedDividerCoverageComponent component;

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::Parse(R"({"styles":true})");
    ASSERT_NE(nonObjectStyles, nullptr);
    component.ApplyPrivateAttributes(nonObjectStyles->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    std::unique_ptr<JsonAdapter> objectStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": {},
            "vertical": {},
            "color": {}
        }
    })");
    ASSERT_NE(objectStyles, nullptr);
    component.ApplyPrivateAttributes(objectStyles->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);
}

TEST(ExtendedDividerComponentCoverageTest, L0_should_cover_divider_invalid_scalar_style_fallback_and_property_metadata)
{
    TestableExtendedDividerCoverageComponent component;

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "2vp",
            "vertical": true,
            "color": "#AA112233"
        }
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyPrivateAttributes(validStyles->GetRoot());
    EXPECT_TRUE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0xAA112233u);

    std::unique_ptr<JsonAdapter> invalidScalarStyles = JsonAdapter::Parse(R"({
        "styles": {
            "vertical": "bad",
            "color": 1
        }
    })");
    ASSERT_NE(invalidScalarStyles, nullptr);
    component.ApplyPrivateAttributes(invalidScalarStyles->GetRoot());
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    PropertyDeclaration strokeWidthDeclaration = component.GetPrivatePropertyDeclaration("strokeWidth");
    PropertyDeclaration verticalDeclaration = component.GetPrivatePropertyDeclaration("vertical");
    PropertyDeclaration colorDeclaration = component.GetPrivatePropertyDeclaration("color");
    EXPECT_TRUE(strokeWidthDeclaration.acceptNumberForString);
    EXPECT_EQ(strokeWidthDeclaration.fallbackString, "1px");
    EXPECT_FALSE(verticalDeclaration.fallbackBool);
    EXPECT_EQ(colorDeclaration.fallbackString, "");
}

TEST(ExtendedDividerComponentCoverageTest, L0_should_fallback_invalid_numeric_stroke_width_to_default)
{
    TestableExtendedDividerCoverageComponent component;

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": -1,
            "vertical": false,
            "color": "#FF112233"
        }
    })");
    ASSERT_NE(styles, nullptr);

    component.ApplyPrivateAttributes(styles->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0xFF112233u);
}

TEST(ExtendedDividerComponentCoverageTest, L0_should_cover_missing_vertical_invalid_string_color_and_percent_thickness)
{
    TestableExtendedDividerCoverageComponent component;

    std::unique_ptr<JsonAdapter> invalidStringStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "bad",
            "color": "bad"
        }
    })");
    ASSERT_NE(invalidStringStyles, nullptr);
    component.ApplyPrivateAttributes(invalidStringStyles->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    std::unique_ptr<JsonAdapter> percentStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "25%",
            "vertical": false
        }
    })");
    ASSERT_NE(percentStyles, nullptr);
    component.ApplyPrivateAttributes(percentStyles->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 25.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "%");
    EXPECT_FALSE(component.GetVerticalForTest());

    std::unique_ptr<JsonAdapter> verticalPercentStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "40%",
            "vertical": true
        }
    })");
    ASSERT_NE(verticalPercentStyles, nullptr);
    component.ApplyPrivateAttributes(verticalPercentStyles->GetRoot());
    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 40.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "%");
    EXPECT_TRUE(component.GetVerticalForTest());
}

TEST(ExtendedDividerComponentCoverageTest,
    L0_should_register_divider_private_style_path_bindings_and_apply_runtime_updates)
{
    TestableExtendedDividerCoverageComponent component;

    std::unique_ptr<JsonAdapter> bindingStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": {
                "path": "/divider/strokeWidth"
            },
            "vertical": {
                "path": "/divider/vertical"
            },
            "color": {
                "path": "/divider/color"
            }
        }
    })");
    ASSERT_NE(bindingStyles, nullptr);

    component.ApplyPrivateAttributes(bindingStyles->GetRoot());

    const std::vector<DataBinding>& bindings = component.GetDataBindings();
    ASSERT_EQ(bindings.size(), 3U);
    EXPECT_EQ(bindings[0].propertyName_, "strokeWidth");
    EXPECT_EQ(bindings[0].dataPath_, "/divider/strokeWidth");
    EXPECT_EQ(bindings[0].type_, BindingType::PATH);
    EXPECT_EQ(bindings[1].propertyName_, "vertical");
    EXPECT_EQ(bindings[1].dataPath_, "/divider/vertical");
    EXPECT_EQ(bindings[1].type_, BindingType::PATH);
    EXPECT_EQ(bindings[2].propertyName_, "color");
    EXPECT_EQ(bindings[2].dataPath_, "/divider/color");
    EXPECT_EQ(bindings[2].type_, BindingType::PATH);

    std::unique_ptr<JsonAdapter> updatedStrokeWidth = JsonAdapter::Parse(R"("2vp")");
    std::unique_ptr<JsonAdapter> updatedVertical = JsonAdapter::Parse("true");
    std::unique_ptr<JsonAdapter> updatedColor = JsonAdapter::Parse(R"("#AA112233")");
    ASSERT_NE(updatedStrokeWidth, nullptr);
    ASSERT_NE(updatedVertical, nullptr);
    ASSERT_NE(updatedColor, nullptr);

    component.OnDataUpdate("strokeWidth", updatedStrokeWidth->GetRoot());
    component.OnDataUpdate("vertical", updatedVertical->GetRoot());
    component.OnDataUpdate("color", updatedColor->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 2.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "vp");
    EXPECT_TRUE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0xAA112233u);
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_skip_stroke_axis_when_common_axis_field_exists)
{
    TestableExtendedDividerCoverageComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ApplyDividerStrokeAndVertical(component, R"("8vp")", true);
    nodeAdapter.SetWidth(240.0F);
    nodeAdapter.SetHeight(120.0F);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "width": 240,
        "height": 120
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), nodeAdapter);

    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 8.0F));
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_HEIGHT, 8.0F));
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_apply_stroke_axis_without_default_length_when_common_missing)
{
    TestableExtendedDividerCoverageComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ApplyDividerStrokeAndVertical(component, R"("25%")", false);

    component.ApplyComponentSpecificStyles(JsonValue(), nodeAdapter);

    EXPECT_EQ(FindLastAttributeCall(node, NODE_WIDTH_PERCENT), nullptr);
    ExpectF32Attribute(node, NODE_HEIGHT_PERCENT, 0.25F);

    ResetTracker();
    ApplyDividerStrokeAndVertical(component, R"("40%")", true);
    component.ApplyComponentSpecificStyles(JsonValue(), nodeAdapter);

    ExpectF32Attribute(node, NODE_WIDTH_PERCENT, 0.4F);
    EXPECT_EQ(FindLastAttributeCall(node, NODE_HEIGHT_PERCENT), nullptr);
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_apply_horizontal_stroke_when_height_field_is_missing)
{
    TestableExtendedDividerCoverageComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ApplyDividerStrokeAndVertical(component, R"("8vp")", true);
    nodeAdapter.SetWidth(240.0F);

    std::unique_ptr<JsonAdapter> verticalStyles = JsonAdapter::Parse(R"({
        "width": 240
    })");
    ASSERT_NE(verticalStyles, nullptr);
    component.ApplyComponentSpecificStyles(verticalStyles->GetRoot(), nodeAdapter);
    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 8.0F));

    PropertyDeclaration verticalDeclaration = component.GetPrivatePropertyDeclaration("vertical");
    ASSERT_TRUE(static_cast<bool>(verticalDeclaration.applyValue));
    std::unique_ptr<JsonAdapter> verticalFalse = JsonAdapter::Parse("false");
    ASSERT_NE(verticalFalse, nullptr);
    verticalDeclaration.applyValue(verticalFalse->GetRoot());
    component.ApplyComponentSpecificStyles(verticalStyles->GetRoot(), nodeAdapter);

    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 8.0F);
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_apply_stroke_width_when_vertical_update_has_no_height_field)
{
    TestableExtendedDividerCoverageComponent component;
    RenderContext context = RenderContext::Create(13, "surface-divider-vertical-update", nullptr, nullptr);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Divider",
        "styles": {
            "strokeWidth": "4vp",
            "vertical": true,
            "width": 240,
            "height": 120
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));
    ASSERT_FALSE(g_tracker.createNodeCalls.empty());
    EXPECT_EQ(g_tracker.createNodeCalls.back().type, ARKUI_NODE_ROW);

    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 4.0F));

    ResetTracker();
    std::unique_ptr<JsonAdapter> verticalFalse = JsonAdapter::Parse("false");
    ASSERT_NE(verticalFalse, nullptr);
    component.OnDataUpdate("styles.vertical", verticalFalse->GetRoot());

    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 4.0F));
    ExpectF32Attribute(node, NODE_HEIGHT, 4.0F);
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_skip_stroke_width_when_common_height_field_exists_for_horizontal)
{
    TestableExtendedDividerCoverageComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ApplyDividerStrokeAndVertical(component, R"("4vp")", false);
    nodeAdapter.SetWidth(240.0F);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "width": 240,
        "height": "bad"
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), nodeAdapter);

    ExpectF32Attribute(node, NODE_WIDTH, 240.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_HEIGHT, 4.0F));
}

TEST_F(ExtendedDividerComponentGeometryTest, L0_should_skip_stroke_width_when_common_width_field_exists_for_vertical)
{
    TestableExtendedDividerCoverageComponent component;
    ArkUINodeApiAdapter nodeAdapter = CreateNodeApiAdapter(component);
    ArkUI_NodeHandle node = component.GetNativeView();
    ASSERT_NE(node, nullptr);
    ApplyDividerStrokeAndVertical(component, R"("4vp")", true);
    nodeAdapter.SetHeight(120.0F);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "width": "bad",
        "height": 120
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), nodeAdapter);

    ExpectF32Attribute(node, NODE_HEIGHT, 120.0F);
    EXPECT_FALSE(HasSetAttributeValue(node, NODE_WIDTH, 4.0F));
}

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_fallback_divider_non_string_private_styles_to_defaults_and_dispatch_schema_warnings)
{
    TestableExtendedDividerCoverageComponent component;
    component.SetRenderId(1001);
    component.SetSurfaceId("surface-divider-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": true,
            "vertical": null,
            "color": false
        }
    })");
    ASSERT_NE(invalidStyles, nullptr);

    component.ApplyPrivateAttributes(invalidStyles->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    std::unique_ptr<JsonAdapter> invalidObjectLiteralStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": {},
            "vertical": {},
            "color": {}
        }
    })");
    ASSERT_NE(invalidObjectLiteralStyles, nullptr);
    component.ApplyPrivateAttributes(invalidObjectLiteralStyles->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 2U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical"), 2U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.color"), 2U);
}

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_report_stroke_width_type_mismatch_from_private_style_only_for_full_descriptor)
{
    TestableExtendedDividerCoverageComponent component;
    RenderContext context = RenderContext::Create(1004, "surface-divider-full-warning", nullptr, nullptr);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Divider",
        "styles": {
            "strokeWidth": true,
            "vertical": true,
            "color": "#33000000"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
    EXPECT_EQ(CountWarningMessages(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects string or number"),
        1U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects number"), 0U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects string value"),
        0U);
}

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_report_vertical_type_mismatch_from_private_style_only_for_full_descriptor)
{
    TestableExtendedDividerCoverageComponent component;
    RenderContext context = RenderContext::Create(1006, "surface-divider-full-vertical-warning", nullptr, nullptr);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Divider",
        "styles": {
            "strokeWidth": "2vp",
            "vertical": "bad",
            "color": "#33000000"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical"), 1U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical", "expects boolean value"), 1U);
    EXPECT_EQ(CountWarningMessages(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical", "compatibility normalization"),
        0U);
}

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_report_stroke_width_type_mismatch_from_private_style_only_for_style_update)
{
    TestableExtendedDividerCoverageComponent component;
    RenderContext context = RenderContext::Create(1005, "surface-divider-style-update-warning", nullptr, nullptr);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Divider",
        "styles": {
            "strokeWidth": "2vp",
            "vertical": true,
            "color": "#33000000"
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> invalidStrokeWidth = JsonAdapter::Parse("true");
    ASSERT_NE(invalidStrokeWidth, nullptr);

    component.OnDataUpdate("styles.strokeWidth", invalidStrokeWidth->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 1U);
    EXPECT_EQ(CountWarningMessages(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects string or number"),
        1U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects number"), 0U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth", "expects string value"),
        0U);
}

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_report_vertical_type_mismatch_from_private_style_only_for_style_update)
{
    TestableExtendedDividerCoverageComponent component;
    RenderContext context =
        RenderContext::Create(1007, "surface-divider-style-update-vertical-warning", nullptr, nullptr);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "id": "root",
        "component": "Divider",
        "styles": {
            "strokeWidth": "2vp",
            "vertical": true,
            "color": "#33000000"
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(component.InitFromDescriptor(descriptor->GetRoot(), context));

    mockNapiPtr_->callFunctionArgsHistory_.clear();
    std::unique_ptr<JsonAdapter> invalidVertical = JsonAdapter::Parse(R"("bad")");
    ASSERT_NE(invalidVertical, nullptr);

    component.OnDataUpdate("styles.vertical", invalidVertical->GetRoot());

    EXPECT_FALSE(component.GetVerticalForTest());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical"), 1U);
    EXPECT_EQ(
        CountWarningMessages(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical", "expects boolean value"), 1U);
    EXPECT_EQ(CountWarningMessages(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical", "compatibility normalization"),
        0U);
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_accept_divider_expression_private_styles_without_dispatching_schema_warnings)
{
    TestableExtendedDividerCoverageComponent component;
    component.SetRenderId(1003);
    component.SetSurfaceId("surface-divider-expression");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> expressionStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "{{ 2 }}",
            "vertical": "{{ true }}",
            "color": "{{ '#AA112233' }}"
        }
    })");
    ASSERT_NE(expressionStyles, nullptr);

    component.ApplyPrivateAttributes(expressionStyles->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 2.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "vp");
    EXPECT_TRUE(component.GetVerticalForTest());
    EXPECT_EQ(component.GetColorForTest(), 0xAA112233u);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.strokeWidth"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.strokeWidth"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.vertical"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.vertical"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.color"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.color"), 0U);
}
#endif

TEST_F(ExtendedDividerComponentSchemaWarningTest,
    L0_should_dispatch_divider_invalid_value_schema_warnings_for_stroke_width_and_color)
{
    TestableExtendedDividerCoverageComponent component;
    component.SetRenderId(1002);
    component.SetSurfaceId("surface-divider-invalid-value-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "styles": {
            "strokeWidth": "-2vp",
            "color": "transparent-blue"
        }
    })");
    ASSERT_NE(invalidStyles, nullptr);

    component.ApplyPrivateAttributes(invalidStyles->GetRoot());

    EXPECT_FLOAT_EQ(component.GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(component.GetStrokeWidthUnitForTest(), "px");
    EXPECT_EQ(component.GetColorForTest(), 0x33000000u);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.strokeWidth"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.color"), 1U);
}

} // namespace
