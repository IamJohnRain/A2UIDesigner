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

/**
 * @file ExtendedStyleResolverDfxTest.cpp
 * @brief DFX (Design for eXcellence) 校验场景测试
 *
 * 根据 A2UI_通用样式属性_DFX校验场景.xlsx 中 22 条校验场景，
 * 结合 style_properties_common_test_cases.xlsx 中通用测试用例的边界值，
 * 验证非法样式输入时 PushStyleValidationIssue 的上报行为。
 *
 * 每个用例验证：
 *   1. issues 向量中包含 ERROR_CODE_INVALID_VALUE
 *   2. issue.path 与属性名匹配
 *   3. 非法值不会设置到 ArkUI 节点
 *   4. onError 回调通过 SurfaceController.emitSchemaWarnings 触发
 */

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

#define private public
#include "components/extended/ExtendedStyleResolver.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleTypes.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#undef private

#include "TestFixture.h"

using namespace NativeModule;

namespace {

// Helper: create an ArkUINodeApiAdapter wired to a dummy node.
ArkUINodeApiAdapter MakeTestApplier(ArkUI_NodeHandle node, std::function<void()> onResetCommonMargin = nullptr,
    std::function<void(const std::function<void()>&)> onClickRegistrar = nullptr)
{
    return ArkUINodeApiAdapter([node]() { return node; }, []() { return std::string("test-component"); },
        ArkUINodeApiAdapter::EdgeSetter(), onResetCommonMargin ? onResetCommonMargin : []() {},
        onClickRegistrar ? onClickRegistrar : [](const std::function<void()>&) {});
}

class ExtendedStyleResolverDfxTest : public A2UITest {
protected:
    ArkUI_NodeHandle testNode_ = reinterpret_cast<ArkUI_NodeHandle>(0xA500);
};

TEST_F(ExtendedStyleResolverDfxTest, CommonStyles_NullValues_ReportInvalidValue)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({
        "width": null,
        "height": null,
        "constraintSize": null,
        "backgroundImage": null,
        "backgroundImageSizeWithStyle": null,
        "margin": null,
        "padding": null,
        "borderRadius": null,
        "borderWidth": null,
        "clip": null,
        "backgroundColor": null,
        "borderColor": null,
        "linearGradient": null,
        "layoutWeight": null,
        "flexShrink": null,
        "shadow": null,
        "visibility": null
    })");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    const std::set<std::string> expectedPaths = { "styles.width", "styles.height", "styles.constraintSize",
        "styles.backgroundImage", "styles.backgroundImageSizeWithStyle", "styles.margin", "styles.padding",
        "styles.borderRadius", "styles.borderWidth", "styles.clip", "styles.backgroundColor", "styles.borderColor",
        "styles.linearGradient", "styles.layoutWeight", "styles.flexShrink", "styles.shadow", "styles.visibility" };
    std::set<std::string> actualPaths;
    for (const auto& issue : issues) {
        EXPECT_EQ(issue.code, "ERROR_CODE_INVALID_VALUE") << issue.path;
        actualPaths.insert(issue.path);
    }
    EXPECT_EQ(actualPaths, expectedPaths);
    EXPECT_EQ(issues.size(), expectedPaths.size());
}

// =============================================================================
// DFX-01: backgroundImageSizeWithStyle 字符串枚举非法
// 场景: backgroundImageSizeWithStyle 传入非法字符串，不在 cover/contain/auto/fill 枚举中
// 举例: backgroundImageSizeWithStyle="square"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyDecorationStyles -> ApplyBackgroundImageSize:
//        ParseBackgroundImageSize -> ParseImageSizeToken -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX01_BackgroundImageSizeWithStyle_InvalidStringEnum)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    // 1. 传入不在 cover/contain/auto/fill 枚举中的字符串 "square"
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "square"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    // 验证: issue 上报 ERROR_CODE_INVALID_VALUE
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");

    // 验证: 非法值应显式恢复为 Auto
    bool restoredAuto = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE && !rec.values.empty()) {
            restoredAuto = true;
            EXPECT_EQ(rec.values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
        }
    }
    EXPECT_TRUE(restoredAuto);
}

// DFX-01 扩展: 通用用例中的其他非法字符串值
TEST_F(ExtendedStyleResolverDfxTest, DFX01_BackgroundImageSizeWithStyle_InvalidString_InvalidLiteral)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": "invalid"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");
}

// DFX-01 扩展: 数值类型传入 backgroundImageSizeWithStyle
TEST_F(ExtendedStyleResolverDfxTest, DFX01_BackgroundImageSizeWithStyle_NumberType)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImageSizeWithStyle": 42})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSizeWithStyle");
}

// =============================================================================
// DFX-02: backgroundImageSizeWithStyle 对象格式非法
// 场景: backgroundImageSizeWithStyle 传入对象 {width, height}，width/height 值非法（非 VP/FP/PERCENT 或负数）
// 举例: backgroundImageSizeWithStyle={width: -10, height: "abc"}
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyBackgroundImageSize: ParseBackgroundImageSizeWithPercent 校验 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX02_BackgroundImageSizeWithStyle_ObjectInvalidValues)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    // 传入 width 为负数, height 为非数值字符串
    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": -10, "height": "abc"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_GE(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSize");
}

// DFX-02 扩展: 对象缺少 width 字段时仅应用 height，不报 issue
TEST_F(ExtendedStyleResolverDfxTest, DFX02_BackgroundImageSizeWithStyle_ObjectMissingWidth)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"height": "200vp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    // 缺少 width 时 width 默认为 0，height 正常应用，不产生 issue
    EXPECT_EQ(issues.size(), 0u);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 0.0f);
            EXPECT_FLOAT_EQ(rec.values[1].f32, 200.0f);
        }
    }
    EXPECT_TRUE(found);
}

// DFX-02 扩展: 对象缺少 height 字段时仅应用 width，不报 issue
TEST_F(ExtendedStyleResolverDfxTest, DFX02_BackgroundImageSizeWithStyle_ObjectMissingHeight)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {"width": "100vp"}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    // 缺少 height 时 height 默认为 0，width 正常应用，不产生 issue
    EXPECT_EQ(issues.size(), 0u);
    bool found = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE) {
            found = true;
            EXPECT_FLOAT_EQ(rec.values[0].f32, 100.0f);
            EXPECT_FLOAT_EQ(rec.values[1].f32, 0.0f);
        }
    }
    EXPECT_TRUE(found);
}

// DFX-02 扩展: 对象为空 (无 width/height)
TEST_F(ExtendedStyleResolverDfxTest, DFX02_BackgroundImageSizeWithStyle_EmptyObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImageSize": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.backgroundImageSize");
}

// =============================================================================
// DFX-03: flexShrink 类型校验
// 场景: flexShrink 传入非数字类型
// 举例: flexShrink="abc"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyCommonNodeStyles: ParseFlexShrink -> PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX03_FlexShrink_InvalidType_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"flexShrink": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.flexShrink");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_FLEX_SHRINK);
    }
}

// DFX-03 扩展: flexShrink 传入布尔值 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX03_FlexShrink_InvalidType_Bool)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"flexShrink": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.flexShrink");
}

// DFX-03 扩展: flexShrink 传入对象 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX03_FlexShrink_InvalidType_Object)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"flexShrink": {"value": 1}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.flexShrink");
}

// =============================================================================
// DFX-04: width 非法单位
// 场景: width 传入不支持的后缀单位
// 举例: width="100px"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE, "unsupported unit")
// 实现: ApplyDimension: ParseDimensionUnitSuffix 失败 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX04_Width_UnsupportedUnit_Px)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"width": "100px"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH);
        EXPECT_NE(rec.attribute, NODE_WIDTH_PERCENT);
    }
}

// DFX-04 扩展: width 传入非数值非字符串 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX04_Width_InvalidType_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"width": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");
}

// DFX-04 扩展: width 传入无法解析的字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX04_Width_UnparseableString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"width": "not_a_dimension"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");
}

// =============================================================================
// DFX-05: width 百分比转换失败
// 场景: width 为 PERCENT/MATCH_PARENT 但转换失败（负值或非有限值）
// 举例: width="-10%"
// 预期: 忽略该字段, PushStyleValidationIssue
// 实现: ApplyDimension: DimensionToFloat -> ConvertDimensionToFloat 内 isfinite/value<0 校验
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX05_Width_NegativePercent)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"width": "-10%"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_WIDTH_PERCENT);
    }
}

// DFX-05 扩展: width 负值 VP (通用用例: 负数数值)
TEST_F(ExtendedStyleResolverDfxTest, DFX05_Width_NegativeNumber)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"width": -100})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.width");
}

// =============================================================================
// DFX-06: height 类型校验
// 场景: height 传入非数字非字符串的非法值
// 举例: height=[]
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplySizeStyles -> ApplyDimension: ParseDimension 校验
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX06_Height_InvalidType_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"height": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.height");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_HEIGHT);
        EXPECT_NE(rec.attribute, NODE_HEIGHT_PERCENT);
    }
}

// DFX-06 扩展: height 布尔类型 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX06_Height_InvalidType_Bool)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"height": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.height");
}

// DFX-06 扩展: height 非法单位 (与 width 对称)
TEST_F(ExtendedStyleResolverDfxTest, DFX06_Height_UnsupportedUnit_Px)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"height": "100px"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.height");
}

// =============================================================================
// DFX-07: constraintSize 非对象
// 场景: constraintSize 传入非 Object 类型
// 举例: constraintSize="100" 或 constraintSize=50
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyCommonNodeStyles: ParseConstraintSizeStyle -> IsObject 校验
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX07_ConstraintSize_NonObject_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"constraintSize": "100"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.constraintSize");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// DFX-07 扩展: constraintSize 传入数值
TEST_F(ExtendedStyleResolverDfxTest, DFX07_ConstraintSize_NonObject_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"constraintSize": 50})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.constraintSize");
}

// DFX-07 扩展: constraintSize 传入布尔值 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX07_ConstraintSize_NonObject_Bool)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"constraintSize": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.constraintSize");
}

// =============================================================================
// DFX-08: constraintSize 字段值非法
// 场景: constraintSize 子字段值非法（非 VP/FP/PERCENT 单位、负数、非有限值）
// 举例: constraintSize={minWidth: "100px", maxWidth: -1}
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ParseConstraintSizeStyle: ParseDimension + ConvertDimensionToFloat + isfinite && >=0
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX08_ConstraintSize_InvalidFieldValues)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "100px", "maxWidth": -1}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.constraintSize");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// DFX-08 扩展: constraintSize 不支持百分比单位 (通用用例: constraintSize 仅支持 vp/fp)
TEST_F(ExtendedStyleResolverDfxTest, DFX08_ConstraintSize_PercentFieldRejected)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    // 无 dispatchContext 时百分比路径无法 dispatch，期望 constraintSize 整体不设置
    auto styles = JsonAdapter::Parse(R"({"constraintSize": {"minWidth": "10%"}})");
    ASSERT_NE(styles, nullptr);
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier);

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CONSTRAINT_SIZE);
    }
}

// =============================================================================
// DFX-09: backgroundImage 类型校验
// 场景: backgroundImage 传入非字符串类型
// 举例: backgroundImage={} 或 backgroundImage=123
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyBackgroundImage: ParseBackgroundImage 要求 IsString
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX09_BackgroundImage_InvalidType_Object)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImage": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImage");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_IMAGE);
    }
}

// DFX-09 扩展: backgroundImage 传入数值
TEST_F(ExtendedStyleResolverDfxTest, DFX09_BackgroundImage_InvalidType_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundImage": 123})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundImage");
}

// =============================================================================
// DFX-10: margin 非法输入
// 场景: margin 传入无法解析的值（非数字、非字符串、非对象）
// 举例: margin=[] 或 margin="abc"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyEdgeStyles: ParseEdgeStyle -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX10_Margin_InvalidInput_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"margin": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
            EXPECT_EQ(issue.code, "ERROR_CODE_INVALID_VALUE");
        }
    }
    EXPECT_TRUE(found);

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_MARGIN);
        EXPECT_NE(rec.attribute, NODE_MARGIN_PERCENT);
    }
}

// DFX-10 扩展: margin 传入无法解析的字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX10_Margin_InvalidInput_UnparseableString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"margin": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// DFX-11: margin 维度转换失败
// 场景: margin 子字段值无法转换为合法浮点数
// 举例: margin={top: "abc", right: -10}
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyEdgeStyles: DimensionToFloat 失败 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX11_Margin_DimensionConversionFailure)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"margin": {"top": "abc", "right": -10}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.margin") {
            found = true;
            EXPECT_EQ(issue.code, "ERROR_CODE_INVALID_VALUE");
        }
    }
    EXPECT_TRUE(found);

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_MARGIN);
    }
}

// =============================================================================
// DFX-12: borderRadius 非法输入
// 场景: borderRadius 传入无法解析的值
// 举例: borderRadius=[] 或 borderRadius="abc"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyDecorationStyles -> ApplyRadius: ParseRadius 失败
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX12_BorderRadius_InvalidInput_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"borderRadius": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderRadius");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_RADIUS);
        EXPECT_NE(rec.attribute, NODE_BORDER_RADIUS_PERCENT);
    }
}

// DFX-12 扩展: borderRadius 传入非法字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX12_BorderRadius_InvalidInput_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"borderRadius": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderRadius");
}

// =============================================================================
// DFX-13: visibility 枚举校验
// 场景: visibility 传入非 visible/hidden/none 枚举的值
// 举例: visibility="collapsed" 或 visibility=0
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyDecorationStyles: ParseVisibility 要求 IsString + 枚举匹配
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX13_Visibility_InvalidEnum_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"visibility": "collapsed"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.visibility");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_VISIBILITY);
    }
}

// DFX-13 扩展: visibility 传入数值 (通用用例: visibility 接受字符串而非布尔值/数值)
TEST_F(ExtendedStyleResolverDfxTest, DFX13_Visibility_InvalidType_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"visibility": 0})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.visibility");
}

// =============================================================================
// DFX-14: clip 类型校验
// 场景: clip 传入非布尔值
// 举例: clip="true" 或 clip=1
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyCommonNodeStyles: ParseClip 要求 IsBool
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX14_Clip_InvalidType_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"clip": "true"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.clip");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_CLIP);
    }
}

// DFX-14 扩展: clip 传入数值 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX14_Clip_InvalidType_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"clip": 1})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.clip");
}

// =============================================================================
// DFX-15: backgroundColor 格式校验
// 场景: backgroundColor 传入无效颜色值
// 举例: backgroundColor="not-a-color" 或 backgroundColor=[]
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyColorStyles: ParseColor 失败 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX15_BackgroundColor_InvalidColor_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundColor": "not-a-color"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundColor");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_COLOR);
    }
}

// DFX-15 扩展: backgroundColor 传入数组 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX15_BackgroundColor_InvalidType_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundColor": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.backgroundColor");
}

// DFX-15 扩展: backgroundColor 传入对象 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX15_BackgroundColor_InvalidType_Object)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"backgroundColor": {"nested": true}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.backgroundColor");
}

// =============================================================================
// DFX-16: borderWidth 非法输入
// 场景: borderWidth 传入无法解析的值
// 举例: borderWidth="abc" 或 borderWidth=[]
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyDecorationStyles: ParseDimension 失败 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX16_BorderWidth_InvalidInput_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"borderWidth": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderWidth");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_BORDER_WIDTH);
        EXPECT_NE(rec.attribute, NODE_BORDER_WIDTH_PERCENT);
    }
}

// DFX-16 扩展: borderWidth 传入布尔值 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX16_BorderWidth_InvalidInput_Bool)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"borderWidth": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.borderWidth");
}

// =============================================================================
// DFX-17: padding 非法输入
// 场景: padding 传入无法解析的值
// 举例: padding=[] 或 padding="abc"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyEdgeStyles: ParseEdgeStyle -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX17_Padding_InvalidInput_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"padding": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.padding") {
            found = true;
            EXPECT_EQ(issue.code, "ERROR_CODE_INVALID_VALUE");
        }
    }
    EXPECT_TRUE(found);

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_PADDING);
        EXPECT_NE(rec.attribute, NODE_PADDING_PERCENT);
    }
}

// DFX-17 扩展: padding 传入无法解析的字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX17_Padding_InvalidInput_UnparseableString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"padding": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.path == "styles.padding") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// DFX-18: layoutWeight 类型校验
// 场景: layoutWeight 传入非数字类型
// 举例: layoutWeight="abc"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyCommonNodeStyles: ParseNumber 失败 -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX18_LayoutWeight_InvalidType_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"layoutWeight": "abc"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.layoutWeight");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
}

// DFX-18 扩展: layoutWeight 传入布尔值 (通用用例边界)
TEST_F(ExtendedStyleResolverDfxTest, DFX18_LayoutWeight_InvalidType_Bool)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"layoutWeight": true})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.layoutWeight");
}

// =============================================================================
// DFX-19: shadow 非法输入
// 场景: shadow 传入无法解析的值（不在 number/string/object 范围，或格式非法）
// 举例: shadow=[] 或 shadow="invalidShadow"
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyDecorationStyles -> ApplyShadow: ParseShadow -> PushStyleValidationIssue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX19_Shadow_InvalidInput_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"shadow": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.shadow");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

// DFX-19 扩展: shadow 传入非法字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX19_Shadow_InvalidInput_InvalidString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"shadow": "invalidShadow"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.shadow");
}

// =============================================================================
// DFX-20: shadow 枚举/范围校验
// 场景: shadow 传入的 style 值不在合法枚举范围 [OUTER_DEFAULT_XS, OUTER_FLOATING_MD]
// 举例: shadow=999 或 shadow="unknownStyle"
// 预期: 忽略该字段, PushStyleValidationIssue
// 实现: ParseShadow: ParseShadowStyleNumberValue/ParseShadowStyleToken 范围校验
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX20_Shadow_OutOfRange_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"shadow": 999})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.shadow");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_SHADOW);
        EXPECT_NE(rec.attribute, NODE_CUSTOM_SHADOW);
    }
}

// DFX-20 扩展: shadow 传入未知样式字符串 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX20_Shadow_OutOfRange_UnknownStyleString)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"shadow": "unknownStyle"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.shadow");
}

// =============================================================================
// DFX-21: linearGradient 非对象
// 场景: linearGradient 传入非 Object 类型
// 举例: linearGradient="red" 或 linearGradient=[]
// 预期: 忽略该字段, PushStyleValidationIssue(ERROR_CODE_INVALID_VALUE)
// 实现: ApplyLinearGradient: ParseLinearGradient 要求 IsObject
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX21_LinearGradient_NonObject_String)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"linearGradient": "red"})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.linearGradient");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LINEAR_GRADIENT);
    }
}

// DFX-21 扩展: linearGradient 传入数组 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX21_LinearGradient_NonObject_Array)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"linearGradient": []})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.linearGradient");
}

// DFX-21 扩展: linearGradient 传入数值 (通用用例)
TEST_F(ExtendedStyleResolverDfxTest, DFX21_LinearGradient_NonObject_Number)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"linearGradient": 42})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.linearGradient");
}

// =============================================================================
// DFX-22: linearGradient direction 枚举非法
// 场景: linearGradient direction 不在合法枚举中
// 举例: linearGradient={direction: "diagonal", colors: [...]}
// 预期: 忽略该字段, PushStyleValidationIssue
// 实现: ParseLinearGradient: ParseLinearGradientDirectionToken 枚举匹配 -> 失败
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX22_LinearGradient_InvalidDirection)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(
        R"({"linearGradient": {"direction": "diagonal", "colors": [["#FF0000", 0.0], ["#00FF00", 1.0]]}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].code, "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(issues[0].path, "styles.linearGradient");

    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        EXPECT_NE(rec.attribute, NODE_LINEAR_GRADIENT);
    }
}

// DFX-22 扩展: linearGradient 空对象 (无 direction/colors)
TEST_F(ExtendedStyleResolverDfxTest, DFX22_LinearGradient_EmptyObject)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({"linearGradient": {}})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].path, "styles.linearGradient");
}

// =============================================================================
// 综合场景: 多个属性同时非法时，每个属性独立上报 issue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX_MultipleInvalidProperties_AllReported)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles = JsonAdapter::Parse(R"({)"
                                     R"("backgroundImageSizeWithStyle": "bogus",)"
                                     R"("flexShrink": "abc",)"
                                     R"("width": "100px",)"
                                     R"("visibility": "collapsed",)"
                                     R"("clip": "true",)"
                                     R"("backgroundColor": "not-a-color",)"
                                     R"("layoutWeight": "abc")"
                                     R"(})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    // 每个非法属性都应独立上报 issue
    EXPECT_GE(issues.size(), 6u);

    // 验证每个属性的 path 都出现了
    std::set<std::string> paths;
    for (const auto& issue : issues) {
        paths.insert(issue.path);
        EXPECT_EQ(issue.code, "ERROR_CODE_INVALID_VALUE");
    }
    EXPECT_TRUE(paths.count("styles.backgroundImageSizeWithStyle") > 0);
    EXPECT_TRUE(paths.count("styles.flexShrink") > 0);
    EXPECT_TRUE(paths.count("styles.width") > 0);
    EXPECT_TRUE(paths.count("styles.visibility") > 0);
    EXPECT_TRUE(paths.count("styles.clip") > 0);
    EXPECT_TRUE(paths.count("styles.backgroundColor") > 0);

    // 除背景图尺寸恢复 Auto 外，其他非法属性都不应设置到节点
    bool restoredBackgroundImageSizeAuto = false;
    for (const auto& rec : mockArkUIPtr_->setAttributeRecords_) {
        if (rec.attribute == NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE && !rec.values.empty()) {
            restoredBackgroundImageSizeAuto = true;
            EXPECT_EQ(rec.values[0].i32, static_cast<int32_t>(A2UIImageSize::AUTO));
            continue;
        }
        EXPECT_NE(rec.attribute, NODE_FLEX_SHRINK);
        EXPECT_NE(rec.attribute, NODE_WIDTH);
        EXPECT_NE(rec.attribute, NODE_VISIBILITY);
        EXPECT_NE(rec.attribute, NODE_CLIP);
        EXPECT_NE(rec.attribute, NODE_BACKGROUND_COLOR);
        EXPECT_NE(rec.attribute, NODE_LAYOUT_WEIGHT);
    }
    EXPECT_TRUE(restoredBackgroundImageSizeAuto);
}

// =============================================================================
// 回归验证: 全部合法属性时不应上报任何 issue
// =============================================================================
TEST_F(ExtendedStyleResolverDfxTest, DFX_AllValidProperties_NoIssues)
{
    ArkUINodeApiAdapter applier = MakeTestApplier(testNode_);
    mockArkUIPtr_->setAttributeRecords_.clear();

    auto styles =
        JsonAdapter::Parse(R"({)"
                           R"("width": "100vp", "height": "50vp", )"
                           R"("backgroundColor": "#FF0000", "borderColor": "#00FF00", )"
                           R"("padding": "10vp", "margin": "5vp", "borderWidth": "2vp", "borderRadius": "8vp", )"
                           R"("visibility": "visible", "clip": true, "flexShrink": 0.5, "layoutWeight": 1, )"
                           R"("backgroundImage": "test.png", "backgroundImageSizeWithStyle": "cover", )"
                           R"("shadow": 0, "linearGradient": {"angle": 90, "colors": [["#FF0000", 0], ["#00FF00", 1]]})"
                           R"(})");
    ASSERT_NE(styles, nullptr);
    std::vector<DescriptorValidationIssue> issues;
    ExtendedStyleResolver::ResolveAndApply(styles->GetRoot(), applier, std::nullopt, issues);

    EXPECT_TRUE(issues.empty());
}

} // namespace
