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

#include "SliderComponent.h"

#include "utils/LogA2UI.h"

#include "SliderTheme.h"

namespace NativeModule {
namespace {
constexpr char CHECK_BINDING_PROPERTY_PREFIX[] = "__checks_dep_";
constexpr float DEFAULT_MIN = 0.0f;
constexpr float DEFAULT_MAX = 100.0f;

float ClampValue(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}
} // namespace

SliderComponent::SliderComponent()
    : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::COLUMN)), textNode_(nullptr), sliderNode_(nullptr),
      minValue_(0.0f), maxValue_(100.0f), value_(0.0f), checksEngine_(std::make_unique<ChecksEngine>([this]() {
          return ChecksResolveContext { GetRenderId(), GetSurfaceId(), GetComponentId() };
      }))
{
    InitializeInternalNodes();
}

PropertyDeclaration SliderComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(SliderComponent&)>> declarations = {
        { "label",
            [](SliderComponent& sliderComponent) {
                return PropertyDeclaration { .name = "label",
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .fallbackString = "",
                    .applyValue = [&sliderComponent](
                                      const JsonValue& value) { sliderComponent.SetLabel(value.GetStringValue("")); } };
            } },
        { "value",
            [](SliderComponent& sliderComponent) {
                return PropertyDeclaration { .name = "value",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .fallbackNumber = 0.0,
                    .applyValue = [&sliderComponent](const JsonValue& value) {
                        sliderComponent.SetValue(static_cast<float>(value.GetNumberValue(0.0)));
                    } };
            } },
        { "min",
            [](SliderComponent& sliderComponent) {
                return PropertyDeclaration { .name = "min",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .fallbackNumber = 0.0,
                    .applyValue = [&sliderComponent](const JsonValue& value) {
                        sliderComponent.SetMinValue(static_cast<float>(value.GetNumberValue(0.0)));
                    } };
            } },
        { "max",
            [](SliderComponent& sliderComponent) {
                return PropertyDeclaration { .name = "max",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = true,
                    .fallbackNumber = 100.0,
                    .applyValue = [&sliderComponent](const JsonValue& value) {
                        sliderComponent.SetMaxValue(static_cast<float>(value.GetNumberValue(100.0)));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

bool SliderComponent::HandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName == "checks") {
        ValidateChecksSpecialProperty(value);
        ParseChecks(value);
        return true;
    }
    return false;
}

bool SliderComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    return A2UIComponent::IsKnownAdditionalDescriptorKey(propertyName) || propertyName == "checks";
}

std::vector<std::string> SliderComponent::GetComponentDirectRequiredPropertyKeys() const
{
    return { "max", "value" };
}

SliderComponent::~SliderComponent()
{
    if (internalNodesMounted_ && textNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, textNode_);
    }
    if (textNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(textNode_);
    }

    if (internalNodesMounted_ && sliderNode_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, sliderNode_);
    }
    if (sliderNode_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(sliderNode_);
    }
}

std::string SliderComponent::GetType() const
{
    return "Slider";
}

void SliderComponent::InitializeInternalNodes()
{
    if (!ArkUINodeApiAdapter::IsAvailable()) {
        return;
    }

    if (textNode_ == nullptr) {
        textNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::TEXT);
    }
    if (sliderNode_ == nullptr) {
        sliderNode_ = ArkUINodeApiAdapter::CreateNode(A2UINodeType::SLIDER);
    }
}

void SliderComponent::AttachNativeSubtree()
{
    if (internalNodesMounted_) {
        return;
    }
    if (!ArkUINodeApiAdapter::IsAvailable() || nativeView_ == nullptr || textNode_ == nullptr ||
        sliderNode_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::AddChild(nativeView_, textNode_);
    ArkUINodeApiAdapter::AddChild(nativeView_, sliderNode_);
    internalNodesMounted_ = true;
}

void SliderComponent::OnAttachToParent()
{
    AttachNativeSubtree();
}

void SliderComponent::SetLabel(const std::string& label)
{
    label_ = label;
    ArkUINodeApiAdapter::SetNodeTextContent(textNode_, label_);
}

void SliderComponent::SetMinValue(float min)
{
    minValue_ = min;
    ArkUINodeApiAdapter::SetNodeSliderMinValue(sliderNode_, minValue_);
}

void SliderComponent::SetMaxValue(float max)
{
    maxValue_ = max;
    ArkUINodeApiAdapter::SetNodeSliderMaxValue(sliderNode_, maxValue_);
}

void SliderComponent::SetValue(float value)
{
    value_ = value;
    ArkUINodeApiAdapter::SetNodeSliderValue(sliderNode_, value_);
}

void SliderComponent::SetStep(float step)
{
    ArkUINodeApiAdapter::SetNodeSliderStep(sliderNode_, step);
}

void SliderComponent::SetStyle(A2UISliderStyle style)
{
    ArkUINodeApiAdapter::SetNodeSliderStyle(sliderNode_, style);
}

void SliderComponent::SetSelectedColor(uint32_t color)
{
    ArkUINodeApiAdapter::SetNodeSliderSelectedColor(sliderNode_, color);
}

void SliderComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    LOG_A2UI(LOG_INFO, "SliderComponent::ApplyPrivateAttributes");

    if (descriptor.Has("checks")) {
        SetPropertyFromDescriptor("checks", descriptor);
    } else {
        ParseChecks(descriptor);
    }
    RefreshEnabledState();

    ApplySchemaProperty("label", descriptor);
    ApplySchemaProperty("min", descriptor);
    ApplySchemaProperty("max", descriptor);

    if (minValue_ >= maxValue_) {
        LOG_A2UI(LOG_WARN, "SliderComponent: min(%{public}f) >= max(%{public}f), using defaults", minValue_, maxValue_);
        SetMinValue(DEFAULT_MIN);
        SetMaxValue(DEFAULT_MAX);
    }

    ApplySchemaProperty("value", descriptor);
    SetValue(ClampValue(value_, minValue_, maxValue_));

    SetStep(1.0f);
    SetStyle(A2UISliderStyle::OUT_SET);
    auto sliderTheme = GetTheme();
    // 缁勪欢閫傞厤theme鏃堕渶瑕佷慨鏀?
    if (sliderTheme != nullptr) {
        SetSelectedColor(sliderTheme->GetSelectedColor());
    }

    ArkUINodeApiAdapter::SetNodeColumnAlignItems(nativeView_, A2UIHorizontalAlignment::CENTER);
}

void SliderComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
    if (IsCheckBindingProperty(property)) {
        RefreshEnabledState();
        return;
    }

    if (property == "min" || property == "max") {
        if (minValue_ >= maxValue_) {
            LOG_A2UI(
                LOG_WARN, "SliderComponent: min(%{public}f) >= max(%{public}f), using defaults", minValue_, maxValue_);
            SetMinValue(DEFAULT_MIN);
            SetMaxValue(DEFAULT_MAX);
        }
    }

    if (property == "value" || property == "min" || property == "max") {
        SetValue(ClampValue(value_, minValue_, maxValue_));
    }
}

void SliderComponent::SetEnabled(bool enabled)
{
    ArkUINodeApiAdapter::SetNodeEnabled(sliderNode_, enabled);
}

void SliderComponent::RefreshEnabledState()
{
    SetEnabled(ValidateChecks());
}

void SliderComponent::ParseChecks(const JsonValue& descriptor)
{
    if (checksEngine_ == nullptr) {
        return;
    }
    checksEngine_->ParseChecks(descriptor);
    for (const auto& path : checksEngine_->GetBindingPaths()) {
        AddCheckBindingPath(path);
    }
}

void SliderComponent::AddCheckBindingPath(const std::string& path)
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

bool SliderComponent::IsCheckBindingProperty(const std::string& property) const
{
    return property.rfind(CHECK_BINDING_PROPERTY_PREFIX, 0) == 0;
}

bool SliderComponent::ValidateChecks() const
{
    if (checksEngine_ == nullptr) {
        return true;
    }
    std::string message;
    bool pass = checksEngine_->Validate(&message);
    if (!pass) {
        LOG_A2UI(LOG_WARN, "Slider check failed, componentId=%{public}s, message=%{public}s", GetComponentId().c_str(),
            message.c_str());
    }
    return pass;
}

std::shared_ptr<SliderTheme> SliderComponent::GetTheme()
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
    theme = std::dynamic_pointer_cast<SliderTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void SliderComponent::OnConfigChange(const ThemeContext& context)
{
    auto sliderTheme = GetTheme();
    if (sliderTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
    SetSelectedColor(sliderTheme->GetSelectedColor());
}

} // namespace NativeModule
