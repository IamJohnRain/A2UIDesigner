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

#include "CheckboxComponent.h"

#include "utils/LogA2UI.h"

#include "CheckboxTheme.h"

namespace NativeModule {
namespace {
constexpr char CHECK_BINDING_PROPERTY_PREFIX[] = "__checks_dep_";
constexpr uint32_t DEFAULT_CHECKBOX_SELECTED_COLOR = 0xFF007DFF;
constexpr float DEFAULT_CHECKBOX_LABEL_FONT_SIZE = 14.0F;
constexpr float DEFAULT_CHECKBOX_LABEL_SPACING = 12.0F;
} // namespace

CheckboxComponent::CheckboxComponent()
    : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::ROW)), textNode_(nullptr), checkboxNode_(nullptr),
      value_(false), checksEngine_(std::make_unique<ChecksEngine>(
                         [this]() { return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() }; }))
{
    InitializeInternalNodes();
}

PropertyDeclaration CheckboxComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(CheckboxComponent&)>> declarations = {
        { "label",
            [](CheckboxComponent& checkboxComponent) {
                return PropertyDeclaration { .name = "label",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .fallbackString = "",
                    .applyValue = [&checkboxComponent](const JsonValue& value) {
                        checkboxComponent.SetLabel(value.GetStringValue(""));
                    } };
            } },
        { "value",
            [](CheckboxComponent& checkboxComponent) {
                return PropertyDeclaration { .name = "value",
                    .type = PropertyValueType::BOOLEAN,
                    .allowDynamic = true,
                    .fallbackBool = false,
                    .applyValue = [&checkboxComponent](const JsonValue& value) {
                        checkboxComponent.SetSelect(value.GetBoolValue(false));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

bool CheckboxComponent::HandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName == "checks") {
        ValidateChecksSpecialProperty(value);
        ParseChecks(value);
        return true;
    }
    return false;
}

bool CheckboxComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "checks";
}

std::vector<std::string> CheckboxComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "label", "value" };
}

CheckboxComponent::~CheckboxComponent()
{
    if (internalNodesMounted_ && textNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, textNode_);
    }
    if (textNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(textNode_);
    }

    if (internalNodesMounted_ && checkboxNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, checkboxNode_);
    }
    if (checkboxNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(checkboxNode_);
    }
}

void CheckboxComponent::InitializeInternalNodes()
{
    if (!ArkUINodeApiAdapter::IsAvailable()) {
        return;
    }

    if (textNode_ == nullptr) {
        textNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    }
    if (checkboxNode_ == nullptr) {
        checkboxNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::CHECKBOX);
    }
    if (textNode_ == nullptr || checkboxNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeFontSize(textNode_, DEFAULT_CHECKBOX_LABEL_FONT_SIZE);

    ArkUINodeApiAdapter::SetNodeMargin(checkboxNode_, 0.0F, 0.0F, 0.0F, DEFAULT_CHECKBOX_LABEL_SPACING);
}

void CheckboxComponent::AttachNativeSubtree()
{
    if (internalNodesMounted_) {
        return;
    }
    if (!ArkUINodeApiAdapter::IsAvailable() || nativeView_ == nullptr || textNode_ == nullptr ||
        checkboxNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::AddChild(nativeView_, textNode_);
    ArkUINodeApiAdapter::AddChild(nativeView_, checkboxNode_);
    internalNodesMounted_ = true;
}

void CheckboxComponent::OnAttachToParent()
{
    AttachNativeSubtree();
}

std::string CheckboxComponent::GetType() const
{
    return "CheckBox";
}

void CheckboxComponent::SetLabel(const std::string& label)
{
    label_ = label;
    ArkUINodeApiAdapter::SetNodeTextContent(textNode_, label_);
}

void CheckboxComponent::SetSelect(bool select)
{
    value_ = select;
    ArkUINodeApiAdapter::SetNodeCheckboxSelect(checkboxNode_, select);
}

void CheckboxComponent::SetSelectColor(uint32_t color)
{
    ArkUINodeApiAdapter::SetNodeCheckboxSelectColor(checkboxNode_, color);
}

void CheckboxComponent::SetCheckboxShape(A2UICheckboxShape shape)
{
    ArkUINodeApiAdapter::SetNodeCheckboxShape(checkboxNode_, shape);
}

void CheckboxComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    LOG_A2UI(LOG_INFO, "CheckboxComponent::ApplyPrivateAttributes");

    if (descriptor.Has("checks")) {
        SetPropertyFromDescriptor("checks", descriptor);
    } else {
        ParseChecks(descriptor);
    }
    RefreshEnabledState();

    ApplySchemaProperty("label", descriptor);
    ApplySchemaProperty("value", descriptor);

    auto checkboxTheme = GetTheme();
    // 缁勪欢閫傞厤theme鏃堕渶瑕佷慨鏀?
    SetSelectColor(checkboxTheme != nullptr ? checkboxTheme->GetSelectedColor() : DEFAULT_CHECKBOX_SELECTED_COLOR);
    SetCheckboxShape(A2UICheckboxShape::CIRCLE);

    ArkUINodeApiAdapter::SetNodeRowAlignItems(nativeView_, A2UIVerticalAlignment::CENTER);
}

void CheckboxComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
    if (IsCheckBindingProperty(property)) {
        RefreshEnabledState();
        return;
    }
}

void CheckboxComponent::SetEnabled(bool enabled)
{
    ArkUINodeApiAdapter::SetNodeEnabled(checkboxNode_, enabled);
}

void CheckboxComponent::RefreshEnabledState()
{
    SetEnabled(ValidateChecks());
}

void CheckboxComponent::ParseChecks(const JsonValue& descriptor)
{
    if (checksEngine_ == nullptr) {
        return;
    }
    checksEngine_->ParseChecks(descriptor);
    for (const auto& path : checksEngine_->GetBindingPaths()) {
        AddCheckBindingPath(path);
    }
}

void CheckboxComponent::AddCheckBindingPath(const std::string& path)
{
    if (path.empty()) {
        return;
    }
    if (!checkBindingPaths_.insert(path).second) {
        return;
    }
    std::string propertyName = std::string(CHECK_BINDING_PROPERTY_PREFIX) + std::to_string(checkBindingPaths_.size());
    AddBinding(propertyName, path);
}

bool CheckboxComponent::IsCheckBindingProperty(const std::string& property) const
{
    return property.rfind(CHECK_BINDING_PROPERTY_PREFIX, 0) == 0;
}

bool CheckboxComponent::ValidateChecks() const
{
    if (checksEngine_ == nullptr) {
        return true;
    }
    std::string message;
    bool pass = checksEngine_->Validate(&message);
    if (!pass) {
        LOG_A2UI(LOG_WARN, "Checkbox check failed, componentId=%{public}s, message=%{public}s",
            GetComponentId().c_str(), message.c_str());
    }
    return pass;
}

std::shared_ptr<CheckboxTheme> CheckboxComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }

    // Cast to specific type and cache it
    theme = std::dynamic_pointer_cast<CheckboxTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void CheckboxComponent::OnConfigChange(const ThemeContext& context)
{
    auto checkboxTheme = GetTheme();
    if (checkboxTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
    SetSelectColor(checkboxTheme->GetSelectedColor());
}

} // namespace NativeModule
