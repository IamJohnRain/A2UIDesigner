#include "components/custom/CustomComponentExpressionBinding.h"

#include <algorithm>
#include <vector>

#include "components/custom/CustomComponent.h"

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/DependencyCollector.h"
#include "expression/ExpressionEngine.h"
#endif

namespace NativeModule {

namespace {

constexpr const char* CUSTOM_EXPRESSION_BINDING_PREFIX = "__a2uiExpr__:";

#ifdef ENABLE_EXPRESSION_ENGINE
std::vector<Dependency> CollectCustomExpressionDependencies(const std::string& expression)
{
    if (expression.empty()) {
        return {};
    }

    auto parseResult = ExpressionEngine::GetInstance().Parse(expression);
    if (!parseResult.success || parseResult.ast == nullptr) {
        return {};
    }

    DependencyCollector collector;
    return collector.Collect(parseResult.ast);
}

void AppendUniquePath(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) != values.end()) {
        return;
    }
    values.push_back(value);
}

void CollectCustomExpressionDataPaths(const JsonValue& value, std::vector<std::string>& dataPaths)
{
    if (!value.IsValid()) {
        return;
    }

    if (IsExpressionStringValue(value)) {
        std::string expression = ExpressionEngine::ExtractExpression(value.GetStringValue(""));
        for (const auto& dependency : CollectCustomExpressionDependencies(expression)) {
            if (dependency.variableName == "__dataModel" && !dependency.path.empty()) {
                AppendUniquePath(dataPaths, dependency.path);
            }
        }
        return;
    }

    if (value.IsObject()) {
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            CollectCustomExpressionDataPaths(child, dataPaths);
        }
        return;
    }

    if (!value.IsArray()) {
        return;
    }

    for (int32_t index = 0; index < value.GetArraySize(); index++) {
        CollectCustomExpressionDataPaths(value.GetArrayItem(index), dataPaths);
    }
}
#endif

} // namespace

bool IsExpressionStringValue(const JsonValue& value)
{
#ifdef ENABLE_EXPRESSION_ENGINE
    return value.IsString() && ExpressionEngine::IsExpression(value.GetStringValue(""));
#else
    (void)value;
    return false;
#endif
}

std::string BuildCustomExpressionBindingKey(const std::string& propertyName)
{
    if (propertyName.empty()) {
        return "";
    }
    return std::string(CUSTOM_EXPRESSION_BINDING_PREFIX) + propertyName;
}

bool IsCustomExpressionBindingProperty(const std::string& propertyName)
{
    return propertyName.rfind(CUSTOM_EXPRESSION_BINDING_PREFIX, 0) == 0;
}

std::string ResolveCustomExpressionSourceProperty(const std::string& propertyName)
{
    if (!IsCustomExpressionBindingProperty(propertyName)) {
        return propertyName;
    }
    return propertyName.substr(std::char_traits<char>::length(CUSTOM_EXPRESSION_BINDING_PREFIX));
}

void RefreshCustomExpressionBindings(
    CustomComponent& component, const std::string& propertyName, const JsonValue& value)
{
    std::string bindingKey = BuildCustomExpressionBindingKey(propertyName);
    if (bindingKey.empty()) {
        return;
    }

    component.RemoveBindingsForProperty(bindingKey);
#ifdef ENABLE_EXPRESSION_ENGINE
    std::vector<std::string> dataPaths;
    CollectCustomExpressionDataPaths(value, dataPaths);
    for (const std::string& path : dataPaths) {
        component.AddBinding(bindingKey, path);
    }
#else
    (void)value;
#endif
}

} // namespace NativeModule
