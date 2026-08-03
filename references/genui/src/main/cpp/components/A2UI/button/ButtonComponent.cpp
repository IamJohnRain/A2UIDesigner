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

#include "ButtonComponent.h"

#include <algorithm>
#include <cctype>

#include "components/actions/NativeActionRegistry.h"
#include "data/DynamicValueResolver.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/ActionParser.h"
#include "functions/EventContextResolver.h"
#include "functions/FunctionBridge.h"
#include "utils/LogA2UI.h"

#include "../text/TextComponent.h"
#include "ButtonTheme.h"
#include "RenderManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {
namespace {
constexpr char CHECK_BINDING_PROPERTY_PREFIX[] = "__checks_dep_";
constexpr char ICON_SIZE_PROPERTY[] = "size";
constexpr char ICON_COLOR_PROPERTY[] = "color";

const std::string& ResolveVariant(const std::string& variant)
{
    static const std::string defaultVariant = "default";
    if (variant == "primary" || variant == "borderless" || variant == "default") {
        return variant;
    }
    // Invalid token fallback.
    return defaultVariant;
}
} // namespace

ButtonComponent::ButtonComponent()
    : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::BUTTON), true, true),
      checksEngine_(std::make_unique<ChecksEngine>(
          [this]() { return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() }; }))
{}

PropertyDeclaration ButtonComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ButtonComponent&)>> declarations = {
        { "variant",
            [](ButtonComponent& buttonComponent) {
                return PropertyDeclaration { .name = "variant",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "default",
                    .enumAllowed = { "default", "primary", "borderless" },
                    .enumFallback = "default",
                    .applyValue = [&buttonComponent](const JsonValue& value) {
                        buttonComponent.SetVariant(value.GetStringValue("default"));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

bool ButtonComponent::HandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName == "action") {
        ValidateActionSpecialProperty(value);
        ActionParseContext parseContext = {
            .renderId = GetRenderId(), .surfaceId = GetSurfaceId(), .componentId = GetComponentId()
        };
        actionInfo_ = ActionParser::Parse(value, parseContext);
        return true;
    }
    if (propertyName == "checks") {
        ValidateChecksSpecialProperty(value);
        ParseChecks(value);
        return true;
    }
    return false;
}

bool ButtonComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "action" ||
           propertyName == "checks";
}

std::vector<std::string> ButtonComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "action" };
}

std::string ButtonComponent::GetType() const
{
    return "Button";
}

void ButtonComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
    if (IsCheckBindingProperty(property)) {
        RefreshEnabledState();
    }
}

void ButtonComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    actionInfo_ = nullptr;
    ApplySchemaProperty("action", descriptor);
    if (descriptor.Has("checks")) {
        SetPropertyFromDescriptor("checks", descriptor);
    } else {
        ParseChecks(descriptor);
    }
    RefreshEnabledState();

    if (actionInfo_ != nullptr && actionInfo_->IsValid()) {
        RegisterOnClick([this]() { DispatchAction(); });
    }

    ApplySchemaProperty("variant", descriptor);
}

void ButtonComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    if (child == nullptr || (child->GetType() != "Text" && child->GetType() != "Icon")) {
        return;
    }
    A2UIComponent::OnAddChild(child, index);
    childComponent_ = child;
    ApplyChildVisualStyle();
}

void ButtonComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    A2UIComponent::OnRemoveChild(child);
    std::shared_ptr<Component> currentChild = childComponent_.lock();
    if (currentChild != child) {
        return;
    }
    childComponent_.reset();
    if (child != nullptr && child->GetType() == "Text") {
        std::shared_ptr<TextComponent> childText = std::static_pointer_cast<TextComponent>(child);
        if (childText != nullptr) {
            childText->ResetFontColor();
        }
    }
}

void ButtonComponent::SetVariant(const std::string& variant)
{
    auto resolvedVariant = ResolveVariant(variant);
    if (variant_ == resolvedVariant) {
        return;
    }
    variant_ = resolvedVariant;
    auto buttonTheme = GetTheme();
    if (buttonTheme != nullptr) {
        SetBackgroundColor(buttonTheme->GetBackgroundColor(variant_));
    }
    ApplyChildVisualStyle();
}

void ButtonComponent::ApplyTextChildStyle(const std::shared_ptr<Component>& child)
{
    if (child == nullptr || child->GetType() != "Text") {
        return;
    }
    std::shared_ptr<TextComponent> childText = std::static_pointer_cast<TextComponent>(child);
    if (childText == nullptr) {
        return;
    }
    childText->SetFontWeight(ButtonTheme::GetFontWeight());
    auto buttonTheme = GetTheme();
    if (buttonTheme != nullptr) {
        childText->SetFontColor(buttonTheme->GetFontColor(variant_));
    }
}

void ButtonComponent::ApplyIconChildStyle(const std::shared_ptr<Component>& child)
{
    if (child == nullptr) {
        return;
    }

    std::unique_ptr<JsonAdapter> iconSize = JsonAdapter::CreateNumber(static_cast<double>(ButtonTheme::GetIconSize()));
    if (iconSize != nullptr) {
        child->OnDataUpdate(ICON_SIZE_PROPERTY, iconSize->GetRoot());
    }
    auto buttonTheme = GetTheme();
    if (buttonTheme == nullptr) {
        return;
    }
    std::unique_ptr<JsonAdapter> iconColor =
        JsonAdapter::CreateNumber(static_cast<double>(buttonTheme->GetIconColor(variant_)));
    if (iconColor != nullptr) {
        child->OnDataUpdate(ICON_COLOR_PROPERTY, iconColor->GetRoot());
    }
}

void ButtonComponent::SetButtonType(A2UIButtonType buttonType)
{
    ArkUINodeApiAdapter::SetNodeButtonType(nativeView_, buttonType);
}

void ButtonComponent::ApplyChildVisualStyle()
{
    std::shared_ptr<Component> child = childComponent_.lock();
    if (child == nullptr) {
        return;
    }
    if (child->GetType() == "Icon") {
        SetWidth(ButtonTheme::GetIconButtonSize());
        SetHeight(ButtonTheme::GetIconButtonSize());
        std::array<float, 4> padding = ButtonTheme::GetIconPadding();
        SetPadding(padding[0], padding[1], padding[2], padding[3]);
        SetButtonType(A2UIButtonType::CIRCLE);
        ApplyIconChildStyle(child);
    } else if (child->GetType() == "Text") {
        SetHeight(ButtonTheme::GetHeight());
        std::array<float, 4> padding = ButtonTheme::GetPadding();
        SetPadding(padding[0], padding[1], padding[2], padding[3]);
        SetButtonType(A2UIButtonType::CAPSULE);
        ArkUINodeApiAdapter::ResetNodeWidth(nativeView_);
        ApplyTextChildStyle(child);
    }
}

void ButtonComponent::SetEnabled(bool enabled)
{
    ArkUINodeApiAdapter::SetNodeEnabled(nativeView_, enabled);
}

void ButtonComponent::RefreshEnabledState()
{
    SetEnabled(ValidateChecks());
}

void ButtonComponent::ParseChecks(const JsonValue& descriptor)
{
    if (checksEngine_ == nullptr) {
        return;
    }
    checksEngine_->ParseChecks(descriptor);
    for (const auto& path : checksEngine_->GetBindingPaths()) {
        AddCheckBindingPath(path);
    }
}

void ButtonComponent::AddCheckBindingPath(const std::string& path)
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

bool ButtonComponent::IsCheckBindingProperty(const std::string& property) const
{
    return property.rfind(CHECK_BINDING_PROPERTY_PREFIX, 0) == 0;
}

bool ButtonComponent::ValidateChecks() const
{
    if (checksEngine_ == nullptr) {
        return true;
    }
    std::string message;
    bool pass = checksEngine_->Validate(&message);
    if (!pass) {
        LOG_A2UI(LOG_WARN, "Button check failed, componentId=%{public}s, message=%{public}s", GetComponentId().c_str(),
            message.c_str());
    }
    return pass;
}

void ButtonComponent::DispatchAction() const
{
    if (actionInfo_ == nullptr) {
        return;
    }
    if (actionInfo_->GetType() == ActionType::FUNCTION_CALL) {
        std::shared_ptr<FunctionCallInfo> functionCall = actionInfo_->GetFunctionCall();
        JsonValue functionCallDescriptor = actionInfo_->GetFunctionCallDescriptor();
        if (functionCallDescriptor.IsValid()) {
            SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(GetRenderId(), GetSurfaceId());
            DynamicResolveContext context = { .renderId = GetRenderId(),
                .surfaceId = GetSurfaceId(),
                .componentId = GetComponentId(),
                .dataModel = surface != nullptr ? surface->GetOrCreateDataModel() : nullptr,
                .allowExpression = true,
                .localVariables = GetLocalVariables() };
            std::shared_ptr<FunctionCallInfo> resolvedFunctionCall =
                DynamicValueResolver::ResolveFunctionCallDescriptor(functionCallDescriptor, context);
            if (resolvedFunctionCall != nullptr) {
                functionCall = resolvedFunctionCall;
            } else {
                LOG_A2UI(LOG_WARN,
                    "ButtonComponent::DispatchAction: resolve functionCall descriptor failed, fallback to raw args");
            }
        }
        if (functionCall == nullptr) {
            LOG_A2UI(LOG_WARN, "ButtonComponent::DispatchAction: functionCall is null");
            return;
        }
        if (NativeActionRegistry::GetInstance().HasAction(functionCall->GetFunctionName())) {
            SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(GetRenderId(), GetSurfaceId());
            EventHandlerChainExecutor::ExecutionContext nativeActionContext;
            nativeActionContext.renderId = GetRenderId();
            nativeActionContext.surfaceId = GetSurfaceId();
            nativeActionContext.componentId = GetComponentId();
            nativeActionContext.dataModel = surface != nullptr ? surface->GetOrCreateDataModel() : nullptr;
            nativeActionContext.localVariables = GetLocalVariables();
            NativeActionRegistry::GetInstance().Execute(
                functionCall->GetFunctionName(), functionCall->GetArgs(), nativeActionContext);
            return;
        }
        FunctionBridge::GetInstance().Invoke(GetRenderId(), GetSurfaceId(), GetComponentId(), functionCall);
        return;
    }
    if (actionInfo_->GetType() == ActionType::EVENT) {
        EventResolveContext context = {
            .renderId = GetRenderId(), .surfaceId = GetSurfaceId(), .componentId = GetComponentId()
        };
        JsonValue resolvedContext = EventContextResolver::Resolve(actionInfo_->GetEventContextDescriptor(), context);
        ActionDispatchBridge::GetInstance().Dispatch(
            GetRenderId(), GetSurfaceId(), GetComponentId(), actionInfo_->GetEventName(), resolvedContext);
    }
}

std::shared_ptr<ButtonTheme> ButtonComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    theme = std::dynamic_pointer_cast<ButtonTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    } else {
        LOG_A2UI(LOG_WARN, "ButtonComponent::GetTheme: ButtonTheme is null, componentId=%{public}s",
            GetComponentId().c_str());
    }

    return theme;
}

void ButtonComponent::OnConfigChange(const ThemeContext& context)
{
    auto buttonTheme = GetTheme();
    if (buttonTheme == nullptr) {
        return;
    }
    SetBackgroundColor(buttonTheme->GetBackgroundColor(variant_));
    ApplyChildVisualStyle();
}

void ButtonComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChild(descriptor.GetItem("child"));
}
} // namespace NativeModule
