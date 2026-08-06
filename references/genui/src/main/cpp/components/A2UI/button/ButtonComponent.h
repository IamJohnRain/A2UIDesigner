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

#ifndef A2UI_BUTTON_COMPONENT_H
#define A2UI_BUTTON_COMPONENT_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "checks/ChecksEngine.h"
#include "functions/ActionInfo.h"

#include "../A2UIComponent.h"
#include "ButtonTheme.h"

namespace NativeModule {

class TextComponent;

class ButtonComponent : public A2UIComponent {
public:
    // Lifecycle / type
    ButtonComponent();
    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<ButtonTheme> GetTheme();

protected:
    // Descriptor / hierarchy hooks
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    void OnRemoveChild(const std::shared_ptr<Component>& child) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool HandleSpecialProperty(const std::string& propertyName, const JsonValue& value) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

    // Theme configuration change hook
    void OnConfigChange(const ThemeContext& context) override;

private:
    // Variant and visual style
    void SetVariant(const std::string& variant);
    void SetFontColor(uint32_t color);
    void SetEnabled(bool enabled);
    void SetButtonType(A2UIButtonType buttonType);
    void ApplyTextChildStyle(const std::shared_ptr<Component>& child);
    void ApplyIconChildStyle(const std::shared_ptr<Component>& child);
    void ApplyChildVisualStyle();

    // Checks lifecycle
    void RefreshEnabledState();
    void ParseChecks(const JsonValue& descriptor);
    void AddCheckBindingPath(const std::string& path);
    bool IsCheckBindingProperty(const std::string& property) const;
    bool ValidateChecks() const;

    // Action dispatch
    void DispatchAction() const;
    void DispatchFunctionCallAction() const;

    // Style and child-text sync state
    std::string variant_ = "";
    std::weak_ptr<Component> childComponent_;

    // Checks state
    std::unique_ptr<ChecksEngine> checksEngine_;
    std::unordered_set<std::string> checkBindingPaths_;

    // Action state
    std::shared_ptr<ActionInfo> actionInfo_;

    // Theme cache
    std::weak_ptr<ButtonTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_BUTTON_COMPONENT_H
