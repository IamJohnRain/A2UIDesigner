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

#include "IfComponent.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/DependencyCollector.h"
#include "expression/ExpressionEngine.h"
#endif

#include "adapter/ArkUINodeApiAdapter.h"
#include "theme/ThemeBase.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "RenderManager.h"
#include "SurfaceSlot.h"
#include "expression/EvaluationContext.h"

namespace NativeModule {

namespace {

constexpr char SCHEMA_ERROR_CODE_REQUIRED_MISS[] = "ERROR_CODE_REQUIRED_MISS";
constexpr char SCHEMA_ERROR_CODE_INVALID_VALUE[] = "ERROR_CODE_INVALID_VALUE";
constexpr char SCHEMA_ERROR_CODE_TYPE_MISMATCH[] = "ERROR_CODE_TYPE_MISMATCH";
constexpr char SCHEMA_ERROR_CODE_UNDEFINED_FIELD[] = "ERROR_CODE_UNDEFINED_FIELD";
constexpr char CONDITION_PROPERTY_NAME[] = "condition";

#ifdef ENABLE_EXPRESSION_ENGINE
std::vector<Dependency> CollectConditionDependencies(ExpressionEngine& engine, const std::string& expression)
{
    std::string dependencyExpression =
        engine.IsExpression(expression) ? engine.ExtractExpression(expression) : expression;
    auto parseResult = engine.Parse(dependencyExpression);
    if (!parseResult.success || parseResult.ast == nullptr) {
        return {};
    }

    DependencyCollector collector;
    return collector.Collect(parseResult.ast);
}

std::string ExtractConditionExpression(ExpressionEngine& engine, const std::string& expression)
{
    return engine.IsExpression(expression) ? engine.ExtractExpression(expression) : expression;
}

void AppendUnique(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    if (std::find(values.begin(), values.end(), value) != values.end()) {
        return;
    }
    values.push_back(value);
}
#endif

bool CountNativeSlotsBeforeTarget(
    const std::shared_ptr<Component>& node, const Component* target, size_t& nativeSlotCount)
{
    if (node == nullptr) {
        return false;
    }
    if (node.get() == target) {
        return true;
    }
    if (node->GetNativeView() != nullptr) {
        ++nativeSlotCount;
        return false;
    }

    for (const auto& child : node->GetChildren()) {
        if (CountNativeSlotsBeforeTarget(child, target, nativeSlotCount)) {
            return true;
        }
    }
    return false;
}

size_t CountPassthroughNativeSlots(const std::shared_ptr<Component>& node)
{
    if (node == nullptr) {
        return 0;
    }
    if (node->GetNativeView() != nullptr) {
        return 1;
    }

    size_t count = 0;
    for (const auto& child : node->GetChildren()) {
        count += CountPassthroughNativeSlots(child);
    }
    return count;
}

void CollectPassthroughNativeViews(const std::shared_ptr<Component>& node, std::vector<ArkUI_NodeHandle>& nativeViews)
{
    if (node == nullptr) {
        return;
    }
    ArkUI_NodeHandle nativeView = node->GetNativeView();
    if (nativeView != nullptr) {
        nativeViews.push_back(nativeView);
        return;
    }

    for (const auto& child : node->GetChildren()) {
        CollectPassthroughNativeViews(child, nativeViews);
    }
}

size_t ResolveBranchNativeInsertIndex(
    const std::list<std::shared_ptr<Component>>& children, const std::shared_ptr<Component>& target, size_t fallback)
{
    size_t nativeIndex = 0;
    for (const auto& child : children) {
        if (child == target) {
            return nativeIndex;
        }
        nativeIndex += CountPassthroughNativeSlots(child);
    }
    return fallback;
}

size_t ResolvePassthroughNativeInsertIndex(Component* ancestor, const Component* target, size_t branchIndex)
{
    if (ancestor == nullptr || target == nullptr) {
        return branchIndex;
    }

    size_t nativeSlotCount = 0;
    for (const auto& child : ancestor->GetChildren()) {
        if (CountNativeSlotsBeforeTarget(child, target, nativeSlotCount)) {
            return nativeSlotCount + branchIndex;
        }
    }
    return branchIndex;
}

bool ShouldReportInvalidFalsyConditionResult(const EvalResult& result)
{
    if (result.IsBoolean()) {
        return false;
    }
    if (result.IsNumber()) {
        return result.numberValue == 0.0 || std::isnan(result.numberValue);
    }
    if (result.IsString()) {
        return result.stringValue.empty();
    }
    return result.IsNull();
}

std::string NumberToConditionExpression(double value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (!std::isfinite(value)) {
        return value > 0.0 ? "Infinity" : "-Infinity";
    }
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return stream.str();
}

std::string TrimConditionExpression(const std::string& expression)
{
    size_t begin = 0;
    while (begin < expression.size() && std::isspace(static_cast<unsigned char>(expression[begin])) != 0) {
        ++begin;
    }
    size_t end = expression.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(expression[end - 1])) != 0) {
        --end;
    }
    return expression.substr(begin, end - begin);
}

std::string BuildConditionBindingExpression(const std::string& expression)
{
    std::string trimmed = TrimConditionExpression(expression);
    if (trimmed.empty()) {
        return "";
    }
#ifdef ENABLE_EXPRESSION_ENGINE
    if (ExpressionEngine::IsExpression(trimmed)) {
        return trimmed;
    }
#endif
    return "{{ " + trimmed + " }}";
}

} // namespace

IfComponent::IfComponent() : ExtendedComponent(nullptr, false, false) {}

IfComponent::~IfComponent() = default;

std::string IfComponent::GetType() const
{
    return "If";
}

bool IfComponent::CreateArkUINode()
{
    return true;
}

void IfComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    (void)descriptor;
    childListDescriptor_ = ChildListDescriptor();
}

void IfComponent::ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor)
{
    if (!normalizedDescriptor.IsObject()) {
        return;
    }

    std::string conditionStr;
    if (normalizedDescriptor.Has("condition")) {
        auto condValue = normalizedDescriptor.GetItem("condition");
        if (condValue.IsString()) {
            conditionStr = condValue.GetStringValue("");
            if (conditionStr.empty()) {
                ReportIfSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property condition is empty, fallback to childrenElse", "condition");
            }
        } else if (condValue.IsNumber()) {
            conditionStr = NumberToConditionExpression(condValue.GetNumberValue(0.0));
            ReportIfSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property condition expects string expression, number literal is coerced by truthiness", "condition");
        } else {
            LOG_A2UI(LOG_WARN,
                "IfComponent::ApplyComponentSpecificAttributes - condition is not string or number, "
                "componentId=%{public}s",
                GetComponentId().c_str());
            ReportIfSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property condition expects string expression or number literal, fallback to childrenElse",
                "condition");
        }
    } else {
        ReportIfSchemaWarning(
            SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property condition is required, fallback to childrenElse", "condition");
    }
    conditionExpression_ = conditionStr;

    childrenIfIds_ = ParseStringArray(
        normalizedDescriptor.Has("childrenIf") ? normalizedDescriptor.GetItem("childrenIf") : JsonValue(),
        "childrenIf");
    childrenElseIds_ = ParseStringArray(
        normalizedDescriptor.Has("childrenElse") ? normalizedDescriptor.GetItem("childrenElse") : JsonValue(),
        "childrenElse");

    bool evalSucceeded = false;
    bool result = EvaluateCondition(conditionExpression_, evalSucceeded, true);
    SyncConditionExpressionBinding(conditionExpression_);
    SelectBranch(result);
    initialized_ = true;
}

void IfComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    (void)styles;
    (void)applier;
}

void IfComponent::RegisterClickHandler() {}

void IfComponent::RegisterComponentSpecificListeners() {}

PropertyDeclaration IfComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName != CONDITION_PROPERTY_NAME) {
        return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
    }
    return PropertyDeclaration { .name = CONDITION_PROPERTY_NAME,
        .type = PropertyValueType::BOOLEAN,
        .allowDynamic = true,
        .allowExpression = true,
        .fallbackBool = false };
}

bool IfComponent::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    if (propertyName == "condition" || propertyName == "childrenIf" || propertyName == "childrenElse") {
        return true;
    }
    return ExtendedComponent::IsKnownAdditionalDescriptorKey(propertyName);
}

void IfComponent::OnConfigChange(const ThemeContext& context)
{
    lastThemeContext_ = context;
    themeContextValid_ = true;
    if (initialized_ && !conditionExpression_.empty()) {
        ReevaluateAndSwitch();
    }
}

void IfComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    (void)value;
    if (property == CONDITION_PROPERTY_NAME) {
        if (!initialized_ || conditionExpression_.empty()) {
            return;
        }
        ReevaluateAndSwitch();
        return;
    }

    if (!initialized_ || conditionExpression_.empty()) {
        return;
    }

    bool hasRelevantDependency = false;
    for (const auto& dep : dependencies_) {
        if (property == dep.variableName || property.find(dep.variableName + ".") == 0 || property == "$" + dep.path ||
            (dep.path != dep.variableName && property.find("$" + dep.path + ".") == 0)) {
            hasRelevantDependency = true;
            break;
        }
    }
    if (!hasRelevantDependency) {
        return;
    }

    ReevaluateAndSwitch();
}

void IfComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    auto ancestor = FindAncestorWithNativeView();
    if (ancestor == nullptr || ancestor->GetNativeView() == nullptr) {
        return;
    }
    std::vector<ArkUI_NodeHandle> nativeViews;
    CollectPassthroughNativeViews(child, nativeViews);
    if (nativeViews.empty()) {
        return;
    }
    size_t branchNativeIndex = ResolveBranchNativeInsertIndex(GetChildren(), child, index);
    size_t nativeIndex = ResolvePassthroughNativeInsertIndex(ancestor, this, branchNativeIndex);
    for (size_t offset = 0; offset < nativeViews.size(); ++offset) {
        ArkUINodeApiAdapter::InsertChildAt(
            ancestor->GetNativeView(), nativeViews[offset], static_cast<int32_t>(nativeIndex + offset));
    }
}

void IfComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    auto ancestor = FindAncestorWithNativeView();
    if (ancestor == nullptr || ancestor->GetNativeView() == nullptr) {
        return;
    }
    std::vector<ArkUI_NodeHandle> nativeViews;
    CollectPassthroughNativeViews(child, nativeViews);
    if (nativeViews.empty()) {
        return;
    }
    for (ArkUI_NodeHandle nativeView : nativeViews) {
        ArkUINodeApiAdapter::RemoveChild(ancestor->GetNativeView(), nativeView);
    }
}

void IfComponent::RemoveAllChildren()
{
    for (const auto& child : GetChildren()) {
        if (child == nullptr || child->GetNativeView() != nullptr) {
            continue;
        }
        OnRemoveChild(child);
    }
    Component::RemoveAllChildren();
}

bool IfComponent::ReevaluateAndSwitch()
{
    bool evalSucceeded = false;
    bool result = EvaluateCondition(conditionExpression_, evalSucceeded, false);
    if (!evalSucceeded && initialized_) {
        return currentBranch_;
    }
    if (initialized_ && result == currentBranch_) {
        return result;
    }
    SelectBranch(result);
    if (initialized_) {
        auto surfaceSlot = GetRuntimeSurfaceSlot();
        if (surfaceSlot != nullptr) {
            ReconcileBranchChildren(surfaceSlot->GetAllComponents());
        }
    }
    return result;
}

std::list<std::string> IfComponent::ParseStringArray(const JsonValue& value, const std::string& propertyName)
{
    std::list<std::string> result;
    if (!value.IsArray()) {
        if (value.IsValid()) {
            LOG_A2UI(LOG_WARN, "IfComponent::ParseStringArray - value is not array, componentId=%{public}s",
                GetComponentId().c_str());
            ReportIfSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + propertyName + " expects string array, fallback to empty array", propertyName);
        }
        return result;
    }
    size_t index = 0;
    for (JsonValue item = value.GetChild(); item.IsValid(); item = item.GetNext()) {
        if (item.IsString()) {
            result.push_back(item.GetStringValue(""));
        } else {
            LOG_A2UI(LOG_WARN, "IfComponent::ParseStringArray - array element is not string, componentId=%{public}s",
                GetComponentId().c_str());
            ReportIfSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property " + propertyName + " expects string item, skip invalid element",
                propertyName + "[" + std::to_string(index) + "]");
        }
        ++index;
    }
    return result;
}

bool IfComponent::EvaluateCondition(const std::string& expression, bool& evalSucceeded, bool isInitialEvaluation)
{
    evalSucceeded = false;

    if (expression.empty()) {
        return false;
    }

#ifdef ENABLE_EXPRESSION_ENGINE
    EvaluationContext context;
    InjectGlobalVariables(context);

    auto surfaceSlot = GetRuntimeSurfaceSlot();
    if (surfaceSlot != nullptr) {
        auto dataModel = surfaceSlot->GetOrCreateDataModel();
        if (dataModel != nullptr) {
            context.SetDataModel(dataModel.get());
        }
    } else if (GetRenderContext().dataModel != nullptr) {
        context.SetDataModel(GetRenderContext().dataModel.get());
    }

    auto& engine = ExpressionEngine::GetInstance();

    std::string evalExpr;
    if (engine.IsExpression(expression)) {
        evalExpr = expression;
    } else {
        evalExpr = "{{ " + expression + " }}";
    }
    EvalResult result = engine.Evaluate(evalExpr, context);
    dependencies_ = CollectConditionDependencies(engine, expression);

    if (result.IsUndefined()) {
        LOG_A2UI(LOG_WARN,
            "IfComponent::EvaluateCondition - expression evaluation returned undefined, componentId=%{public}s",
            GetComponentId().c_str());
        ReportIfSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            isInitialEvaluation ? "Property condition initial evaluation failed, fallback to childrenElse"
                                : "Property condition re-evaluation failed, keep current branch",
            "condition");
        return false;
    }
    if (ShouldReportInvalidFalsyConditionResult(result)) {
        ReportIfSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property condition evaluates to invalid falsy non-boolean value, fallback to childrenElse", "condition");
    }
    evalSucceeded = true;
    return result.AsBool();
#else
    LOG_A2UI(LOG_WARN, "IfComponent::EvaluateCondition - expression engine not enabled, componentId=%{public}s",
        GetComponentId().c_str());
    ReportIfSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
        isInitialEvaluation ? "Property condition initial evaluation failed, fallback to childrenElse"
                            : "Property condition re-evaluation failed, keep current branch",
        "condition");
    return false;
#endif
}

void IfComponent::SyncConditionExpressionBinding(const std::string& expression)
{
    RemoveBindingsForProperty(CONDITION_PROPERTY_NAME);
    if (BuildConditionBindingExpression(expression).empty()) {
        return;
    }

#ifdef ENABLE_EXPRESSION_ENGINE
    auto& engine = ExpressionEngine::GetInstance();
    std::string bindingExpression = ExtractConditionExpression(engine, expression);
    auto dependencies = CollectConditionDependencies(engine, expression);
    std::vector<std::string> dependencyNames;
    std::vector<std::string> dataModelPaths;
    for (const auto& dependency : dependencies) {
        AppendUnique(dependencyNames, dependency.variableName);
        if (dependency.variableName == "__dataModel" && !dependency.path.empty()) {
            AppendUnique(dataModelPaths, dependency.path);
        }
    }
    if (dependencyNames.empty()) {
        return;
    }
    if (dataModelPaths.empty()) {
        dataBindings_.emplace_back(CONDITION_PROPERTY_NAME, bindingExpression, dependencyNames);
        return;
    }
    for (const auto& dataPath : dataModelPaths) {
        DataBinding expressionBinding(CONDITION_PROPERTY_NAME, bindingExpression, dependencyNames);
        expressionBinding.dataPath_ = dataPath;
        dataBindings_.push_back(std::move(expressionBinding));
    }
#endif
}

void IfComponent::InjectGlobalVariables(EvaluationContext& context)
{
    const ThemeContext* themeContext = nullptr;
    if (themeContextValid_) {
        themeContext = &lastThemeContext_;
    } else {
        auto surfaceSlot = GetRuntimeSurfaceSlot();
        if (surfaceSlot != nullptr) {
            auto themeManager = surfaceSlot->GetThemeManager();
            if (themeManager != nullptr) {
                themeContext = &themeManager->GetContext();
            }
        }
        if (themeContext == nullptr) {
            auto theme = GetTheme();
            if (theme != nullptr) {
                themeContext = &theme->GetContext();
            }
        }
    }

    if (themeContext != nullptr) {
        context.SetThemeContext(themeContext);
    }
}

void IfComponent::SelectBranch(bool conditionResult)
{
    currentBranch_ = conditionResult;
    childListDescriptor_ = ChildListDescriptor();

    const auto& ids = conditionResult ? childrenIfIds_ : childrenElseIds_;
    if (ids.empty()) {
        return;
    }

    childListDescriptor_.type = ChildListType::STATIC_IDS;
    childListDescriptor_.staticChildIds = ids;
}

Component* IfComponent::FindAncestorWithNativeView()
{
    auto current = GetParent();
    while (current != nullptr) {
        if (current->GetNativeView() != nullptr) {
            return current.get();
        }
        current = current->GetParent();
    }
    return nullptr;
}

SurfaceSlot* IfComponent::GetRuntimeSurfaceSlot() const
{
    const RenderContext& context = GetRenderContext();
    if (context.renderId < 0 || context.surfaceId.empty()) {
        return nullptr;
    }
    return RenderManager::GetInstance().FindSurface(context.renderId, context.surfaceId);
}

void IfComponent::BuildBranchChildren(SurfaceSlot& surfaceSlot)
{
    if (!IsChildIdsUnchanged(childListDescriptor_.staticChildIds)) {
        ReportMissingBranchChildren(surfaceSlot.GetAllComponents());
    }
    BuildChildren(surfaceSlot);
}

void IfComponent::ReportMissingBranchChildren(const std::map<std::string, std::shared_ptr<Component>>& allComponents)
{
    const auto& childIds = childListDescriptor_.staticChildIds;
    for (const auto& childId : childIds) {
        auto childIt = allComponents.find(childId);
        if (childIt != allComponents.end() && childIt->second != nullptr) {
            continue;
        }
        ReportIfSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property " + std::string(currentBranch_ ? "childrenIf" : "childrenElse") +
                " references undefined component id, skip child",
            currentBranch_ ? "childrenIf" : "childrenElse");
    }
}

void IfComponent::ReconcileBranchChildren(std::map<std::string, std::shared_ptr<Component>>& allComponents)
{
    ReportMissingBranchChildren(allComponents);
    AttachStaticChildrenByIds(childListDescriptor_.staticChildIds, allComponents);
}

void IfComponent::ReportIfSchemaWarning(
    const std::string& code, const std::string& message, const std::string& propertyPath) const
{
    ReportExtendedSchemaWarning(code, message, propertyPath);
}

} // namespace NativeModule
