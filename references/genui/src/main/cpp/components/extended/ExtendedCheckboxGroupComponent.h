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

#ifndef A2UI_EXTENDED_CHECKBOX_GROUP_COMPONENT_H
#define A2UI_EXTENDED_CHECKBOX_GROUP_COMPONENT_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "components/A2UI/checkbox/CheckboxGroupTheme.h"
#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedCheckboxGroupComponent : public ExtendedComponent {
public:
    ExtendedCheckboxGroupComponent();
    ~ExtendedCheckboxGroupComponent() override;

    std::string GetType() const override;

    std::shared_ptr<CheckboxGroupTheme> GetTheme();
    void OnConfigChange(const ThemeContext& context) override;

    int32_t GetSelectAllStatus() const
    {
        return selectAllStatus_;
    }

    bool GetSelectAll() const
    {
        return selectAll_;
    }

    int32_t GetShape() const
    {
        return static_cast<int32_t>(shape_);
    }

    std::string GetGroup() const
    {
        return group_;
    }

    const std::vector<std::string>& GetSelectedNames() const
    {
        return selectedNames_;
    }

#ifdef TDD_BUILD
    bool GetSelectAllForTest() const
    {
        return selectAll_;
    }

    uint32_t GetSelectedColorForTest() const
    {
        return selectedColor_;
    }

    uint32_t GetUnselectedColorForTest() const
    {
        return unselectedColor_;
    }

    uint32_t GetMarkStrokeColorForTest() const
    {
        return markStrokeColor_;
    }

    float GetMarkSizeForTest() const
    {
        return markSize_;
    }

    float GetMarkStrokeWidthForTest() const
    {
        return markStrokeWidth_;
    }

    int32_t GetShapeForTest() const
    {
        return static_cast<int32_t>(shape_);
    }

    std::string GetGroupForTest() const
    {
        return group_;
    }

    ArkUI_NodeHandle GetCheckboxGroupNodeForTest() const
    {
        return checkboxGroupNode_;
    }

    int32_t GetSelectAllStatusForTest() const
    {
        return selectAllStatus_;
    }

    const std::vector<std::string>& GetSelectedNamesForTest() const
    {
        return selectedNames_;
    }

    bool HasSelectedColorOverrideForTest() const
    {
        return selectedColorOverridden_;
    }

    bool HasUnselectedColorOverrideForTest() const
    {
        return unselectedColorOverridden_;
    }

    bool HasMarkOverrideForTest() const
    {
        return markOverridden_;
    }
#endif

protected:
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier) override;
    void ValidateComponentSpecificStylesSchema(const JsonValue& styles) override;
    void RegisterComponentSpecificListeners() override;
    void OnPropertyRemoved(const std::string& propertyName) override;

private:
    static void NodeEventReceiver(A2UINodeEvent* event);

    void ValidateStylesSchema(const JsonValue& styles);
    std::unique_ptr<JsonAdapter> ResolveMarkDynamicMembers(const JsonValue& markValue) const;
    void ValidateResolvedMarkDfx(const JsonValue& markValue);
    void ApplyStyleColor(const JsonValue& styles, const char* propertyName, uint32_t fallbackColor, bool& overridden,
        void (ExtendedCheckboxGroupComponent::*setter)(uint32_t));
    void HandleNodeEvent(A2UINodeEvent* event);
    void ParseNamesFromEventString(const std::string& eventStr);
    void ParseStatusFromEventString(const std::string& eventStr);
    void HandleCheckboxGroupChange();
    void UpdateChangeEventRegistration();
    void SetSelectAll(bool selectAll);
    void SetSelectedColor(uint32_t color);
    void SetUnselectedColor(uint32_t color);
    void SetMark(uint32_t strokeColor, float size, float strokeWidth);
    void SetShape(A2UICheckboxShape shape);
    void SetGroup(const std::string& group);

    ArkUI_NodeHandle checkboxGroupNode_ = nullptr;
    int32_t selectAllStatus_ = 2;
    bool selectAll_ = false;
    std::vector<std::string> selectedNames_;
    uint32_t selectedColor_ = 0xFF007DFF;
    uint32_t unselectedColor_ = 0x66182431;
    uint32_t markStrokeColor_ = 0xFFFFFFFF;
    float markSize_ = 20.0f;
    float markStrokeWidth_ = 2.0f;
    A2UICheckboxShape shape_ = A2UICheckboxShape::CIRCLE;
    std::string group_;
    bool changeEventRegistered_ = false;
    std::weak_ptr<CheckboxGroupTheme> cachedTheme_;
    bool selectedColorOverridden_ = false;
    bool unselectedColorOverridden_ = false;
    bool markOverridden_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_CHECKBOX_GROUP_COMPONENT_H
