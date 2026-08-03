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

#ifndef A2UI_EXTENDED_TOGGLE_COMPONENT_H
#define A2UI_EXTENDED_TOGGLE_COMPONENT_H

#include <cstdint>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedToggleComponent : public ExtendedComponent {
public:
    ExtendedToggleComponent();
    ~ExtendedToggleComponent() override;

    std::string GetType() const override;
    bool GetIsOn() const
    {
        return isOn_;
    }

    const std::string& GetLabel() const
    {
        return label_;
    }

#ifdef TDD_BUILD
    bool GetIsOnForTest() const
    {
        return isOn_;
    }

    bool GetEnabledForTest() const
    {
        return enabled_;
    }

    uint32_t GetSelectedColorForTest() const
    {
        return selectedColor_;
    }

    uint32_t GetUnSelectedColorForTest() const
    {
        return unSelectedColor_;
    }

    uint32_t GetSwitchPointColorForTest() const
    {
        return switchPointColor_;
    }

    std::string GetLabelForTest() const
    {
        return label_;
    }

    ArkUI_NodeHandle GetToggleNodeForTest() const
    {
        return toggleNode_;
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
    ThemeMode ResolveThemeMode() const;
    void ApplyStyleColor(const JsonValue& styles, const char* propertyName, uint32_t defaultColor, uint32_t& outColor,
        bool& overridden, void (ExtendedToggleComponent::*setter)(uint32_t));
    void HandleNodeEvent(A2UINodeEvent* event);
    void HandleToggleChange(bool isOn);
    void UpdateChangeEventRegistration();
    void SetIsOn(bool isOn);
    void SetEnabled(bool enabled);
    void SetSelectedColor(uint32_t color);
    void SetUnSelectedColor(uint32_t color);
    void SetSwitchPointColor(uint32_t color);
    void SetLabel(const std::string& label);
    std::string ResolveIsOnBindingPath() const;
    void SyncIsOnToBoundDataModel(bool value);

    ArkUI_NodeHandle toggleNode_ = nullptr;
    ArkUI_NodeHandle textNode_ = nullptr;
    bool isOn_ = false;
    bool enabled_ = true;
    uint32_t selectedColor_ = 0xFF007DFF;
    uint32_t unSelectedColor_ = 0x19000000u;
    uint32_t switchPointColor_ = 0xFFFFFFFF;
    bool selectedColorOverridden_ = false;
    bool unSelectedColorOverridden_ = false;
    bool switchPointColorOverridden_ = false;
    std::string label_;
    bool changeEventRegistered_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_TOGGLE_COMPONENT_H
