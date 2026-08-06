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

#ifndef A2UI_TEXT_FIELD_COMPONENT_H
#define A2UI_TEXT_FIELD_COMPONENT_H

#include <memory>
#include <string>
#include <unordered_set>

#include "checks/ChecksEngine.h"

#include "../A2UIComponent.h"
#include "TextFieldTheme.h"

namespace NativeModule {

class TextFieldComponent : public A2UIComponent {
public:
    // Lifecycle / type
    TextFieldComponent();
    ~TextFieldComponent() override;
    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<TextFieldTheme> GetTheme();

protected:
    // Descriptor handling
    void OnAttachToParent() override;
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    bool HandleSpecialProperty(const std::string& propertyName, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    std::vector<std::string> GetComponentDirectRequiredPropertyKeys() const override;

private:
    // Native input event bridge
    static void NodeEventReceiver(A2UINodeEvent* event);
    void HandleNodeEvent(A2UINodeEvent* event);
    void HandleInputValueChange(const std::string& nextValue);

    // Internal node initialization and attribute setters
    void InitializeInternalNodes();
    void AttachNativeSubtree();
    void AttachInputNodeIfNeeded(ArkUI_NodeHandle inputNode, bool& mountedFlag);
    void SetLabelText(const std::string& text);
    void SetValueText(const std::string& text);
    void SetErrorText(const std::string& text);
    void SetFontColor(uint32_t labelFontColor, uint32_t errorFontColor);
    void SetVariant(const std::string& variant);
    void SetValidationRegexp(const std::string& regexp);
    bool EnsureTextInputNode();
    bool EnsureTextAreaNode();
    void SetInputMode(bool useTextArea);
    void SetTextInputType(A2UITextInputType inputType);
    PropertyDeclaration CreateStringPropertyDeclaration(
        const std::string& propertyName, bool allowDynamic, void (TextFieldComponent::*setter)(const std::string&));
    PropertyDeclaration CreateVariantPropertyDeclaration();

    // validationRegexp / value sync
    bool ValidationRegexpCheck();
    std::string ResolveValueBindingPath() const;
    void SyncValueToBoundDataModel(const std::string& value);

    // Checks lifecycle
    void ParseChecks(const JsonValue& descriptor);
    void AddCheckBindingPath(const std::string& path);
    bool IsCheckBindingProperty(const std::string& property) const;
    bool ValidateChecks();

    // Internal node handles
    ArkUI_NodeHandle labelNode_ = nullptr;
    ArkUI_NodeHandle inputNode_ = nullptr; // Active input node
    ArkUI_NodeHandle textInputNode_ = nullptr;
    ArkUI_NodeHandle textAreaNode_ = nullptr;
    ArkUI_NodeHandle errorNode_ = nullptr;
    bool useTextArea_ = false;
    bool internalNodesMounted_ = false;
    bool textInputMounted_ = false;
    bool textAreaMounted_ = false;

    // Field state
    std::string labelText_;
    std::string valueText_;
    std::string valueBindingPath_;
    std::string errorText_;
    std::string variant_ = "";

    // Checks state
    std::unique_ptr<ChecksEngine> checksEngine_;
    std::unordered_set<std::string> checkBindingPaths_;
    bool isCheckPass_ = true;

    // Validation state
    std::string validationRegexp_;
    mutable std::string latestValidationMessage_;
    bool isValidationRegexpPass_ = true;
    bool hasReportedInvalidValidationRegexpError_ = false;

    // Theme cache
    std::weak_ptr<TextFieldTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_TEXT_FIELD_COMPONENT_H
