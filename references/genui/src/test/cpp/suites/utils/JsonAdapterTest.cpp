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

#include "utils/JsonAdapter.h"

#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "components/extended/ExtendedStyleResolver.h"
#include "composition/ChildListParser.h"
#include "composition/TemplateInstantiator.h"
#include "data/DataModel.h"
#include "data/PathValidator.h"
#include "functions/ActionInfo.h"
#include "functions/FunctionCallInfo.h"
#include "functions/FunctionResult.h"
#include "styles/StyleApplyUtils.h"
#include "styles/StyleParser.h"
#include "styles/StyleResolver.h"

#include "A2UIArkUITypeConverter.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "mock_arkui_native_provider.h"

using namespace NativeModule;

namespace {

bool HasStringValue(const std::vector<std::string>& values, const std::string& expected)
{
    for (const auto& value : values) {
        if (value == expected) {
            return true;
        }
    }
    return false;
}

const StyleBindingPlan* FindStyleBinding(const std::vector<StyleBindingPlan>& bindings, const std::string& property)
{
    for (const auto& binding : bindings) {
        if (binding.bindingProperty == property) {
            return &binding;
        }
    }
    return nullptr;
}

bool HasStyleBindingDataPath(
    const std::vector<StyleBindingPlan>& bindings, const std::string& property, const std::string& dataPath)
{
    for (const auto& binding : bindings) {
        if (binding.bindingProperty == property && binding.dataPath == dataPath) {
            return true;
        }
    }
    return false;
}

bool BindingHasGlobalDependency(const StyleBindingPlan* binding, const std::string& globalName)
{
    if (binding == nullptr) {
        return false;
    }
    return HasStringValue(binding->globalVarDeps, globalName);
}

MockArkUINativeProvider& GetRecordingProvider()
{
    MockArkUINativeProvider* activeProvider = MockArkUINativeProvider::GetActiveInstance();
    return activeProvider != nullptr ? *activeProvider : MockArkUINativeProvider::GetInstance();
}

class RecordingCommonStyleApplier : public ArkUINodeApiAdapter {
public:
    RecordingCommonStyleApplier()
        : ArkUINodeApiAdapter([this]() { return GetRootNode(); }, []() { return std::string(); },
              ArkUINodeApiAdapter::EdgeSetter(), []() {}, [](const std::function<void()>&) {})
    {
        const auto& provider = GetRecordingProvider();
        setAttributeRecordStart_ = provider.setAttributeRecords_.size();
        resetAttributeRecordStart_ = provider.resetAttributeRecords_.size();
    }

    ArkUI_NodeHandle GetRootNode() const
    {
        return const_cast<ArkUI_Node*>(&rootNode_);
    }

    void SetWidth(float width)
    {
        static_cast<void>(width);
        hasWidth_ = true;
    }

    void SetHeight(float height)
    {
        static_cast<void>(height);
        hasHeight_ = true;
    }

    void SetWidthPercent(float percent)
    {
        widthPercent_ = percent;
        hasWidthPercent_ = true;
    }

    void SetHeightPercent(float percent)
    {
        heightPercent_ = percent;
        hasHeightPercent_ = true;
    }

    void SetBackgroundColor(uint32_t color)
    {
        static_cast<void>(color);
    }

    void SetBorderRadius(float radius)
    {
        static_cast<void>(radius);
    }

    void SetBorderRadiusPercent(float topLeft, float topRight, float bottomLeft, float bottomRight)
    {
        borderRadiusPercent_[0] = topLeft;
        borderRadiusPercent_[1] = topRight;
        borderRadiusPercent_[2] = bottomLeft;
        borderRadiusPercent_[3] = bottomRight;
        hasBorderRadiusPercent_ = true;
    }

    void SetPadding(float top, float right, float bottom, float left)
    {
        static_cast<void>(top);
        static_cast<void>(right);
        static_cast<void>(bottom);
        static_cast<void>(left);
    }

    void SetPaddingPercent(float top, float right, float bottom, float left)
    {
        paddingPercent_[0] = top;
        paddingPercent_[1] = right;
        paddingPercent_[2] = bottom;
        paddingPercent_[3] = left;
        hasPaddingPercent_ = true;
    }

    void SetMargin(float top, float right, float bottom, float left)
    {
        static_cast<void>(top);
        static_cast<void>(right);
        static_cast<void>(bottom);
        static_cast<void>(left);
    }

    void SetMarginPercent(float top, float right, float bottom, float left)
    {
        marginPercent_[0] = top;
        marginPercent_[1] = right;
        marginPercent_[2] = bottom;
        marginPercent_[3] = left;
        hasMarginPercent_ = true;
    }

    void SetBorderWidthPercent(float width)
    {
        borderWidthPercent_ = width;
        hasBorderWidthPercent_ = true;
    }

    void SetNodeFloat(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, float value)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_FLEX_SHRINK) {
            hasFlexShrink_ = true;
            flexShrink_ = value;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_FONT_SIZE) {
            hasFontSize_ = true;
            fontSize_ = value;
        }
    }

    void SetNodeInt32(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, int32_t value)
    {
        if (nodeHandle != GetRootNode()) {
            return;
        }
        if (attribute == NODE_TEXT_MAX_LINES) {
            hasMaxLines_ = true;
            maxLines_ = value;
        } else if (attribute == NODE_TEXT_OVERFLOW) {
            hasTextOverflow_ = true;
            textOverflow_ = value;
        } else if (attribute == NODE_TEXT_ALIGN) {
            hasTextAlign_ = true;
            textAlign_ = value;
        } else if (attribute == NODE_FONT_WEIGHT) {
            hasFontWeight_ = true;
            fontWeight_ = value;
        }
    }

    void SetNodeUint32(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, uint32_t value)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_FONT_COLOR) {
            hasFontColor_ = true;
            fontColor_ = value;
        }
    }

    void SetNodeBool(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, bool value)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_CLIP) {
            hasClip_ = true;
            clip_ = value;
        }
    }

    void SetNodeString(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const std::string& value)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_BACKGROUND_IMAGE) {
            hasBackgroundImage_ = true;
            backgroundImage_ = value;
        }
    }

    void SetNodeNumberArray(
        ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const std::vector<ArkUI_NumberValue>& values)
    {
        if (nodeHandle == GetRootNode() && attribute == NODE_TEXT_DECORATION) {
            hasDecoration_ = true;
            decorationValues_ = values;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_SHADOW) {
            hasShadow_ = true;
            shadowValues_ = values;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_CUSTOM_SHADOW) {
            hasCustomShadow_ = true;
            customShadowValues_ = values;
        }
    }

    void SetLinearGradient(ArkUI_NodeHandle nodeHandle, const StyleLinearGradient& gradient)
    {
        static_cast<void>(nodeHandle);
        static_cast<void>(gradient);
    }

    void ResetNodeAttribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute)
    {
        if (nodeHandle == GetRootNode()) {
            resetAttributes_.push_back(attribute);
        }
        if (nodeHandle == GetRootNode() && attribute == NODE_BACKGROUND_IMAGE) {
            backgroundImageReset_ = true;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_SHADOW) {
            shadowReset_ = true;
        } else if (nodeHandle == GetRootNode() && attribute == NODE_CUSTOM_SHADOW) {
            customShadowReset_ = true;
        }
    }

    void RegisterOnClick(const std::function<void()>& onClick)
    {
        static_cast<void>(onClick);
    }

    bool HasWidth() const
    {
        return hasWidth_ || FindLastSetAttribute(NODE_WIDTH) != nullptr;
    }

    bool HasHeight() const
    {
        return hasHeight_ || FindLastSetAttribute(NODE_HEIGHT) != nullptr;
    }

    bool HasWidthPercent() const
    {
        return hasWidthPercent_ || FindLastSetAttribute(NODE_WIDTH_PERCENT) != nullptr;
    }

    bool HasHeightPercent() const
    {
        return hasHeightPercent_ || FindLastSetAttribute(NODE_HEIGHT_PERCENT) != nullptr;
    }

    // match_parent/wrap_content/fix_at_ideal_size now route to the ArkUI layout-policy API
    // (NODE_WIDTH_LAYOUTPOLICY / NODE_HEIGHT_LAYOUTPOLICY) instead of the legacy percent/reset path.
    bool HasLayoutPolicy(ArkUI_NodeAttributeType attribute, int32_t expectedPolicy) const
    {
        const auto* record = FindLastSetAttribute(attribute);
        return record != nullptr && !record->values.empty() && record->values.front().i32 == expectedPolicy;
    }

    bool HasPaddingPercent() const
    {
        return hasPaddingPercent_ || FindLastSetAttribute(NODE_PADDING_PERCENT) != nullptr;
    }

    const float* GetPaddingPercent() const
    {
        CopyRecordedFloatValues(NODE_PADDING_PERCENT, paddingPercent_, 4);
        return paddingPercent_;
    }

    bool HasMarginPercent() const
    {
        return hasMarginPercent_ || FindLastSetAttribute(NODE_MARGIN_PERCENT) != nullptr;
    }

    const float* GetMarginPercent() const
    {
        CopyRecordedFloatValues(NODE_MARGIN_PERCENT, marginPercent_, 4);
        return marginPercent_;
    }

    bool HasBorderRadiusPercent() const
    {
        return hasBorderRadiusPercent_ || FindLastSetAttribute(NODE_BORDER_RADIUS_PERCENT) != nullptr;
    }

    const float* GetBorderRadiusPercent() const
    {
        CopyRecordedFloatValues(NODE_BORDER_RADIUS_PERCENT, borderRadiusPercent_, 4);
        return borderRadiusPercent_;
    }

    bool HasBorderWidthPercent() const
    {
        return hasBorderWidthPercent_ || FindLastSetAttribute(NODE_BORDER_WIDTH_PERCENT) != nullptr;
    }

    float GetBorderWidthPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_BORDER_WIDTH_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return borderWidthPercent_;
    }

    float GetWidthPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_WIDTH_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return widthPercent_;
    }

    float GetHeightPercent() const
    {
        const auto* record = FindLastSetAttribute(NODE_HEIGHT_PERCENT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return heightPercent_;
    }

    bool HasFlexShrink() const
    {
        return hasFlexShrink_ || FindLastSetAttribute(NODE_FLEX_SHRINK) != nullptr;
    }

    float GetFlexShrink() const
    {
        const auto* record = FindLastSetAttribute(NODE_FLEX_SHRINK);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return flexShrink_;
    }

    bool HasBackgroundImage() const
    {
        return hasBackgroundImage_ || FindLastSetAttribute(NODE_BACKGROUND_IMAGE) != nullptr;
    }

    const std::string& GetBackgroundImage() const
    {
        const auto* record = FindLastSetAttribute(NODE_BACKGROUND_IMAGE);
        if (record != nullptr) {
            backgroundImage_ = record->stringValue;
        }
        return backgroundImage_;
    }

    bool HasClip() const
    {
        return hasClip_ || FindLastSetAttribute(NODE_CLIP) != nullptr;
    }

    bool GetClip() const
    {
        const auto* record = FindLastSetAttribute(NODE_CLIP);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32 != 0;
        }
        return clip_;
    }

    bool WasBackgroundImageReset() const
    {
        return backgroundImageReset_ || WasAttributeReset(NODE_BACKGROUND_IMAGE);
    }

    bool HasFontWeight() const
    {
        return hasFontWeight_ || FindLastSetAttribute(NODE_FONT_WEIGHT) != nullptr;
    }

    int32_t GetFontWeight() const
    {
        const auto* record = FindLastSetAttribute(NODE_FONT_WEIGHT);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32;
        }
        return fontWeight_;
    }

    bool HasFontColor() const
    {
        return hasFontColor_ || FindLastSetAttribute(NODE_FONT_COLOR) != nullptr;
    }

    uint32_t GetFontColor() const
    {
        const auto* record = FindLastSetAttribute(NODE_FONT_COLOR);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().u32;
        }
        return fontColor_;
    }

    bool HasFontSize() const
    {
        return hasFontSize_ || FindLastSetAttribute(NODE_FONT_SIZE) != nullptr;
    }

    float GetFontSize() const
    {
        const auto* record = FindLastSetAttribute(NODE_FONT_SIZE);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().f32;
        }
        return fontSize_;
    }

    bool HasMaxLines() const
    {
        return hasMaxLines_ || FindLastSetAttribute(NODE_TEXT_MAX_LINES) != nullptr;
    }

    int32_t GetMaxLines() const
    {
        const auto* record = FindLastSetAttribute(NODE_TEXT_MAX_LINES);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32;
        }
        return maxLines_;
    }

    bool HasTextOverflow() const
    {
        return hasTextOverflow_ || FindLastSetAttribute(NODE_TEXT_OVERFLOW) != nullptr;
    }

    int32_t GetTextOverflow() const
    {
        const auto* record = FindLastSetAttribute(NODE_TEXT_OVERFLOW);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32;
        }
        return textOverflow_;
    }

    bool HasTextAlign() const
    {
        return hasTextAlign_ || FindLastSetAttribute(NODE_TEXT_ALIGN) != nullptr;
    }

    int32_t GetTextAlign() const
    {
        const auto* record = FindLastSetAttribute(NODE_TEXT_ALIGN);
        if (record != nullptr && !record->values.empty()) {
            return record->values.front().i32;
        }
        return textAlign_;
    }

    bool HasDecoration() const
    {
        return hasDecoration_ || FindLastSetAttribute(NODE_TEXT_DECORATION) != nullptr;
    }

    const std::vector<ArkUI_NumberValue>& GetDecorationValues() const
    {
        CopyRecordedValues(NODE_TEXT_DECORATION, decorationValues_);
        return decorationValues_;
    }

    bool HasShadow() const
    {
        return hasShadow_ || FindLastSetAttribute(NODE_SHADOW) != nullptr;
    }

    const std::vector<ArkUI_NumberValue>& GetShadowValues() const
    {
        CopyRecordedValues(NODE_SHADOW, shadowValues_);
        return shadowValues_;
    }

    bool HasCustomShadow() const
    {
        return hasCustomShadow_ || FindLastSetAttribute(NODE_CUSTOM_SHADOW) != nullptr;
    }

    const std::vector<ArkUI_NumberValue>& GetCustomShadowValues() const
    {
        CopyRecordedValues(NODE_CUSTOM_SHADOW, customShadowValues_);
        return customShadowValues_;
    }

    bool WasShadowReset() const
    {
        return shadowReset_ || WasAttributeReset(NODE_SHADOW);
    }

    bool WasCustomShadowReset() const
    {
        return customShadowReset_ || WasAttributeReset(NODE_CUSTOM_SHADOW);
    }

    bool WasAttributeReset(ArkUI_NodeAttributeType attribute) const
    {
        for (ArkUI_NodeAttributeType resetAttribute : resetAttributes_) {
            if (resetAttribute == attribute) {
                return true;
            }
        }
        const auto& resetRecords = GetRecordingProvider().resetAttributeRecords_;
        ArkUI_NodeHandle rootNode = GetRootNode();
        for (size_t index = resetRecords.size(); index > resetAttributeRecordStart_; --index) {
            const auto& record = resetRecords[index - 1];
            if (record.nodeHandle == rootNode && record.attribute == attribute) {
                return true;
            }
        }
        return false;
    }

private:
    const MockArkUINativeProvider::SetAttributeRecord* FindLastSetAttribute(ArkUI_NodeAttributeType attribute) const
    {
        const auto& records = GetRecordingProvider().setAttributeRecords_;
        ArkUI_NodeHandle rootNode = GetRootNode();
        for (size_t index = records.size(); index > setAttributeRecordStart_; --index) {
            const auto& record = records[index - 1];
            if (record.nodeHandle == rootNode && record.attribute == attribute) {
                return &record;
            }
        }
        return nullptr;
    }

    void CopyRecordedValues(ArkUI_NodeAttributeType attribute, std::vector<ArkUI_NumberValue>& values) const
    {
        const auto* record = FindLastSetAttribute(attribute);
        if (record != nullptr) {
            values = record->values;
        }
    }

    void CopyRecordedFloatValues(ArkUI_NodeAttributeType attribute, float* values, size_t valueCount) const
    {
        const auto* record = FindLastSetAttribute(attribute);
        if (record == nullptr || record->values.size() < valueCount) {
            return;
        }
        for (size_t index = 0; index < valueCount; ++index) {
            values[index] = record->values[index].f32;
        }
    }

    size_t setAttributeRecordStart_ = 0;
    size_t resetAttributeRecordStart_ = 0;
    mutable ArkUI_Node rootNode_ {};
    float widthPercent_ = 0.0F;
    float heightPercent_ = 0.0F;
    mutable float paddingPercent_[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    mutable float marginPercent_[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    mutable float borderRadiusPercent_[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    float borderWidthPercent_ = 0.0F;
    bool hasWidth_ = false;
    bool hasHeight_ = false;
    bool hasWidthPercent_ = false;
    bool hasHeightPercent_ = false;
    bool hasPaddingPercent_ = false;
    bool hasMarginPercent_ = false;
    bool hasBorderRadiusPercent_ = false;
    bool hasBorderWidthPercent_ = false;
    float flexShrink_ = 0.0F;
    float fontSize_ = 0.0F;
    uint32_t fontColor_ = 0;
    bool clip_ = false;
    int32_t maxLines_ = 0;
    int32_t textOverflow_ = 0;
    int32_t textAlign_ = 0;
    bool hasFlexShrink_ = false;
    bool hasBackgroundImage_ = false;
    bool hasClip_ = false;
    bool backgroundImageReset_ = false;
    bool hasFontColor_ = false;
    bool hasFontSize_ = false;
    bool hasFontWeight_ = false;
    bool hasMaxLines_ = false;
    bool hasTextOverflow_ = false;
    bool hasTextAlign_ = false;
    bool hasDecoration_ = false;
    int32_t fontWeight_ = 0;
    mutable std::string backgroundImage_;
    mutable std::vector<ArkUI_NumberValue> decorationValues_;
    mutable std::vector<ArkUI_NumberValue> shadowValues_;
    mutable std::vector<ArkUI_NumberValue> customShadowValues_;
    std::vector<ArkUI_NodeAttributeType> resetAttributes_;
    bool hasShadow_ = false;
    bool hasCustomShadow_ = false;
    bool shadowReset_ = false;
    bool customShadowReset_ = false;
    bool hasBorderRadius_ = false;
    float borderRadius_ = 0.0F;
    bool hasMargin_ = false;
    bool hasBorderWidth_ = false;
    float borderWidth_ = 0.0F;
    bool hasPadding_ = false;
    bool hasWordBreak_ = false;
    int32_t wordBreak_ = 0;
    bool hasConstraintSize_ = false;
    std::vector<ArkUI_NumberValue> constraintSizeValues_;
    bool hasBackgroundImageSize_ = false;
    std::vector<ArkUI_NumberValue> backgroundImageSizeValues_;
    bool hasBackgroundImageSizeWithStyle_ = false;
    int32_t backgroundImageSizeWithStyle_ = 0;
    bool hasWidthLayoutPolicy_ = false;
    bool hasHeightLayoutPolicy_ = false;
    int32_t widthLayoutPolicy_ = 0;
    int32_t heightLayoutPolicy_ = 0;
};

} // namespace

/**
 * @tc.name: PathValidatorTest001
 * @tc.desc: Verify the following PathValidator behavior: return valid when path starts with slash.
 * @tc.type: FUNC
 */
TEST(PathValidatorTest, PathValidatorTest001)
{
    /**
     * @tc.steps: step1. Validate the target data paths through PathValidator.
     * @tc.expected: The path validation results match the expectation.
     */

    EXPECT_TRUE(IsValidDataPath("/users"));
    EXPECT_TRUE(IsValidDataPath("/users/name"));
    EXPECT_TRUE(IsValidDataPath("/a/b/c"));
}

/**
 * @tc.name: PathValidatorTest002
 * @tc.desc: Verify the following PathValidator behavior: return invalid when path does not start with slash.
 * @tc.type: FUNC
 */
TEST(PathValidatorTest, PathValidatorTest002)
{
    /**
     * @tc.steps: step1. Validate the target data paths through PathValidator.
     * @tc.expected: The path validation results match the expectation.
     */

    EXPECT_FALSE(IsValidDataPath(""));
    EXPECT_FALSE(IsValidDataPath("users"));
    EXPECT_FALSE(IsValidDataPath(" /users"));
}

/**
 * @tc.name: JsonAdapterTest001
 * @tc.desc: Verify the following JsonAdapter behavior: parse valid JSON object.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest001)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"key": "value"})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("key"), "value");
}

/**
 * @tc.name: JsonAdapterTest002
 * @tc.desc: Verify the following JsonAdapter behavior: return nullptr when parse invalid JSON.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest002)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse("{invalid}");
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @tc.name: JsonAdapterTest003
 * @tc.desc: Verify the following JsonAdapter behavior: handle array.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest003)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"([1, 2, 3])");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    ASSERT_TRUE(root.IsArray());
    EXPECT_EQ(root.GetArraySize(), 3);
    EXPECT_EQ(root.GetArrayItem(0).GetNumberValue(), 1.0);
}

/**
 * @tc.name: JsonAdapterTest004
 * @tc.desc: Verify the following JsonAdapter behavior: handle nested object.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest004)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"user": {"name": "Alice", "age": 30}})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    JsonValue user = root.GetItem("user");
    ASSERT_TRUE(user.IsObject());
    EXPECT_EQ(user.GetString("name"), "Alice");
    EXPECT_EQ(user.GetNumber("age"), 30.0);
}

/**
 * @tc.name: JsonAdapterTest005
 * @tc.desc: Verify the following JsonAdapter behavior: handle bool values.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest005)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"flag": true, "neg": false})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    EXPECT_TRUE(root.GetBool("flag"));
    EXPECT_FALSE(root.GetBool("neg"));
}

/**
 * @tc.name: JsonAdapterTest006
 * @tc.desc: Verify the following JsonAdapter behavior: handle null values.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest006)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"value": null})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    JsonValue value = root.GetItem("value");
    EXPECT_TRUE(value.IsNull());
}

/**
 * @tc.name: JsonAdapterTest007
 * @tc.desc: Verify the following JsonAdapter behavior: put and replace values.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest007)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"a": 1})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    root.PutString("b", "hello");
    EXPECT_EQ(root.GetString("b"), "hello");
    root.ReplaceNumber("a", 42.0);
    EXPECT_EQ(root.GetNumber("a"), 42.0);
}

/**
 * @tc.name: JsonAdapterTest008
 * @tc.desc: Verify the following JsonAdapter behavior: get string with fallback.
 * @tc.type: FUNC
 */
TEST(JsonAdapterTest, JsonAdapterTest008)
{
    /**
     * @tc.steps: step1. Parse or modify the target JSON content through JsonAdapter.
     * @tc.expected: The parsed JSON structure or queried value matches the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"key": "value"})");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    EXPECT_EQ(root.GetString("key", "default"), "value");
    EXPECT_EQ(root.GetString("missing", "default"), "default");
}

/**
 * @tc.name: ChildListParserTest001
 * @tc.desc: Verify the following ChildListParser behavior: parse static ids.
 * @tc.type: FUNC
 */
TEST(ChildListParserTest, ChildListParserTest001)
{
    /**
     * @tc.steps: step1. Parse the child list descriptor from the target JSON node.
     * @tc.expected: The parsed child list type and content match the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"children": ["id1", "id2", "id3"]})");
    ASSERT_NE(adapter, nullptr);
    ChildListDescriptor desc = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));
    EXPECT_EQ(desc.type, ChildListType::STATIC_IDS);
    EXPECT_EQ(desc.staticChildIds.size(), 3u);
    auto it = desc.staticChildIds.begin();
    EXPECT_EQ(*it++, "id1");
    EXPECT_EQ(*it++, "id2");
    EXPECT_EQ(*it++, "id3");
}

/**
 * @tc.name: ChildListParserTest002
 * @tc.desc: Verify the following ChildListParser behavior: parse template path.
 * @tc.type: FUNC
 */
TEST(ChildListParserTest, ChildListParserTest002)
{
    /**
     * @tc.steps: step1. Parse the child list descriptor from the target JSON node.
     * @tc.expected: The parsed child list type and content match the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({"children": {"componentId": "tmpl1", "path": "/items"}})");
    ASSERT_NE(adapter, nullptr);
    ChildListDescriptor desc = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));
    EXPECT_EQ(desc.type, ChildListType::TEMPLATE_PATH);
    EXPECT_EQ(desc.templateComponentId, "tmpl1");
    EXPECT_EQ(desc.templatePath, "/items");
}

/**
 * @tc.name: ChildListParserTest003
 * @tc.desc: Verify the following ChildListParser behavior: return invalid when children missing.
 * @tc.type: FUNC
 */
TEST(ChildListParserTest, ChildListParserTest003)
{
    /**
     * @tc.steps: step1. Parse the child list descriptor from the target JSON node.
     * @tc.expected: The parsed child list type and content match the expectation.
     */

    auto adapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(adapter, nullptr);
    ChildListDescriptor desc = ChildListParser::ParseChildren(adapter->GetRoot().GetItem("children"));
    EXPECT_EQ(desc.type, ChildListType::INVALID);
}

/**
 * @tc.name: TemplateInstantiatorTest001
 * @tc.desc: Verify the following TemplateInstantiator behavior: build instance id.
 * @tc.type: FUNC
 */
TEST(TemplateInstantiatorTest, TemplateInstantiatorTest001)
{
    /**
     * @tc.steps: step1. Build a template instance id with the target component id and index.
     * @tc.expected: The generated instance id matches the expected format.
     */

    std::string id = TemplateInstantiator::BuildInstanceId("tmpl", 0);
    EXPECT_EQ(id, "tmpl:idx:0");
    std::string id2 = TemplateInstantiator::BuildInstanceId("tmpl", 42);
    EXPECT_EQ(id2, "tmpl:idx:42");
}

/**
 * @tc.name: CatalogItemTest001
 * @tc.desc: Verify the following CatalogItem behavior: store name and type.
 * @tc.type: FUNC
 */
TEST(CatalogItemTest, CatalogItemTest001)
{
    /**
     * @tc.steps: step1. Create or update the CatalogItem instance with the target metadata.
     * @tc.expected: The CatalogItem state matches the expectation.
     */

    CatalogItem item("Button", CatalogItemType::COMPONENT);
    EXPECT_EQ(item.GetName(), "Button");
    EXPECT_EQ(item.GetType(), CatalogItemType::COMPONENT);
}

/**
 * @tc.name: CatalogItemTest002
 * @tc.desc: Verify the following CatalogItem behavior: set category and inner native.
 * @tc.type: FUNC
 */
TEST(CatalogItemTest, CatalogItemTest002)
{
    /**
     * @tc.steps: step1. Create or update the CatalogItem instance with the target metadata.
     * @tc.expected: The CatalogItem state matches the expectation.
     */

    CatalogItem item("MyComp", CatalogItemType::COMPONENT);
    item.SetCategory(CatalogCategory::OHOS_EXTENDS);
    item.SetInnerNative(true);
    EXPECT_EQ(item.GetCategory(), CatalogCategory::OHOS_EXTENDS);
    EXPECT_TRUE(item.IsInnerNative());
}

/**
 * @tc.name: CatalogItemTest003
 * @tc.desc: Verify the following CatalogItem behavior: inner native flag can be toggled independently.
 * @tc.type: FUNC
 */
TEST(CatalogItemTest, CatalogItemTest003)
{
    /**
     * @tc.steps: step1. Toggle inner native flag on the CatalogItem instance.
     * @tc.expected: The inner native flag follows the current value.
     */

    CatalogItem item("ExtButton", CatalogItemType::COMPONENT);
    EXPECT_FALSE(item.IsInnerNative());

    item.SetInnerNative(true);
    EXPECT_TRUE(item.IsInnerNative());

    item.SetInnerNative(false);
    EXPECT_FALSE(item.IsInnerNative());
}

/**
 * @tc.name: StyleParserTest001
 * @tc.desc: Verify style parser classifies static, path, call, expression and composite values.
 * @tc.type: FUNC
 */
TEST(StyleParserTest, StyleParserTest001)
{
    auto adapter = JsonAdapter::Parse(R"({
        "width": "matchParent",
        "backgroundColor": { "path": "/theme/bg" },
        "fontColor": { "call": "getThemeColor", "args": { "role": "text" } },
        "padding": { "top": 8, "right": "4%", "bottom": 8, "left": "4%" },
        "constraintSize": { "minWidth": 10 },
        "height": "{{ $__widthBreakpoint == 'sm' ? 40 : 48 }}"
    })");
    ASSERT_NE(adapter, nullptr);

    StyleParseResult result = StyleParser::Parse(adapter->GetRoot());
    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.properties.size(), 6u);

    auto findKind = [](const StyleParseResult& parseResult, const std::string& rawName) {
        for (const auto& property : parseResult.properties) {
            if (property.rawName == rawName) {
                return property.kind;
            }
        }
        return StyleValueKind::INVALID;
    };

    EXPECT_EQ(StyleParser::ToPropertyName("width"), StylePropertyName::WIDTH);
    EXPECT_EQ(StyleParser::ToPropertyName("flexShrink"), StylePropertyName::FLEX_SHRINK);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImage"), StylePropertyName::BACKGROUND_IMAGE);
    // backgroundImageSize is no longer registered in the name map; the resolver still applies it
    // through its raw JSON key, so ToPropertyName resolves it to UNKNOWN.
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSize"), StylePropertyName::UNKNOWN);
    EXPECT_EQ(StyleParser::ToPropertyName("backgroundImageSizeWithStyle"), StylePropertyName::BACKGROUND_IMAGE_SIZE);
    EXPECT_EQ(StyleParser::ToPropertyName("linearGradient"), StylePropertyName::LINEAR_GRADIENT);
    EXPECT_EQ(StyleParser::ToPropertyName("clip"), StylePropertyName::CLIP);
    EXPECT_EQ(StyleParser::ToPropertyName("textOverflow"), StylePropertyName::TEXT_OVERFLOW);
    EXPECT_EQ(StyleParser::ToPropertyName("decoration"), StylePropertyName::DECORATION);
    EXPECT_EQ(findKind(result, "width"), StyleValueKind::STATIC_VALUE);
    EXPECT_EQ(findKind(result, "backgroundColor"), StyleValueKind::PATH_BINDING);
    EXPECT_EQ(findKind(result, "fontColor"), StyleValueKind::FUNCTION_CALL);
    EXPECT_EQ(findKind(result, "padding"), StyleValueKind::COMPOSITE_OBJECT);
    EXPECT_EQ(findKind(result, "constraintSize"), StyleValueKind::COMPOSITE_OBJECT);
    EXPECT_EQ(findKind(result, "height"), StyleValueKind::EXPRESSION);
}

/**
 * @tc.name: StyleResolverTest001
 * @tc.desc: Verify style resolver resets removed style keys and clears prior style bindings.
 * @tc.type: FUNC
 */
TEST(StyleResolverTest, StyleResolverTest001)
{
    auto adapter = JsonAdapter::Parse(R"({
        "backgroundColor": { "path": "/theme/bg" },
        "width": 120
    })");
    ASSERT_NE(adapter, nullptr);

    StyleParseResult parseResult = StyleParser::Parse(adapter->GetRoot());
    RenderContext context;
    context.renderId = 1;
    context.surfaceId = "surface-1";

    std::set<std::string> previousKeys = { "height", "backgroundColor" };
    StyleResolveResult result = StyleResolver::Resolve(parseResult, context, "root", previousKeys);

    EXPECT_EQ(result.currentStyleKeys.size(), 2u);
    ASSERT_EQ(result.resetProperties.size(), 1u);
    EXPECT_EQ(result.resetProperties[0].rawName, "height");
    EXPECT_EQ(result.resetProperties[0].name, StylePropertyName::HEIGHT);
    EXPECT_EQ(result.bindings.size(), 1u);
    ASSERT_FALSE(result.bindings.empty());
    EXPECT_EQ(result.bindings[0].bindingProperty, "styles.backgroundColor");
    EXPECT_EQ(result.bindings[0].dataPath, "/theme/bg");
    EXPECT_EQ(result.clearBindingProperties.size(), 3u);
    EXPECT_TRUE(HasStringValue(result.clearBindingProperties, "styles.backgroundColor"));
    EXPECT_TRUE(HasStringValue(result.clearBindingProperties, "styles.width"));
    EXPECT_TRUE(HasStringValue(result.clearBindingProperties, "styles.height"));
}

#ifdef ENABLE_EXPRESSION_ENGINE
/**
 * @tc.name: StyleResolverTest002
 * @tc.desc: Verify expression style values register dependency-aware style bindings.
 * @tc.type: FUNC
 */
TEST(StyleResolverTest, StyleResolverTest002)
{
    auto adapter = JsonAdapter::Parse(R"({
        "fontColor": "{{ $__colorMode == 'dark' ? '#FFFFFFFF' : '#FF000000' }}"
    })");
    ASSERT_NE(adapter, nullptr);

    constexpr int32_t renderId = 7001;
    const std::string surfaceId = "style-expression-binding-surface";
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    ASSERT_NE(renderSlot.GetSurfaceManager(), nullptr);
    renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    renderSlot.GetSurfaceManager()->UpdateThemeMode(ThemeMode::LIGHT);

    StyleParseResult parseResult = StyleParser::Parse(adapter->GetRoot());
    RenderContext context;
    context.renderId = renderId;
    context.surfaceId = surfaceId;

    StyleResolveResult result = StyleResolver::Resolve(parseResult, context, "root", {});

    ASSERT_EQ(result.bindings.size(), 1u);
    const StyleBindingPlan& binding = result.bindings[0];
    EXPECT_EQ(binding.bindingProperty, "styles.fontColor");
    EXPECT_EQ(binding.kind, StyleBindingKind::EXPRESSION);
    EXPECT_EQ(binding.expression, "$__colorMode == 'dark' ? '#FFFFFFFF' : '#FF000000'");
    EXPECT_TRUE(binding.dataPath.empty());
    ASSERT_EQ(binding.globalVarDeps.size(), 1u);
    EXPECT_EQ(binding.globalVarDeps[0], "__colorMode");

    RenderManager::GetInstance().RemoveRenderSlot(renderId);
}

/**
 * @tc.name: StyleResolverTest005
 * @tc.desc: Verify object style descriptors recursively resolve expressions and register descriptor bindings.
 * @tc.type: FUNC
 */
TEST(StyleResolverTest, StyleResolverTest005)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {
            "top": "{{ $mode == 'large' ? 16 : 8 }}",
            "right": 1,
            "bottom": 2,
            "left": 3
        }
    })");
    ASSERT_NE(adapter, nullptr);

    auto modeValue = JsonAdapter::CreateString("large");
    ASSERT_NE(modeValue, nullptr);
    std::map<std::string, JsonValue> localVariables = { { "mode", modeValue->GetRoot() } };

    StyleParseResult parseResult = StyleParser::Parse(adapter->GetRoot());
    RenderContext context;
    context.renderId = -1;
    context.surfaceId = "style-object-expression-surface";

    StyleResolveResult result = StyleResolver::Resolve(parseResult, context, "root", {}, localVariables);

    EXPECT_TRUE(result.success);
    ASSERT_TRUE(result.resolvedStyles.IsObject());
    JsonValue padding = result.resolvedStyles.GetItem("padding");
    ASSERT_TRUE(padding.IsObject());
    EXPECT_DOUBLE_EQ(padding.GetItem("top").GetNumberValue(0.0), 16.0);
    EXPECT_DOUBLE_EQ(padding.GetItem("right").GetNumberValue(0.0), 1.0);
    ASSERT_EQ(result.bindings.size(), 1u);
    EXPECT_EQ(result.bindings[0].bindingProperty, "styles.padding");
    EXPECT_EQ(result.bindings[0].kind, StyleBindingKind::FUNCTION_CALL);
    ASSERT_EQ(result.bindings[0].globalVarDeps.size(), 1u);
    EXPECT_EQ(result.bindings[0].globalVarDeps[0], "mode");
    EXPECT_TRUE(result.dynamicallyResolvedStyleKeys.find("padding") != result.dynamicallyResolvedStyleKeys.end());
}
#endif

/**
 * @tc.name: StyleResolverTest003
 * @tc.desc: Verify object styles recursively resolve expression DSL values and
 * register style-level bindings.
 * @tc.type: FUNC
 */
TEST(StyleResolverTest, StyleResolverTest003)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {
            "top": "{{ $__colorMode == 'dark' ? 16 : 8 }}",
            "right": 4,
            "bottom": 4,
            "left": 4
        },
        "constraintSize": {
            "minWidth": "{{ '12vp' }}",
            "maxWidth": "100vp"
        },
        "linearGradient": {
            "colors": ["{{ $__colorMode == 'dark' ? '#FF000000' : '#FFFFFFFF' }}", "#FF00FF00"],
            "stops": [0, 1]
        },
        "shadow": {
            "radius": "{{ 10 }}",
            "color": "{{ $__colorMode == 'dark' ? '#FF000000' : '#FFFFFFFF' }}",
            "offsetX": 0,
            "offsetY": 0
        }
    })");
    ASSERT_NE(adapter, nullptr);

    StyleParseResult parseResult = StyleParser::Parse(adapter->GetRoot());
    RenderContext context;
    context.renderId = -1;
    context.surfaceId = "style-object-surface";

    StyleResolveResult result = StyleResolver::Resolve(parseResult, context, "style-object-root", {});

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.resolvedStyles.IsObject());
    EXPECT_EQ(result.resolvedStyles.GetItem("padding").GetNumber("top", 0.0), 8.0);
    EXPECT_EQ(result.resolvedStyles.GetItem("constraintSize").GetString("minWidth", ""), "12vp");
    EXPECT_EQ(result.resolvedStyles.GetItem("linearGradient").GetItem("colors").GetArrayItem(0).GetStringValue(""),
        "#FFFFFFFF");
    EXPECT_EQ(result.resolvedStyles.GetItem("shadow").GetNumber("radius", 0.0), 10.0);
    EXPECT_EQ(result.resolvedStyles.GetItem("shadow").GetString("color", ""), "#FFFFFFFF");

    const StyleBindingPlan* paddingBinding = FindStyleBinding(result.bindings, "styles.padding");
    const StyleBindingPlan* gradientBinding = FindStyleBinding(result.bindings, "styles.linearGradient");
    const StyleBindingPlan* shadowBinding = FindStyleBinding(result.bindings, "styles.shadow");
    ASSERT_NE(paddingBinding, nullptr);
    ASSERT_NE(gradientBinding, nullptr);
    ASSERT_NE(shadowBinding, nullptr);
    EXPECT_EQ(paddingBinding->kind, StyleBindingKind::FUNCTION_CALL);
    EXPECT_TRUE(BindingHasGlobalDependency(paddingBinding, "__colorMode"));
    EXPECT_TRUE(BindingHasGlobalDependency(gradientBinding, "__colorMode"));
    EXPECT_TRUE(BindingHasGlobalDependency(shadowBinding, "__colorMode"));
}

/**
 * @tc.name: StyleResolverTest004
 * @tc.desc: Verify object styles keep static and resolved members when one nested dynamic member fails.
 * @tc.type: FUNC
 */
TEST(StyleResolverTest, StyleResolverTest004)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": {
            "top": 8,
            "right": { "path": "/missingPaddingRight" },
            "bottom": { "path": "/paddingBottom" },
            "left": 4
        },
        "cancelButton": {
            "style": "constant",
            "fontSize": 16,
            "fontColor": { "path": "/missingCancelColor" }
        },
        "linearGradient": {
            "colors": [
                { "path": "/missingGradientColor" },
                "#FF00FF00"
            ],
            "stops": [0.25, 0.75]
        }
    })");
    ASSERT_NE(adapter, nullptr);

    std::shared_ptr<DataModel> dataModel = std::make_shared<DataModel>("style-partial-object-surface");
    std::unique_ptr<JsonAdapter> paddingBottom = JsonAdapter::CreateNumber(12.0);
    ASSERT_NE(paddingBottom, nullptr);
    dataModel->UpdateByPath("/paddingBottom", paddingBottom->GetRoot());

    StyleParseResult parseResult = StyleParser::Parse(adapter->GetRoot());
    RenderContext context;
    context.renderId = -1;
    context.surfaceId = "style-partial-object-surface";
    context.dataModel = dataModel;

    StyleResolveResult result = StyleResolver::Resolve(parseResult, context, "style-partial-object-root", {});

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.resolvedStyles.IsObject());
    JsonValue padding = result.resolvedStyles.GetItem("padding");
    ASSERT_TRUE(padding.IsObject());
    EXPECT_EQ(padding.GetNumber("top", 0.0), 8.0);
    EXPECT_FALSE(padding.GetItem("right").IsValid());
    EXPECT_EQ(padding.GetNumber("bottom", 0.0), 12.0);
    EXPECT_EQ(padding.GetNumber("left", 0.0), 4.0);

    JsonValue cancelButton = result.resolvedStyles.GetItem("cancelButton");
    ASSERT_TRUE(cancelButton.IsObject());
    EXPECT_EQ(cancelButton.GetString("style", ""), "constant");
    EXPECT_EQ(cancelButton.GetNumber("fontSize", 0.0), 16.0);
    EXPECT_FALSE(cancelButton.GetItem("fontColor").IsValid());

    JsonValue linearGradient = result.resolvedStyles.GetItem("linearGradient");
    ASSERT_TRUE(linearGradient.IsObject());
    EXPECT_FALSE(linearGradient.GetItem("colors").IsValid());
    ASSERT_TRUE(linearGradient.GetItem("stops").IsArray());
    EXPECT_EQ(linearGradient.GetItem("stops").GetArraySize(), 2);

    EXPECT_TRUE(HasStyleBindingDataPath(result.bindings, "styles.padding", "/missingPaddingRight"));
    EXPECT_TRUE(HasStyleBindingDataPath(result.bindings, "styles.padding", "/paddingBottom"));
    EXPECT_TRUE(HasStyleBindingDataPath(result.bindings, "styles.cancelButton", "/missingCancelColor"));
    EXPECT_TRUE(HasStyleBindingDataPath(result.bindings, "styles.linearGradient", "/missingGradientColor"));
    EXPECT_NE(result.dynamicallyResolvedStyleKeys.find("padding"), result.dynamicallyResolvedStyleKeys.end());
    EXPECT_NE(result.dynamicallyResolvedStyleKeys.find("cancelButton"), result.dynamicallyResolvedStyleKeys.end());
    EXPECT_NE(result.dynamicallyResolvedStyleKeys.find("linearGradient"), result.dynamicallyResolvedStyleKeys.end());
}

/**
 * @tc.name: ExtendedStyleResolverTest001
 * @tc.desc: Verify percent width and height are converted to ArkUI ratio values.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest001)
{
    auto adapter = JsonAdapter::Parse(R"({"width": "80%", "height": "50%"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasWidth());
    EXPECT_FALSE(applier.HasHeight());
    ASSERT_TRUE(applier.HasWidthPercent());
    ASSERT_TRUE(applier.HasHeightPercent());
    EXPECT_FLOAT_EQ(applier.GetWidthPercent(), 0.8F);
    EXPECT_FLOAT_EQ(applier.GetHeightPercent(), 0.5F);
}

/**
 * @tc.name: ExtendedStyleResolverTest002
 * @tc.desc: Verify matchParent, match_parent and fill route to the ArkUI layout-policy API (MATCH_PARENT).
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest002)
{
    auto adapter = JsonAdapter::Parse(R"({"width": "match_parent", "height": "fill"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));
    ASSERT_TRUE(applier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::MATCH_PARENT)));
}

/**
 * @tc.name: ExtendedStyleResolverTest003
 * @tc.desc: Verify percent edge and decoration styles are applied through percent ArkUI attributes.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest003)
{
    auto adapter = JsonAdapter::Parse(R"({
        "padding": "10% 20% 30% 40%",
        "margin": "5% 6% 7% 8%",
        "borderWidth": "25%",
        "borderRadius": {
            "topLeft": "10%",
            "topRight": "20%",
            "bottomLeft": "30%",
            "bottomRight": "40%"
        }
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasPaddingPercent());
    EXPECT_FLOAT_EQ(applier.GetPaddingPercent()[0], 0.1F);
    EXPECT_FLOAT_EQ(applier.GetPaddingPercent()[1], 0.2F);
    EXPECT_FLOAT_EQ(applier.GetPaddingPercent()[2], 0.3F);
    EXPECT_FLOAT_EQ(applier.GetPaddingPercent()[3], 0.4F);

    ASSERT_TRUE(applier.HasMarginPercent());
    EXPECT_FLOAT_EQ(applier.GetMarginPercent()[0], 0.05F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercent()[1], 0.06F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercent()[2], 0.07F);
    EXPECT_FLOAT_EQ(applier.GetMarginPercent()[3], 0.08F);

    ASSERT_TRUE(applier.HasBorderWidthPercent());
    EXPECT_FLOAT_EQ(applier.GetBorderWidthPercent(), 0.25F);

    ASSERT_TRUE(applier.HasBorderRadiusPercent());
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercent()[0], 10.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercent()[1], 20.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercent()[2], 30.0F);
    EXPECT_FLOAT_EQ(applier.GetBorderRadiusPercent()[3], 40.0F);
}

/**
 * @tc.name: ExtendedStyleResolverTest004
 * @tc.desc: Verify fixAtIdealSize routes to the ArkUI layout-policy API (FIX_AT_IDEAL_SIZE) for width and height.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTest, ExtendedStyleResolverTest004)
{
    auto adapter = JsonAdapter::Parse(R"({"width": "fixAtIdealSize", "height": "fix_at_ideal_size"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasWidth());
    EXPECT_FALSE(applier.HasHeight());
    EXPECT_FALSE(applier.HasWidthPercent());
    EXPECT_FALSE(applier.HasHeightPercent());
    EXPECT_TRUE(applier.HasLayoutPolicy(
        NODE_WIDTH_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::FIX_AT_IDEAL_SIZE)));
    EXPECT_TRUE(applier.HasLayoutPolicy(
        NODE_HEIGHT_LAYOUTPOLICY, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::FIX_AT_IDEAL_SIZE)));
}

/**
 * @tc.name: StyleApplyUtilsTest001
 * @tc.desc: Verify backgroundImageSize supports SizeOptions and ImageSize values.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest001)
{
    auto sizeAdapter = JsonAdapter::Parse(R"({"width": 120})");
    ASSERT_NE(sizeAdapter, nullptr);

    StyleBackgroundImageSize size;
    ASSERT_TRUE(StyleApplyUtils::ParseBackgroundImageSize(sizeAdapter->GetRoot(), size));
    EXPECT_EQ(size.kind, StyleBackgroundImageSizeKind::SIZE);
    EXPECT_FLOAT_EQ(size.width, 120.0F);
    EXPECT_FLOAT_EQ(size.height, 0.0F);

    auto imageSizeAdapter = JsonAdapter::Parse(R"("contain")");
    ASSERT_NE(imageSizeAdapter, nullptr);

    StyleBackgroundImageSize imageSize;
    ASSERT_TRUE(StyleApplyUtils::ParseBackgroundImageSize(imageSizeAdapter->GetRoot(), imageSize));
    EXPECT_EQ(imageSize.kind, StyleBackgroundImageSizeKind::IMAGE_SIZE);
    EXPECT_EQ(imageSize.imageSize, ARKUI_IMAGE_SIZE_CONTAIN);

    auto fillImageSizeAdapter = JsonAdapter::Parse(R"("fill")");
    ASSERT_NE(fillImageSizeAdapter, nullptr);

    StyleBackgroundImageSize fillImageSize;
    ASSERT_TRUE(StyleApplyUtils::ParseBackgroundImageSize(fillImageSizeAdapter->GetRoot(), fillImageSize));
    EXPECT_EQ(fillImageSize.kind, StyleBackgroundImageSizeKind::IMAGE_SIZE);
    EXPECT_EQ(fillImageSize.imageSize, 3);
}

/**
 * @tc.name: StyleApplyUtilsTest002
 * @tc.desc: Verify linearGradient parses angle, colors and stop normalization.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest002)
{
    auto adapter = JsonAdapter::Parse(R"({
        "angle": "0.5turn",
        "direction": "left",
        "repeating": true,
        "colors": [["#FF0000", 0.7], ["#00FF00", 0.3], {"color": "#0000FF", "stop": 2}]
    })");
    ASSERT_NE(adapter, nullptr);

    StyleLinearGradient gradient;
    ASSERT_TRUE(StyleApplyUtils::ParseLinearGradient(adapter->GetRoot(), gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 180.0F);
    EXPECT_EQ(gradient.direction, ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM);
    EXPECT_TRUE(gradient.repeating);
    ASSERT_EQ(gradient.colors.size(), 3u);
    ASSERT_EQ(gradient.stops.size(), 3u);
    EXPECT_EQ(gradient.colors[0], 0xFFFF0000u);
    EXPECT_EQ(gradient.colors[1], 0xFF00FF00u);
    EXPECT_EQ(gradient.colors[2], 0xFF0000FFu);
    EXPECT_FLOAT_EQ(gradient.stops[0], 0.7F);
    EXPECT_FLOAT_EQ(gradient.stops[1], 0.7F);
    EXPECT_FLOAT_EQ(gradient.stops[2], 1.0F);
}

/**
 * @tc.name: StyleApplyUtilsTest003
 * @tc.desc: Verify fontWeight supports numeric values, keywords and invalid fallback.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest003)
{
    auto numberAdapter = JsonAdapter::Parse("700");
    ASSERT_NE(numberAdapter, nullptr);

    int32_t fontWeight = ARKUI_FONT_WEIGHT_NORMAL;
    ASSERT_TRUE(StyleApplyUtils::ParseFontWeight(numberAdapter->GetRoot(), fontWeight));
    EXPECT_EQ(fontWeight, ARKUI_FONT_WEIGHT_W700);

    auto keywordAdapter = JsonAdapter::Parse(R"(" bold ")");
    ASSERT_NE(keywordAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseFontWeight(keywordAdapter->GetRoot(), fontWeight));
    EXPECT_EQ(fontWeight, ARKUI_FONT_WEIGHT_BOLD);

    auto invalidAdapter = JsonAdapter::Parse(R"("unexpected")");
    ASSERT_NE(invalidAdapter, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseFontWeight(invalidAdapter->GetRoot(), fontWeight));
    EXPECT_EQ(fontWeight, ARKUI_FONT_WEIGHT_W400);
}

/**
 * @tc.name: StyleApplyUtilsTest004
 * @tc.desc: Verify ParseDividerStrokeWidth supports vp/fp/px/percent, treats numbers as vp, and rejects invalid input.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest004)
{
    auto adapter = JsonAdapter::Parse(R"({
        "vp": "12vp",
        "percent": "25%",
        "number": 8,
        "px": "10px",
        "fp": "100fp",
        "invalidNegative": "-2vp"
    })");
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();

    float strokeWidth = 0.0F;
    std::string unit;

    ASSERT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("vp"), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 12.0F);
    EXPECT_EQ(unit, "vp");

    ASSERT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("percent"), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 25.0F);
    EXPECT_EQ(unit, "%");

    ASSERT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("number"), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 8.0F);
    EXPECT_EQ(unit, "vp");

    ASSERT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("px"), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 10.0F);
    EXPECT_EQ(unit, "px");

    ASSERT_TRUE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("fp"), strokeWidth, unit));
    EXPECT_FLOAT_EQ(strokeWidth, 100.0F);
    EXPECT_EQ(unit, "fp");

    EXPECT_FALSE(StyleApplyUtils::ParseDividerStrokeWidth(root.GetItem("invalidNegative"), strokeWidth, unit));
}

/**
 * @tc.name: StyleApplyUtilsTest005
 * @tc.desc: Verify ParseHexColorString supports #RRGGBB/#AARRGGBB and rejects invalid color strings.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest005)
{
    uint32_t color = 0;

    ASSERT_TRUE(StyleApplyUtils::ParseHexColorString("#112233", color));
    EXPECT_EQ(color, 0xFF112233u);

    ASSERT_TRUE(StyleApplyUtils::ParseHexColorString("  #AA112233  ", color));
    EXPECT_EQ(color, 0xAA112233u);

    EXPECT_FALSE(StyleApplyUtils::ParseHexColorString("112233", color));
    EXPECT_FALSE(StyleApplyUtils::ParseHexColorString("#GG112233", color));
    EXPECT_FALSE(StyleApplyUtils::ParseHexColorString("#12345", color));
}

/**
 * @tc.name: StyleApplyUtilsTest006
 * @tc.desc: Verify ParseDimension rejects negative values and supports fixAtIdealSize keywords.
 * @tc.type: FUNC
 */
TEST(StyleApplyUtilsTest, StyleApplyUtilsTest006)
{
    StyleDimension dimension;

    auto negativeNumber = JsonAdapter::Parse("-1");
    ASSERT_NE(negativeNumber, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseDimension(negativeNumber->GetRoot(), dimension));

    auto negativeToken = JsonAdapter::Parse(R"("-1vp")");
    ASSERT_NE(negativeToken, nullptr);
    EXPECT_FALSE(StyleApplyUtils::ParseDimension(negativeToken->GetRoot(), dimension));

    auto fixAtIdealSize = JsonAdapter::Parse(R"("fixAtIdealSize")");
    ASSERT_NE(fixAtIdealSize, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(fixAtIdealSize->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::FIX_AT_IDEAL_SIZE);
    EXPECT_FLOAT_EQ(dimension.value, 0.0F);

    auto fixAtIdealSizeSnake = JsonAdapter::Parse(R"("fix_at_ideal_size")");
    ASSERT_NE(fixAtIdealSizeSnake, nullptr);
    ASSERT_TRUE(StyleApplyUtils::ParseDimension(fixAtIdealSizeSnake->GetRoot(), dimension));
    EXPECT_EQ(dimension.unit, StyleDimensionUnit::FIX_AT_IDEAL_SIZE);
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest001
 * @tc.desc: Verify common node styles flexShrink, backgroundImage and clip parse and apply.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest001)
{
    auto adapter = JsonAdapter::Parse(R"({
        "flexShrink": 0.5,
        "backgroundImage": " resources/base/media/background.png ",
        "clip": true
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasFlexShrink());
    EXPECT_FLOAT_EQ(applier.GetFlexShrink(), 0.5F);
    ASSERT_TRUE(applier.HasBackgroundImage());
    EXPECT_EQ(applier.GetBackgroundImage(), "resources/base/media/background.png");
    ASSERT_TRUE(applier.HasClip());
    EXPECT_TRUE(applier.GetClip());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest002
 * @tc.desc: Verify backgroundImage empty string resets native attribute.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest002)
{
    auto adapter = JsonAdapter::Parse(R"({"backgroundImage": ""})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    EXPECT_FALSE(applier.HasBackgroundImage());
    EXPECT_TRUE(applier.WasBackgroundImageReset());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest003
 * @tc.desc: Verify shadow style values are applied through NODE_SHADOW and reset custom shadow.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest003)
{
    auto adapter = JsonAdapter::Parse(R"({"shadow": "outerFloatingMD"})");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasShadow());
    ASSERT_EQ(applier.GetShadowValues().size(), 1u);
    EXPECT_EQ(applier.GetShadowValues()[0].i32, ARKUI_SHADOW_STYLE_OUTER_FLOATING_MD);
    EXPECT_TRUE(applier.WasCustomShadowReset());
    EXPECT_FALSE(applier.HasCustomShadow());
}

/**
 * @tc.name: ExtendedStyleResolverCommonStyleTest004
 * @tc.desc: Verify custom shadow options are applied through NODE_CUSTOM_SHADOW and reset shadow style.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverCommonStyleTest, ExtendedStyleResolverCommonStyleTest004)
{
    auto adapter = JsonAdapter::Parse(R"({
        "shadow": {
            "radius": -12,
            "offsetX": 3,
            "offsetY": 4,
            "type": "blur",
            "color": "#FF112233",
            "fill": true
        }
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasCustomShadow());
    const std::vector<ArkUI_NumberValue>& values = applier.GetCustomShadowValues();
    ASSERT_EQ(values.size(), 7u);
    EXPECT_FLOAT_EQ(values[0].f32, 0.0F);
    EXPECT_EQ(values[1].i32, 0);
    EXPECT_FLOAT_EQ(values[2].f32, 3.0F);
    EXPECT_FLOAT_EQ(values[3].f32, 4.0F);
    EXPECT_EQ(values[4].i32, ARKUI_SHADOW_TYPE_BLUR);
    EXPECT_EQ(values[5].u32, 0xFF112233u);
    EXPECT_EQ(values[6].u32, 1u);
    EXPECT_TRUE(applier.WasShadowReset());
    EXPECT_FALSE(applier.HasShadow());
}

/**
 * @tc.name: ExtendedStyleResolverTextStyleTest001
 * @tc.desc: Verify Text component styles are parsed by StyleApplyUtils and applied through ArkUINodeApiAdapter.
 * @tc.type: FUNC
 */
TEST(ExtendedStyleResolverTextStyleTest, ExtendedStyleResolverTextStyleTest001)
{
    auto adapter = JsonAdapter::Parse(R"({
        "fontColor": "#FF112233",
        "fontSize": 18,
        "fontWeight": 700,
        "maxLines": 2,
        "textOverflow": "ellipsis",
        "decoration": {
            "type": "underline",
            "color": "#ff007dff",
            "style": "solid",
            "thicknessScale": 1.5
        },
        "textAlign": "center"
    })");
    ASSERT_NE(adapter, nullptr);

    RecordingCommonStyleApplier applier;
    ExtendedStyleResolver::ResolveAndApply(adapter->GetRoot(), applier);
    ExtendedStyleResolver::ApplyTextComponentStyles(adapter->GetRoot(), applier);

    ASSERT_TRUE(applier.HasFontColor());
    EXPECT_EQ(applier.GetFontColor(), 0xFF112233u);
    ASSERT_TRUE(applier.HasFontSize());
    EXPECT_FLOAT_EQ(applier.GetFontSize(), 18.0F);
    ASSERT_TRUE(applier.HasFontWeight());
    EXPECT_EQ(applier.GetFontWeight(), ARKUI_FONT_WEIGHT_W700);
    ASSERT_TRUE(applier.HasMaxLines());
    EXPECT_EQ(applier.GetMaxLines(), 2);
    ASSERT_TRUE(applier.HasTextOverflow());
    EXPECT_EQ(applier.GetTextOverflow(), 2);
    ASSERT_TRUE(applier.HasTextAlign());
    EXPECT_EQ(applier.GetTextAlign(), 1);

    ASSERT_TRUE(applier.HasDecoration());
    const std::vector<ArkUI_NumberValue>& values = applier.GetDecorationValues();
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(values[0].i32, 1);
    EXPECT_EQ(values[1].u32, 0xFF007DFFu);
    EXPECT_EQ(values[2].i32, 0);
    EXPECT_FLOAT_EQ(values[3].f32, 1.5F);
}

/**
 * @tc.name: CatalogTest001
 * @tc.desc: Verify the following Catalog behavior: add and find components.
 * @tc.type: FUNC
 */
TEST(CatalogTest, CatalogTest001)
{
    /**
     * @tc.steps: step1. Add or query catalog items in the target Catalog instance.
     * @tc.expected: The catalog lookup result matches the expectation.
     */

    Catalog catalog("test-catalog");
    auto item = std::make_shared<CatalogItem>("Button", CatalogItemType::COMPONENT);
    catalog.AddComponent(item);
    auto found = catalog.GetCatalogItemByName("Button");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "Button");
}

/**
 * @tc.name: CatalogTest002
 * @tc.desc: Verify the following Catalog behavior: return nullptr when component not found.
 * @tc.type: FUNC
 */
TEST(CatalogTest, CatalogTest002)
{
    /**
     * @tc.steps: step1. Add or query catalog items in the target Catalog instance.
     * @tc.expected: The catalog lookup result matches the expectation.
     */

    Catalog catalog("test-catalog");
    auto found = catalog.GetCatalogItemByName("NonExistent");
    EXPECT_EQ(found, nullptr);
}

/**
 * @tc.name: CatalogTest003
 * @tc.desc: Verify the following Catalog behavior: add and find functions.
 * @tc.type: FUNC
 */
TEST(CatalogTest, CatalogTest003)
{
    /**
     * @tc.steps: step1. Add or query catalog items in the target Catalog instance.
     * @tc.expected: The catalog lookup result matches the expectation.
     */

    Catalog catalog("test-catalog");
    auto func = std::make_shared<CatalogItem>("formatNumber", CatalogItemType::LOCAL_FUNCTION);
    catalog.AddFunction(func);
    EXPECT_TRUE(catalog.HasFunction("formatNumber"));
    auto found = catalog.GetFunctionItemByName("formatNumber");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "formatNumber");
}

/**
 * @tc.name: FunctionResultTest001
 * @tc.desc: Verify the following FunctionResult behavior: create string result.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest001)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult result(std::string("hello"));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ(result.GetStringValue(), "hello");
    EXPECT_EQ(result.ToJsonLiteral(), "\"hello\"");
}

/**
 * @tc.name: FunctionResultTest002
 * @tc.desc: Verify the following FunctionResult behavior: create int result.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest002)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult result(42);
    EXPECT_TRUE(result.IsInt());
    EXPECT_EQ(result.GetIntValue(), 42);
}

/**
 * @tc.name: FunctionResultTest003
 * @tc.desc: Verify the following FunctionResult behavior: create bool result.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest003)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult result(true);
    EXPECT_TRUE(result.IsBool());
    EXPECT_TRUE(result.GetBoolValue());
}

/**
 * @tc.name: FunctionResultTest004
 * @tc.desc: Verify the following FunctionResult behavior: create double result.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest004)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult result(3.14);
    EXPECT_TRUE(result.IsDouble());
    EXPECT_DOUBLE_EQ(result.GetDoubleValue(), 3.14);
}

/**
 * @tc.name: FunctionResultTest005
 * @tc.desc: Verify the following FunctionResult behavior: create null result.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest005)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult result;
    EXPECT_TRUE(result.IsNull());
}

/**
 * @tc.name: FunctionResultTest006
 * @tc.desc: Verify the following FunctionResult behavior: compare equality.
 * @tc.type: FUNC
 */
TEST(FunctionResultTest, FunctionResultTest006)
{
    /**
     * @tc.steps: step1. Construct a FunctionResult with the target value.
     * @tc.expected: The stored type information and serialized value match the expectation.
     */

    FunctionResult a(42);
    FunctionResult b(42);
    FunctionResult c(99);
    EXPECT_TRUE(a.Equals(b));
    EXPECT_FALSE(a.Equals(c));
}

/**
 * @tc.name: FunctionCallInfoTest001
 * @tc.desc: Verify the following FunctionCallInfo behavior: store function data.
 * @tc.type: FUNC
 */
TEST(FunctionCallInfoTest, FunctionCallInfoTest001)
{
    /**
     * @tc.steps: step1. Create the FunctionCallInfo instance with the target metadata.
     * @tc.expected: The stored function call metadata matches the expectation.
     */

    auto argsAdapter = JsonAdapter::Parse(R"(["USD", 1234.56])");
    ASSERT_NE(argsAdapter, nullptr);
    FunctionCallInfo info("formatNumber", argsAdapter->GetRoot(), "string");
    EXPECT_EQ(info.GetFunctionName(), "formatNumber");
    EXPECT_EQ(info.GetArgs().ToJsonLiteral(), R"(["USD",1234.56])");
    EXPECT_EQ(info.GetReturnType(), "string");
}

/**
 * @tc.name: ActionInfoTest001
 * @tc.desc: Verify the following ActionInfo behavior: create function call action.
 * @tc.type: FUNC
 */
TEST(ActionInfoTest, ActionInfoTest001)
{
    /**
     * @tc.steps: step1. Create the ActionInfo instance with the target input payload.
     * @tc.expected: The action type and stored payload match the expectation.
     */

    auto argsAdapter = JsonAdapter::Parse("[]");
    ASSERT_NE(argsAdapter, nullptr);
    auto callInfo = std::make_shared<FunctionCallInfo>("formatNumber", argsAdapter->GetRoot(), "string");

    auto descriptorAdapter = JsonAdapter::Parse(R"({"call":"formatNumber","args":[]})");
    ASSERT_NE(descriptorAdapter, nullptr);
    ActionInfo action(callInfo, descriptorAdapter->GetRoot());
    EXPECT_EQ(action.GetType(), ActionType::FUNCTION_CALL);
    EXPECT_TRUE(action.IsValid());
    EXPECT_NE(action.GetFunctionCall(), nullptr);
    EXPECT_EQ(action.GetFunctionCall()->GetFunctionName(), "formatNumber");
}

/**
 * @tc.name: ActionInfoTest002
 * @tc.desc: Verify the following ActionInfo behavior: create event action.
 * @tc.type: FUNC
 */
TEST(ActionInfoTest, ActionInfoTest002)
{
    /**
     * @tc.steps: step1. Create the ActionInfo instance with the target input payload.
     * @tc.expected: The action type and stored payload match the expectation.
     */

    auto contextAdapter = JsonAdapter::Parse(R"({"componentId":"btn1"})");
    ASSERT_NE(contextAdapter, nullptr);
    ActionInfo action("onClick", contextAdapter->GetRoot());
    EXPECT_EQ(action.GetType(), ActionType::EVENT);
    EXPECT_TRUE(action.IsValid());
    EXPECT_EQ(action.GetEventName(), "onClick");
}

/**
 * @tc.name: ActionInfoTest003
 * @tc.desc: Verify the following ActionInfo behavior: be invalid when default constructed.
 * @tc.type: FUNC
 */
TEST(ActionInfoTest, ActionInfoTest003)
{
    /**
     * @tc.steps: step1. Create the ActionInfo instance with the target input payload.
     * @tc.expected: The action type and stored payload match the expectation.
     */

    ActionInfo action;
    EXPECT_EQ(action.GetType(), ActionType::UNKNOWN);
    EXPECT_FALSE(action.IsValid());
}
