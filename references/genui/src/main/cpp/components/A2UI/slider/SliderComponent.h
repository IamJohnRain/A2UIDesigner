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

#ifndef A2UI_SLIDER_COMPONENT_H
#define A2UI_SLIDER_COMPONENT_H

#include <memory>
#include <string>
#include <unordered_set>

#include "checks/ChecksEngine.h"

#include "../A2UIComponent.h"
#include "SliderTheme.h"

namespace NativeModule {

class SliderComponent : public A2UIComponent {
public:
    SliderComponent();
    ~SliderComponent() override;

    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<SliderTheme> GetTheme();

    void SetMinValue(float min);
    void SetMaxValue(float max);
    void SetValue(float value);
    void SetStep(float step);
    void SetStyle(A2UISliderStyle style);
    void SetSelectedColor(uint32_t color);
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;

protected:
    void OnAttachToParent() override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool HandleSpecialProperty(const std::string& propertyName, const JsonValue& value) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

private:
    void InitializeInternalNodes();
    void AttachNativeSubtree();
    void SetLabel(const std::string& label);
    void SetEnabled(bool enabled);
    void RefreshEnabledState();
    void ParseChecks(const JsonValue& descriptor);
    void AddCheckBindingPath(const std::string& path);
    bool IsCheckBindingProperty(const std::string& property) const;
    bool ValidateChecks() const;

    ArkUI_NodeHandle textNode_;
    ArkUI_NodeHandle sliderNode_;
    std::string label_;
    float minValue_;
    float maxValue_;
    float value_;
    bool internalNodesMounted_ = false;

    std::unique_ptr<ChecksEngine> checksEngine_;
    std::unordered_set<std::string> checkBindingPaths_;

    // Theme cache
    std::weak_ptr<SliderTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_SLIDER_COMPONENT_H
