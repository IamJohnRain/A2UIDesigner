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
#include <limits>

#include "components/extended/ExtendedTextComponent.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "SchemaWarningTestHelper.h"

using namespace NativeModule;

namespace {

class TestableExtendedTextCoverageComponent : public ExtendedTextComponent {
public:
    using ExtendedTextComponent::ApplyComponentSpecificStyles;
    using ExtendedTextComponent::ApplyPrivateAttributes;
    using ExtendedTextComponent::OnDataUpdate;
    using ExtendedTextComponent::SetApplyingStyleDeltaUpdateForTest;
};

class ExtendedTextComponentSchemaWarningTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        callbacks_ = TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    }

    TestHelpers::DispatchCallbacks callbacks_;
};

TEST(ExtendedTextComponentCoverageTest,
    L0_should_fallback_invalid_full_text_style_payloads_and_warn_for_overflow_without_max_lines)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> invalidFullStyles = JsonAdapter::Parse(R"({
        "fontWeight": "950",
        "fontColor": "invalid",
        "minFontScale": "bad",
        "maxFontScale": false,
        "fontScaleMode": "broken",
        "fontSize": 0,
        "maxLines": -1,
        "textOverflow": "ellipsis",
        "textAlign": "ltr",
        "wordBreak": "bad",
        "minFontSize": 20,
        "maxFontSize": 10,
        "decoration": {
            "type": "underline",
            "style": "bad",
            "thicknessScale": "bad"
        }
    })");
    ASSERT_NE(invalidFullStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidFullStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 0.0F);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_EQ(component.GetTextOverflowForTest(), 2);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
}

TEST(ExtendedTextComponentCoverageTest, L0_should_fallback_text_style_state_to_defaults_for_invalid_style_deltas)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "fontWeight": 700,
        "fontColor": "#FF123456",
        "minFontScale": 0.6,
        "maxFontScale": 1.8,
        "fontScaleMode": "custom",
        "fontSize": 18,
        "maxLines": 3,
        "minFontSize": 10,
        "maxFontSize": 20,
        "textOverflow": "ellipsis",
        "textAlign": "center",
        "wordBreak": "breakAll",
        "decoration": {
            "type": "underline",
            "color": "#FF654321"
        }
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> invalidDeltaStyles = JsonAdapter::Parse(R"({
        "fontWeight": "400",
        "fontColor": false,
        "minFontScale": "bad",
        "maxFontScale": false,
        "fontScaleMode": 1,
        "fontSize": "16",
        "maxLines": "2",
        "minFontSize": false,
        "maxFontSize": true,
        "textOverflow": 1,
        "textAlign": 1,
        "wordBreak": 1,
        "decoration": "invalid"
    })");
    ASSERT_NE(invalidDeltaStyles, nullptr);
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStyles(invalidDeltaStyles->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> invalidStringFontScaleModeDelta = JsonAdapter::Parse(R"({
        "fontScaleMode": "broken"
    })");
    ASSERT_NE(invalidStringFontScaleModeDelta, nullptr);
    component.ApplyComponentSpecificStyles(invalidStringFontScaleModeDelta->GetRoot(), applier);
    component.SetApplyingStyleDeltaUpdateForTest(false);

    std::unique_ptr<JsonAdapter> invalidFontWeightDelta = JsonAdapter::Parse("true");
    ASSERT_NE(invalidFontWeightDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("fontWeight"), invalidFontWeightDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidFontColorDelta = JsonAdapter::Parse("false");
    ASSERT_NE(invalidFontColorDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("fontColor"), invalidFontColorDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidMinFontScaleDelta = JsonAdapter::Parse(R"("bad")");
    ASSERT_NE(invalidMinFontScaleDelta, nullptr);
    component.OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("minFontScale"), invalidMinFontScaleDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidMaxFontScaleDelta = JsonAdapter::Parse("false");
    ASSERT_NE(invalidMaxFontScaleDelta, nullptr);
    component.OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("maxFontScale"), invalidMaxFontScaleDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidFontScaleModeDelta = JsonAdapter::Parse("1");
    ASSERT_NE(invalidFontScaleModeDelta, nullptr);
    component.OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("fontScaleMode"), invalidFontScaleModeDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidFontSizeDelta = JsonAdapter::Parse("false");
    ASSERT_NE(invalidFontSizeDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("fontSize"), invalidFontSizeDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidMaxLinesDelta = JsonAdapter::Parse("true");
    ASSERT_NE(invalidMaxLinesDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("maxLines"), invalidMaxLinesDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidTextOverflowDelta = JsonAdapter::Parse("1");
    ASSERT_NE(invalidTextOverflowDelta, nullptr);
    component.OnDataUpdate(
        StyleResolver::BuildStyleBindingProperty("textOverflow"), invalidTextOverflowDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidTextAlignDelta = JsonAdapter::Parse("1");
    ASSERT_NE(invalidTextAlignDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("textAlign"), invalidTextAlignDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidWordBreakDelta = JsonAdapter::Parse("1");
    ASSERT_NE(invalidWordBreakDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("wordBreak"), invalidWordBreakDelta->GetRoot());

    std::unique_ptr<JsonAdapter> invalidDecorationDelta = JsonAdapter::Parse(R"("invalid")");
    ASSERT_NE(invalidDecorationDelta, nullptr);
    component.OnDataUpdate(StyleResolver::BuildStyleBindingProperty("decoration"), invalidDecorationDelta->GetRoot());

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 0.0F);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    EXPECT_EQ(component.GetDecorationForTest().type, 0);
    EXPECT_TRUE(component.GetDecorationForTest().hasColor);
    EXPECT_EQ(component.GetDecorationForTest().color, 0xFF000000u);
    EXPECT_TRUE(component.GetDecorationForTest().hasStyle);
    EXPECT_EQ(component.GetDecorationForTest().style, 0);
    EXPECT_TRUE(component.GetDecorationForTest().hasThicknessScale);
    EXPECT_FLOAT_EQ(component.GetDecorationForTest().thicknessScale, 1.0F);
}

TEST(ExtendedTextComponentCoverageTest, L0_should_cover_text_full_type_mismatch_and_overflow_warning_paths)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "fontWeight": 700,
        "fontColor": "#FF123456",
        "fontScaleMode": "custom",
        "fontSize": 18,
        "maxLines": 3,
        "textOverflow": "ellipsis",
        "textAlign": "center",
        "wordBreak": "breakAll",
        "minFontSize": 10,
        "maxFontSize": 20,
        "decoration": {
            "type": "underline",
            "color": "#FF654321"
        }
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> fullTypeMismatchStyles = JsonAdapter::Parse(R"({
        "fontWeight": true,
        "fontColor": false,
        "fontScaleMode": 1,
        "fontSize": {},
        "maxLines": [],
        "textOverflow": "ellipsis",
        "textAlign": 1,
        "wordBreak": 1,
        "minFontSize": true,
        "maxFontSize": false,
        "decoration": {
            "type": "underline",
            "style": "bad",
            "thicknessScale": "bad"
        }
    })");
    ASSERT_NE(fullTypeMismatchStyles, nullptr);
    component.ApplyComponentSpecificStyles(fullTypeMismatchStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_EQ(component.GetTextOverflowForTest(), 2);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);

    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    std::unique_ptr<JsonAdapter> fullInvalidNumericFontWeight = JsonAdapter::Parse(R"({
        "fontWeight": 50
    })");
    ASSERT_NE(fullInvalidNumericFontWeight, nullptr);
    component.ApplyComponentSpecificStyles(fullInvalidNumericFontWeight->GetRoot(), applier);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);

    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    component.SetApplyingStyleDeltaUpdateForTest(true);
    component.ApplyComponentSpecificStyles(fullTypeMismatchStyles->GetRoot(), applier);
    component.SetApplyingStyleDeltaUpdateForTest(false);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_EQ(component.GetTextOverflowForTest(), 2);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
}

TEST(ExtendedTextComponentCoverageTest, L0_should_cover_text_numeric_font_weight_and_font_size_edge_cases)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    const char* numericWeightCases[] = { R"({"fontWeight":50})", R"({"fontWeight":950})", R"({"fontWeight":250})",
        R"({"fontWeight":100.5})", R"({"fontWeight":100.1})", R"({"fontWeight":"250"})", R"({"fontWeight":"100.5"})",
        R"({"fontWeight":"100.1"})" };
    for (const char* testCase : numericWeightCases) {
        std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(testCase);
        ASSERT_NE(styles, nullptr);
        component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);
        EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    }

    std::unique_ptr<JsonAdapter> nanStyleObject = JsonAdapter::CreateObject();
    ASSERT_NE(nanStyleObject, nullptr);
    JsonValue nanRoot = nanStyleObject->GetRoot();
    ASSERT_TRUE(nanRoot.PutNumber("minFontSize", std::numeric_limits<double>::quiet_NaN()));
    ASSERT_TRUE(nanRoot.PutNumber("maxFontSize", std::numeric_limits<double>::quiet_NaN()));
    component.ApplyComponentSpecificStyles(nanRoot, applier);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), -1.0F);

    std::unique_ptr<JsonAdapter> zeroFontSizes = JsonAdapter::Parse(R"({
        "minFontSize": 0,
        "maxFontSize": 0
    })");
    ASSERT_NE(zeroFontSizes, nullptr);
    component.ApplyComponentSpecificStyles(zeroFontSizes->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), -1.0F);

    std::unique_ptr<JsonAdapter> nanFontWeight = JsonAdapter::CreateObject();
    ASSERT_NE(nanFontWeight, nullptr);
    JsonValue nanFontWeightRoot = nanFontWeight->GetRoot();
    ASSERT_TRUE(nanFontWeightRoot.PutNumber("fontWeight", std::numeric_limits<double>::quiet_NaN()));
    component.ApplyComponentSpecificStyles(nanFontWeightRoot, applier);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);

    std::unique_ptr<JsonAdapter> invalidTypedStyles = JsonAdapter::Parse(R"({
        "fontWeight": "700",
        "fontColor": 4279383126,
        "fontSize": "18",
        "maxLines": "2",
        "minFontSize": 12,
        "maxFontSize": 24,
        "decoration": {
            "type": "underline",
            "color": 4278190335,
            "thicknessScale": "1.5"
        }
    })");
    ASSERT_NE(invalidTypedStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidTypedStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), 12.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), 24.0F);

    TextDecorationState decoration = component.GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF000000u);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);
}

TEST(ExtendedTextComponentCoverageTest, L0_should_warn_when_text_overflow_is_set_without_max_lines)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "textOverflow": "ellipsis"
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_EQ(component.GetTextOverflowForTest(), 2);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);

    std::unique_ptr<JsonAdapter> defaultOverflow = JsonAdapter::Parse(R"({
        "textOverflow": "clip"
    })");
    ASSERT_NE(defaultOverflow, nullptr);
    component.ApplyComponentSpecificStyles(defaultOverflow->GetRoot(), applier);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
}

TEST(ExtendedTextComponentCoverageTest,
    L0_should_cover_non_object_content_invalid_string_text_styles_and_set_max_lines_guards)
{
    TestableExtendedTextCoverageComponent component;
    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> nonObjectDescriptor = JsonAdapter::Parse("true");
    ASSERT_NE(nonObjectDescriptor, nullptr);
    component.ApplyPrivateAttributes(nonObjectDescriptor->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "");

    std::unique_ptr<JsonAdapter> invalidStringStyles = JsonAdapter::Parse(R"({
        "fontWeight": "semiBold",
        "textOverflow": "broken"
    })");
    ASSERT_NE(invalidStringStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStringStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);

    component.SetMaxLinesForTest(-5);
    EXPECT_EQ(component.GetMaxLinesForTest(), std::numeric_limits<int32_t>::max());

    ArkUI_NodeHandle nativeView = component.GetNativeViewHandleRawForTest();
    void* nativeNodeApi = component.GetNativeNodeApiRawForTest();

    component.SetNativeViewHandleRawForTest(nullptr);
    component.SetMaxLinesForTest(3);
    EXPECT_EQ(component.GetMaxLinesForTest(), 3);
    component.SetNativeViewHandleRawForTest(nativeView);

    component.SetNativeNodeApiRawForTest(nullptr);
    component.SetMaxLinesForTest(4);
    EXPECT_EQ(component.GetMaxLinesForTest(), 4);
    component.SetNativeNodeApiRawForTest(nativeNodeApi);
}

TEST(ExtendedTextComponentCoverageTest, L0_should_register_text_content_path_binding_and_apply_runtime_updates)
{
    TestableExtendedTextCoverageComponent component;

    std::unique_ptr<JsonAdapter> bindingContent = JsonAdapter::Parse(R"({
        "content": {
            "path": "/textContent"
        }
    })");
    ASSERT_NE(bindingContent, nullptr);
    component.ApplyPrivateAttributes(bindingContent->GetRoot());

    const std::vector<DataBinding>& bindings = component.GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "content");
    EXPECT_EQ(bindings[0].dataPath_, "/textContent");
    EXPECT_EQ(bindings[0].type_, BindingType::PATH);

    std::unique_ptr<JsonAdapter> updatedContent = JsonAdapter::Parse(R"("updated by binding")");
    ASSERT_NE(updatedContent, nullptr);
    component.OnDataUpdate("content", updatedContent->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "updated by binding");
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_fallback_text_private_fields_to_defaults_and_dispatch_schema_warnings_for_type_mismatches)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1002);
    component.SetSurfaceId("surface-text-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> invalidContent = JsonAdapter::Parse(R"({"content":true})");
    ASSERT_NE(invalidContent, nullptr);
    component.ApplyPrivateAttributes(invalidContent->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "");

    std::unique_ptr<JsonAdapter> nullContent = JsonAdapter::Parse(R"({"content":null})");
    ASSERT_NE(nullContent, nullptr);
    component.ApplyPrivateAttributes(nullContent->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "");

    std::unique_ptr<JsonAdapter> bindingPathContent = JsonAdapter::Parse(R"({
        "content": {
            "path": "/textContent"
        }
    })");
    ASSERT_NE(bindingPathContent, nullptr);
    component.ApplyPrivateAttributes(bindingPathContent->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "");
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "content"), 2U);

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "fontWeight": false,
        "fontColor": true,
        "minFontScale": true,
        "maxFontScale": false,
        "fontScaleMode": null,
        "fontSize": false,
        "maxLines": true,
        "textOverflow": false,
        "textAlign": null,
        "wordBreak": true,
        "minFontSize": true,
        "maxFontSize": false,
        "decoration": false
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 0.0F);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), -1.0F);
    TextDecorationState decoration = component.GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF000000u);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);

    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "content"), 2U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.fontWeight"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.fontColor"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.minFontScale"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.maxFontScale"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.fontScaleMode"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.fontSize"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.maxLines"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.textOverflow"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.textAlign"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.wordBreak"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.minFontSize"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.maxFontSize"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration"), 1U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_fallback_text_decoration_subfields_to_defaults_and_dispatch_schema_warnings)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1005);
    component.SetSurfaceId("surface-text-decoration-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> defaultDecoration = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "underline"
        }
    })");
    ASSERT_NE(defaultDecoration, nullptr);
    component.ApplyComponentSpecificStyles(defaultDecoration->GetRoot(), applier);
    uint32_t defaultDecorationColor = component.GetDecorationForTest().color;

    std::unique_ptr<JsonAdapter> invalidSubfields = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "underline",
            "color": true,
            "style": false,
            "thicknessScale": null
        }
    })");
    ASSERT_NE(invalidSubfields, nullptr);
    component.ApplyComponentSpecificStyles(invalidSubfields->GetRoot(), applier);

    TextDecorationState decoration = component.GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, defaultDecorationColor);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);

    std::unique_ptr<JsonAdapter> invalidType = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "strikethrough"
        }
    })");
    ASSERT_NE(invalidType, nullptr);
    component.ApplyComponentSpecificStyles(invalidType->GetRoot(), applier);

    decoration = component.GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, defaultDecorationColor);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);

    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.decoration.type"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration.color"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration.style"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration.thicknessScale"),
        1U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_clamp_text_invalid_numeric_font_scale_ranges_and_dispatch_schema_warnings)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1006);
    component.SetSurfaceId("surface-text-font-scale-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "minFontScale": 0.8,
        "maxFontScale": 1.5
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 0.8F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 1.5F);

    std::unique_ptr<JsonAdapter> invalidNumericStyles = JsonAdapter::Parse(R"({
        "minFontScale": 1.5,
        "maxFontScale": 0.5
    })");
    ASSERT_NE(invalidNumericStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidNumericStyles->GetRoot(), applier);

    EXPECT_FLOAT_EQ(component.GetMinFontScaleForTest(), 1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontScaleForTest(), 1.0F);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.minFontScale"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.maxFontScale"), 1U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest, L0_should_accept_zero_text_max_lines_without_dispatching_warning)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1009);
    component.SetSurfaceId("surface-text-zero-max-lines");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "maxLines": 0
    })");
    ASSERT_NE(styles, nullptr);
    component.ApplyComponentSpecificStyles(styles->GetRoot(), applier);

    EXPECT_EQ(component.GetMaxLinesForTest(), 0);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.maxLines"), 0U);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.maxLines"), 0U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_dispatch_text_schema_warnings_for_missing_content_and_invalid_scalar_style_fields)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1007);
    component.SetSurfaceId("surface-text-invalid-value-warning");
    component.SetComponentId("root");

    std::unique_ptr<JsonAdapter> missingContent = JsonAdapter::Parse(R"({})");
    ASSERT_NE(missingContent, nullptr);
    component.ApplyPrivateAttributes(missingContent->GetRoot());
    EXPECT_EQ(component.GetTextValueForTest(), "");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "fontWeight": "950",
        "fontColor": "invalid",
        "fontScaleMode": "broken",
        "fontSize": 0,
        "maxLines": -1,
        "textOverflow": "broken",
        "textAlign": "right",
        "wordBreak": "bad",
        "minFontSize": 0,
        "maxFontSize": -1
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(component.GetFontColorForTest(), 0xE5000000u);
    EXPECT_EQ(component.GetFontScaleModeForTest(), "followSystem");
    EXPECT_FLOAT_EQ(component.GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(component.GetMaxLinesForTest(), -1);
    EXPECT_EQ(component.GetTextOverflowForTest(), 1);
    EXPECT_EQ(component.GetTextAlignForTest(), 0);
    EXPECT_EQ(component.GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), -1.0F);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "content"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.fontWeight"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.fontColor"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.fontScaleMode"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.fontSize"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.maxLines"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.textOverflow"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.textAlign"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.wordBreak"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.minFontSize"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.maxFontSize"), 1U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_dispatch_text_cross_field_and_decoration_invalid_value_schema_warnings)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1008);
    component.SetSurfaceId("surface-text-cross-field-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> crossFieldStyles = JsonAdapter::Parse(R"({
        "textOverflow": "ellipsis",
        "minFontSize": 20,
        "maxFontSize": 10
    })");
    ASSERT_NE(crossFieldStyles, nullptr);
    component.ApplyComponentSpecificStyles(crossFieldStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetTextOverflowForTest(), 2);
    EXPECT_FLOAT_EQ(component.GetMinFontSizeForTest(), 20.0F);
    EXPECT_FLOAT_EQ(component.GetMaxFontSizeForTest(), 10.0F);

    std::unique_ptr<JsonAdapter> invalidDecorationType = JsonAdapter::Parse(R"({
        "decoration": {
            "type": false
        }
    })");
    ASSERT_NE(invalidDecorationType, nullptr);
    component.ApplyComponentSpecificStyles(invalidDecorationType->GetRoot(), applier);

    std::unique_ptr<JsonAdapter> invalidDecorationValues = JsonAdapter::Parse(R"({
        "decoration": {
            "type": "underline",
            "color": "not-a-color",
            "style": "dashDot",
            "thicknessScale": "bold"
        }
    })");
    ASSERT_NE(invalidDecorationValues, nullptr);
    component.ApplyComponentSpecificStyles(invalidDecorationValues->GetRoot(), applier);

    TextDecorationState decoration = component.GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.0F);

    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.textOverflow"), 1U);
    EXPECT_GE(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.minFontSize"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration.type"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.decoration.color"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.decoration.style"), 1U);
    EXPECT_GE(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.decoration.thicknessScale"),
        1U);
}

TEST_F(ExtendedTextComponentSchemaWarningTest,
    L0_should_accept_text_font_weight_keywords_and_treat_legacy_decoration_aliases_as_invalid)
{
    TestableExtendedTextCoverageComponent component;
    component.SetRenderId(1010);
    component.SetSurfaceId("surface-text-legacy-alias-warning");
    component.SetComponentId("root");

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);

    std::unique_ptr<JsonAdapter> validStyles = JsonAdapter::Parse(R"({
        "fontWeight": 700,
        "decoration": {
            "type": "linethrough"
        }
    })");
    ASSERT_NE(validStyles, nullptr);
    component.ApplyComponentSpecificStyles(validStyles->GetRoot(), applier);
    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(component.GetDecorationForTest().type, 3);

    std::unique_ptr<JsonAdapter> invalidStyles = JsonAdapter::Parse(R"({
        "fontWeight": "bold",
        "decoration": {
            "type": "linethrough"
        }
    })");
    ASSERT_NE(invalidStyles, nullptr);
    component.ApplyComponentSpecificStyles(invalidStyles->GetRoot(), applier);

    EXPECT_EQ(component.GetFontWeightForTest(), ARKUI_FONT_WEIGHT_BOLD);
    EXPECT_EQ(component.GetDecorationForTest().type, 3);
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "styles.fontWeight"), 0U);
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "styles.decoration.type"), 0U);
}

} // namespace
