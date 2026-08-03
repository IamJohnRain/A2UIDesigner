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

#ifndef A2UI_CHECKBOX_COMPONENT_H
#define A2UI_CHECKBOX_COMPONENT_H

#include <memory>
#include <string>
#include <unordered_set>

#include "checks/ChecksEngine.h"

#include "../A2UIComponent.h"
#include "CheckboxTheme.h"

namespace NativeModule {

class CheckboxComponent : public A2UIComponent {
public:
    CheckboxComponent();
    ~CheckboxComponent() override;

    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<CheckboxTheme> GetTheme();

    void SetSelect(bool select);
    void SetSelectColor(uint32_t color);
    void SetCheckboxShape(A2UICheckboxShape shape);
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;

protected:
    void OnAttachToParent() override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool HandleSpecialProperty(const std::string& propertyName, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;
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
    ArkUI_NodeHandle checkboxNode_;
    std::string label_;
    bool value_;
    bool internalNodesMounted_ = false;

    std::unique_ptr<ChecksEngine> checksEngine_;
    std::unordered_set<std::string> checkBindingPaths_;

    // Theme cache
    std::weak_ptr<CheckboxTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_CHECKBOX_COMPONENT_H
