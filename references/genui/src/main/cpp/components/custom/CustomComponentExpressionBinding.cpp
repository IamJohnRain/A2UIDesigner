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

#include "components/custom/CustomComponentExpressionBinding.h"

#include <algorithm>
#include <vector>

#include "components/custom/CustomComponent.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

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

JsonValue CustomComponent::CloneExpressionValue(const JsonValue& node) const
{
    std::unique_ptr<JsonAdapter> cloned = JsonAdapter::Clone(node);
    return cloned != nullptr ? cloned->GetRoot() : JsonValue();
}

JsonValue CustomComponent::ResolveExpressionsInObject(const JsonValue& node, const std::string& path) const
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return JsonValue();
    }

    JsonValue result = adapter->GetRoot();
    for (JsonValue child = node.GetChild(); child.IsValid(); child = child.GetNext()) {
        JsonValue resolvedChild = ResolveExpressionsInValue(child, path + "." + child.GetKey());
        if (resolvedChild.IsValid()) {
            result.Put(child.GetKey().c_str(), resolvedChild);
        }
    }
    return result;
}

JsonValue CustomComponent::ResolveExpressionsInArray(const JsonValue& node, const std::string& path) const
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateArray();
    if (adapter == nullptr) {
        return JsonValue();
    }

    JsonValue result = adapter->GetRoot();
    for (int index = 0; index < node.GetArraySize(); ++index) {
        JsonValue resolvedItem =
            ResolveExpressionsInValue(node.GetArrayItem(index), path + "[" + std::to_string(index) + "]");
        if (resolvedItem.IsValid()) {
            result.Append(resolvedItem);
        }
    }
    return result;
}

JsonValue CustomComponent::ResolveExpressionsInValue(const JsonValue& node, const std::string& path) const
{
    if (!node.IsValid()) {
        return JsonValue();
    }
#ifdef ENABLE_EXPRESSION_ENGINE
    if (node.IsString()) {
        std::string raw = node.GetStringValue("");
        if (ExpressionEngine::IsExpression(raw)) {
            JsonValue resolved = EvaluateCustomExpression(raw);
            if (resolved.IsValid()) {
                return resolved;
            }
            LOG_A2UI(LOG_WARN,
                "CustomComponent: expression resolve failed, raw value kept, type=%{public}s, path=%{public}s",
                descriptor_.type.c_str(), path.c_str());
        }
        return CloneExpressionValue(node);
    }
    if (node.IsObject()) {
        return ResolveExpressionsInObject(node, path);
    }
    if (node.IsArray()) {
        return ResolveExpressionsInArray(node, path);
    }
#endif
    return CloneExpressionValue(node);
}

} // namespace NativeModule
