#ifndef A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H
#define A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H

#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

class CustomComponent;

bool IsExpressionStringValue(const JsonValue& value);
std::string BuildCustomExpressionBindingKey(const std::string& propertyName);
bool IsCustomExpressionBindingProperty(const std::string& propertyName);
std::string ResolveCustomExpressionSourceProperty(const std::string& propertyName);
void RefreshCustomExpressionBindings(
    CustomComponent& component, const std::string& propertyName, const JsonValue& value);

} // namespace NativeModule

#endif // A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H
