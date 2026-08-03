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

#include "Component.h"

#include <algorithm>
#include <cmath>

#include "composition/ChildListParser.h"
#include "composition/TemplateAdapterNode.h"
#include "composition/TemplateInstantiator.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "data/PathValidator.h"
#include "data/ResolvedValue.h"
#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#ifdef ENABLE_EXPRESSION_ENGINE
#include "expression/DependencyCollector.h"
#include "expression/EvaluationContext.h"
#include "expression/ExpressionEngine.h"
#endif

#include "../SchemaErrorCodes.h"
#include "../SurfaceErrorCodes.h"
#include "../SurfaceSlot.h"
#include "NativeComponentFactory.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {
constexpr char ACCESSIBILITY_LABEL_PROPERTY[] = "accessibility.label";
constexpr char ACCESSIBILITY_DESCRIPTION_PROPERTY[] = "accessibility.description";

struct LocalVariableStore {
    std::vector<std::unique_ptr<JsonAdapter>> adapters;
    std::map<std::string, JsonValue> variables;
};

thread_local std::map<std::string, LocalVariableStore> g_pendingLocalVariables;

struct NormalizationIssue {
    bool hasWarning = false;
    std::string code;
    std::string message;
};

bool IsEnumValueAllowed(const std::string& value, const std::vector<std::string>& enumAllowed)
{
    return std::find(enumAllowed.begin(), enumAllowed.end(), value) != enumAllowed.end();
}

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

bool CloneLocalVariableStore(const std::map<std::string, JsonValue>& input, LocalVariableStore& output)
{
    output.adapters.clear();
    output.variables.clear();
    for (const auto& [name, value] : input) {
        if (name.empty() || !value.IsValid()) {
            continue;
        }
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(value);
        if (adapter == nullptr) {
            continue;
        }
        JsonValue clonedValue = adapter->GetRoot();
        if (!clonedValue.IsValid()) {
            continue;
        }
        output.variables[name] = clonedValue;
        output.adapters.push_back(std::move(adapter));
    }
    return !output.variables.empty();
}

void ConsumePendingLocalVariables(Component& component)
{
    auto iter = g_pendingLocalVariables.find(component.GetComponentId());
    if (iter == g_pendingLocalVariables.end()) {
        return;
    }
    component.SetLocalVariables(iter->second.variables);
    g_pendingLocalVariables.erase(iter);
}

JsonValue BuildStringValue(const std::string& value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue BuildNumberValue(double value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue BuildBoolValue(bool value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateBool(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

std::map<std::string, JsonValue> BuildTemplateLocalVariables(
    const ChildListDescriptor& childList, const JsonValue& itemValue, int32_t itemIndex)
{
    std::map<std::string, JsonValue> localVariables;
    if (itemValue.IsValid() && !childList.resolvedItemVarName.empty()) {
        localVariables[childList.resolvedItemVarName] = itemValue;
    }
    if (!childList.resolvedIndexVarName.empty()) {
        JsonValue indexValue = BuildNumberValue(static_cast<double>(itemIndex));
        if (indexValue.IsValid()) {
            localVariables[childList.resolvedIndexVarName] = indexValue;
        }
    }
    return localVariables;
}

std::map<std::string, JsonValue> MergeLocalVariables(
    const std::map<std::string, JsonValue>& outerVariables, const std::map<std::string, JsonValue>& innerVariables)
{
    std::map<std::string, JsonValue> mergedVariables = outerVariables;
    for (const auto& [name, value] : innerVariables) {
        if (!name.empty() && value.IsValid()) {
            mergedVariables[name] = value;
        }
    }
    return mergedVariables;
}

#ifdef ENABLE_EXPRESSION_ENGINE
void SetupExpressionEvaluationContext(
    EvaluationContext& evalContext, int32_t renderId, const std::string& surfaceId, const std::string& componentId)
{
    evalContext.SetRenderId(renderId);
    evalContext.SetSurfaceId(surfaceId);
    evalContext.SetComponentId(componentId);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        return;
    }

    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return;
    }
    evalContext.SetThemeContext(&surfaceManager->GetThemeContext());

    if (surfaceId.empty()) {
        return;
    }

    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface(surfaceId);
    if (surfaceSlot == nullptr || surfaceSlot->GetBindingEngine() == nullptr) {
        return;
    }

    std::shared_ptr<DataModel> dataModel = surfaceSlot->GetBindingEngine()->GetOrCreateDataModel(surfaceId);
    if (dataModel != nullptr) {
        evalContext.SetDataModel(dataModel.get());
    }
}

bool HasExpressionContextError(const EvaluationContext& evalContext)
{
    return evalContext.lastError != ExpressionError::NONE && !evalContext.errorMessage.empty();
}

bool IsIllegalExpressionContextError(ExpressionError error)
{
    return error == ExpressionError::PARSE_UNEXPECTED_TOKEN;
}

bool IsSoftExpressionContextError(ExpressionError error)
{
    return error == ExpressionError::EVAL_PATH_NOT_FOUND || error == ExpressionError::EVAL_NO_GLOBAL_VARIABLE ||
           error == ExpressionError::EVAL_UNDEFINED_VARIABLE || IsIllegalExpressionContextError(error);
}

int32_t ResolveExpressionRuntimeErrorCode(ExpressionError error)
{
    if (error == ExpressionError::EVAL_NO_GLOBAL_VARIABLE) {
        return SURFACE_ERROR_GLOBAL_VARIABLE_NOT_FOUND;
    }
    if (IsIllegalExpressionContextError(error)) {
        return SURFACE_ERROR_ILLEGAL_EXPRESSION;
    }
    return SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED;
}

std::string ResolveExpressionRuntimeErrorMessage(const EvaluationContext& evalContext)
{
    if (IsIllegalExpressionContextError(evalContext.lastError) &&
        evalContext.errorMessage.rfind("illegal expression", 0) != 0) {
        return "illegal expression: " + evalContext.errorMessage;
    }
    return evalContext.errorMessage;
}

void DispatchExpressionRuntimeError(int32_t renderId, const std::string& surfaceId, const std::string& componentId,
    const EvaluationContext& evalContext)
{
    if (renderId < 0 || !HasExpressionContextError(evalContext)) {
        return;
    }
    RuntimeErrorDispatchBridge::GetInstance().Dispatch(renderId, surfaceId, componentId,
        ResolveExpressionRuntimeErrorCode(evalContext.lastError), ResolveExpressionRuntimeErrorMessage(evalContext),
        "Component");
}

JsonValue EvaluateExpressionBindingValue(const std::string& expression, int32_t renderId, const std::string& surfaceId,
    const std::string& componentId, const std::map<std::string, JsonValue>& localVariables)
{
    if (expression.empty()) {
        return JsonValue();
    }

    EvaluationContext evalContext;
    SetupExpressionEvaluationContext(evalContext, renderId, surfaceId, componentId);
    if (!localVariables.empty()) {
        evalContext.PushScope();
        for (const auto& [name, value] : localVariables) {
            if (!name.empty() && value.IsValid()) {
                evalContext.SetLocalVariable(name, EvalResult::FromJson(value));
            }
        }
    }
    JsonValue resolvedValue =
        ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ " + expression + " }}", evalContext);
    if (HasExpressionContextError(evalContext) &&
        (IsSoftExpressionContextError(evalContext.lastError) || !resolvedValue.IsValid())) {
        DispatchExpressionRuntimeError(renderId, surfaceId, componentId, evalContext);
    }
    return resolvedValue;
}

std::vector<Dependency> CollectExpressionDependencies(const std::string& expression)
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

void AppendUniqueString(std::vector<std::string>& values, const std::string& value)
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

bool NormalizePropertyValue(const PropertyDeclaration& declaration, const JsonValue& input, JsonValue& normalizedValue,
    NormalizationIssue* issue = nullptr)
{
    if (issue != nullptr) {
        issue->hasWarning = false;
        issue->code.clear();
        issue->message.clear();
    }
    switch (declaration.type) {
        case PropertyValueType::STRING:
            if (input.IsString()) {
                normalizedValue = input;
                return true;
            }
            if (declaration.acceptNumberForString && input.IsNumber()) {
                normalizedValue = input;
                return true;
            }
            if (input.IsValid() && issue != nullptr) {
                issue->hasWarning = true;
                issue->code = SCHEMA_ERROR_CODE_TYPE_MISMATCH;
                issue->message = "Property " + declaration.name + " expects string value, got type '" +
                                 input.GetTypeName() + "', value has been coerced to string";
            }
            normalizedValue = BuildStringValue(input.ToString(declaration.fallbackString));
            return normalizedValue.IsValid();
        case PropertyValueType::NUMBER:
            if (input.IsNumber()) {
                normalizedValue = input;
                return true;
            }
            if (input.IsValid() && issue != nullptr) {
                issue->hasWarning = true;
                issue->code = SCHEMA_ERROR_CODE_TYPE_MISMATCH;
                issue->message = "Property " + declaration.name + " expects number value, got type '" +
                                 input.GetTypeName() + "', compatibility normalization has been applied";
            }
            normalizedValue = BuildNumberValue(input.ToNumber(declaration.fallbackNumber));
            return normalizedValue.IsValid();
        case PropertyValueType::BOOLEAN:
            if (input.IsBool()) {
                normalizedValue = input;
                return true;
            }
            if (input.IsValid() && issue != nullptr) {
                issue->hasWarning = true;
                issue->code = SCHEMA_ERROR_CODE_TYPE_MISMATCH;
                issue->message = "Property " + declaration.name + " expects boolean value, got type '" +
                                 input.GetTypeName() + "', compatibility normalization has been applied";
            }
            normalizedValue = BuildBoolValue(input.ToBool(declaration.fallbackBool));
            return normalizedValue.IsValid();
        case PropertyValueType::ENUM_STRING: {
            std::string fallback =
                declaration.enumFallback.empty() ? declaration.fallbackString : declaration.enumFallback;
            if (input.IsString()) {
                std::string candidate = input.GetStringValue(fallback);
                if (IsEnumValueAllowed(candidate, declaration.enumAllowed)) {
                    normalizedValue = input;
                    return true;
                }
                if (issue != nullptr) {
                    issue->hasWarning = true;
                    issue->code = SCHEMA_ERROR_CODE_INVALID_VALUE;
                    issue->message = "Property " + declaration.name + " got invalid enum value '" + candidate +
                                     "', fallback to '" + fallback + "'";
                }
                LOG_A2UI(LOG_WARN,
                    "ApplyRuntimeProperty: enum property got invalid value '%{public}s', fallback '%{public}s'",
                    candidate.c_str(), fallback.c_str());
            } else if (input.IsValid()) {
                std::string actualType = input.GetTypeName();
                if (issue != nullptr) {
                    issue->hasWarning = true;
                    issue->code = SCHEMA_ERROR_CODE_TYPE_MISMATCH;
                    issue->message = "Property " + declaration.name + " expects string enum value, got type '" +
                                     actualType + "', fallback to '" + fallback + "'";
                }
                LOG_A2UI(LOG_WARN,
                    "ApplyRuntimeProperty: enum property expects string value, got type '%{public}s', fallback "
                    "'%{public}s'",
                    actualType.c_str(), fallback.c_str());
            }
            std::string candidate = input.ToString(fallback);
            if (!IsEnumValueAllowed(candidate, declaration.enumAllowed)) {
                candidate = fallback;
            }
            normalizedValue = BuildStringValue(candidate);
            return normalizedValue.IsValid();
        }
        case PropertyValueType::OBJECT:
            if (!input.IsObject()) {
                return false;
            }
            return CloneJsonValue(input, normalizedValue);
        default:
            return CloneJsonValue(input, normalizedValue);
    }
}

} // namespace

Component::Component(ArkUI_NodeHandle nativeView, bool ownsNativeView, bool isCompositeType)
    : nativeView_(nativeView), ownsNativeView_(ownsNativeView), isCompositeType_(isCompositeType)
{
    if (isCompositeType_) {
        ArkUINodeApiAdapter::SetNodeAccessibilityGroup(nativeView_, true);
    }
}

Component::~Component()
{
    ClearChildren();
    if (ownsNativeView_ && nativeView_ != nullptr) {
        ArkUINodeApiAdapter::DisposeNode(nativeView_);
        nativeView_ = nullptr;
    }
}

bool Component::HasChild(const std::shared_ptr<Component>& child) const
{
    for (const auto& c : children_) {
        if (c == child) {
            return true;
        }
    }
    return false;
}

void Component::AddChild(const std::shared_ptr<Component>& child)
{
    AddChildAt(child, children_.size());
}

size_t Component::ResolveDesiredChildOrder(const std::shared_ptr<Component>& child, size_t fallbackOrder) const
{
    if (child == nullptr) {
        return fallbackOrder;
    }
    auto orderIt = desiredChildOrder_.find(child);
    if (orderIt != desiredChildOrder_.end()) {
        return orderIt->second;
    }
    return fallbackOrder;
}

std::pair<std::list<std::shared_ptr<Component>>::iterator, size_t> Component::ResolveInsertPositionByDesiredOrder(
    size_t desiredOrder, std::list<std::shared_ptr<Component>>::iterator excludeIt)
{
    auto targetIt = children_.end();
    size_t targetIndex = 0;
    size_t fallbackOrder = 0;
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it == excludeIt) {
            continue;
        }
        size_t siblingOrder = ResolveDesiredChildOrder(*it, fallbackOrder);
        ++fallbackOrder;
        if (siblingOrder > desiredOrder) {
            targetIt = it;
            break;
        }
        ++targetIndex;
    }
    return { targetIt, targetIndex };
}

void Component::AddChildAt(const std::shared_ptr<Component>& child, size_t index)
{
    if (child == nullptr) {
        return;
    }

    std::shared_ptr<Component> mountedParent = child->GetParent();
    if (mountedParent != nullptr && mountedParent.get() != this) {
        LOG_A2UI(LOG_WARN,
            "AddChildAt: reject double mount, childId=%{public}s, sourceParentId=%{public}s, targetParentId=%{public}s",
            child->GetComponentId().c_str(), mountedParent->GetComponentId().c_str(), componentId_.c_str());
        return;
    }

    child->SetParentId(componentId_);
    child->SetParent(shared_from_this());

    desiredChildOrder_[child] = index;
    auto existingIt = std::find(children_.begin(), children_.end(), child);
    if (existingIt == children_.end()) {
        auto [insertIt, targetIndex] = ResolveInsertPositionByDesiredOrder(index, children_.end());
        children_.insert(insertIt, child);
        OnAddChild(child, targetIndex);
        child->OnAttachToParent();
        return;
    }

    size_t currentIndex = static_cast<size_t>(std::distance(children_.begin(), existingIt));
    auto [targetIt, targetIndex] = ResolveInsertPositionByDesiredOrder(index, existingIt);
    if (currentIndex == targetIndex) {
        return;
    }

    children_.splice(targetIt, children_, existingIt);
    OnMoveChild(child, currentIndex, targetIndex);
}

void Component::ClearChildren()
{
    if (children_.empty()) {
        return;
    }

    for (const auto& child : children_) {
        if (child == nullptr || child->GetNativeView() == nullptr) {
            continue;
        }
        OnRemoveChild(child);
    }
    RemoveAllChildren();
}

void Component::RemoveAllChildren()
{
    children_.clear();
    desiredChildOrder_.clear();
}

void Component::ApplyDescriptor(const JsonValue& descriptor)
{
    SetComponentId(descriptor.GetString("id", componentId_));
    ConsumePendingLocalVariables(*this);
    if (ShouldValidateUnknownDescriptorFields()) {
        ValidateUnknownDescriptorFields(descriptor);
    }
    ValidateComponentDirectRequiredProperties(descriptor);
    ValidateComponentDescriptorSchema(descriptor);
    ApplyCommonAttributes(descriptor);
    ApplyPrivateAttributes(descriptor);
    CollectChildListDescriptor(descriptor);
}

void Component::ApplyParentsRelations(SurfaceSlot* surfaceSlot)
{
    if (surfaceSlot != nullptr && !componentId_.empty()) {
        std::map<std::string, std::string>& parentsRelations = surfaceSlot->GetParentsRelations();
        for (const auto& childId : childListDescriptor_.staticChildIds) {
            if (!childId.empty()) {
                parentsRelations[childId] = componentId_;
            }
        }
    }
}

void Component::SetComponentId(const std::string& componentId)
{
    if (componentId_ == componentId) {
        return;
    }
    componentId_ = componentId;
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeId(nativeView_, componentId_);
}

const std::string& Component::GetComponentId() const
{
    return componentId_;
}

void Component::SetSurfaceId(const std::string& surfaceId)
{
    surfaceId_ = surfaceId;
}

const std::string& Component::GetSurfaceId() const
{
    return surfaceId_;
}

void Component::SetRenderId(int32_t renderId)
{
    renderId_ = renderId;
}

int32_t Component::GetRenderId() const
{
    return renderId_;
}

void Component::SetSurfaceContext(const SurfaceContext& surfaceContext)
{
    surfaceContext_ = surfaceContext;
}

const SurfaceContext& Component::GetSurfaceContext() const
{
    return surfaceContext_;
}

void Component::SetLocalVariables(const std::map<std::string, JsonValue>& localVariables)
{
    LocalVariableStore store;
    if (!CloneLocalVariableStore(localVariables, store)) {
        localVariables_.clear();
        localVariableAdapters_.clear();
        return;
    }
    localVariables_ = store.variables;
    localVariableAdapters_ = std::move(store.adapters);
}

const std::map<std::string, JsonValue>& Component::GetLocalVariables() const
{
    return localVariables_;
}

void Component::RegisterPendingLocalVariablesForComponents(
    const std::map<std::string, JsonValue>& descriptorsById, const std::map<std::string, JsonValue>& localVariables)
{
    if (descriptorsById.empty() || localVariables.empty()) {
        return;
    }
    for (const auto& [componentId, descriptor] : descriptorsById) {
        if (componentId.empty() || !descriptor.IsValid()) {
            continue;
        }
        LocalVariableStore store;
        if (CloneLocalVariableStore(localVariables, store)) {
            g_pendingLocalVariables[componentId] = std::move(store);
        }
    }
}

void Component::ClearPendingLocalVariablesForComponents(const std::map<std::string, JsonValue>& descriptorsById)
{
    for (const auto& [componentId, descriptor] : descriptorsById) {
        static_cast<void>(descriptor);
        g_pendingLocalVariables.erase(componentId);
    }
}

ArkUI_NodeHandle Component::GetNativeView() const
{
    return nativeView_;
}

ArkUI_NodeHandle Component::GetHandle() const
{
    return nativeView_;
}

const CommonMargin& Component::GetCommonMargin() const
{
    return commonMargin_;
}

std::string Component::GetType() const
{
    return "";
}

void Component::SetParentId(const std::string& parentId)
{
    parentId_ = parentId;
}

const std::string& Component::GetParentId() const
{
    return parentId_;
}

void Component::ClearParentId()
{
    parentId_.clear();
}

void Component::SetParent(const std::shared_ptr<Component>& parent)
{
    parent_ = parent;
}

std::shared_ptr<Component> Component::GetParent() const
{
    return parent_.lock();
}

void Component::ClearParent()
{
    parent_.reset();
}

void Component::SetBuildDepth(int32_t depth)
{
    buildDepth_ = std::max(0, depth);
}

int32_t Component::GetBuildDepth() const
{
    return buildDepth_;
}

void Component::CollectChildListDescriptor(const JsonValue& descriptor)
{
    return;
}

const ChildListDescriptor& Component::GetChildListDescriptor() const
{
    return childListDescriptor_;
}

const std::list<std::string>& Component::GetChildIds() const
{
    return childIds_;
}

bool Component::HasChildId(const std::string& childId) const
{
    if (childId.empty()) {
        return false;
    }
    return std::find(childIds_.begin(), childIds_.end(), childId) != childIds_.end();
}

bool Component::IsChildIdsUnchanged(const std::list<std::string>& childIds) const
{
    return childIds_ == childIds;
}

void Component::SetChildIds(const std::list<std::string>& childIds)
{
    childIds_ = childIds;
}

bool Component::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    return false;
}

bool Component::AcceptsChild(const std::shared_ptr<Component>& child) const
{
    return child != nullptr;
}

bool Component::ExpandTemplateChildrenEager(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    childIds.clear();
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();
    std::map<std::string, std::shared_ptr<Component>>& allComponents = surfaceSlot.GetAllComponents();
    std::map<std::string, JsonValue> generatedDescriptors;
    auto templateComponentId = childList.templateComponentId;

    auto templateIt = descriptorStore.find(templateComponentId);
    if (templateIt == descriptorStore.end()) {
        LOG_A2UI(LOG_WARN,
            "ExpandTemplateChildrenEager: template descriptor not found, parentId=%{public}s, templateId=%{public}s",
            componentId_.c_str(), templateComponentId.c_str());
        return false;
    }

    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    if (dataModel == nullptr) {
        LOG_A2UI(
            LOG_WARN, "ExpandTemplateChildrenEager: data model is null, parentId=%{public}s", componentId_.c_str());
        return false;
    }
    std::optional<JsonValue> arrayValueOpt = dataModel->GetNode(childList.templatePath);
    if (!arrayValueOpt.has_value()) {
        LOG_A2UI(LOG_WARN,
            "ExpandTemplateChildrenEager: template path is not array, parentId=%{public}s, path=%{public}s",
            componentId_.c_str(), childList.templatePath.c_str());
        return false;
    }

    JsonValue arrayValue = arrayValueOpt.value();
    if (!arrayValue.IsArray()) {
        LOG_A2UI(LOG_WARN,
            "ExpandTemplateChildrenEager: template path is not array, parentId=%{public}s, path=%{public}s",
            componentId_.c_str(), childList.templatePath.c_str());
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property children.path expects array data source, got type '" + std::string(arrayValue.GetTypeName()) +
                "'",
            "children.path");
        return false;
    }

    int itemCount = arrayValue.GetArraySize();
    for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
        generatedDescriptors.clear();
        std::string generatedInstanceId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(templateComponentId,
            templateComponentId, childList.templatePath, itemIndex, &descriptorStore, &generatedDescriptors);
        if (generatedInstanceId.empty()) {
            LOG_A2UI(LOG_WARN,
                "ExpandTemplateChildrenEager: failed to build template instance descriptor, parentId=%{public}s, "
                "templateId=%{public}s, itemIndex=%{public}d",
                componentId_.c_str(), templateComponentId.c_str(), itemIndex);
            continue;
        }

        bool hasProcessedNode = false;
        bool sawRootDescriptor = false;
        std::map<std::string, JsonValue> currentLocalVariables =
            BuildTemplateLocalVariables(childList, arrayValue.GetArrayItem(itemIndex), itemIndex);
        std::map<std::string, JsonValue> localVariables = MergeLocalVariables(localVariables_, currentLocalVariables);
        Component::RegisterPendingLocalVariablesForComponents(generatedDescriptors, localVariables);
        auto node = surfaceSlot.BuildRootFromComponents(
            generatedInstanceId, generatedDescriptors, hasProcessedNode, sawRootDescriptor, false);
        Component::ClearPendingLocalVariablesForComponents(generatedDescriptors);
        if (node == nullptr) {
            continue;
        }
        childIds.push_back(generatedInstanceId);
    }
    return true;
}

void Component::DetachCurrentChildren()
{
    for (const auto& child : children_) {
        if (child == nullptr) {
            continue;
        }
        if (child->GetParentId() == componentId_) {
            child->ClearParentId();
            child->ClearParent();
        }
    }
    ClearChildren();
}

void Component::AttachStaticChildrenByIds(
    const std::list<std::string>& childIds, const std::map<std::string, std::shared_ptr<Component>>& allComponents)
{
    if (IsChildIdsUnchanged(childIds)) {
        return;
    }
    DetachCurrentChildren();

    size_t declaredIndex = 0;
    for (const auto& childId : childIds) {
        auto childIt = allComponents.find(childId);
        if (childIt == allComponents.end() || childIt->second == nullptr) {
            ++declaredIndex;
            continue;
        }
        if (!childIt->second->GetParentId().empty() && componentId_ != childIt->second->GetParentId()) {
            ++declaredIndex;
            continue;
        }
        if (!AcceptsChild(childIt->second)) {
            ++declaredIndex;
            continue;
        }
        childIt->second->SetParentId(componentId_);
        childIt->second->SetParent(shared_from_this());
        AddChildAt(childIt->second, declaredIndex);
        ++declaredIndex;
    }
    SetChildIds(childIds);
}

void Component::BuildChildren(SurfaceSlot& surfaceSlot)
{
    std::map<std::string, std::shared_ptr<Component>>& allComponents = surfaceSlot.GetAllComponents();

    if (childListDescriptor_.type == ChildListType::STATIC_IDS) {
        AttachStaticChildrenByIds(childListDescriptor_.staticChildIds, allComponents);
        return;
    }

    if (childListDescriptor_.type == ChildListType::TEMPLATE_PATH) {
        std::list<std::string> childIds;
        if (!ExpandTemplateChildren(childListDescriptor_, surfaceSlot, childIds)) {
            return;
        }
        AttachStaticChildrenByIds(childIds, allComponents);
        return;
    }

    AttachStaticChildrenByIds({}, allComponents);
}

size_t Component::ResolveInsertIndexInParent(const std::shared_ptr<Component>& parentNode) const
{
    if (parentNode == nullptr) {
        return 0;
    }
    size_t declaredIndex = 0;
    for (const auto& siblingId : parentNode->GetChildIds()) {
        if (siblingId == componentId_) {
            return declaredIndex;
        }
        ++declaredIndex;
    }
    return parentNode->GetChildren().size();
}

void Component::AttachToParentIfNeeded(
    const std::map<std::string, std::shared_ptr<Component>>& allComponents, std::string& parentId)
{
    if (parentId.empty()) {
        return;
    }
    std::shared_ptr<Component> self = shared_from_this();
    if (self == nullptr) {
        return;
    }

    std::shared_ptr<Component> mountedParent = GetParent();
    if (mountedParent != nullptr) {
        if (mountedParent->HasChild(self)) {
            return;
        }
        ClearParent();
    }

    std::shared_ptr<Component> parentNode = nullptr;
    auto parentIt = allComponents.find(parentId);
    if (parentIt != allComponents.end()) {
        parentNode = parentIt->second;
    }

    if (parentNode == nullptr) {
        return;
    }
    if (!parentNode->HasChildId(self->GetComponentId())) {
        return;
    }
    if (!parentNode->AcceptsChild(self)) {
        parentId.clear();
        return;
    }
    size_t insertIndex = ResolveInsertIndexInParent(parentNode);
    parentNode->AddChildAt(self, insertIndex);
}

void Component::AddBinding(const std::string& prop, const std::string& path)
{
    if (prop.empty() || path.empty()) {
        return;
    }

    auto existing =
        std::find_if(dataBindings_.begin(), dataBindings_.end(), [&prop, &path](const DataBinding& binding) {
            return binding.propertyName_ == prop && binding.dataPath_ == path;
        });
    if (existing != dataBindings_.end()) {
        return;
    }

    dataBindings_.emplace_back(prop, path);
}

void Component::AddFunctionCallBinding(const std::string& prop, const std::string& path, const JsonValue& descriptor)
{
    dataBindings_.emplace_back(prop, path, BindingType::FUNCTION_CALL, descriptor);
}

void Component::RemoveBindingsForProperty(const std::string& property)
{
    dataBindings_.erase(std::remove_if(dataBindings_.begin(), dataBindings_.end(),
                            [&property](const DataBinding& binding) { return binding.propertyName_ == property; }),
        dataBindings_.end());
}

void Component::MarkDescriptorDynamicBindingsResolved()
{
    descriptorDynamicBindingsResolved_ = true;
}

bool Component::ConsumeDescriptorDynamicBindingsResolved()
{
    bool resolved = descriptorDynamicBindingsResolved_;
    descriptorDynamicBindingsResolved_ = false;
    return resolved;
}

void Component::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    for (const auto& binding : dataBindings_) {
        if (binding.propertyName_ == property && binding.type_ == BindingType::EXPRESSION) {
#ifdef ENABLE_EXPRESSION_ENGINE
            JsonValue resolvedValue = EvaluateExpressionBindingValue(
                binding.expression_, renderId_, surfaceId_, componentId_, localVariables_);
            if (resolvedValue.IsValid()) {
                ApplyRuntimeProperty(property, resolvedValue, true);
            }
#endif
            return;
        }
        if (binding.propertyName_ == property && binding.type_ == BindingType::FUNCTION_CALL) {
            DynamicResolveContext context = { .renderId = renderId_,
                .surfaceId = surfaceId_,
                .componentId = componentId_,
                .dataModel = GetDynamicResolveDataModel(),
                .allowExpression = true,
                .localVariables = localVariables_ };
            ResolvedValue resolved = DynamicValueResolver::Resolve(binding.functionCallDescriptor_, context);
            if (resolved.success && resolved.value.IsValid()) {
                ApplyRuntimeProperty(property, resolved.value, true);
            }
            return;
        }
    }
    ApplyRuntimeProperty(property, value, true);
}

void Component::ApplyRuntimeProperty(const std::string& property, const JsonValue& value, bool fromDynamicUpdate)
{
    PropertyDeclaration declaration;
    if (TryGetPropertyDeclaration(property, declaration)) {
        if (fromDynamicUpdate && !declaration.allowDynamic) {
            return;
        }

        JsonValue normalizedValue;
        NormalizationIssue issue;
        if (!NormalizePropertyValue(declaration, value, normalizedValue, &issue) || !normalizedValue.IsValid()) {
            return;
        }
        if (issue.hasWarning) {
            ReportSchemaWarning(issue.code, issue.message, property);
        }
        if (fromDynamicUpdate && declaration.reportDynamicNumberRange && value.IsNumber()) {
            double number = value.GetNumberValue(declaration.fallbackNumber);
            bool belowMin = declaration.dynamicNumberMinExclusive ? number <= declaration.dynamicNumberMin
                                                                  : number < declaration.dynamicNumberMin;
            if (!std::isfinite(number) || belowMin) {
                ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                    "Property " + declaration.name + " is out of range, fallback/reset has been applied", property);
            }
        }
        OnPropertyApplied(property, normalizedValue);
        if (declaration.applyValue != nullptr) {
            declaration.applyValue(normalizedValue);
        }
        return;
    }

    JsonValue storedValue;
    if (!CloneJsonValue(value, storedValue)) {
        return;
    }
    OnPropertyApplied(property, storedValue);
    LOG_A2UI(LOG_INFO, "OnDataUpdate property: %{public}s", property.c_str());
}

PropertyDeclaration Component::GetCommonPropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(Component&)>> declarations = {
        { ACCESSIBILITY_LABEL_PROPERTY,
            [](Component& component) {
                return PropertyDeclaration { .name = ACCESSIBILITY_LABEL_PROPERTY,
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .fallbackString = "",
                    .applyValue = [&component](const JsonValue& value) {
                        component.SetAccessibilityLabel(value.GetStringValue(""));
                    } };
            } },
        { ACCESSIBILITY_DESCRIPTION_PROPERTY,
            [](Component& component) {
                return PropertyDeclaration { .name = ACCESSIBILITY_DESCRIPTION_PROPERTY,
                    .type = PropertyValueType::STRING,
                    .allowDynamic = true,
                    .fallbackString = "",
                    .applyValue = [&component](const JsonValue& value) {
                        component.SetAccessibilityDescription(value.GetStringValue(""));
                    } };
            } },
        { "weight",
            [](Component& component) {
                return PropertyDeclaration { .name = "weight",
                    .type = PropertyValueType::NUMBER,
                    .allowDynamic = false,
                    .fallbackNumber = 0.0,
                    .applyValue = [&component](const JsonValue& value) {
                        component.SetLayoutWeight(static_cast<float>(value.GetNumberValue(0.0)));
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

PropertyDeclaration Component::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    (void)propertyName;
    return {};
}

bool Component::HandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
{
    (void)propertyName;
    (void)value;
    return false;
}

bool Component::ShouldValidateUnknownDescriptorFields() const
{
    return true;
}

bool Component::IsExpressionSupported() const
{
    return false;
}

bool Component::IsExpressionCandidate(const JsonValue& value) const
{
    return false;
}

std::shared_ptr<DataModel> Component::GetDynamicResolveDataModel() const
{
    return nullptr;
}

bool Component::IsKnownAdditionalDescriptorKey(const std::string& propertyName) const
{
    (void)propertyName;
    return false;
}

bool Component::IsKnownNestedDescriptorKey(const std::string& objectName, const std::string& propertyName) const
{
    if (objectName == "accessibility") {
        return propertyName == "label" || propertyName == "description";
    }
    return false;
}

std::vector<std::string> Component::GetComponentDirectRequiredPropertyKeys() const
{
    return {};
}

void Component::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    (void)descriptor;
}

bool Component::TryGetPropertyDeclaration(const std::string& property, PropertyDeclaration& declaration)
{
    declaration = GetPrivatePropertyDeclaration(property);
    if (!declaration.name.empty()) {
        return true;
    }

    declaration = GetCommonPropertyDeclaration(property);
    return !declaration.name.empty();
}

bool Component::IsKnownDescriptorKey(const std::string& propertyName)
{
    if (propertyName == "id" || propertyName == "component" || propertyName == "type" || propertyName == "weight" ||
        propertyName == "accessibility" || propertyName == "child" || propertyName == "children" ||
        propertyName == "tabs") {
        return true;
    }

    PropertyDeclaration declaration;
    if (TryGetPropertyDeclaration(propertyName, declaration)) {
        return true;
    }

    return IsKnownAdditionalDescriptorKey(propertyName);
}

void Component::ValidateComponentDirectRequiredProperties(const JsonValue& descriptor)
{
    if (!descriptor.IsObject()) {
        return;
    }

    const std::vector<std::string> requiredKeys = GetComponentDirectRequiredPropertyKeys();
    for (const std::string& key : requiredKeys) {
        if (descriptor.Has(key.c_str())) {
            continue;
        }

        ReportSchemaWarning(
            SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property " + key + " is required, fallback to default value", key);
        LOG_A2UI(LOG_WARN,
            "Component::ValidateComponentDirectRequiredProperties: "
            "property '%{public}s' is required for component "
            "'%{public}s'",
            key.c_str(), GetType().c_str());
    }
}

void Component::ValidateUnknownDescriptorFields(const JsonValue& descriptor)
{
    if (!descriptor.IsObject()) {
        return;
    }

    for (JsonValue child = descriptor.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }

        if (IsKnownDescriptorKey(key)) {
            if (key == "accessibility" && child.IsObject()) {
                ValidateUnknownObjectFields(child, key, key);
            }
            continue;
        }

        ReportSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property " + key + " is undefined in native local schema and has been ignored", key);
        LOG_A2UI(LOG_WARN,
            "Component::ValidateUnknownDescriptorFields: property '%{public}s' is undefined for component '%{public}s'",
            key.c_str(), GetType().c_str());
    }
}

void Component::ValidateUnknownObjectFields(
    const JsonValue& objectValue, const std::string& propertyPath, const std::string& objectName)
{
    if (!objectValue.IsObject()) {
        return;
    }

    for (JsonValue child = objectValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty() || IsKnownNestedDescriptorKey(objectName, key)) {
            continue;
        }

        std::string nestedPath = propertyPath + "." + key;
        ReportSchemaWarning(SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property " + nestedPath + " is undefined in native local schema and has been ignored", nestedPath);
        LOG_A2UI(LOG_WARN,
            "Component::ValidateUnknownObjectFields: property '%{public}s' is undefined for object '%{public}s'",
            nestedPath.c_str(), objectName.c_str());
    }
}

void Component::Render() const
{
    LOG_A2UI(LOG_INFO, "Render: %{public}s, type: %{public}s", componentId_.c_str(), GetType().c_str());
}

const std::vector<DataBinding>& Component::GetDataBindings() const
{
    return dataBindings_;
}

void Component::RemoveProperty(const std::string& propertyName)
{
    RemoveBindingsForProperty(propertyName);
    OnPropertyRemoved(propertyName);
}

void Component::OnPropertyRemoved(const std::string& propertyName)
{
    if (propertyName == ACCESSIBILITY_LABEL_PROPERTY) {
        SetAccessibilityLabel("");
        return;
    }

    if (propertyName == ACCESSIBILITY_DESCRIPTION_PROPERTY) {
        SetAccessibilityDescription("");
    }
}

void Component::OnPropertyApplied(const std::string& propertyName, const JsonValue& value)
{
    (void)propertyName;
    (void)value;
}

void Component::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    static_cast<void>(child);
    static_cast<void>(index);
}

void Component::OnAttachToParent() {}

void Component::OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    static_cast<void>(currentIndex);
    OnAddChild(child, targetIndex);
}

void Component::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::RemoveChild(nativeView_, child->GetNativeView());
}

std::shared_ptr<Component> Component::GetChildAtIndex(size_t index) const
{
    const auto& children = GetChildren();
    if (index >= children.size()) {
        return nullptr;
    }
    auto childIt = children.begin();
    std::advance(childIt, index);
    return childIt != children.end() ? *childIt : nullptr;
}

void Component::ApplyChildSpacingForIndex(size_t index)
{
    auto child = GetChildAtIndex(index);
    if (child != nullptr) {
        ApplyChildSpacing(child, index);
    }
}

void Component::ClearChildSpacing(const std::shared_ptr<Component>& child)
{
    if (child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeMargin(child->GetNativeView(), 0.0F, 0.0F, 0.0F, 0.0F);
}

void Component::CalculateChildSpacing(size_t index, size_t totalChildren, float& startMargin, float& endMargin)
{
    startMargin = 0.0F;
    endMargin = 0.0F;

    if (totalChildren == 1) {
        return;
    }

    if (index == 0) {
        endMargin = spacing_ / 2.0F;
    } else if (index == totalChildren - 1) {
        startMargin = spacing_ / 2.0F;
    } else {
        startMargin = spacing_ / 2.0F;
        endMargin = spacing_ / 2.0F;
    }
}

void Component::ApplyChildSpacing(const std::shared_ptr<Component>& child, size_t index)
{
    if (child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }

    const auto& children = GetChildren();
    size_t totalChildren = children.size();

    float startMargin = 0.0F;
    float endMargin = 0.0F;
    CalculateChildSpacing(index, totalChildren, startMargin, endMargin);
    ApplyMarginToChild(child, startMargin, endMargin);
}

void Component::RefreshSpacingOnChildAdded(const std::shared_ptr<Component>& child, size_t index)
{
    size_t totalChildren = GetChildren().size();
    ApplyChildSpacing(child, index);
    if (index == 0) {
        ApplyChildSpacingForIndex(1);
    } else if (index == totalChildren - 1) {
        ApplyChildSpacingForIndex(totalChildren - 2);
    }
}

void Component::RefreshSpacingOnChildMoved(
    const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    if (currentIndex == targetIndex) {
        return;
    }

    size_t totalChildren = GetChildren().size();
    ApplyChildSpacing(child, targetIndex);
    if (targetIndex == 0) {
        ApplyChildSpacingForIndex(1);
    } else if (targetIndex == totalChildren - 1) {
        ApplyChildSpacingForIndex(totalChildren - 2);
    }
    if (currentIndex == 0) {
        ApplyChildSpacingForIndex(0);
    } else if (currentIndex == totalChildren - 1) {
        ApplyChildSpacingForIndex(totalChildren - 1);
    }
}

void Component::ApplyMarginToChild(const std::shared_ptr<Component>& child, float startMargin, float endMargin) {}

void Component::ApplyCommonAttributes(const JsonValue& descriptor)
{
    if (descriptor.IsObject() && descriptor.Has("accessibility")) {
        JsonValue accessibilityValue = descriptor.GetItem("accessibility");
        if (!accessibilityValue.IsObject()) {
            ReportSchemaWarning(
                SCHEMA_ERROR_CODE_TYPE_MISMATCH, "Property accessibility expects object value", "accessibility");
            LOG_A2UI(LOG_WARN, "Component::ApplyCommonAttributes: accessibility is not an object");
            RemoveProperty(ACCESSIBILITY_LABEL_PROPERTY);
            RemoveProperty(ACCESSIBILITY_DESCRIPTION_PROPERTY);
        } else {
            if (accessibilityValue.Has("label")) {
                SetPropertyFromDescriptor("label", accessibilityValue, ACCESSIBILITY_LABEL_PROPERTY);
            } else {
                RemoveProperty(ACCESSIBILITY_LABEL_PROPERTY);
            }
            if (accessibilityValue.Has("description")) {
                SetPropertyFromDescriptor("description", accessibilityValue, ACCESSIBILITY_DESCRIPTION_PROPERTY);
            } else {
                RemoveProperty(ACCESSIBILITY_DESCRIPTION_PROPERTY);
            }
        }
    } else {
        RemoveProperty(ACCESSIBILITY_LABEL_PROPERTY);
        RemoveProperty(ACCESSIBILITY_DESCRIPTION_PROPERTY);
    }

    if (descriptor.Has("weight")) {
        SetPropertyFromDescriptor("weight", descriptor);
    }
}

void Component::ApplyPrivateAttributes(const JsonValue& descriptor) {}

void Component::SetLayoutWeight(float weight)
{
    if (nativeView_ == nullptr) {
        return;
    }

    uint32_t normalizedWeight = 0;
    if (std::isfinite(weight) && weight > 0.0f) {
        normalizedWeight = static_cast<uint32_t>(weight);
    }

    ArkUINodeApiAdapter::SetNodeLayoutWeight(nativeView_, normalizedWeight);
}

void Component::SetMargin(float top, float right, float bottom, float left)
{
    SetCommonMargin(top, right, bottom, left);
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeMargin(nativeView_, top, right, bottom, left);
}

void Component::SetAccessibilityLabel(const std::string& label)
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeAccessibilityText(nativeView_, label);
}

void Component::SetAccessibilityDescription(const std::string& description)
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeAccessibilityDescription(nativeView_, description);
}

void Component::SetCommonMargin(float top, float right, float bottom, float left)
{
    commonMargin_ = { .top = top, .right = right, .bottom = bottom, .left = left };
}

void Component::ValidateChecksSpecialProperty(const JsonValue& value)
{
    if (!value.IsArray()) {
        std::string actualType = value.GetTypeName();
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property checks expects array value, got type '" + actualType + "', field has been ignored", "checks");
        return;
    }

    int32_t checkCount = value.GetArraySize();
    for (int32_t index = 0; index < checkCount; ++index) {
        JsonValue itemValue = value.GetArrayItem(index);
        std::string itemPath = "checks[" + std::to_string(index) + "]";
        if (!itemValue.IsObject()) {
            std::string actualType = itemValue.GetTypeName();
            ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property checks item expects object value, got type '" + actualType + "', item has been ignored",
                itemPath);
            continue;
        }

        if (!itemValue.Has("condition")) {
            ReportSchemaWarning(SCHEMA_ERROR_CODE_REQUIRED_MISS,
                "Property condition is required, field has been ignored", itemPath + ".condition");
        } else {
            JsonValue conditionValue = itemValue.GetItem("condition");
            if (!conditionValue.IsObject()) {
                std::string actualType = conditionValue.GetTypeName();
                ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                    "Property condition expects object value, got type '" + actualType + "', field has been ignored",
                    itemPath + ".condition");
            }
        }

        if (!itemValue.Has("message")) {
            ReportSchemaWarning(SCHEMA_ERROR_CODE_REQUIRED_MISS,
                "Property message is required, fallback to default value", itemPath + ".message");
            continue;
        }

        JsonValue messageValue = itemValue.GetItem("message");
        if (!messageValue.IsString()) {
            std::string actualType = messageValue.GetTypeName();
            ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property message expects string value, got type '" + actualType + "', fallback to default value",
                itemPath + ".message");
        }
    }
}

void Component::ValidateActionSpecialProperty(const JsonValue& value)
{
    if (!value.IsObject()) {
        std::string actualType = value.GetTypeName();
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property action expects object value, got type '" + actualType + "', field has been ignored", "action");
        return;
    }

    bool hasEvent = value.Has("event");
    bool hasFunctionCall = value.Has("functionCall");
    if (hasEvent && hasFunctionCall) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property action cannot contain both event and functionCall, field has been ignored", "action");
        return;
    }
    if (!hasEvent && !hasFunctionCall) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_REQUIRED_MISS,
            "Property action requires event or functionCall, field has been ignored", "action");
        return;
    }

    if (hasEvent) {
        JsonValue eventValue = value.GetItem("event");
        if (!eventValue.IsObject()) {
            std::string actualType = eventValue.GetTypeName();
            ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property event expects object value, got type '" + actualType + "', field has been ignored",
                "action.event");
            return;
        }
        if (!eventValue.Has("name")) {
            ReportSchemaWarning(SCHEMA_ERROR_CODE_REQUIRED_MISS, "Property name is required, field has been ignored",
                "action.event.name");
            return;
        }

        JsonValue nameValue = eventValue.GetItem("name");
        if (!nameValue.IsString()) {
            std::string actualType = nameValue.GetTypeName();
            ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                "Property name expects string value, got type '" + actualType + "', field has been ignored",
                "action.event.name");
        }
        if (eventValue.Has("context")) {
            JsonValue contextValue = eventValue.GetItem("context");
            if (!contextValue.IsObject()) {
                std::string actualType = contextValue.GetTypeName();
                ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                    "Property context expects object value, got type '" + actualType + "', fallback to empty object",
                    "action.event.context");
            }
        }
        return;
    }

    JsonValue functionCallValue = value.GetItem("functionCall");
    if (!functionCallValue.IsObject()) {
        std::string actualType = functionCallValue.GetTypeName();
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property functionCall expects object value, got type '" + actualType + "', field has been ignored",
            "action.functionCall");
    }
}

void Component::SetVisibility(A2UIVisibility visibility)
{
    if (nativeView_ == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeVisibility(nativeView_, visibility);
}

void Component::SetPropertyFromDescriptor(
    const std::string& propertyKey, const JsonValue& descriptor, const std::string& bindingKey)
{
    std::string resolvedBindingKey = bindingKey.empty() ? propertyKey : bindingKey;
    std::string declarationKey = resolvedBindingKey;
    if (!descriptor.IsValid() || propertyKey.empty()) {
        LOG_A2UI(LOG_WARN, "SetPropertyFromDescriptor: invalid input, property='%{public}s'", propertyKey.c_str());
        return;
    }
    if (!descriptor.IsObject() || !descriptor.Has(propertyKey.c_str())) {
        LOG_A2UI(
            LOG_WARN, "SetPropertyFromDescriptor: property '%{public}s' not found in descriptor", propertyKey.c_str());
        return;
    }

    JsonValue valueJson = descriptor.GetItem(propertyKey.c_str());
    if (!valueJson.IsValid()) {
        LOG_A2UI(LOG_ERROR, "SetPropertyFromDescriptor: property '%{public}s' value is invalid", propertyKey.c_str());
        return;
    }

    if (HandleSpecialProperty(propertyKey, valueJson)) {
        RemoveBindingsForProperty(resolvedBindingKey);
        return;
    }

    ResolveAndBindProperty(propertyKey, declarationKey, resolvedBindingKey, valueJson);
}

void Component::ApplySchemaProperty(const std::string& propertyKey, const JsonValue& descriptor)
{
    if (descriptor.IsObject() && descriptor.Has(propertyKey.c_str())) {
        SetPropertyFromDescriptor(propertyKey, descriptor);
        return;
    }

    ApplyRuntimeProperty(propertyKey, JsonValue(), false);
}

void Component::ResolveAndBindProperty(const std::string& descriptorKey, const std::string& declarationKey,
    const std::string& bindingKey, const JsonValue& valueJson)
{
    std::string resolvedBindingKey = bindingKey.empty() ? declarationKey : bindingKey;
    PropertyDeclaration declaration;
    bool hasDeclaration = TryGetPropertyDeclaration(declarationKey, declaration);
    bool shouldFallbackOnNullOrEmptyObject = hasDeclaration && declaration.type != PropertyValueType::OBJECT;
    bool allowExpression = hasDeclaration && declaration.allowExpression && IsExpressionSupported();

    if (IsExpressionCandidate(valueJson) && !allowExpression) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + resolvedBindingKey + " does not support expression values, field has been ignored",
            resolvedBindingKey);
        LOG_A2UI(LOG_WARN, "SetPropertyFromDescriptor: property '%{public}s' does not support expression values",
            declarationKey.c_str());
        ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, JsonValue());
        return;
    }

    // other properties will reach here
    if (valueJson.IsObject() && !valueJson.Has("path") && !valueJson.Has("call")) {
        if (hasDeclaration && declaration.type == PropertyValueType::OBJECT) {
            ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, valueJson);
            return;
        }
        ReportSchemaWarning(SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + resolvedBindingKey +
                " expects direct value or dynamic descriptor, object literal has been dropped",
            resolvedBindingKey);
        if (shouldFallbackOnNullOrEmptyObject) {
            ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, JsonValue());
            return;
        }
        LOG_A2UI(LOG_WARN,
            "SetPropertyFromDescriptor: unsupported object descriptor for property '%{public}s', field dropped",
            descriptorKey.c_str());
        RemoveProperty(resolvedBindingKey);
        return;
    }

    DynamicResolveContext context = { .renderId = renderId_,
        .surfaceId = surfaceId_,
        .componentId = componentId_,
        .dataModel = GetDynamicResolveDataModel(),
        .allowExpression = allowExpression,
        .localVariables = localVariables_ };
    ResolvedValue resolved = DynamicValueResolver::Resolve(valueJson, context);

    if (resolved.source == ResolveSource::PATH) {
        if (hasDeclaration && !declaration.allowDynamic) {
            RemoveBindingsForProperty(resolvedBindingKey);
            ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + declarationKey + " does not support dynamic binding, path '" + resolved.path +
                    "' has been ignored",
                resolvedBindingKey);
            LOG_A2UI(LOG_WARN,
                "SetPropertyFromDescriptor: property '%{public}s' does not support dynamic binding, ignore path "
                "'%{public}s'",
                declarationKey.c_str(), resolved.path.c_str());
            return;
        }

        SyncPathBinding(resolvedBindingKey, resolved.path);
        if (resolved.success && resolved.value.IsValid()) {
            if (resolved.value.IsNull()) {
                if (shouldFallbackOnNullOrEmptyObject) {
                    JsonValue fallbackValue;
                    if (NormalizePropertyValue(declaration, JsonValue(), fallbackValue) && fallbackValue.IsValid()) {
                        ApplyRuntimeProperty(declarationKey, JsonValue(), false);
                        if (resolvedBindingKey != declarationKey) {
                            OnPropertyApplied(resolvedBindingKey, fallbackValue);
                        }
                    }
                }
            } else {
                JsonValue initialValue;
                if (CloneJsonValue(resolved.value, initialValue)) {
                    ApplyRuntimeProperty(declarationKey, initialValue, true);
                    MarkDescriptorDynamicBindingsResolved();
                    if (resolvedBindingKey != declarationKey) {
                        OnPropertyApplied(resolvedBindingKey, initialValue);
                    }
                }
            }
        }
        if (!resolved.success) {
            LOG_A2UI(LOG_WARN, "SetPropertyFromDescriptor: path not found for property '%{public}s', path='%{public}s'",
                resolvedBindingKey.c_str(), resolved.path.c_str());
        }
        return;
    }

    if (resolved.source == ResolveSource::FUNCTION_CALL) {
        if (hasDeclaration && !declaration.allowDynamic) {
            RemoveBindingsForProperty(resolvedBindingKey);
            ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property " + declarationKey + " does not support dynamic functionCall binding", resolvedBindingKey);
            return;
        }
        if (resolved.success && resolved.value.IsValid() && !resolved.value.IsNull()) {
            JsonValue initialValue;
            if (CloneJsonValue(resolved.value, initialValue)) {
                ApplyRuntimeProperty(declarationKey, initialValue, true);
                MarkDescriptorDynamicBindingsResolved();
                if (resolvedBindingKey != declarationKey) {
                    OnPropertyApplied(resolvedBindingKey, initialValue);
                }
            }
        }
        SyncFunctionCallBindings(resolvedBindingKey, valueJson);
        return;
    }

    if (resolved.source == ResolveSource::INVALID && valueJson.IsObject() && valueJson.Has("path")) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + resolvedBindingKey + " contains invalid path binding and has been dropped",
            resolvedBindingKey);
        LOG_A2UI(LOG_WARN, "SetPropertyFromDescriptor: invalid path binding for property '%{public}s', field dropped",
            resolvedBindingKey.c_str());
        RemoveProperty(resolvedBindingKey);
        return;
    }

    if (resolved.source == ResolveSource::EXPRESSION && !resolved.success) {
        ReportSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property " + resolvedBindingKey + " expression evaluation failed, fallback has been applied",
            resolvedBindingKey);
        LOG_A2UI(LOG_WARN,
            "SetPropertyFromDescriptor: expression resolve failed for property '%{public}s', reason='%{public}s'",
            resolvedBindingKey.c_str(), resolved.errorMessage.c_str());
        ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, JsonValue());
        return;
    }

    if (resolved.source == ResolveSource::EXPRESSION) {
        JsonValue propValue = resolved.value;
        if (!propValue.IsValid() || propValue.IsNull()) {
            if (shouldFallbackOnNullOrEmptyObject && propValue.IsNull()) {
                ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, JsonValue());
                return;
            }
            LOG_A2UI(LOG_INFO, "SetPropertyFromDescriptor: property '%{public}s' resolved to invalid/null, skipped",
                resolvedBindingKey.c_str());
            return;
        }

        RemoveBindingsForProperty(resolvedBindingKey);
#ifdef ENABLE_EXPRESSION_ENGINE
        std::string expression =
            valueJson.IsString() ? ExpressionEngine::ExtractExpression(valueJson.GetStringValue("")) : "";
        std::vector<Dependency> dependencies = CollectExpressionDependencies(expression);
        std::vector<std::string> dependencyNames;
        std::vector<std::string> dataModelPaths;
        for (const auto& dependency : dependencies) {
            AppendUniqueString(dependencyNames, dependency.variableName);
            if (dependency.variableName == "__dataModel") {
                AppendUniqueString(dataModelPaths, dependency.path);
            }
        }

        if (!expression.empty() && !dependencyNames.empty()) {
            if (dataModelPaths.empty()) {
                dataBindings_.emplace_back(resolvedBindingKey, expression, dependencyNames);
            } else {
                for (const auto& dataPath : dataModelPaths) {
                    DataBinding expressionBinding(resolvedBindingKey, expression, dependencyNames);
                    expressionBinding.dataPath_ = dataPath;
                    dataBindings_.push_back(std::move(expressionBinding));
                }
            }
        }
#endif
        ApplyRuntimeProperty(declarationKey, propValue, false);
        if (resolvedBindingKey != declarationKey) {
            OnPropertyApplied(resolvedBindingKey, propValue);
        }
        return;
    }

    JsonValue propValue = resolved.success ? resolved.value : valueJson;
    if (!propValue.IsValid() || propValue.IsNull()) {
        if (shouldFallbackOnNullOrEmptyObject && propValue.IsNull()) {
            ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, JsonValue());
            return;
        }
        LOG_A2UI(LOG_INFO, "SetPropertyFromDescriptor: property '%{public}s' resolved to invalid/null, skipped",
            resolvedBindingKey.c_str());
        return;
    }

    ApplyResolvedPropertyValue(declarationKey, resolvedBindingKey, propValue);
}

void Component::SyncPathBinding(const std::string& bindingKey, const std::string& path)
{
    bool hasExactBinding = false;
    size_t samePropertyBindingCount = 0;
    for (const auto& binding : dataBindings_) {
        if (binding.propertyName_ != bindingKey) {
            continue;
        }
        ++samePropertyBindingCount;
        if (binding.dataPath_ == path) {
            hasExactBinding = true;
        }
    }
    if (!hasExactBinding || samePropertyBindingCount != 1) {
        RemoveBindingsForProperty(bindingKey);
        AddBinding(bindingKey, path);
        LOG_A2UI(LOG_INFO, "SetPropertyFromDescriptor: Added binding for property '%{public}s' -> path '%{public}s'",
            bindingKey.c_str(), path.c_str());
    }
}

void Component::SyncFunctionCallBindings(const std::string& bindingKey, const JsonValue& descriptor)
{
    RemoveBindingsForProperty(bindingKey);
    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor);
    for (const auto& path : dependencies.dataPaths) {
        DataBinding binding(bindingKey, path, BindingType::FUNCTION_CALL, descriptor);
        binding.globalVarDeps_ = dependencies.globalVariables;
        dataBindings_.push_back(std::move(binding));
    }
    if (dependencies.dataPaths.empty() && !dependencies.globalVariables.empty()) {
        DataBinding binding(bindingKey, "", BindingType::FUNCTION_CALL, descriptor);
        binding.globalVarDeps_ = dependencies.globalVariables;
        dataBindings_.push_back(std::move(binding));
    }
    if (!dependencies.dataPaths.empty() || !dependencies.globalVariables.empty()) {
        LOG_A2UI(LOG_INFO,
            "SetPropertyFromDescriptor: Added functionCall binding for property '%{public}s', paths=%{public}zu, "
            "globals=%{public}zu",
            bindingKey.c_str(), dependencies.dataPaths.size(), dependencies.globalVariables.size());
    } else {
        LOG_A2UI(LOG_INFO,
            "SetPropertyFromDescriptor: functionCall descriptor for property '%{public}s' has no data path "
            "dependencies, property will not react to data changes",
            bindingKey.c_str());
    }
}

void Component::ApplyResolvedPropertyValue(
    const std::string& declarationKey, const std::string& bindingKey, const JsonValue& value)
{
    RemoveBindingsForProperty(bindingKey);
    ApplyRuntimeProperty(declarationKey, value, false);
    if (bindingKey != declarationKey) {
        OnPropertyApplied(bindingKey, value);
    }
}

JsonValue Component::EvaluateCustomExpression(const std::string& rawExpr) const
{
#ifdef ENABLE_EXPRESSION_ENGINE
    if (!ExpressionEngine::IsExpression(rawExpr)) {
        return JsonValue();
    }
    EvaluationContext evalContext;
    SetupExpressionEvaluationContext(evalContext, renderId_, surfaceId_, componentId_);
    JsonValue value = ExpressionEngine::GetInstance().EvaluateAsJsonValue(rawExpr, evalContext);
    if (HasExpressionContextError(evalContext) &&
        (IsSoftExpressionContextError(evalContext.lastError) || !value.IsValid())) {
        DispatchExpressionRuntimeError(renderId_, surfaceId_, componentId_, evalContext);
    }
    return value;
#else
    return JsonValue();
#endif
}

void Component::OnConfigChange(const ThemeContext& context)
{
    // Default implementation does nothing
    // Subclasses can override to handle theme configuration changes
}

std::shared_ptr<ThemeBase> Component::GetTheme()
{
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    if (themeManager == nullptr) {
        return nullptr;
    }

    // Get theme by component type
    return themeManager->GetTheme(GetType());
}

std::shared_ptr<ThemeManager> Component::GetThemeManager() const
{
    // Get RenderSlot through RenderManager
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(renderId_);
    if (renderSlot == nullptr) {
        return nullptr;
    }

    // Get SurfaceManager from RenderSlot
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return nullptr;
    }

    // Get SurfaceSlot from SurfaceManager using surfaceId
    SurfaceSlot* surfaceSlot = surfaceManager->FindSurface(surfaceId_);
    if (surfaceSlot == nullptr) {
        return nullptr;
    }

    // Get ThemeManager from SurfaceSlot
    return surfaceSlot->GetThemeManager();
}

void Component::ReportSchemaWarning(
    const std::string& code, const std::string& message, const std::string& propertyPath) const
{
    if (renderId_ < 0) {
        return;
    }
    WarningDispatchBridge::GetInstance().Dispatch(renderId_, surfaceId_, componentId_, code, message,
        BuildSchemaWarningPath(propertyPath), "component", ResolveSchemaWarningItemName());
}

std::string Component::BuildSchemaWarningPath(const std::string& propertyPath) const
{
    if (componentId_.empty()) {
        return propertyPath;
    }
    if (propertyPath.empty()) {
        return componentId_;
    }
    return componentId_ + "." + propertyPath;
}

std::string Component::ResolveSchemaWarningItemName() const
{
    std::string componentType = GetType();
    return componentType.empty() ? "component" : componentType;
}

} // namespace NativeModule
