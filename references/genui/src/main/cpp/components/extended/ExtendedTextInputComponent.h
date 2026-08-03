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

#ifndef A2UI_EXTENDED_TEXT_INPUT_COMPONENT_H
#define A2UI_EXTENDED_TEXT_INPUT_COMPONENT_H

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedTextInputComponent : public ExtendedComponent {
public:
    ExtendedTextInputComponent();
    ~ExtendedTextInputComponent() override;

    std::string GetType() const override;

#ifdef TDD_BUILD
    const std::string& GetTextForTest() const
    {
        return textValue_;
    }

    const std::string& GetPlaceholderForTest() const
    {
        return placeholderValue_;
    }

    bool GetEnabledForTest() const
    {
        return enabled_;
    }

    int32_t GetMaxLengthForTest() const
    {
        return maxLength_;
    }

    int32_t GetInputTypeForTest() const
    {
        return static_cast<int32_t>(inputType_);
    }

    int32_t GetFontWeightForTest() const
    {
        return static_cast<int32_t>(fontWeight_);
    }

    int32_t GetTextAlignForTest() const
    {
        return textAlign_;
    }

    float GetMaxFontSizeForTest() const
    {
        return maxFontSize_;
    }

    float GetMinFontSizeForTest() const
    {
        return minFontSize_;
    }

    uint32_t GetCaretColorForTest() const
    {
        return caretColor_;
    }

    uint32_t GetSelectedBackgroundColorForTest() const
    {
        return selectedBackgroundColor_;
    }

    bool HasCancelButtonForTest() const
    {
        return hasCancelButton_;
    }

    int32_t GetCancelButtonStyleForTest() const
    {
        return static_cast<int32_t>(cancelButtonStyle_);
    }

    bool HasCancelButtonIconSizeForTest() const
    {
        return hasCancelButtonIconSize_;
    }

    float GetCancelButtonIconSizeForTest() const
    {
        return cancelButtonIconSize_;
    }

    bool HasCancelButtonIconColorForTest() const
    {
        return hasCancelButtonIconColor_;
    }

    uint32_t GetCancelButtonIconColorForTest() const
    {
        return cancelButtonIconColor_;
    }

    bool HasCancelButtonIconSrcForTest() const
    {
        return hasCancelButtonIconSrc_;
    }

    const std::string& GetCancelButtonIconSrcForTest() const
    {
        return cancelButtonIconSrc_;
    }

    bool GetShowUnderlineForTest() const
    {
        return showUnderline_;
    }

    int32_t GetWordBreakForTest() const
    {
        return static_cast<int32_t>(wordBreak_);
    }

    bool HasUnderlineColorForTest() const
    {
        return hasUnderlineColor_;
    }

    uint32_t GetUnderlineColorTypingForTest() const
    {
        return underlineColorTyping_;
    }

    uint32_t GetUnderlineColorNormalForTest() const
    {
        return underlineColorNormal_;
    }

    uint32_t GetUnderlineColorErrorForTest() const
    {
        return underlineColorError_;
    }

    uint32_t GetUnderlineColorDisableForTest() const
    {
        return underlineColorDisable_;
    }

    const std::string& GetFontScaleModeForTest() const
    {
        return fontScaleMode_;
    }

    float ComputeEffectiveFontSizeForTest(float baseFontSize) const
    {
        return ComputeEffectiveFontSize(baseFontSize);
    }

    float GetMinFontScaleForTest() const
    {
        return minFontScale_;
    }

    float GetMaxFontScaleForTest() const
    {
        return maxFontScale_;
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void OnConfigChange(const ThemeContext& context) override;
    void RegisterComponentSpecificListeners() override;
    void OnPropertyRemoved(const std::string& propertyName) override;

private:
    static void NodeEventReceiver(A2UINodeEvent* event);

    void ValidateStylesSchema(const JsonValue& styles);
    std::unique_ptr<JsonAdapter> ResolveStyleObjectDynamicMembers(const JsonValue& styleObjectValue) const;
    std::unique_ptr<JsonAdapter> ResolveCancelButtonDynamicMembers(const JsonValue& cancelButtonValue) const;
    std::unique_ptr<JsonAdapter> ResolveUnderlineColorDynamicMembers(const JsonValue& underlineColorValue) const;
    void ValidateResolvedCancelButtonDfx(const JsonValue& cancelButtonValue);
    void ValidateResolvedUnderlineColorDfx(const JsonValue& underlineColorValue);
    ThemeMode ResolveThemeMode() const;
    void HandleNodeEvent(A2UINodeEvent* event);
    void HandleInputValueChange(const std::string& nextValue);
    void UpdateChangeEventRegistration();
    void ResetTextPropertyIfMissing();
    void ResetNodeFontSize();
    void ResetNodeTextInputNumberOfLines();
    void ResetNodeFontWeight();
    void ResetNodeTextAlign();
    void ResetNodeTextInputShowUnderline();
    void ResetNodeTextInputWordBreak();
    void ResetNodeTextInputMaxLength();
    void ResetNodeTextMinFontSize();
    void ResetNodeTextMaxFontSize();
    void ResetNodeTextInputCancelButton();
    void ResetNodeTextInputUnderlineColor();
    void SetText(const std::string& text);
    void SetPlaceholder(const std::string& placeholder);
    void SetEnabled(bool enabled);
    void SetMaxLength(int32_t maxLength);
    void SetInputType(A2UITextInputType inputType);
    void SetFontColor(uint32_t color, bool userOverride = true);
    void SetPlaceholderColor(uint32_t color, bool userOverride = true);
    void SetFontWeight(A2UIFontWeight fontWeight);
    void SetTextAlign(int32_t textAlign);
    void SetMinFontSize(float minFontSize);
    void SetMaxFontSize(float maxFontSize);
    void SetCaretColor(uint32_t color, bool userOverride = true);
    void SetSelectedBackgroundColor(uint32_t color, bool userOverride = true);
    void ResetCancelButton();
    void SetCancelButton(A2UICancelButtonStyle style, bool hasIconSize, float iconSize, bool hasIconColor,
        uint32_t iconColor, bool hasIconSrc, const std::string& iconSrc);
    void SetShowUnderline(bool showUnderline);
    void ResetUnderlineColor();
    void SetUnderlineColor(uint32_t typingColor, uint32_t normalColor, uint32_t errorColor, uint32_t disableColor,
        bool userOverride = true);
    void SetWordBreak(A2UIWordBreak wordBreak);
    void SetFontScaleMode(const std::string& mode);
    float ComputeEffectiveFontSize(float baseFontSize) const;
    void OnFontSizeScaleChanged(float newScale) override;
    void SetMinFontScale(float minFontScale);
    void SetMaxFontScale(float maxFontScale);
    std::string ResolveTextBindingPath() const;
    void SyncTextToBoundDataModel(const std::string& value);

    std::string textValue_;
    std::string fontScaleMode_ = "followSystem";
    float fontSize_ = 0.0F;
    std::string placeholderValue_;
    bool enabled_ = true;
    int32_t maxLength_ = std::numeric_limits<int32_t>::max();
    A2UITextInputType inputType_ = A2UITextInputType::NORMAL;
    A2UIFontWeight fontWeight_ = A2UIFontWeight::NORMAL;
    int32_t textAlign_ = 0;
    float minFontSize_ = 0.0F;
    float maxFontSize_ = 0.0F;
    float minFontScale_ = 0.0F;
    float maxFontScale_ = 0.0F;
    uint32_t fontColor_ = 0xFF182431u;
    uint32_t placeholderColor_ = 0x99182431u;
    uint32_t caretColor_ = 0xFF007DFFu;
    uint32_t selectedBackgroundColor_ = 0x33007DFFu;
    bool hasCancelButton_ = false;
    A2UICancelButtonStyle cancelButtonStyle_ = A2UICancelButtonStyle::INPUT;
    bool hasCancelButtonIconSize_ = false;
    float cancelButtonIconSize_ = 0.0F;
    bool hasCancelButtonIconColor_ = false;
    uint32_t cancelButtonIconColor_ = 0;
    bool hasCancelButtonIconSrc_ = false;
    std::string cancelButtonIconSrc_;
    bool showUnderline_ = false;
    bool hasUnderlineColor_ = false;
    bool hasFontColorOverride_ = false;
    bool hasPlaceholderColorOverride_ = false;
    bool hasCaretColorOverride_ = false;
    bool hasSelectedBackgroundColorOverride_ = false;
    uint32_t underlineColorTyping_ = 0x33182431u;
    uint32_t underlineColorNormal_ = 0x33182431u;
    uint32_t underlineColorError_ = 0x33182431u;
    uint32_t underlineColorDisable_ = 0x33182431u;
    A2UIWordBreak wordBreak_ = A2UIWordBreak::NORMAL;
    bool changeEventRegistered_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_TEXT_INPUT_COMPONENT_H
