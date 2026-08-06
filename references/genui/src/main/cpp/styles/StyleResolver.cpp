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

#include "StyleResolver.h"

#include "data/DynamicValueResolver.h"
#include "utils/LogA2UI.h"

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/ExpressionEngine.h"
#endif

#include "StyleApplyUtils.h"
#include "StyleParser.h"

namespace NativeModule {

namespace {

void AppendError(
    StyleResolveResult& result, StyleErrorCode code, const std::string& property, const std::string& message)
{
    result.errors.push_back({ .code = code, .property = property, .message = message });
}

const char* BoolToString(bool value)
{
    return value ? "true" : "false";
}

#ifdef ENABLE_EXPRESSION_ENGINE
constexpr const char DATA_MODEL_DEPENDENCY_NAME[] = "__dataModel";

void AppendExpressionBindingPlan(const StyleProperty& property, const std::string& expression,
    const std::vector<std::string>& dependencyNames, const std::string& dataPath, StyleResolveResult& result)
{
    StyleBindingPlan bindingPlan;
    bindingPlan.bindingProperty = StyleResolver::BuildStyleBindingProperty(property.rawName);
    bindingPlan.dataPath = dataPath;
    bindingPlan.property = property;
    bindingPlan.kind = StyleBindingKind::EXPRESSION;
    bindingPlan.expression = expression;
    bindingPlan.globalVarDeps = dependencyNames;
    result.bindings.push_back(std::move(bindingPlan));
}

void CollectExpressionBindingPlans(const StyleProperty& property, StyleResolveResult& result)
{
    if (!property.rawValue.IsString()) {
        return;
    }

    std::string expression = ExpressionEngine::ExtractExpression(property.rawValue.GetStringValue(""));
    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(property.rawValue);
    std::vector<std::string> dependencyNames = dependencies.globalVariables;
    if (!dependencies.dataPaths.empty()) {
        dependencyNames.push_back(DATA_MODEL_DEPENDENCY_NAME);
    }

    if (expression.empty() || dependencyNames.empty()) {
        return;
    }

    if (dependencies.dataPaths.empty()) {
        AppendExpressionBindingPlan(property, expression, dependencyNames, "", result);
        return;
    }

    for (const auto& dataPath : dependencies.dataPaths) {
        AppendExpressionBindingPlan(property, expression, dependencyNames, dataPath, result);
    }
}
#endif

bool HasDynamicDependencies(const DynamicValueDependencies& dependencies)
{
    return !dependencies.dataPaths.empty() || !dependencies.globalVariables.empty();
}

bool ContainsDynamicDescriptorValue(const JsonValue& value, int32_t depth = 0)
{
    constexpr int32_t maxDynamicDescriptorScanDepth = 16;
    if (!value.IsValid() || depth > maxDynamicDescriptorScanDepth) {
        return false;
    }
    if (value.IsObject()) {
        if (value.Has("path") || value.Has("call")) {
            return true;
        }
        for (JsonValue child = value.GetChild(); child.IsValid(); child = child.GetNext()) {
            if (ContainsDynamicDescriptorValue(child, depth + 1)) {
                return true;
            }
        }
        return false;
    }
    if (value.IsArray()) {
        int itemCount = value.GetArraySize();
        for (int index = 0; index < itemCount; ++index) {
            if (ContainsDynamicDescriptorValue(value.GetArrayItem(index), depth + 1)) {
                return true;
            }
        }
        return false;
    }
    return value.IsString() && StyleApplyUtils::IsExpressionString(value.GetStringValue(""));
}

} // namespace

StyleResolveResult StyleResolver::Resolve(const StyleParseResult& parseResult, const RenderContext& renderContext,
    const std::string& componentId, const std::set<std::string>& previousStyleKeys,
    const std::map<std::string, JsonValue>& localVariables)
{
    StyleResolveResult result;
    result.success = parseResult.success;
    result.errors = parseResult.errors;
    result.resolvedAdapter = JsonAdapter::CreateObject();
    if (result.resolvedAdapter == nullptr) {
        result.success = false;
        AppendError(result, StyleErrorCode::RESOLVE_FAILED, "", "failed to create resolved styles object");
        LOG_A2UI(LOG_ERROR, "StyleResolver::Resolve - create resolved styles object failed, componentId=%{public}s",
            componentId.c_str());
        return result;
    }
    result.resolvedStyles = result.resolvedAdapter->GetRoot();

    for (const auto& property : parseResult.properties) {
        if (!property.rawName.empty()) {
            result.currentStyleKeys.insert(property.rawName);
        }
    }

    BuildClearBindingPlan(result, previousStyleKeys);
    BuildResetPlan(result, previousStyleKeys);
    if (!parseResult.success) {
        LOG_A2UI(LOG_WARN, "StyleResolver::Resolve - parse failed, componentId=%{public}s, errorCount=%{public}zu",
            componentId.c_str(), result.errors.size());
        return result;
    }

    DynamicResolveContext dynamicContext = { .renderId = renderContext.renderId,
        .surfaceId = renderContext.surfaceId,
        .componentId = componentId,
        .dataModel = renderContext.dataModel,
        .allowExpression = true,
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE,
        .localVariables = localVariables };

    for (const auto& property : parseResult.properties) {
        if (!ResolveProperty(property, dynamicContext, result)) {
            result.success = false;
        }
    }

    LOG_A2UI(LOG_DEBUG,
        "StyleResolver::Resolve - completed, componentId=%{public}s, success=%{public}s, "
        "resolvedStyleCount=%{public}zu, bindingCount=%{public}zu, resetCount=%{public}zu, errorCount=%{public}zu",
        componentId.c_str(), BoolToString(result.success), result.currentStyleKeys.size(), result.bindings.size(),
        result.resetProperties.size(), result.errors.size());
    return result;
}

bool StyleResolver::IsStyleBindingProperty(const std::string& property)
{
    return property.size() > std::string(STYLE_BINDING_PREFIX).size() &&
           property.compare(0, std::string(STYLE_BINDING_PREFIX).size(), STYLE_BINDING_PREFIX) == 0;
}

std::string StyleResolver::ExtractStyleNameFromBindingProperty(const std::string& property)
{
    if (!IsStyleBindingProperty(property)) {
        return "";
    }
    return property.substr(std::string(STYLE_BINDING_PREFIX).size());
}

std::string StyleResolver::BuildStyleBindingProperty(const std::string& styleName)
{
    return std::string(STYLE_BINDING_PREFIX) + styleName;
}

void StyleResolver::BuildClearBindingPlan(StyleResolveResult& result, const std::set<std::string>& previousStyleKeys)
{
    std::set<std::string> bindingStyleKeys = result.currentStyleKeys;
    bindingStyleKeys.insert(previousStyleKeys.begin(), previousStyleKeys.end());

    for (const auto& styleName : bindingStyleKeys) {
        if (!styleName.empty()) {
            result.clearBindingProperties.push_back(BuildStyleBindingProperty(styleName));
        }
    }
}

void StyleResolver::BuildResetPlan(StyleResolveResult& result, const std::set<std::string>& previousStyleKeys)
{
    std::set<StylePropertyName> currentPropertyNames;
    for (const auto& property : result.currentStyleKeys) {
        currentPropertyNames.insert(StyleParser::ToPropertyName(property));
    }

    std::set<StylePropertyName> scheduledResetProperties;
    for (const auto& previousStyleKey : previousStyleKeys) {
        if (previousStyleKey.empty()) {
            continue;
        }

        StylePropertyName propertyName = StyleParser::ToPropertyName(previousStyleKey);
        if (propertyName == StylePropertyName::UNKNOWN) {
            continue;
        }
        if (currentPropertyNames.find(propertyName) != currentPropertyNames.end()) {
            continue;
        }
        if (!scheduledResetProperties.insert(propertyName).second) {
            continue;
        }

        result.resetProperties.push_back({ .rawName = previousStyleKey, .name = propertyName });
    }
}

bool StyleResolver::ResolveProperty(
    const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result)
{
    if (property.rawName.empty()) {
        return true;
    }

    if (property.kind == StyleValueKind::PATH_BINDING || property.kind == StyleValueKind::FUNCTION_CALL) {
        return ResolveDynamicProperty(property, dynamicContext, result);
    }

    if (property.kind == StyleValueKind::EXPRESSION) {
#ifdef ENABLE_EXPRESSION_ENGINE
        CollectExpressionBindingPlans(property, result);
#endif
        ResolvedValue resolved = DynamicValueResolver::Resolve(property.rawValue, dynamicContext);
        if (!resolved.success || !resolved.value.IsValid()) {
            AppendError(result, StyleErrorCode::RESOLVE_FAILED, property.rawName,
                resolved.errorMessage.empty() ? "expression style resolve failed" : resolved.errorMessage);
            LOG_A2UI(LOG_WARN,
                "StyleResolver::ResolveProperty: expression style resolve failed, style=%{public}s, reason=%{public}s",
                property.rawName.c_str(), resolved.errorMessage.c_str());
            return false;
        }
        bool putResult = PutResolvedValue(result, property, resolved.value);
        if (putResult) {
            result.dynamicallyResolvedStyleKeys.insert(property.rawName);
        }
        return putResult;
    }

    if (property.kind == StyleValueKind::COMPOSITE_OBJECT ||
        (property.kind == StyleValueKind::STATIC_VALUE && property.rawValue.IsObject())) {
        return ResolveDynamicDescriptorProperty(property, dynamicContext, result);
    }

    if (property.kind == StyleValueKind::INVALID) {
        AppendError(result, StyleErrorCode::UNSUPPORTED_VALUE_TYPE, property.rawName, "invalid style value");
        LOG_A2UI(LOG_WARN,
            "StyleResolver::ResolveProperty - invalid style value, componentId=%{public}s, style=%{public}s",
            dynamicContext.componentId.c_str(), property.rawName.c_str());
        return false;
    }

    bool putResult = PutResolvedValue(result, property, property.rawValue);
    LOG_A2UI(LOG_DEBUG,
        "StyleResolver::ResolveProperty - static resolve completed, componentId=%{public}s, style=%{public}s, "
        "result=%{public}s",
        dynamicContext.componentId.c_str(), property.rawName.c_str(), BoolToString(putResult));
    return putResult;
}

bool StyleResolver::ResolveDynamicProperty(
    const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result)
{
    if (property.kind == StyleValueKind::PATH_BINDING) {
        std::string path = property.rawValue.GetString("path", "");
        if (!path.empty()) {
            StyleBindingPlan bindingPlan;
            bindingPlan.bindingProperty = BuildStyleBindingProperty(property.rawName);
            bindingPlan.dataPath = path;
            bindingPlan.property = property;
            result.bindings.push_back(bindingPlan);
            LOG_A2UI(LOG_DEBUG,
                "StyleResolver::ResolveDynamicProperty - register style binding, componentId=%{public}s, "
                "style=%{public}s, bindingProperty=%{public}s, dataPath=%{public}s",
                dynamicContext.componentId.c_str(), property.rawName.c_str(), bindingPlan.bindingProperty.c_str(),
                path.c_str());
        } else {
            LOG_A2UI(LOG_WARN,
                "StyleResolver::ResolveDynamicProperty - empty binding path, componentId=%{public}s, style=%{public}s",
                dynamicContext.componentId.c_str(), property.rawName.c_str());
        }
    }

    ResolvedValue resolved = property.kind == StyleValueKind::FUNCTION_CALL
                                 ? DynamicValueResolver::ResolveRecursively(property.rawValue, dynamicContext)
                                 : DynamicValueResolver::Resolve(property.rawValue, dynamicContext);
    if (!resolved.success || !resolved.value.IsValid()) {
        AppendError(result, StyleErrorCode::RESOLVE_FAILED, property.rawName,
            resolved.errorMessage.empty() ? "dynamic style resolve failed" : resolved.errorMessage);
        LOG_A2UI(LOG_WARN,
            "StyleResolver::ResolveDynamicProperty: dynamic style resolve failed, "
            "style=%{public}s, reason=%{public}s",
            dynamicContext.componentId.c_str(), property.rawName.c_str());
        if (property.kind == StyleValueKind::FUNCTION_CALL && property.name != StylePropertyName::UNKNOWN) {
            result.resetProperties.push_back({ property.rawName, property.name });
        }
        return false;
    }

    if (property.kind == StyleValueKind::FUNCTION_CALL) {
        DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(property.rawValue);
        RegisterDescriptorBindings(property, dependencies, result);
    }
    bool putResult = PutResolvedValue(result, property, resolved.value);
    if (putResult) {
        result.dynamicallyResolvedStyleKeys.insert(property.rawName);
    }
    LOG_A2UI(LOG_DEBUG,
        "StyleResolver::ResolveDynamicProperty - resolved dynamic style, componentId=%{public}s, style=%{public}s, "
        "result=%{public}s, valueType=%{public}s",
        dynamicContext.componentId.c_str(), property.rawName.c_str(), BoolToString(putResult),
        resolved.value.GetTypeName());
    return putResult;
}

bool StyleResolver::ResolveDynamicDescriptorProperty(
    const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result)
{
    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(property.rawValue);
    RegisterDescriptorBindings(property, dependencies, result);

    ResolvedValue resolved = DynamicValueResolver::ResolveRecursivelyAllowPartial(property.rawValue, dynamicContext);
    if (!resolved.success || !resolved.value.IsValid()) {
        if (property.name == StylePropertyName::UNKNOWN && property.rawValue.IsObject() &&
            ContainsDynamicDescriptorValue(property.rawValue)) {
            bool putRawResult = PutResolvedValue(result, property, property.rawValue);
            if (putRawResult) {
                result.dynamicallyResolvedStyleKeys.insert(property.rawName);
            }
            return putRawResult;
        }
        AppendError(result, StyleErrorCode::RESOLVE_FAILED, property.rawName,
            resolved.errorMessage.empty() ? "dynamic descriptor style resolve failed" : resolved.errorMessage);
        LOG_A2UI(LOG_WARN,
            "StyleResolver::ResolveDynamicDescriptorProperty - resolve failed, componentId=%{public}s, "
            "style=%{public}s, reason=%{public}s",
            dynamicContext.componentId.c_str(), property.rawName.c_str(), resolved.errorMessage.c_str());
        if (property.name != StylePropertyName::UNKNOWN) {
            result.resetProperties.push_back({ property.rawName, property.name });
        }
        return false;
    }

    bool putResult = PutResolvedValue(result, property, resolved.value);
    if (putResult && (HasDynamicDependencies(dependencies) || ContainsDynamicDescriptorValue(property.rawValue))) {
        result.dynamicallyResolvedStyleKeys.insert(property.rawName);
    }
    LOG_A2UI(LOG_DEBUG,
        "StyleResolver::ResolveDynamicDescriptorProperty - resolved style descriptor, componentId=%{public}s, "
        "style=%{public}s, result=%{public}s, dataPathDeps=%{public}zu, globalDeps=%{public}zu",
        dynamicContext.componentId.c_str(), property.rawName.c_str(), BoolToString(putResult),
        dependencies.dataPaths.size(), dependencies.globalVariables.size());
    return putResult;
}

void StyleResolver::RegisterDescriptorBindings(
    const StyleProperty& property, const DynamicValueDependencies& dependencies, StyleResolveResult& result)
{
    if (property.rawName.empty() || !HasDynamicDependencies(dependencies)) {
        return;
    }

    auto appendBinding = [&property, &dependencies, &result](const std::string& dataPath) {
        StyleBindingPlan bindingPlan;
        bindingPlan.bindingProperty = BuildStyleBindingProperty(property.rawName);
        bindingPlan.dataPath = dataPath;
        bindingPlan.property = property;
        bindingPlan.kind = StyleBindingKind::FUNCTION_CALL;
        bindingPlan.functionCallDescriptor = property.rawValue;
        bindingPlan.globalVarDeps = dependencies.globalVariables;
        result.bindings.push_back(bindingPlan);
    };

    if (dependencies.dataPaths.empty()) {
        appendBinding("");
        return;
    }

    for (const auto& dataPath : dependencies.dataPaths) {
        appendBinding(dataPath);
    }
}

bool StyleResolver::PutResolvedValue(StyleResolveResult& result, const StyleProperty& property, const JsonValue& value)
{
    if (!result.resolvedStyles.IsObject() || property.rawName.empty() || !value.IsValid()) {
        LOG_A2UI(LOG_WARN,
            "StyleResolver::PutResolvedValue - skipped, style=%{public}s, resolvedStylesObject=%{public}s, "
            "valueValid=%{public}s",
            property.rawName.c_str(), BoolToString(result.resolvedStyles.IsObject()), BoolToString(value.IsValid()));
        return false;
    }
    if (!result.resolvedStyles.Put(property.rawName.c_str(), value)) {
        AppendError(result, StyleErrorCode::RESOLVE_FAILED, property.rawName, "failed to write resolved style");
        LOG_A2UI(LOG_WARN, "StyleResolver::PutResolvedValue - write resolved style failed, style=%{public}s",
            property.rawName.c_str());
        return false;
    }
    return true;
}

} // namespace NativeModule
