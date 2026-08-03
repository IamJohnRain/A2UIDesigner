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

#ifndef A2UI_EXTENDED_CHECKBOX_COMPONENT_H
#define A2UI_EXTENDED_CHECKBOX_COMPONENT_H

#include <cstdint>
#include <memory>

#include "components/A2UI/checkbox/CheckboxTheme.h"
#include "components/extended/ExtendedComponent.h"

namespace NativeModule {

class ExtendedCheckboxComponent : public ExtendedComponent {
public:
    ExtendedCheckboxComponent();
    ~ExtendedCheckboxComponent() override;

    std::string GetType() const override;

    bool GetSelect() const;
    std::string GetValue() const;
    std::string GetLabel() const;
    std::string GetGroup() const;
    std::string GetRuntimeStateScope() const override;
    std::string GetRuntimeStateKey() const override;
    JsonValue CaptureRuntimeState() const override;
    void RestoreRuntimeState(const JsonValue& state) override;
    bool HasExplicitSelect() const;
    bool HasExplicitShape() const;
    void ApplyInheritedSelect(bool select);
    void ApplyInheritedShape(int32_t shape);

    std::shared_ptr<CheckboxTheme> GetTheme();
    void OnConfigChange(const ThemeContext& context) override;

#ifdef TDD_BUILD
    bool GetSelectForTest() const
    {
        return select_;
    }

    std::string GetValueForTest() const
    {
        return value_;
    }

    std::string GetLabelForTest() const
    {
        return label_;
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

    ArkUI_NodeHandle GetCheckboxNodeForTest() const
    {
        return checkboxNode_;
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
    void ApplyStyleColor(const JsonValue& styles, const char* propertyName, const char* aliasName,
        uint32_t fallbackColor, bool& overridden, void (ExtendedCheckboxComponent::*setter)(uint32_t));
    void HandleNodeEvent(A2UINodeEvent* event);
    void HandleCheckboxChange(bool isChecked);
    void UpdateChangeEventRegistration();
    void SetSelect(bool select);
    void SetValue(const std::string& value);
    void SetSelectedColor(uint32_t color);
    void SetUnselectedColor(uint32_t color);
    void SetMark(uint32_t strokeColor, float size, float strokeWidth);
    void SetShape(A2UICheckboxShape shape);
    void SetLabel(const std::string& label);
    void SetGroup(const std::string& group);
    std::string ResolveRuntimeStateValue() const;
    bool TryRestoreSelectFromRuntimeState();
    void SyncRuntimeStateToSurface();
    std::string ResolveSelectBindingPath() const;
    void SyncSelectToBoundDataModel(bool select);

    ArkUI_NodeHandle checkboxNode_ = nullptr;
    ArkUI_NodeHandle textNode_ = nullptr;
    bool select_ = false;
    uint32_t selectedColor_ = 0xFF317AF7;
    uint32_t unselectedColor_ = 0x33FFFFFF;
    uint32_t markStrokeColor_ = 0xFFFFFFFF;
    float markSize_ = 20.0f;
    float markStrokeWidth_ = 2.0f;
    A2UICheckboxShape shape_ = A2UICheckboxShape::CIRCLE;
    std::string label_;
    std::string group_;
    std::string value_;
    bool changeEventRegistered_ = false;
    std::weak_ptr<CheckboxTheme> cachedTheme_;
    bool selectedColorOverridden_ = false;
    bool unselectedColorOverridden_ = false;
    bool markOverridden_ = false;
    bool hasExplicitSelect_ = false;
    bool hasExplicitShape_ = false;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_CHECKBOX_COMPONENT_H
