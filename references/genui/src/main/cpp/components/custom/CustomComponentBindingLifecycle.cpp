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

    if (!isCustomExpressionBinding) {
        JsonValue callbackValue = GetCustomProperty(sourceProperty);
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
}

} // namespace NativeModule
