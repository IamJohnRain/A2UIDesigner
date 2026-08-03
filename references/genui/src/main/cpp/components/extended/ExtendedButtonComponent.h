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

#ifndef A2UI_EXTENDED_BUTTON_COMPONENT_H
#define A2UI_EXTENDED_BUTTON_COMPONENT_H

#include <cstdint>
#include <memory>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedButtonComponent : public ExtendedComponent {
public:
    ExtendedButtonComponent();
    ~ExtendedButtonComponent() override = default;

    std::string GetType() const override;
    void OnConfigChange(const ThemeContext& context) override;

#ifdef TDD_BUILD
    const std::string& GetLabelForTest() const
    {
        return labelValue_;
    }

    float GetFontSizeForTest() const
    {
        return fontSize_;
    }

    int32_t GetFontWeightForTest() const
    {
        return static_cast<int32_t>(fontWeight_);
    }

    bool GetEnabledForTest() const
    {
        return enabled_;
    }

    float GetMaxFontSizeForTest() const
    {
        return maxFontSize_;
    }

    float GetMinFontSizeForTest() const
    {
        return minFontSize_;
    }

    float GetMinFontScaleForTest() const
    {
        return minFontScale_;
    }

    float GetMaxFontScaleForTest() const
    {
        return maxFontScale_;
    }

    bool HasFontColorForTest() const
    {
        return hasFontColor_;
    }

    uint32_t GetFontColorForTest() const
    {
        return fontColor_;
    }

    const std::string& GetFontScaleModeForTest() const
    {
        return fontScaleMode_;
    }

    float ComputeEffectiveFontSizeForTest(float baseFontSize) const
    {
        return ComputeEffectiveFontSize(baseFontSize);
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void RegisterClickHandler() override;
    void OnPropertyRemoved(const std::string& propertyName) override;

private:
    void ValidateStylesSchema(const JsonValue& styles);
    void SetLabel(const std::string& label);
    void SetFontSize(float fontSize);
    void SetFontWeight(A2UIFontWeight fontWeight);
    void SetEnabled(bool enabled);
    void SetFontColor(uint32_t color, bool userOverride = true);
    void ApplyDefaultFontColor();
    void ResetFontColor();
    void SetBackgroundColor(uint32_t color, bool userOverride = true);
    void ApplyDefaultBackgroundColor();
    void SetMinFontSize(float minFontSize);
    void SetMaxFontSize(float maxFontSize);
    void SetMinFontScale(float minFontScale);
    void SetMaxFontScale(float maxFontScale);
    void ResetNodeFontColor();
    void ResetNodeTextMinFontSize();
    void ResetNodeTextMaxFontSize();
    void ResetNodeButtonMinFontScale();
    void ResetNodeButtonMaxFontScale();
    void SetFontScaleMode(const std::string& mode);
    void SetAction(const JsonValue& actionValue);
    void ClearAction();
    float ComputeEffectiveFontSize(float baseFontSize) const;
    void OnFontSizeScaleChanged(float newScale) override;

    std::string labelValue_;
    float fontSize_ = 16.0F;
    A2UIFontWeight fontWeight_ = A2UIFontWeight::W500;
    bool enabled_ = true;
    bool hasFontColor_ = false;
    uint32_t fontColor_ = 0;
    bool hasBackgroundColor_ = false;
    uint32_t backgroundColor_ = 0;
    float minFontSize_ = 0.0F;
    float maxFontSize_ = 0.0F;
    float minFontScale_ = 0.0F;
    float maxFontScale_ = 0.0F;
    std::string fontScaleMode_ = "followSystem";
    std::shared_ptr<ActionInfo> actionInfo_;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_BUTTON_COMPONENT_H
