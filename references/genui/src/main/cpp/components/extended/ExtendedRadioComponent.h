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

#ifndef A2UI_EXTENDED_RADIO_COMPONENT_H
#define A2UI_EXTENDED_RADIO_COMPONENT_H

#include <cstdint>
#include <string>

#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedRadioComponent : public ExtendedComponent {
public:
    ExtendedRadioComponent();
    ~ExtendedRadioComponent() override;

    std::string GetType() const override;
    bool GetChecked() const
    {
        return checked_;
    }

    const std::string& GetValue() const
    {
        return value_;
    }

    const std::string& GetGroup() const
    {
        return group_;
    }

#ifdef TDD_BUILD
    bool GetCheckedForTest() const
    {
        return checked_;
    }

    const std::string& GetValueForTest() const
    {
        return value_;
    }

    const std::string& GetGroupForTest() const
    {
        return group_;
    }

    uint32_t GetCheckedBackgroundColorForTest() const
    {
        return checkedBackgroundColor_;
    }

    uint32_t GetUncheckedBackgroundColorForTest() const
    {
        return uncheckedBackgroundColor_;
    }

    uint32_t GetIndicatorColorForTest() const
    {
        return indicatorColor_;
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
    void ApplyStyleColor(
        const JsonValue& styles, const char* propertyName, uint32_t defaultColor, uint32_t& outColor, bool& overridden);
    void HandleNodeEvent(A2UINodeEvent* event);
    void HandleRadioChange(bool checked);
    void UpdateChangeEventRegistration();
    void SetChecked(bool checked);
    void SetValue(const std::string& value);
    void SetGroup(const std::string& group);
    void ApplyRadioStyle();
    void SyncSiblingCheckedState();
    std::string ResolveCheckedBindingPath() const;
    void SyncCheckedToBoundDataModel(bool value);

    bool checked_ = false;
    std::string value_;
    std::string group_;
    uint32_t checkedBackgroundColor_ = 0xFF0A59F7u;
    uint32_t uncheckedBackgroundColor_ = 0x33FFFFFFu;
    uint32_t indicatorColor_ = 0xFFFFFFFF;
    bool checkedBackgroundColorOverridden_ = false;
    bool uncheckedBackgroundColorOverridden_ = false;
    bool indicatorColorOverridden_ = false;
    bool changeEventRegistered_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_RADIO_COMPONENT_H
