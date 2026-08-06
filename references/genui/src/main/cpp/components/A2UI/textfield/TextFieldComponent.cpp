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

#include "TextFieldComponent.h"

#include <algorithm>
#include <cctype>
#include <regex>

#include "data/BindingEngine.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/LogA2UI.h"

#include "RenderManager.h"
#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"
#include "TextFieldTheme.h"

namespace NativeModule {

namespace {

constexpr char CHECK_BINDING_PROPERTY_PREFIX[] = "__checks_dep_";
std::string VALIDATION_REGEXP_ERROR_MESSAGE = "Input does not match validationRegexp";
std::string NO_ERROR_MESSAGE = "";

std::string BuildValidationRegexpWarningPath(const std::string& componentId)
{
    return componentId.empty() ? "validationRegexp" : componentId + ".validationRegexp";
}

const std::string& ResolveVariant(const std::string& variant)
{
    static const std::string defaultVariant = "shortText";
    if (variant == "shortText" || variant == "number" || variant == "obscured" || variant == "longText") {
        return variant;
    }
    // Invalid token fallback.
    return defaultVariant;
}
} // namespace

TextFieldComponent::TextFieldComponent()
    : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN), true, true),
      checksEngine_(std::make_unique<ChecksEngine>(
          [this]() { return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() }; }))
{
    InitializeInternalNodes();
}

void TextFieldComponent::AttachNativeSubtree()
{
    if (internalNodesMounted_) {
        return;
    }
    if (!ArkUINodeApiAdapter::IsAvailable() || nativeView_ == nullptr || labelNode_ == nullptr ||
        errorNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::AddChild(nativeView_, labelNode_);
    if (textInputNode_ != nullptr) {
        ArkUINodeApiAdapter::AddChild(nativeView_, textInputNode_);
        textInputMounted_ = true;
    }
    if (textAreaNode_ != nullptr) {
        ArkUINodeApiAdapter::AddChild(nativeView_, textAreaNode_);
        textAreaMounted_ = true;
    }
    ArkUINodeApiAdapter::AddChild(nativeView_, errorNode_);
    internalNodesMounted_ = true;
}

void TextFieldComponent::OnAttachToParent()
{
    AttachNativeSubtree();
}

PropertyDeclaration TextFieldComponent::CreateStringPropertyDeclaration(
    const std::string& propertyName, bool allowDynamic, void (TextFieldComponent::*setter)(const std::string&))
{
    return PropertyDeclaration { .name = propertyName,
        .type = PropertyValueType::STRING,
        .allowDynamic = allowDynamic,
        .fallbackString = "",
        .applyValue = [this, setter](const JsonValue& value) { (this->*setter)(value.GetStringValue("")); } };
}

PropertyDeclaration TextFieldComponent::CreateVariantPropertyDeclaration()
{
    return PropertyDeclaration { .name = "variant",
        .type = PropertyValueType::ENUM_STRING,
        .allowDynamic = false,
        .fallbackString = "shortText",
        .enumAllowed = { "shortText", "number", "obscured", "longText" },
        .enumFallback = "shortText",
        .applyValue = [this](const JsonValue& value) { SetVariant(value.GetStringValue("shortText")); } };
}

PropertyDeclaration TextFieldComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName == "label") {
        return CreateStringPropertyDeclaration("label", true, &TextFieldComponent::SetLabelText);
    }
    if (propertyName == "value") {
        return CreateStringPropertyDeclaration("value", true, &TextFieldComponent::SetValueText);
    }
    if (propertyName == "variant") {
        return CreateVariantPropertyDeclaration();
    }
    if (propertyName == "validationRegexp") {
        return CreateStringPropertyDeclaration("validationRegexp", false, &TextFieldComponent::SetValidationRegexp);
    }
    return {};
}

bool TextFieldComponent::HandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName == "checks") {
        ValidateChecksSpecialProperty(value);
        ParseChecks(value);
        return true;
    }
    return false;
}

bool TextFieldComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "checks";
}

std::vector<std::string> TextFieldComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "label" };
}

TextFieldComponent::~TextFieldComponent()
{
    if (!ArkUINodeApiAdapter::IsAvailable()) {
        return;
    }

    if (textInputNode_ != nullptr) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(textInputNode_, A2UINodeEventType::TEXT_INPUT_ON_CHANGE);
        ArkUINodeApiAdapter::RemoveNodeEventReceiver(textInputNode_, TextFieldComponent::NodeEventReceiver);
        ArkUINodeApiAdapter::SetUserData(textInputNode_, nullptr);
    }

    if (textAreaNode_ != nullptr) {
        ArkUINodeApiAdapter::UnregisterNodeEvent(textAreaNode_, A2UINodeEventType::TEXT_AREA_ON_CHANGE);
        ArkUINodeApiAdapter::RemoveNodeEventReceiver(textAreaNode_, TextFieldComponent::NodeEventReceiver);
        ArkUINodeApiAdapter::SetUserData(textAreaNode_, nullptr);
    }

    if (internalNodesMounted_ && labelNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, labelNode_);
    }
    if (labelNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(labelNode_);
        labelNode_ = nullptr;
    }

    if (textInputMounted_ && textInputNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, textInputNode_);
    }
    if (textInputNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(textInputNode_);
        textInputNode_ = nullptr;
    }

    if (textAreaMounted_ && textAreaNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, textAreaNode_);
    }
    if (textAreaNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(textAreaNode_);
        textAreaNode_ = nullptr;
    }
    inputNode_ = nullptr;

    if (internalNodesMounted_ && errorNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, errorNode_);
    }
    if (errorNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(errorNode_);
        errorNode_ = nullptr;
    }
}

std::string TextFieldComponent::GetType() const
{
    return "TextField";
}

void TextFieldComponent::InitializeInternalNodes()
{
    if (labelNode_ != nullptr && errorNode_ != nullptr) {
        return;
    }
    if (!ArkUINodeApiAdapter::IsAvailable() || nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeColumnAlignItems(nativeView_, A2UIHorizontalAlignment::START);

    ArkUINodeApiAdapter::SetNodeColumnJustifyContent(nativeView_, A2UIFlexAlignment::START);

    if (labelNode_ == nullptr) {
        labelNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    }
    if (errorNode_ == nullptr) {
        errorNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    }
    if (labelNode_ == nullptr || errorNode_ == nullptr) {
        LOG_A2UI(LOG_ERROR, "TextFieldComponent::InitializeInternalNodes: create inner node failed");
        return;
    }

    ArkUINodeApiAdapter::SetNodeFontSize(labelNode_, TextFieldTheme::GetLabelFontSize());

    std::array<float, 4> labelPaddingValues = TextFieldTheme::GetLabelPadding();
    ArkUINodeApiAdapter::SetNodePadding(
        labelNode_, labelPaddingValues[0], labelPaddingValues[1], labelPaddingValues[2], labelPaddingValues[3]);

    ArkUINodeApiAdapter::SetNodeFontWeight(labelNode_, TextFieldTheme::GetLabelFontWeight());

    ArkUINodeApiAdapter::SetNodeFontSize(errorNode_, TextFieldTheme::GetErrorFontSize());

    std::array<float, 4> errorPaddingValues = TextFieldTheme::GetErrorPadding();
    ArkUINodeApiAdapter::SetNodePadding(
        errorNode_, errorPaddingValues[0], errorPaddingValues[1], errorPaddingValues[2], errorPaddingValues[3]);
}

void TextFieldComponent::AttachInputNodeIfNeeded(ArkUI_NodeHandle inputNode, bool& mountedFlag)
{
    if (!internalNodesMounted_ || mountedFlag || inputNode == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::InsertChildAt(nativeView_, inputNode, 1);
    mountedFlag = true;
}

void TextFieldComponent::SetLabelText(const std::string& text)
{
    if (labelText_ == text) {
        return;
    }
    labelText_ = text;
    if (labelNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextContent(labelNode_, labelText_);
}

void TextFieldComponent::SetValueText(const std::string& text)
{
    if (valueText_ == text) {
        return;
    }
    valueText_ = text;
    ValidationRegexpCheck();
    if (inputNode_ == nullptr) {
        return;
    }
    if (textInputNode_ == inputNode_) {
        ArkUINodeApiAdapter::SetNodeTextInputText(textInputNode_, valueText_);
        return;
    }
    if (textAreaNode_ == inputNode_) {
        ArkUINodeApiAdapter::SetNodeTextAreaText(textAreaNode_, valueText_);
        return;
    }
}

void TextFieldComponent::NodeEventReceiver(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    auto* textField = reinterpret_cast<TextFieldComponent*>(ArkUINodeApiAdapter::GetUserData(nodeHandle));
    if (textField != nullptr) {
        textField->HandleNodeEvent(event);
    }
}

void TextFieldComponent::HandleNodeEvent(A2UINodeEvent* event)
{
    if (event == nullptr) {
        return;
    }
    auto nodeHandle = ArkUIOHApiAdapter::NodeEventGetNodeHandle(event);
    if (inputNode_ != nullptr && nodeHandle != inputNode_) {
        return;
    }
    A2UINodeEventType eventType = ArkUIOHApiAdapter::NodeEventGetEventType(event);
    if (eventType != A2UINodeEventType::TEXT_INPUT_ON_CHANGE && eventType != A2UINodeEventType::TEXT_AREA_ON_CHANGE) {
        return;
    }
    A2UIStringAsyncEvent* stringEvent = ArkUIOHApiAdapter::NodeEventGetStringAsyncEvent(event);
    if (stringEvent == nullptr || stringEvent->pStr == nullptr) {
        return;
    }
    std::string nextValue = stringEvent->pStr;
    HandleInputValueChange(nextValue);
}

std::string TextFieldComponent::ResolveValueBindingPath() const
{
    const auto& bindings = GetDataBindings();
    for (auto iter = bindings.rbegin(); iter != bindings.rend(); ++iter) {
        if (iter->propertyName_ == "value" && !iter->dataPath_.empty()) {
            return iter->dataPath_;
        }
    }
    return "";
}

void TextFieldComponent::SyncValueToBoundDataModel(const std::string& value)
{
    if (valueBindingPath_.empty()) {
        return;
    }

    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr) {
        LOG_A2UI(LOG_WARN, "TextFieldComponent::SyncValueToBoundDataModel: surface not found, surfaceId=%{public}s",
            surfaceId.c_str());
        return;
    }
    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        LOG_A2UI(LOG_WARN,
            "TextFieldComponent::SyncValueToBoundDataModel: binding engine is null, surfaceId=%{public}s",
            surfaceId.c_str());
        return;
    }

    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateString(value);
    if (valueAdapter == nullptr) {
        return;
    }
    bindingEngine->UpdateDataModelByPath(surfaceId, valueBindingPath_, valueAdapter->GetRoot());
}

void TextFieldComponent::HandleInputValueChange(const std::string& nextValue)
{
    if (nextValue == valueText_) {
        return;
    }
    valueText_ = nextValue;
    // New spec: user input should always sync to bound data model in realtime.
    if (!valueBindingPath_.empty()) {
        SyncValueToBoundDataModel(valueText_);
    }
    ValidationRegexpCheck();
}

bool TextFieldComponent::ValidationRegexpCheck()
{
    bool result = true;
    if (!validationRegexp_.empty()) {
        try {
            result = std::regex_match(valueText_, std::regex(validationRegexp_));
        } catch (const std::regex_error& error) {
            LOG_A2UI(LOG_WARN, "TextFieldComponent::ValidationRegexpCheck: invalid regexp=%{public}s",
                validationRegexp_.c_str());
            if (!hasReportedInvalidValidationRegexpError_ && renderId_ >= 0) {
                WarningDispatchBridge::GetInstance().Dispatch(renderId_, surfaceId_, componentId_,
                    SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property validationRegexp is invalid regex '" + validationRegexp_ + "': " + error.what(),
                    BuildValidationRegexpWarningPath(componentId_), "component", GetType());
                hasReportedInvalidValidationRegexpError_ = true;
            }
            result = false;
        }
    }
    isValidationRegexpPass_ = result;
    SetErrorText(result ? NO_ERROR_MESSAGE : VALIDATION_REGEXP_ERROR_MESSAGE);
    return result;
}

void TextFieldComponent::SetErrorText(const std::string& text)
{
    if ((!isValidationRegexpPass_ || !isCheckPass_) && text == NO_ERROR_MESSAGE) {
        return;
    } else if (!isCheckPass_ && text == VALIDATION_REGEXP_ERROR_MESSAGE) {
        return;
    }
    if (errorText_ == text) {
        return;
    }
    errorText_ = text;
    if (errorNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextContent(errorNode_, errorText_);
}

bool TextFieldComponent::EnsureTextInputNode()
{
    if (textInputNode_ != nullptr) {
        AttachInputNodeIfNeeded(textInputNode_, textInputMounted_);
        return true;
    }

    textInputNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT_INPUT);
    if (textInputNode_ == nullptr) {
        LOG_A2UI(LOG_ERROR, "TextFieldComponent::EnsureTextInputNode: create text input node failed");
        return false;
    }

    ArkUINodeApiAdapter::SetUserData(textInputNode_, this);
    ArkUINodeApiAdapter::AddNodeEventReceiver(textInputNode_, TextFieldComponent::NodeEventReceiver);
    ArkUINodeApiAdapter::RegisterNodeEvent(textInputNode_, A2UINodeEventType::TEXT_INPUT_ON_CHANGE, 0, this);
    AttachInputNodeIfNeeded(textInputNode_, textInputMounted_);
    inputNode_ = textInputNode_;
    return true;
}

bool TextFieldComponent::EnsureTextAreaNode()
{
    if (textAreaNode_ != nullptr) {
        AttachInputNodeIfNeeded(textAreaNode_, textAreaMounted_);
        return true;
    }

    textAreaNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT_AREA);
    if (textAreaNode_ == nullptr) {
        LOG_A2UI(LOG_ERROR, "TextFieldComponent::EnsureTextAreaNode: create text area node failed");
        return false;
    }

    ArkUINodeApiAdapter::SetUserData(textAreaNode_, this);
    ArkUINodeApiAdapter::AddNodeEventReceiver(textAreaNode_, TextFieldComponent::NodeEventReceiver);
    ArkUINodeApiAdapter::RegisterNodeEvent(textAreaNode_, A2UINodeEventType::TEXT_AREA_ON_CHANGE, 0, this);
    AttachInputNodeIfNeeded(textAreaNode_, textAreaMounted_);
    inputNode_ = textAreaNode_;
    return true;
}

void TextFieldComponent::SetInputMode(bool useTextArea)
{
    if (useTextArea && !EnsureTextAreaNode()) {
        return;
    } else if (!EnsureTextInputNode()) {
        return;
    }

    if (useTextArea_ == useTextArea) {
        return;
    }
    useTextArea_ = useTextArea;
    constexpr A2UIVisibility VISIBLE = A2UIVisibility::VISIBLE;
    constexpr A2UIVisibility HIDDEN = A2UIVisibility::NONE;
    if (useTextArea_) {
        inputNode_ = textAreaNode_;
        if (textInputNode_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeVisibility(textInputNode_, HIDDEN);
        }
        if (textAreaNode_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeVisibility(textAreaNode_, VISIBLE);
            ArkUINodeApiAdapter::SetNodeTextAreaText(textAreaNode_, valueText_);
        }
    } else {
        inputNode_ = textInputNode_;
        if (textInputNode_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeVisibility(textInputNode_, VISIBLE);
            ArkUINodeApiAdapter::SetNodeTextInputText(textInputNode_, valueText_);
        }
        if (textAreaNode_ != nullptr) {
            ArkUINodeApiAdapter::SetNodeVisibility(textAreaNode_, HIDDEN);
        }
    }
}

void TextFieldComponent::SetTextInputType(A2UITextInputType inputType)
{
    if (textInputNode_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeTextInputType(textInputNode_, inputType);
}

void TextFieldComponent::SetVariant(const std::string& variant)
{
    auto resolvedVariant = ResolveVariant(variant);
    if (variant_ == resolvedVariant) {
        return;
    }
    variant_ = resolvedVariant;
    if (variant_ == "longText") {
        SetInputMode(true);
        return;
    }
    if (variant_ == "number") {
        SetInputMode(false);
        SetTextInputType(A2UITextInputType::NUMBER);
        return;
    }
    if (variant_ == "obscured") {
        SetInputMode(false);
        SetTextInputType(A2UITextInputType::PASSWORD);
        return;
    }
    SetInputMode(false);
    SetTextInputType(A2UITextInputType::NORMAL);
}

void TextFieldComponent::SetValidationRegexp(const std::string& regexp)
{
    if (validationRegexp_ == regexp) {
        return;
    }
    validationRegexp_ = regexp;
    hasReportedInvalidValidationRegexpError_ = false;
    ValidationRegexpCheck();
}

void TextFieldComponent::ParseChecks(const JsonValue& descriptor)
{
    if (checksEngine_ == nullptr) {
        return;
    }
    checksEngine_->ParseChecks(descriptor);
    for (const auto& path : checksEngine_->GetBindingPaths()) {
        AddCheckBindingPath(path);
    }
    ValidateChecks();
}

void TextFieldComponent::AddCheckBindingPath(const std::string& path)
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

bool TextFieldComponent::IsCheckBindingProperty(const std::string& property) const
{
    return property.rfind(CHECK_BINDING_PROPERTY_PREFIX, 0) == 0;
}

bool TextFieldComponent::ValidateChecks()
{
    latestValidationMessage_.clear();
    if (checksEngine_ == nullptr) {
        return true;
    }
    bool pass = checksEngine_->Validate(&latestValidationMessage_);
    isCheckPass_ = pass;
    SetErrorText(pass ? NO_ERROR_MESSAGE : latestValidationMessage_);
    return pass;
}

void TextFieldComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    auto textFieldTheme = GetTheme();
    if (textFieldTheme != nullptr) {
        SetFontColor(textFieldTheme->GetLabelFontColor(), textFieldTheme->GetErrorFontColor());
    }
    if (descriptor.Has("checks")) {
        SetPropertyFromDescriptor("checks", descriptor);
    } else {
        ParseChecks(descriptor);
    }
    ApplySchemaProperty("label", descriptor);
    ApplySchemaProperty("variant", descriptor);
    ApplySchemaProperty("validationRegexp", descriptor);
    ApplySchemaProperty("value", descriptor);
    valueBindingPath_ = ResolveValueBindingPath();
}

void TextFieldComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
    if (IsCheckBindingProperty(property)) {
        ValidateChecks();
    }
}

std::shared_ptr<TextFieldTheme> TextFieldComponent::GetTheme()
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
    theme = std::dynamic_pointer_cast<TextFieldTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void TextFieldComponent::SetFontColor(uint32_t labelFontColor, uint32_t errorFontColor)
{
    ArkUINodeApiAdapter::SetNodeFontColor(labelNode_, labelFontColor);

    ArkUINodeApiAdapter::SetNodeFontColor(errorNode_, errorFontColor);
}

void TextFieldComponent::OnConfigChange(const ThemeContext& context)
{
    auto textFieldTheme = GetTheme();
    if (textFieldTheme == nullptr) {
        return;
    }
    SetFontColor(textFieldTheme->GetLabelFontColor(), textFieldTheme->GetErrorFontColor());
}

} // namespace NativeModule
