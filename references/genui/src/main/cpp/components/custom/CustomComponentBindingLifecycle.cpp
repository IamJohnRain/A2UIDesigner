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

#include "components/custom/CustomComponent.h"
#include "components/custom/CustomComponentExpressionBinding.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr const char* SIZE_KEY = "size";
constexpr const char* COLOR_KEY = "color";

} // namespace

void CustomComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    bool isCustomExpressionBinding = IsCustomExpressionBindingProperty(property);
    std::string sourceProperty = ResolveCustomExpressionSourceProperty(property);
    if (!isCustomExpressionBinding) {
        Component::OnDataUpdate(property, value);
    }
    bool isDynamicResolverBinding =
        dynamicResolverBindingKeys_.find(sourceProperty) != dynamicResolverBindingKeys_.end();

    if (descriptor_.type == "Icon" && (sourceProperty == SIZE_KEY || sourceProperty == COLOR_KEY)) {
        customPropertyNames_.insert(sourceProperty);
    }

    if (sourceProperty == "accessibility.label") {
        descriptor_.properties.accessibilityLabel = value.ToString("");
        descriptor_.properties.hasAccessibilityLabel = true;
        LOG_A2UI(LOG_INFO, "CustomComponent::OnDataUpdate: accessibility.label=%{public}s, type=%{public}s",
            descriptor_.properties.accessibilityLabel.c_str(), descriptor_.type.c_str());
    }
    if (sourceProperty == "accessibility.description") {
        descriptor_.properties.accessibilityDescription = value.ToString("");
        descriptor_.properties.hasAccessibilityDescription = true;
        LOG_A2UI(LOG_INFO, "CustomComponent::OnDataUpdate: accessibility.description=%{public}s, type=%{public}s",
            descriptor_.properties.accessibilityDescription.c_str(), descriptor_.type.c_str());
    }

    if (!isCustomExpressionBinding && !(isDynamicResolverBinding && !value.IsValid())) {
        JsonValue callbackValue = value.IsValid() ? GetCustomProperty(sourceProperty) : JsonValue();
        DispatchDynamicValueCallback(sourceProperty, callbackValue.IsValid() ? callbackValue : value);
    }
    bool shouldUpdateCustomComponent = isCustomExpressionBinding || !isDynamicResolverBinding ||
                                       customPropertyNames_.find(sourceProperty) != customPropertyNames_.end() ||
                                       sourceProperty == "accessibility.label" ||
                                       sourceProperty == "accessibility.description" ||
                                       sourceProperty.find("tabs[") == 0;

    descriptor_.customProps = BuildCustomProps();

    LOG_A2UI(LOG_INFO, "CustomComponent::OnDataUpdate: property=%{public}s, sourceProperty=%{public}s, type=%{public}s",
        property.c_str(), sourceProperty.c_str(), descriptor_.type.c_str());

    if (hasCreatedCustomComponent_ && shouldUpdateCustomComponent) {
        UpdateCustomComponent();
    }
}

void CustomComponent::OnPropertyRemoved(const std::string& propertyName)
{
    Component::OnPropertyRemoved(propertyName);
    properties_.erase(propertyName);
    rawDynamicProperties_.erase(propertyName);
    if (!IsCustomExpressionBindingProperty(propertyName)) {
        RemoveBindingsForProperty(BuildCustomExpressionBindingKey(propertyName));
    }

    if (propertyName == "accessibility.label") {
        descriptor_.properties.accessibilityLabel.clear();
        descriptor_.properties.hasAccessibilityLabel = false;
    }
    if (propertyName == "accessibility.description") {
        descriptor_.properties.accessibilityDescription.clear();
        descriptor_.properties.hasAccessibilityDescription = false;
    }
    if (propertyName == "styles" && flexShrinkStyleState_ != FlexShrinkStyleState::UNSPECIFIED) {
        flexShrinkStyleState_ = FlexShrinkStyleState::PARENT_DEFAULT;
        SyncFlexShrinkParentDefaultProperties();
    }
}

} // namespace NativeModule
