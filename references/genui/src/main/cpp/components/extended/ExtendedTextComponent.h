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

#ifndef A2UI_EXTENDED_TEXT_COMPONENT_H
#define A2UI_EXTENDED_TEXT_COMPONENT_H

#include <cstdint>
#include <string>

#include "components/extended/ExtendedComponent.h"
#include "styles/StyleTypes.h"

namespace NativeModule {

using TextDecorationState = StyleTextDecoration;

class ExtendedTextComponent : public ExtendedComponent {
public:
    ExtendedTextComponent();
    ~ExtendedTextComponent() override = default;

    std::string GetType() const override;
    void OnConfigChange(const ThemeContext& context) override;

#ifdef TDD_BUILD
    const std::string& GetTextValueForTest() const
    {
        return textValue_;
    }

    uint32_t GetFontColorForTest() const
    {
        return fontColor_;
    }

    float GetFontSizeForTest() const
    {
        return fontSize_;
    }

    int32_t GetFontWeightForTest() const
    {
        return static_cast<int32_t>(fontWeight_);
    }

    int32_t GetMaxLinesForTest() const
    {
        return maxLines_;
    }

    float GetMinFontSizeForTest() const
    {
        return minFontSize_;
    }

    float GetMaxFontSizeForTest() const
    {
        return maxFontSize_;
    }

    int32_t GetTextOverflowForTest() const
    {
        return textOverflow_;
    }

    int32_t GetTextAlignForTest() const
    {
        return textAlign_;
    }

    TextDecorationState GetDecorationForTest() const
    {
        return decoration_;
    }

    int32_t GetWordBreakForTest() const
    {
        return static_cast<int32_t>(wordBreak_);
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

    void SetMaxLinesForTest(int32_t maxLines)
    {
        SetMaxLines(maxLines);
    }

    ArkUI_NodeHandle GetNativeViewHandleRawForTest() const
    {
        return nativeView_;
    }

    void SetNativeViewHandleRawForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }

    void* GetNativeNodeApiRawForTest() const
    {
        return ArkUINodeApiAdapter::GetNativeNodeAPI();
    }

    void SetNativeNodeApiRawForTest(void* nativeNodeApi)
    {
        static_cast<void>(nativeNodeApi);
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;

private:
    void ReportStyleWarning(const std::string& code, const std::string& styleName, const std::string& message) const;
    void ApplyDefaultTextStyles();
    uint32_t ResolveDefaultFontColor() const;
    uint32_t ResolveDefaultDecorationColor() const;
    bool ResolveDecorationWithFallback(
        const JsonValue& styles, TextDecorationState& decoration, bool* usedDefaultColor = nullptr) const;
    void ApplyDecorationStyleWithFallback(const JsonValue& styles);
    void ValidateDecorationFields(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyFontWeightAndColorStyle(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyFontScaleRangeStyle(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyFontScaleModeAndSizeStyle(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyMaxLinesAndOverflowStyle(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyTextAlignAndBreakStyle(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyMinFontSizeStyle(const JsonValue& styles, bool isDeltaUpdate, float& minFontSize);
    void ApplyMaxFontSizeStyle(const JsonValue& styles, bool isDeltaUpdate, float minFontSize);
    void ApplyDecorationState(const TextDecorationState& decoration);
    void SetFontColor(uint32_t fontColor);
    void SetFontSize(float fontSize);
    void SetFontWeight(A2UIFontWeight fontWeight);
    void SetMaxLines(int32_t maxLines);
    void SetTextAlign(int32_t textAlign);
    void SetTextOverflow(int32_t textOverflow);
    void SetWordBreak(A2UIWordBreak wordBreak);
    void SetText(const std::string& text);
    void SetFontScaleMode(const std::string& mode);
    float ComputeEffectiveFontSize(float baseFontSize) const;
    void OnFontSizeScaleChanged(float newScale) override;
    void SetMinFontScale(float minFontScale);
    void SetMaxFontScale(float maxFontScale);
#ifdef TDD_BUILD
    void ApplyTextStyleStateForTest(const JsonValue& styles);
    void ApplyFontColorFontSizeWeightForTest(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyFontScaleRangeAndModeForTest(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyMaxLinesAndAdaptiveFontForTest(const JsonValue& styles, bool isDeltaUpdate);
    void ApplyOverflowAlignBreakForTest(const JsonValue& styles, bool isDeltaUpdate);
#endif

    std::string textValue_;
    std::string fontScaleMode_ = "followSystem";
    float fontSize_ = 16.0F;
    float minFontScale_ = 0.0F;
    float maxFontScale_ = 0.0F;
    bool useDefaultFontColor_ = true;
    bool hasAppliedDecoration_ = false;
    bool useDefaultDecorationColor_ = false;
    TextDecorationState appliedDecoration_;
    int32_t maxLines_ = 2147483647;
#ifdef TDD_BUILD
    uint32_t fontColor_ = 0xE5000000u;
    A2UIFontWeight fontWeight_ = A2UIFontWeight::W400;
    float minFontSize_ = -1.0F;
    float maxFontSize_ = -1.0F;
    int32_t textOverflow_ = 1;
    int32_t textAlign_ = 0;
    A2UIWordBreak wordBreak_ = A2UIWordBreak::BREAK_WORD;
    TextDecorationState decoration_;
#endif
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_TEXT_COMPONENT_H
