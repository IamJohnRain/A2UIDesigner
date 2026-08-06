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
#include <set>
#include <string>
#include <vector>

#include "components/A2UI/A2UIComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedComponentStyleValidation.h"
#include "components/extended/ExtendedGridComponent.h"
#include "components/extended/ExtendedListComponent.h"
#include "components/extended/ExtendedRowComponent.h"
#include "components/extended/ExtendedStackComponent.h"
#include "utils/JsonAdapter.h"

#include "A2UIComponentTddTestHelper.h"
#include "SchemaErrorCodes.h"
#include "SchemaWarningTestHelper.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

template<typename TComponent>
void PrepareWarningComponent(TComponent& component, const std::string& componentId)
{
    component.SetRenderId(1001);
    component.SetSurfaceId("surface-dynamic-style-validation");
    component.SetComponentId(componentId);
}

size_t CountWarnings(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    return TestHelpers::CountWarningRequests(mockNapi, code, pathFragment);
}

void ClearWarnings(MockNapiProvider* mockNapi)
{
    ASSERT_NE(mockNapi, nullptr);
    mockNapi->callFunctionArgsHistory_.clear();
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeForNode(
    const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto iter = provider.setAttributeRecords_.rbegin(); iter != provider.setAttributeRecords_.rend(); ++iter) {
        if (iter->nodeHandle == node && iter->attribute == attribute) {
            return &(*iter);
        }
    }
    return nullptr;
}

class DynamicStyleValidationProbeComponent : public ExtendedColumnComponent {
public:
    using ExtendedColumnComponent::HasDynamicStyleValue;
    using ExtendedColumnComponent::ReportDynamicStyleInvalidValue;
    using ExtendedColumnComponent::ReportDynamicStyleTypeMismatch;
    using ExtendedColumnComponent::ValidateDynamicStyleEnumProperty;
    using ExtendedColumnComponent::ValidateDynamicStyleNumberProperty;
};

class DynamicStyleColumnProbeComponent : public ExtendedColumnComponent {
public:
    void ValidateDynamicStylesForTest(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
    {
        ValidateComponentSpecificDynamicStylesDfx(styles, dynamicStyleKeys);
    }

    void ApplyStylesForTest(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    PropertyDeclaration GetPrivatePropertyDeclarationForTest(const std::string& propertyName)
    {
        return GetPrivatePropertyDeclaration(propertyName);
    }

    void OnAddChildForTest(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void OnMoveChildForTest(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void OnRemoveChildForTest(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class DynamicStyleRowProbeComponent : public ExtendedRowComponent {
public:
    void ValidateDynamicStylesForTest(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
    {
        ValidateComponentSpecificDynamicStylesDfx(styles, dynamicStyleKeys);
    }

    void ApplyStylesForTest(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class DynamicStyleListProbeComponent : public ExtendedListComponent {
public:
    void ValidateDynamicStylesForTest(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
    {
        ValidateComponentSpecificDynamicStylesDfx(styles, dynamicStyleKeys);
    }

    void ApplyStylesForTest(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void OnAddChildForTest(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void OnMoveChildForTest(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class DynamicStyleGridProbeComponent : public ExtendedGridComponent {
public:
    void ValidateDynamicStylesForTest(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
    {
        ValidateComponentSpecificDynamicStylesDfx(styles, dynamicStyleKeys);
    }
};

class DynamicStyleStackProbeComponent : public ExtendedStackComponent {
public:
    void ValidateDynamicStylesForTest(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
    {
        ValidateComponentSpecificDynamicStylesDfx(styles, dynamicStyleKeys);
    }
};

class ExtendedComponentDynamicStyleValidationTddTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        callbacks_ = TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    }

    TestHelpers::DispatchCallbacks callbacks_;
};

/**
 * @tc.name: L0_should_identify_component_specific_style_keys_after_validation_refactor
 * @tc.desc: 覆盖抽出的组件专属样式白名单 helper 的命中、未命中和未知组件分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest,
    L0_should_identify_component_specific_style_keys_after_validation_refactor)
{
    EXPECT_TRUE(IsExtendedComponentSpecificStyleKey("Grid", "columnsTemplate"));
    EXPECT_TRUE(IsExtendedComponentSpecificStyleKey("List", "nestedScroll"));
    EXPECT_FALSE(IsExtendedComponentSpecificStyleKey("Grid", "alignItems"));
    EXPECT_FALSE(IsExtendedComponentSpecificStyleKey("Unknown", "columnsTemplate"));
}

/**
 * @tc.name: L0_should_cover_dynamic_style_validation_common_helpers
 * @tc.desc: 覆盖动态样式校验公共 helper 的空 path、缺失动态 key、类型错误、非法值和合法值分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest, L0_should_cover_dynamic_style_validation_common_helpers)
{
    DynamicStyleValidationProbeComponent component;
    PrepareWarningComponent(component, "probe");

    auto styles = ParseJson(R"({
        "enumValue": "center",
        "badEnum": "invalid",
        "enumTypeMismatch": 1,
        "numberValue": 4,
        "negativeNumber": -1,
        "zeroNumber": 0,
        "numberTypeMismatch": "bad"
    })");
    ASSERT_NE(styles, nullptr);
    const std::set<std::string> dynamicKeys = { "enumValue", "badEnum", "enumTypeMismatch", "numberValue",
        "negativeNumber", "zeroNumber", "numberTypeMismatch" };

    EXPECT_TRUE(component.HasDynamicStyleValue(styles->GetRoot(), dynamicKeys, "enumValue"));
    EXPECT_FALSE(component.HasDynamicStyleValue(styles->GetRoot(), dynamicKeys, "missing"));
    EXPECT_FALSE(component.HasDynamicStyleValue(styles->GetRoot(), {}, "enumValue"));
    std::unique_ptr<JsonAdapter> nonObject = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(nonObject, nullptr);
    EXPECT_FALSE(component.HasDynamicStyleValue(nonObject->GetRoot(), dynamicKeys, "enumValue"));

    component.ReportDynamicStyleTypeMismatch("", "string");
    component.ReportDynamicStyleInvalidValue("", "is invalid");
    EXPECT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 0U);

    component.ReportDynamicStyleTypeMismatch("styles.directType", "string");
    component.ReportDynamicStyleInvalidValue("styles.directValue", "is invalid");
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.directType"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.directValue"), 1U);

    ClearWarnings(mockNapiPtr_);
    component.ValidateDynamicStyleEnumProperty(styles->GetRoot(), dynamicKeys, "missing", { "center" });
    component.ValidateDynamicStyleEnumProperty(styles->GetRoot(), dynamicKeys, "enumValue", { nullptr, "center" });
    component.ValidateDynamicStyleNumberProperty(styles->GetRoot(), dynamicKeys, "missing");
    component.ValidateDynamicStyleNumberProperty(styles->GetRoot(), dynamicKeys, "numberValue", true);
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());

    component.ValidateDynamicStyleEnumProperty(styles->GetRoot(), dynamicKeys, "enumTypeMismatch", { "center" });
    component.ValidateDynamicStyleEnumProperty(styles->GetRoot(), dynamicKeys, "badEnum", { "center" });
    component.ValidateDynamicStyleNumberProperty(styles->GetRoot(), dynamicKeys, "numberTypeMismatch");
    component.ValidateDynamicStyleNumberProperty(styles->GetRoot(), dynamicKeys, "negativeNumber");
    component.ValidateDynamicStyleNumberProperty(styles->GetRoot(), dynamicKeys, "zeroNumber", true);

    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.enumTypeMismatch"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.badEnum"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "styles.numberTypeMismatch"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.negativeNumber"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "styles.zeroNumber"), 1U);
}

/**
 * @tc.name: L0_should_validate_row_column_and_stack_dynamic_style_values_in_own_components
 * @tc.desc: 覆盖 Row、Column、Stack 下沉后的组件专属动态样式 enum 校验
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest,
    L0_should_validate_row_column_and_stack_dynamic_style_values_in_own_components)
{
    DynamicStyleColumnProbeComponent column;
    PrepareWarningComponent(column, "column");
    auto columnStyles = ParseJson(R"({
        "justifyContent": "spaceAround",
        "alignItems": "bad"
    })");
    ASSERT_NE(columnStyles, nullptr);
    column.ValidateDynamicStylesForTest(columnStyles->GetRoot(), { "justifyContent", "alignItems" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "column.styles.alignItems"), 1U);

    ClearWarnings(mockNapiPtr_);
    DynamicStyleRowProbeComponent row;
    PrepareWarningComponent(row, "row");
    auto rowStyles = ParseJson(R"({
        "justifyContent": "spaceEvenly",
        "alignItems": 1,
        "wrap": "bad"
    })");
    ASSERT_NE(rowStyles, nullptr);
    row.ValidateDynamicStylesForTest(rowStyles->GetRoot(), { "justifyContent", "alignItems", "wrap" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "row.styles.alignItems"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.wrap"), 1U);

    ClearWarnings(mockNapiPtr_);
    DynamicStyleStackProbeComponent stack;
    PrepareWarningComponent(stack, "stack");
    auto stackStyles = ParseJson(R"({ "alignContent": "bottomEnd" })");
    ASSERT_NE(stackStyles, nullptr);
    stack.ValidateDynamicStylesForTest(stackStyles->GetRoot(), { "alignContent" });
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());

    stackStyles = ParseJson(R"({ "alignContent": "bad" })");
    ASSERT_NE(stackStyles, nullptr);
    stack.ValidateDynamicStylesForTest(stackStyles->GetRoot(), { "alignContent" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "stack.styles.alignContent"), 1U);
}

/**
 * @tc.name: L0_should_validate_grid_dynamic_template_and_gap_values_in_grid_component
 * @tc.desc: 覆盖 Grid template 参数错误上报 warning，以及 gap 数字校验分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest,
    L0_should_validate_grid_dynamic_template_and_gap_values_in_grid_component)
{
    DynamicStyleGridProbeComponent grid;
    PrepareWarningComponent(grid, "grid");

    auto validStyles = ParseJson(R"({
        "columnsTemplate": "1fr 1fr",
        "rowsTemplate": { "xs": "1fr", "md": "2fr" },
        "columnsGap": 0,
        "rowsGap": 8
    })");
    ASSERT_NE(validStyles, nullptr);
    grid.ValidateDynamicStylesForTest(
        validStyles->GetRoot(), { "columnsTemplate", "rowsTemplate", "columnsGap", "rowsGap" });
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());

    auto invalidStyles = ParseJson(R"({
        "columnsTemplate": "",
        "rowsTemplate": 1,
        "columnsGap": "bad",
        "rowsGap": -1
    })");
    ASSERT_NE(invalidStyles, nullptr);
    grid.ValidateDynamicStylesForTest(
        invalidStyles->GetRoot(), { "columnsTemplate", "rowsTemplate", "columnsGap", "rowsGap" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.columnsTemplate"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.rowsTemplate"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.columnsGap"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.rowsGap"), 1U);

    ClearWarnings(mockNapiPtr_);
    auto breakpointTypeMismatch = ParseJson(R"({ "columnsTemplate": { "xs": 1 } })");
    ASSERT_NE(breakpointTypeMismatch, nullptr);
    grid.ValidateDynamicStylesForTest(breakpointTypeMismatch->GetRoot(), { "columnsTemplate" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.columnsTemplate.xs"), 1U);

    ClearWarnings(mockNapiPtr_);
    auto breakpointEmpty = ParseJson(R"({ "columnsTemplate": { "sm": "" } })");
    ASSERT_NE(breakpointEmpty, nullptr);
    grid.ValidateDynamicStylesForTest(breakpointEmpty->GetRoot(), { "columnsTemplate" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.columnsTemplate.sm"), 1U);

    ClearWarnings(mockNapiPtr_);
    auto emptyObject = ParseJson(R"({ "columnsTemplate": {} })");
    ASSERT_NE(emptyObject, nullptr);
    grid.ValidateDynamicStylesForTest(emptyObject->GetRoot(), { "columnsTemplate" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.columnsTemplate"), 1U);
}

/**
 * @tc.name: L0_should_validate_list_dynamic_nested_scroll_values_in_list_component
 * @tc.desc: 覆盖 List 下沉后的方向、滚动条、nestedScroll 字符串和对象子字段动态校验分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest,
    L0_should_validate_list_dynamic_nested_scroll_values_in_list_component)
{
    DynamicStyleListProbeComponent list;
    PrepareWarningComponent(list, "list");

    auto validStyles = ParseJson(R"({
        "listDirection": "horizontal",
        "scrollBar": "auto",
        "nestedScroll": { "scrollForward": "selfOnly", "scrollBackward": "parentFirst" }
    })");
    ASSERT_NE(validStyles, nullptr);
    list.ValidateDynamicStylesForTest(validStyles->GetRoot(), { "listDirection", "scrollBar", "nestedScroll" });
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());

    auto invalidStyles = ParseJson(R"({
        "listDirection": "diagonal",
        "scrollBar": 1,
        "nestedScroll": "bad"
    })");
    ASSERT_NE(invalidStyles, nullptr);
    list.ValidateDynamicStylesForTest(invalidStyles->GetRoot(), { "listDirection", "scrollBar", "nestedScroll" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.listDirection"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "list.styles.scrollBar"), 1U);
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.nestedScroll"), 1U);

    ClearWarnings(mockNapiPtr_);
    auto nonObjectNested = ParseJson(R"({ "nestedScroll": 1 })");
    ASSERT_NE(nonObjectNested, nullptr);
    list.ValidateDynamicStylesForTest(nonObjectNested->GetRoot(), { "nestedScroll" });
    EXPECT_EQ(CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "list.styles.nestedScroll"), 1U);

    ClearWarnings(mockNapiPtr_);
    auto nestedObjectInvalid = ParseJson(R"({
        "nestedScroll": { "scrollForward": 1, "scrollBackward": "bad" }
    })");
    ASSERT_NE(nestedObjectInvalid, nullptr);
    list.ValidateDynamicStylesForTest(nestedObjectInvalid->GetRoot(), { "nestedScroll" });
    EXPECT_EQ(
        CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "list.styles.nestedScroll.scrollForward"), 1U);
    EXPECT_EQ(
        CountWarnings(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.nestedScroll.scrollBackward"), 1U);
}

/**
 * @tc.name: L0_should_cover_column_style_application_and_child_margin_guards
 * @tc.desc: 覆盖 Column 样式枚举映射、非 object styles、itemMargin 声明和 child margin 保护分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest, L0_should_cover_column_style_application_and_child_margin_guards)
{
    DynamicStyleColumnProbeComponent column;

    struct JustifyCase {
        const char* value;
        int32_t expected;
    };
    const std::vector<JustifyCase> justifyCases = { { "center", ARKUI_FLEX_ALIGNMENT_CENTER },
        { "end", ARKUI_FLEX_ALIGNMENT_END }, { "spaceAround", ARKUI_FLEX_ALIGNMENT_SPACE_AROUND },
        { "spaceBetween", ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN }, { "spaceEvenly", ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY },
        { "bad", ARKUI_FLEX_ALIGNMENT_START } };

    for (const auto& testCase : justifyCases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        auto styles = ParseJson(R"({ "justifyContent": ")" + std::string(testCase.value) + R"(" })");
        ASSERT_NE(styles, nullptr);
        column.ApplyStylesForTest(styles->GetRoot());
        const auto* record =
            FindLastAttributeForNode(*mockArkUIPtr_, column.GetNativeView(), NODE_COLUMN_JUSTIFY_CONTENT);
        ASSERT_NE(record, nullptr);
        ASSERT_FALSE(record->values.empty());
        EXPECT_EQ(record->values[0].i32, testCase.expected);
    }

    struct AlignCase {
        const char* value;
        int32_t expected;
    };
    const std::vector<AlignCase> alignCases = { { "end", ARKUI_HORIZONTAL_ALIGNMENT_END },
        { "bad", ARKUI_HORIZONTAL_ALIGNMENT_START } };
    for (const auto& testCase : alignCases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        auto styles = ParseJson(R"({ "alignItems": ")" + std::string(testCase.value) + R"(" })");
        ASSERT_NE(styles, nullptr);
        column.ApplyStylesForTest(styles->GetRoot());
        const auto* record = FindLastAttributeForNode(*mockArkUIPtr_, column.GetNativeView(), NODE_COLUMN_ALIGN_ITEMS);
        ASSERT_NE(record, nullptr);
        ASSERT_FALSE(record->values.empty());
        EXPECT_EQ(record->values[0].i32, testCase.expected);
    }

    mockArkUIPtr_->setAttributeRecords_.clear();
    auto nonObjectStyles = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    column.ApplyStylesForTest(nonObjectStyles->GetRoot());
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    PropertyDeclaration declaration = column.GetPrivatePropertyDeclarationForTest("itemMargin");
    ASSERT_EQ(declaration.name, "itemMargin");
    ASSERT_TRUE(static_cast<bool>(declaration.applyValue));
    auto itemMargin = JsonAdapter::CreateNumber(10.0);
    ASSERT_NE(itemMargin, nullptr);
    declaration.applyValue(itemMargin->GetRoot());

    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xD001), false);
    column.OnAddChildForTest(child, 0);
    column.OnMoveChildForTest(child, 0, 0);

    ArkUI_NodeHandle originalView = column.GetNativeView();
    column.SetNativeViewForTest(nullptr);
    auto styles = ParseJson(R"({ "justifyContent": "center" })");
    ASSERT_NE(styles, nullptr);
    column.ApplyStylesForTest(styles->GetRoot());
    column.OnRemoveChildForTest(child);
    column.SetNativeViewForTest(originalView);
}

/**
 * @tc.name: L0_should_cover_row_style_application_and_native_view_guards
 * @tc.desc: 覆盖 Row justify/align/wrap 映射、非 object styles 以及 nativeView 为空保护分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest, L0_should_cover_row_style_application_and_native_view_guards)
{
    DynamicStyleRowProbeComponent row;

    struct RowCase {
        const char* justify;
        const char* align;
        int32_t expectedJustify;
        int32_t expectedAlign;
    };
    const std::vector<RowCase> cases = { { "end", "top", ARKUI_FLEX_ALIGNMENT_END, ARKUI_ITEM_ALIGNMENT_START },
        { "spaceAround", "bad", ARKUI_FLEX_ALIGNMENT_SPACE_AROUND, ARKUI_ITEM_ALIGNMENT_CENTER },
        { "spaceBetween", "bottom", ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN, ARKUI_ITEM_ALIGNMENT_END },
        { "spaceEvenly", "center", ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY, ARKUI_ITEM_ALIGNMENT_CENTER } };

    for (const auto& testCase : cases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        auto styles = ParseJson(R"({
            "justifyContent": ")" +
                                std::string(testCase.justify) + R"(",
            "alignItems": ")" + std::string(testCase.align) +
                                R"(",
            "wrap": "wrap"
        })");
        ASSERT_NE(styles, nullptr);
        row.ApplyStylesForTest(styles->GetRoot());
        const auto* record = FindLastAttributeForNode(*mockArkUIPtr_, row.GetNativeView(), NODE_FLEX_OPTION);
        ASSERT_NE(record, nullptr);
        ASSERT_GE(record->values.size(), 4U);
        EXPECT_EQ(record->values[2].i32, testCase.expectedJustify);
        EXPECT_EQ(record->values[3].i32, testCase.expectedAlign);
    }

    mockArkUIPtr_->setAttributeRecords_.clear();
    auto nonObjectStyles = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    row.ApplyStylesForTest(nonObjectStyles->GetRoot());
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    ArkUI_NodeHandle originalView = row.GetNativeView();
    row.SetNativeViewForTest(nullptr);
    auto styles = ParseJson(R"({ "justifyContent": "center", "wrap": "wrap" })");
    ASSERT_NE(styles, nullptr);
    row.ApplyStylesForTest(styles->GetRoot());
    row.SetNativeViewForTest(originalView);
}

/**
 * @tc.name: L0_should_cover_list_style_application_and_lazy_child_guards
 * @tc.desc: 覆盖 List scrollBar/nestedScroll 映射、非 object styles、lazy add child 和 move child 保护分支
 * @tc.type:
 */
TEST_F(ExtendedComponentDynamicStyleValidationTddTest, L0_should_cover_list_style_application_and_lazy_child_guards)
{
    DynamicStyleListProbeComponent list;

    auto styles = ParseJson(R"({
        "listDirection": "horizontal",
        "scrollBar": "off",
        "nestedScroll": { "scrollForward": "selfOnly", "scrollBackward": "paraller" }
    })");
    ASSERT_NE(styles, nullptr);
    list.ApplyStylesForTest(styles->GetRoot());
    const auto* direction = FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_LIST_DIRECTION);
    const auto* scrollBar =
        FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_SCROLL_BAR_DISPLAY_MODE);
    const auto* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(scrollBar, nullptr);
    ASSERT_NE(nestedScroll, nullptr);
    EXPECT_EQ(direction->values[0].i32, ARKUI_AXIS_HORIZONTAL);
    EXPECT_EQ(scrollBar->values[0].i32, ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_PARALLEL);

    mockArkUIPtr_->setAttributeRecords_.clear();
    styles = ParseJson(R"({ "scrollBar": "on", "nestedScroll": "selfOnly" })");
    ASSERT_NE(styles, nullptr);
    list.ApplyStylesForTest(styles->GetRoot());
    scrollBar = FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_SCROLL_BAR_DISPLAY_MODE);
    nestedScroll = FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(scrollBar, nullptr);
    ASSERT_NE(nestedScroll, nullptr);
    EXPECT_EQ(scrollBar->values[0].i32, ARKUI_SCROLL_BAR_DISPLAY_MODE_ON);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);

    mockArkUIPtr_->setAttributeRecords_.clear();
    auto nonObjectStyles = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    list.ApplyStylesForTest(nonObjectStyles->GetRoot());
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    list.OnConfigChange(ThemeContext());
    auto adapterNode = std::make_shared<ListAdapterNode>();
    list.SetAdapterNode(adapterNode);
    mockArkUIPtr_->setAttributeRecords_.clear();
    list.SetLazyMode(true);
    EXPECT_NE(FindLastAttributeForNode(*mockArkUIPtr_, list.GetNativeView(), NODE_LIST_NODE_ADAPTER), nullptr);

    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xD101), false);
    list.OnAddChildForTest(child, 0);

    DynamicStyleListProbeComponent noViewList;
    ArkUI_NodeHandle originalView = noViewList.GetNativeView();
    noViewList.SetNativeViewForTest(nullptr);
    noViewList.OnAddChildForTest(child, 0);
    noViewList.OnMoveChildForTest(nullptr, 0, 0);
    noViewList.SetNativeViewForTest(originalView);
}

} // namespace
