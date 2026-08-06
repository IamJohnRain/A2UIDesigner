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

#include "ModalCoordinator.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>

#include "../../../data/DynamicValueResolver.h"
#include "../../../functions/WarningDispatchBridge.h"
#include "../../../utils/LogA2UI.h"
#include "../A2UIComponent.h"
#include "ArkUINodeApiAdapter.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr char MODAL_CLICK_BINDING_KEY[] = "__modal_binding__";
constexpr char MODAL_COMPONENT_TYPE[] = "Modal";
constexpr char MODAL_ITEM_TYPE[] = "component";
constexpr char MODAL_ITEM_NAME[] = "Modal";
constexpr char ACCESSIBILITY_PROPERTY[] = "accessibility";
// Registry used to recover the owning coordinator from static dismiss callbacks.
std::unordered_map<std::string, ModalCoordinator*>& GetModalCoordinatorRegistry()
{
    static std::unordered_map<std::string, ModalCoordinator*> registry;
    return registry;
}

std::string BuildSchemaWarningPath(const std::string& modalId, const std::string& propertyPath)
{
    if (modalId.empty()) {
        return propertyPath;
    }
    if (propertyPath.empty()) {
        return modalId;
    }
    return modalId + "." + propertyPath;
}

void DispatchSchemaWarning(int32_t renderId, const std::string& surfaceId, const std::string& modalId,
    const std::string& code, const std::string& message, const std::string& propertyPath)
{
    if (renderId < 0) {
        return;
    }
    WarningDispatchBridge::GetInstance().Dispatch(renderId, surfaceId, modalId, code, message,
        BuildSchemaWarningPath(modalId, propertyPath), MODAL_ITEM_TYPE, MODAL_ITEM_NAME);
}

bool IsKnownAccessibilityField(const std::string& key)
{
    return key == "label" || key == "description";
}

bool IsKnownModalField(const std::string& key)
{
    return key == "id" || key == "component" || key == "trigger" || key == "content" || key == ACCESSIBILITY_PROPERTY ||
           key == "weight";
}

std::string SerializeDynamicStringValue(const JsonValue& value);

void ValidateUnknownAccessibilityFields(
    const JsonValue& accessibilityValue, int32_t renderId, const std::string& surfaceId, const std::string& modalId)
{
    if (!accessibilityValue.IsObject()) {
        DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property accessibility expects object value", ACCESSIBILITY_PROPERTY);
        return;
    }

    for (JsonValue child = accessibilityValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty() || IsKnownAccessibilityField(key)) {
            continue;
        }

        std::string nestedPath = std::string(ACCESSIBILITY_PROPERTY) + "." + key;
        DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property " + nestedPath + " is undefined in native local schema and has been ignored", nestedPath);
        LOG_A2UI(LOG_WARN,
            "ModalCoordinator::TryCreateDescriptor: property '%{public}s' is undefined for object 'accessibility'",
            nestedPath.c_str());
    }
}

void ValidateUnknownDescriptorFields(
    const JsonValue& nodeValue, int32_t renderId, const std::string& surfaceId, const std::string& modalId)
{
    if (!nodeValue.IsObject()) {
        return;
    }

    for (JsonValue child = nodeValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }

        if (IsKnownModalField(key)) {
            if (key == ACCESSIBILITY_PROPERTY) {
                ValidateUnknownAccessibilityFields(child, renderId, surfaceId, modalId);
            }
            continue;
        }

        DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_UNDEFINED_FIELD,
            "Property " + key + " is undefined in native local schema and has been ignored", key);
        LOG_A2UI(LOG_WARN,
            "ModalCoordinator::TryCreateDescriptor: property '%{public}s' is undefined for component 'Modal'",
            key.c_str());
    }
}

std::string ReadStringProperty(const JsonValue& nodeValue, const std::string& propertyName, bool required,
    int32_t renderId, const std::string& surfaceId, const std::string& modalId)
{
    if (!nodeValue.IsObject() || !nodeValue.Has(propertyName.c_str())) {
        if (required) {
            DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_REQUIRED_MISS,
                "Property " + propertyName + " is required, fallback to default value", propertyName);
        }
        return "";
    }

    JsonValue propertyValue = nodeValue.GetItem(propertyName.c_str());
    if (!propertyValue.IsString()) {
        DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property " + propertyName + " expects string value, got type '" + propertyValue.GetTypeName() +
                "', fallback to default value",
            propertyName);
        return "";
    }

    return propertyValue.GetStringValue("");
}

std::string ReadDynamicStringPropertyLiteral(const JsonValue& nodeValue, const std::string& propertyName, bool required,
    int32_t renderId, const std::string& surfaceId, const std::string& modalId, std::string* directValue = nullptr)
{
    if (directValue != nullptr) {
        directValue->clear();
    }

    if (!nodeValue.IsObject() || !nodeValue.Has(propertyName.c_str())) {
        if (required) {
            DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_REQUIRED_MISS,
                "Property " + propertyName + " is required, fallback to default value", propertyName);
        }
        return "";
    }

    JsonValue propertyValue = nodeValue.GetItem(propertyName.c_str());
    std::string literal = SerializeDynamicStringValue(propertyValue);
    if (!literal.empty()) {
        if (directValue != nullptr && propertyValue.IsString()) {
            *directValue = propertyValue.GetStringValue("");
        }
        return literal;
    }

    DispatchSchemaWarning(renderId, surfaceId, modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
        "Property " + propertyName + " expects string value or dynamic descriptor, got type '" +
            propertyValue.GetTypeName() + "', fallback to default value",
        propertyName);
    return "";
}

// Keep modal-trigger weight behavior aligned with Component::SetLayoutWeight
// while keeping the change scoped to ModalCoordinator only.
void SetTriggerLayoutWeight(const std::shared_ptr<Component>& triggerComponent, float weight)
{
    if (!ArkUINodeApiAdapter::IsAvailable() || triggerComponent == nullptr ||
        triggerComponent->GetNativeView() == nullptr) {
        return;
    }

    uint32_t normalizedWeight = 0;
    if (std::isfinite(weight) && weight > 0.0F) {
        normalizedWeight = static_cast<uint32_t>(weight);
        if (normalizedWeight == 0) {
            normalizedWeight = 1;
        }
    }

    ArkUINodeApiAdapter::SetNodeLayoutWeight(triggerComponent->GetNativeView(), normalizedWeight);
}

void SetTriggerAccessibilityLabel(const std::shared_ptr<Component>& triggerComponent, const std::string& label)
{
    if (!ArkUINodeApiAdapter::IsAvailable() || triggerComponent == nullptr ||
        triggerComponent->GetNativeView() == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeAccessibilityText(triggerComponent->GetNativeView(), label);
}

void SetTriggerAccessibilityDescription(
    const std::shared_ptr<Component>& triggerComponent, const std::string& description)
{
    if (!ArkUINodeApiAdapter::IsAvailable() || triggerComponent == nullptr ||
        triggerComponent->GetNativeView() == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::SetNodeAccessibilityDescription(triggerComponent->GetNativeView(), description);
}

bool CanResetTriggerAttribute(const std::shared_ptr<Component>& triggerComponent)
{
    return ArkUINodeApiAdapter::IsAvailable() && triggerComponent != nullptr &&
           triggerComponent->GetNativeView() != nullptr;
}

void ResetTriggerLayoutWeight(const std::shared_ptr<Component>& triggerComponent)
{
    if (CanResetTriggerAttribute(triggerComponent)) {
        ArkUINodeApiAdapter::ResetNodeLayoutWeight(triggerComponent->GetNativeView());
    }
}

void ResetTriggerAccessibilityText(const std::shared_ptr<Component>& triggerComponent)
{
    if (CanResetTriggerAttribute(triggerComponent)) {
        ArkUINodeApiAdapter::ResetNodeAccessibilityText(triggerComponent->GetNativeView());
    }
}

void ResetTriggerAccessibilityDescription(const std::shared_ptr<Component>& triggerComponent)
{
    if (CanResetTriggerAttribute(triggerComponent)) {
        ArkUINodeApiAdapter::ResetNodeAccessibilityDescription(triggerComponent->GetNativeView());
    }
}

// Modal forwards accessibility to its trigger. Accept either a plain string or
// a dynamic expression and resolve it using the modal component as context.
std::optional<std::string> ResolveForwardedAccessibilityText(
    const std::string& jsonLiteral, const DynamicResolveContext& context)
{
    if (jsonLiteral.empty()) {
        return std::nullopt;
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(jsonLiteral);
    if (adapter == nullptr) {
        return std::nullopt;
    }

    JsonValue value = adapter->GetRoot();
    if (!value.IsValid()) {
        return std::nullopt;
    }

    if (value.IsString()) {
        return value.GetStringValue();
    }

    if (!value.IsObject() || (!value.Has("path") && !value.Has("call"))) {
        return std::nullopt;
    }

    ResolvedValue resolved = DynamicValueResolver::Resolve(value, context);
    if (!resolved.success) {
        return std::nullopt;
    }

    std::unique_ptr<JsonAdapter> resolvedAdapter = JsonAdapter::Parse(resolved.value.ToJsonLiteral());
    if (resolvedAdapter == nullptr) {
        return std::nullopt;
    }

    JsonValue resolvedValue = resolvedAdapter->GetRoot();
    if (!resolvedValue.IsString()) {
        return std::nullopt;
    }
    return resolvedValue.GetStringValue();
}

std::string SerializeDynamicStringValue(const JsonValue& value)
{
    if (!value.IsValid()) {
        return "";
    }
    if (value.IsString()) {
        return value.ToJsonLiteral();
    }
    if (value.IsObject() && (value.Has("path") || value.Has("call"))) {
        return value.ToJsonLiteral();
    }
    return "";
}

std::optional<std::string> ResolveDynamicStringLiteral(
    const std::string& jsonLiteral, const DynamicResolveContext& context)
{
    if (jsonLiteral.empty()) {
        return std::nullopt;
    }

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(jsonLiteral);
    if (adapter == nullptr) {
        return std::nullopt;
    }

    ResolvedValue resolved = DynamicValueResolver::Resolve(adapter->GetRoot(), context);
    if (!resolved.success || !resolved.value.IsString()) {
        return std::nullopt;
    }
    return resolved.value.GetStringValue("");
}

} // namespace

ModalCoordinator::~ModalCoordinator()
{
    DetachRetiredDialogDismissContexts();
    UnregisterOwner();
}

void ModalCoordinator::ParseAccessibility(const JsonValue& nodeValue, ModalDescriptor& descriptor) const
{
    JsonValue accessibilityValue = nodeValue.GetItem("accessibility");
    if (accessibilityValue.IsObject()) {
        JsonValue labelValue = accessibilityValue.GetItem("label");
        if (labelValue.IsValid()) {
            descriptor.accessibilityLabelJson = SerializeDynamicStringValue(labelValue);
            if (!descriptor.accessibilityLabelJson.empty()) {
                descriptor.hasAccessibilityLabel = true;
            } else {
                DispatchSchemaWarning(renderId_, surfaceId_, descriptor.modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                    std::string("Property accessibility.label expects string value or dynamic descriptor, got type '") +
                        labelValue.GetTypeName() + "', field has been ignored",
                    "accessibility.label");
            }
        }

        JsonValue descriptionValue = accessibilityValue.GetItem("description");
        if (descriptionValue.IsValid()) {
            descriptor.accessibilityDescriptionJson = SerializeDynamicStringValue(descriptionValue);
            if (!descriptor.accessibilityDescriptionJson.empty()) {
                descriptor.hasAccessibilityDescription = true;
            } else {
                DispatchSchemaWarning(renderId_, surfaceId_, descriptor.modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                    std::string(
                        "Property accessibility.description expects string value or dynamic descriptor, got type '") +
                        descriptionValue.GetTypeName() + "', field has been ignored",
                    "accessibility.description");
            }
        }
    }
}

bool ModalCoordinator::TryCreateDescriptor(const JsonValue& nodeValue, ModalDescriptor& descriptor) const
{
    if (!nodeValue.IsObject()) {
        return false;
    }
    std::string componentType = nodeValue.GetString("component", "");
    if (componentType != MODAL_COMPONENT_TYPE) {
        return false;
    }

    descriptor = ModalDescriptor {};
    descriptor.modalId = ReadStringProperty(nodeValue, "id", true, renderId_, surfaceId_, "");
    ValidateUnknownDescriptorFields(nodeValue, renderId_, surfaceId_, descriptor.modalId);

    descriptor.triggerJsonLiteral = ReadDynamicStringPropertyLiteral(
        nodeValue, "trigger", true, renderId_, surfaceId_, descriptor.modalId, &descriptor.triggerId);
    descriptor.contentJsonLiteral = ReadDynamicStringPropertyLiteral(
        nodeValue, "content", true, renderId_, surfaceId_, descriptor.modalId, &descriptor.contentId);

    JsonValue weightValue = nodeValue.GetItem("weight");
    if (weightValue.IsValid()) {
        if (!weightValue.IsNumber()) {
            DispatchSchemaWarning(renderId_, surfaceId_, descriptor.modalId, SCHEMA_ERROR_CODE_TYPE_MISMATCH,
                std::string("Property weight expects number value, got type '") + weightValue.GetTypeName() +
                    "', fallback to default value",
                "weight");
            LOG_A2UI(LOG_WARN, "ModalCoordinator::TryCreateDescriptor: weight is not a number, modalId=%{public}s",
                descriptor.modalId.c_str());
        } else {
            descriptor.hasWeight = true;
            descriptor.weight = static_cast<float>(weightValue.GetNumberValue());
        }
    }

    ParseAccessibility(nodeValue, descriptor);

    return true;
}

void ModalCoordinator::SetOwnerContext(int32_t renderId, const std::string& surfaceId)
{
    UnregisterOwner();
    renderId_ = renderId;
    surfaceId_ = surfaceId;
    RegisterOwner();
}

void ModalCoordinator::DismissActiveModal()
{
    if (openModalStack_.empty() && activeDialogs_.empty() && !dialogCloseInProgress_) {
        return;
    }

    openModalStack_.clear();
    ForceCloseAllActiveDialogs();
}

void ModalCoordinator::Dispose()
{
    ClearModalTriggerBindings();
    openModalStack_.clear();
    ForceCloseAllActiveDialogs();
    modalDescriptors_.clear();
    modalBindings_.clear();
    DetachRetiredDialogDismissContexts();
    allComponents_ = nullptr;
    parentsRelations_ = nullptr;
    UnregisterOwner();
    renderId_ = -1;
    surfaceId_.clear();
}

void ModalCoordinator::HandlePendingModalDescriptors(const std::vector<ModalDescriptor>& modalDescriptors,
    const std::map<std::string, std::shared_ptr<Component>>& allComponents,
    const std::map<std::string, std::string>& parentsRelations)
{
    allComponents_ = &allComponents;
    parentsRelations_ = &parentsRelations;

    if (modalDescriptors.empty()) {
        modalDescriptors_.clear();
        ClearModalTriggerBindings();
        openModalStack_.clear();
        ForceCloseAllActiveDialogs();
        return;
    }

    std::vector<ModalDescriptor> retainedModalDescriptors;
    ApplyModalBindings(modalDescriptors, &retainedModalDescriptors);
    modalDescriptors_ = retainedModalDescriptors;
    if (modalBindings_.empty()) {
        openModalStack_.clear();
        ForceCloseAllActiveDialogs();
    }
}

void ModalCoordinator::RefreshModalBindings()
{
    if (modalDescriptors_.empty()) {
        return;
    }
    ApplyModalBindings(modalDescriptors_);
    if (modalBindings_.empty()) {
        openModalStack_.clear();
        ForceCloseAllActiveDialogs();
    }
}

#ifdef TDD_BUILD
void ModalCoordinator::RequestOpenModalForTest(const std::string& modalId)
{
    HandleModalTriggerClick(modalId);
}
#endif

void ModalCoordinator::HandleModalTriggerClick(const std::string& modalId)
{
    if (modalId.empty()) {
        return;
    }

    auto existingIt = std::find(openModalStack_.begin(), openModalStack_.end(), modalId);
    if (existingIt == openModalStack_.end()) {
        openModalStack_.push_back(modalId);
    } else if (std::next(existingIt) == openModalStack_.end()) {
        openModalStack_.pop_back();
    } else {
        openModalStack_.erase(std::next(existingIt), openModalStack_.end());
    }

    UpdateModalPresentation();
}

void ModalCoordinator::BuildDesiredDialogStack(std::vector<DesiredDialogState>& desiredDialogs)
{
    desiredDialogs.clear();
    desiredDialogs.reserve(openModalStack_.size());

    size_t validCount = 0;
    for (const std::string& modalId : openModalStack_) {
        auto bindingIt = modalBindings_.find(modalId);
        if (bindingIt == modalBindings_.end()) {
            break;
        }

        const ModalBinding& binding = bindingIt->second;
        if (binding.contentComponent == nullptr || binding.contentComponent->GetNativeView() == nullptr) {
            LOG_A2UI(
                LOG_WARN, "BuildDesiredDialogStack: content component invalid, modalId=%{public}s", modalId.c_str());
            break;
        }

        desiredDialogs.push_back({ .modalId = modalId, .contentId = binding.contentId, .binding = &binding });
        ++validCount;
    }

    if (validCount < openModalStack_.size()) {
        openModalStack_.resize(validCount);
    }
}

void ModalCoordinator::ClearModalTriggerBindings()
{
    if (allComponents_ != nullptr) {
        for (const auto& [_, binding] : modalBindings_) {
            auto triggerIt = allComponents_->find(binding.triggerId);
            if (triggerIt == allComponents_->end()) {
                continue;
            }

            auto triggerComponent = std::dynamic_pointer_cast<A2UIComponent>(triggerIt->second);
            if (triggerComponent == nullptr) {
                continue;
            }
            triggerComponent->RemoveAuxiliaryOnClick(MODAL_CLICK_BINDING_KEY);
            ResetTriggerCommonAttributes(binding, triggerIt->second);
        }
    }
    modalBindings_.clear();
}

void ModalCoordinator::ResetTriggerCommonAttributes(
    const ModalBinding& binding, const std::shared_ptr<Component>& triggerComponent)
{
    if (binding.hasForwardedWeight) {
        ResetTriggerLayoutWeight(triggerComponent);
    }
    if (binding.hasForwardedAccessibilityLabel) {
        ResetTriggerAccessibilityText(triggerComponent);
    }
    if (binding.hasForwardedAccessibilityDescription) {
        ResetTriggerAccessibilityDescription(triggerComponent);
    }
}

void ModalCoordinator::ApplyModalBindings(
    const std::vector<ModalDescriptor>& modalDescriptors, std::vector<ModalDescriptor>* retainedModalDescriptors)
{
    ClearModalTriggerBindings();
    if (retainedModalDescriptors != nullptr) {
        retainedModalDescriptors->clear();
    }
    if (allComponents_ == nullptr || parentsRelations_ == nullptr) {
        LOG_A2UI(LOG_WARN, "ModalCoordinator::ApplyModalBindings: component relations are unavailable");
        return;
    }

    std::set<std::string> boundTriggerIds;
    std::set<std::string> boundContentIds;

    for (const auto& descriptor : modalDescriptors) {
        ResolvedModalBinding binding = { .descriptor = &descriptor,
            .resolvedDescriptor = descriptor,
            .retainedModalDescriptors = retainedModalDescriptors };
        if (!ValidateModalDescriptorForBinding(
                descriptor, binding.resolvedDescriptor, binding.retainedModalDescriptors)) {
            continue;
        }
        if (!ResolveModalBindingComponents(binding)) {
            continue;
        }
        if (!ReserveModalBindingIds(descriptor, binding.resolvedDescriptor, binding.retainedModalDescriptors,
                boundTriggerIds, boundContentIds)) {
            continue;
        }
        ApplyResolvedModalBinding(binding);
    }

    UpdateModalPresentation();
}

void ModalCoordinator::RetainModalDescriptor(
    const ModalDescriptor& descriptor, std::vector<ModalDescriptor>* retainedModalDescriptors) const
{
    if (retainedModalDescriptors != nullptr) {
        retainedModalDescriptors->push_back(descriptor);
    }
}

bool ModalCoordinator::ValidateModalDescriptorForBinding(const ModalDescriptor& descriptor,
    ModalDescriptor& resolvedDescriptor, std::vector<ModalDescriptor>* retainedModalDescriptors) const
{
    if (!ResolveModalDescriptorIds(resolvedDescriptor)) {
        RetainModalDescriptor(descriptor, retainedModalDescriptors);
        LOG_A2UI(LOG_WARN, "Modal binding skipped: id/trigger/content is missing, modalId=%{public}s",
            descriptor.modalId.c_str());
        return false;
    }
    if (resolvedDescriptor.contentId != "root") {
        return true;
    }

    RetainModalDescriptor(descriptor, retainedModalDescriptors);
    LOG_A2UI(LOG_WARN, "Modal binding skipped: root cannot be used as modal content");
    return false;
}

bool ModalCoordinator::ResolveModalBindingComponents(ResolvedModalBinding& binding) const
{
    const ModalDescriptor& descriptor = *binding.descriptor;
    const ModalDescriptor& resolvedDescriptor = binding.resolvedDescriptor;
    auto triggerIt = allComponents_->find(resolvedDescriptor.triggerId);
    if (triggerIt == allComponents_->end()) {
        LOG_A2UI(LOG_WARN,
            "Modal binding skipped: trigger component not found, modalId=%{public}s, triggerId=%{public}s",
            resolvedDescriptor.modalId.c_str(), resolvedDescriptor.triggerId.c_str());
        return false;
    }
    auto contentIt = allComponents_->find(resolvedDescriptor.contentId);
    if (contentIt == allComponents_->end()) {
        LOG_A2UI(LOG_WARN,
            "Modal binding skipped: content component not found, modalId=%{public}s, contentId=%{public}s",
            resolvedDescriptor.modalId.c_str(), resolvedDescriptor.contentId.c_str());
        return false;
    }

    binding.triggerComponent = triggerIt->second;
    binding.clickableTrigger = std::dynamic_pointer_cast<A2UIComponent>(binding.triggerComponent);
    if (binding.clickableTrigger == nullptr) {
        RetainModalDescriptor(descriptor, binding.retainedModalDescriptors);
        LOG_A2UI(LOG_WARN, "Modal binding skipped: trigger does not support click events, triggerId=%{public}s",
            resolvedDescriptor.triggerId.c_str());
        return false;
    }

    binding.contentComponent = contentIt->second;
    if (binding.contentComponent == nullptr || binding.contentComponent->GetNativeView() == nullptr) {
        RetainModalDescriptor(descriptor, binding.retainedModalDescriptors);
        LOG_A2UI(LOG_WARN, "Modal binding skipped: content has no native view, contentId=%{public}s",
            resolvedDescriptor.contentId.c_str());
        return false;
    }
    if (parentsRelations_->find(resolvedDescriptor.contentId) == parentsRelations_->end()) {
        return true;
    }

    RetainModalDescriptor(descriptor, binding.retainedModalDescriptors);
    LOG_A2UI(LOG_WARN, "Modal binding skipped: content is already mounted in normal tree, contentId=%{public}s",
        resolvedDescriptor.contentId.c_str());
    return false;
}

bool ModalCoordinator::ReserveModalBindingIds(const ModalDescriptor& descriptor,
    const ModalDescriptor& resolvedDescriptor, std::vector<ModalDescriptor>* retainedModalDescriptors,
    std::set<std::string>& boundTriggerIds, std::set<std::string>& boundContentIds) const
{
    if (!boundTriggerIds.insert(resolvedDescriptor.triggerId).second) {
        RetainModalDescriptor(descriptor, retainedModalDescriptors);
        LOG_A2UI(LOG_WARN, "Modal binding skipped: duplicate trigger id is unsupported, triggerId=%{public}s",
            resolvedDescriptor.triggerId.c_str());
        return false;
    }
    if (boundContentIds.insert(resolvedDescriptor.contentId).second) {
        return true;
    }

    RetainModalDescriptor(descriptor, retainedModalDescriptors);
    LOG_A2UI(LOG_WARN, "Modal binding skipped: duplicate content id is unsupported, contentId=%{public}s",
        resolvedDescriptor.contentId.c_str());
    return false;
}

void ModalCoordinator::ApplyResolvedModalBinding(const ResolvedModalBinding& binding)
{
    const ModalDescriptor& descriptor = *binding.descriptor;
    const ModalDescriptor& resolvedDescriptor = binding.resolvedDescriptor;
    RetainModalDescriptor(descriptor, binding.retainedModalDescriptors);
    ApplyTriggerCommonAttributes(resolvedDescriptor, binding.triggerComponent);
    modalBindings_[resolvedDescriptor.modalId] = { .triggerId = resolvedDescriptor.triggerId,
        .contentId = resolvedDescriptor.contentId,
        .contentComponent = binding.contentComponent,
        .hasForwardedWeight = resolvedDescriptor.hasWeight,
        .hasForwardedAccessibilityLabel = resolvedDescriptor.hasAccessibilityLabel,
        .hasForwardedAccessibilityDescription = resolvedDescriptor.hasAccessibilityDescription };

    std::string modalId = resolvedDescriptor.modalId;
    binding.clickableTrigger->SetAuxiliaryOnClick(
        MODAL_CLICK_BINDING_KEY, [this, modalId]() { HandleModalTriggerClick(modalId); });
}

bool ModalCoordinator::ResolveModalDescriptorIds(ModalDescriptor& descriptor) const
{
    if (descriptor.modalId.empty()) {
        return false;
    }

    DynamicResolveContext context = { .renderId = renderId_,
        .surfaceId = surfaceId_,
        .componentId = descriptor.modalId,
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };

    if (!descriptor.triggerJsonLiteral.empty()) {
        std::optional<std::string> triggerId = ResolveDynamicStringLiteral(descriptor.triggerJsonLiteral, context);
        if (!triggerId.has_value()) {
            LOG_A2UI(LOG_WARN, "ModalCoordinator::ResolveModalDescriptorIds: trigger is invalid, modalId=%{public}s",
                descriptor.modalId.c_str());
            return false;
        }
        descriptor.triggerId = triggerId.value();
    }

    if (!descriptor.contentJsonLiteral.empty()) {
        std::optional<std::string> contentId = ResolveDynamicStringLiteral(descriptor.contentJsonLiteral, context);
        if (!contentId.has_value()) {
            LOG_A2UI(LOG_WARN, "ModalCoordinator::ResolveModalDescriptorIds: content is invalid, modalId=%{public}s",
                descriptor.modalId.c_str());
            return false;
        }
        descriptor.contentId = contentId.value();
    }

    return !descriptor.triggerId.empty() && !descriptor.contentId.empty();
}

void ModalCoordinator::ApplyTriggerCommonAttributes(
    const ModalDescriptor& descriptor, const std::shared_ptr<Component>& triggerComponent)
{
    if (triggerComponent == nullptr) {
        return;
    }

    // Apply modal public props directly to the trigger node so the feature can
    // stay local to ModalCoordinator without expanding generic Component logic.
    if (descriptor.hasWeight) {
        SetTriggerLayoutWeight(triggerComponent, descriptor.weight);
    }

    if (descriptor.hasAccessibilityLabel) {
        DynamicResolveContext context = { .renderId = renderId_,
            .surfaceId = surfaceId_,
            .componentId = descriptor.modalId,
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        std::optional<std::string> label =
            ResolveForwardedAccessibilityText(descriptor.accessibilityLabelJson, context);
        if (label.has_value()) {
            SetTriggerAccessibilityLabel(triggerComponent, label.value());
        } else {
            LOG_A2UI(LOG_WARN,
                "ModalCoordinator::ApplyTriggerCommonAttributes: accessibility.label is invalid, modalId=%{public}s",
                descriptor.modalId.c_str());
        }
    }

    if (descriptor.hasAccessibilityDescription) {
        DynamicResolveContext context = { .renderId = renderId_,
            .surfaceId = surfaceId_,
            .componentId = descriptor.modalId,
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        std::optional<std::string> description =
            ResolveForwardedAccessibilityText(descriptor.accessibilityDescriptionJson, context);
        if (description.has_value()) {
            SetTriggerAccessibilityDescription(triggerComponent, description.value());
        } else {
            LOG_A2UI(LOG_WARN,
                "ModalCoordinator::ApplyTriggerCommonAttributes: accessibility.description is invalid, "
                "modalId=%{public}s",
                descriptor.modalId.c_str());
        }
    }
}

void ModalCoordinator::UpdateModalPresentation()
{
    std::vector<DesiredDialogState> desiredDialogs;
    BuildDesiredDialogStack(desiredDialogs);

    if (dialogCloseInProgress_) {
        return;
    }

    size_t commonPrefixLength = 0;
    while (commonPrefixLength < desiredDialogs.size() && commonPrefixLength < activeDialogs_.size()) {
        const DesiredDialogState& desiredDialog = desiredDialogs[commonPrefixLength];
        const ActiveDialogState& activeDialog = activeDialogs_[commonPrefixLength];
        if (desiredDialog.modalId != activeDialog.modalId || desiredDialog.contentId != activeDialog.contentId) {
            break;
        }
        ++commonPrefixLength;
    }

    if (activeDialogs_.size() > commonPrefixLength) {
        size_t previousActiveDialogCount = activeDialogs_.size();
        if (!CloseTopActiveDialogForTransition()) {
            LOG_A2UI(LOG_ERROR, "UpdateModalPresentation: failed to close top dialog during stack transition");
        } else if (!dialogCloseInProgress_ && activeDialogs_.size() < previousActiveDialogCount) {
            UpdateModalPresentation();
        }
        return;
    }

    if (desiredDialogs.size() <= activeDialogs_.size()) {
        return;
    }

    const DesiredDialogState& nextDialog = desiredDialogs[activeDialogs_.size()];
    if (nextDialog.binding == nullptr || !PresentNativeDialog(*nextDialog.binding, nextDialog.modalId)) {
        LOG_A2UI(LOG_ERROR, "UpdateModalPresentation: failed to present native dialog, modalId=%{public}s",
            nextDialog.modalId.c_str());
        openModalStack_.resize(activeDialogs_.size());
        return;
    }

    if (!dialogCloseInProgress_ && desiredDialogs.size() > activeDialogs_.size()) {
        UpdateModalPresentation();
    }
}

bool ModalCoordinator::ConfigureNativeDialog(A2UINativeDialogHandle dialogHandle, const ModalBinding& binding,
    const std::string& modalId, DialogDismissContext* dismissContext) const
{
    if (ArkUINodeApiAdapter::DialogSetContent(dialogHandle, binding.contentComponent->GetNativeView()) !=
        A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(
            LOG_ERROR, "PresentNativeDialog failed: setContent returned error, modalId=%{public}s", modalId.c_str());
        return false;
    }
    if (ArkUINodeApiAdapter::DialogEnableCustomStyle(dialogHandle, true) != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(
            LOG_WARN, "PresentNativeDialog: enableCustomStyle returned error, modalId=%{public}s", modalId.c_str());
    }
    if (ArkUINodeApiAdapter::DialogSetContentAlignment(dialogHandle, A2UIAlignment::CENTER, 0.0f, 0.0f) !=
        A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(LOG_ERROR, "PresentNativeDialog failed: setContentAlignment returned error, modalId=%{public}s",
            modalId.c_str());
        return false;
    }
    if (ArkUINodeApiAdapter::DialogSetModalMode(dialogHandle, true) != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(
            LOG_ERROR, "PresentNativeDialog failed: setModalMode returned error, modalId=%{public}s", modalId.c_str());
        return false;
    }
    if (ArkUINodeApiAdapter::DialogSetAutoCancel(dialogHandle, true) != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(
            LOG_ERROR, "PresentNativeDialog failed: setAutoCancel returned error, modalId=%{public}s", modalId.c_str());
        return false;
    }
    if (ArkUINodeApiAdapter::DialogRegisterOnWillDismissWithUserData(
            dialogHandle, dismissContext, ModalCoordinator::OnNativeDialogWillDismiss) != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(LOG_ERROR,
            "PresentNativeDialog failed: registerOnWillDismissWithUserData returned error, modalId=%{public}s",
            modalId.c_str());
        return false;
    }
    if (ArkUINodeApiAdapter::DialogShow(dialogHandle, false) != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(LOG_ERROR, "PresentNativeDialog failed: show returned error, modalId=%{public}s", modalId.c_str());
        return false;
    }
    return true;
}

bool ModalCoordinator::PresentNativeDialog(const ModalBinding& binding, const std::string& modalId)
{
    if (binding.contentComponent == nullptr || binding.contentComponent->GetNativeView() == nullptr) {
        LOG_A2UI(
            LOG_WARN, "PresentNativeDialog skipped: content component is invalid, modalId=%{public}s", modalId.c_str());
        return false;
    }

    A2UINativeDialogHandle dialogHandle = ArkUINodeApiAdapter::DialogCreate();
    if (dialogHandle == nullptr) {
        LOG_A2UI(LOG_ERROR, "PresentNativeDialog failed: dialog create returned nullptr");
        return false;
    }

    auto* dismissContext = new DialogDismissContext { .renderId = renderId_, .surfaceId = surfaceId_ };

    auto cleanup = [dialogHandle, dismissContext]() {
        ArkUINodeApiAdapter::DialogDispose(dialogHandle);
        delete dismissContext;
    };

    if (!ConfigureNativeDialog(dialogHandle, binding, modalId, dismissContext)) {
        cleanup();
        return false;
    }

    activeDialogs_.push_back({ .handle = dialogHandle,
        .dismissContext = dismissContext,
        .modalId = modalId,
        .contentId = binding.contentId });
    dialogCloseInProgress_ = false;
    return true;
}

bool ModalCoordinator::CloseTopActiveDialogForTransition()
{
    if (activeDialogs_.empty()) {
        dialogCloseInProgress_ = false;
        return true;
    }

    ActiveDialogState activeDialog = activeDialogs_.back();
    activeDialogs_.pop_back();
    RetireDialogDismissContext(activeDialog.dismissContext);
    dialogCloseInProgress_ = true;
    int32_t closeResult = ArkUINodeApiAdapter::DialogClose(activeDialog.handle);
    if (closeResult != A2UI_ERROR_CODE_NO_ERROR) {
        LOG_A2UI(LOG_WARN, "CloseTopActiveDialogForTransition: close returned error, forcing reset");
    }

    ArkUINodeApiAdapter::DialogDispose(activeDialog.handle);

    dialogCloseInProgress_ = false;
    return true;
}

void ModalCoordinator::ForceCloseAllActiveDialogs()
{
    while (!activeDialogs_.empty()) {
        if (!CloseTopActiveDialogForTransition()) {
            break;
        }
    }
    dialogCloseInProgress_ = false;
}

void ModalCoordinator::RemoveActiveDialogState(size_t index)
{
    if (index >= activeDialogs_.size()) {
        return;
    }

    ActiveDialogState activeDialog = activeDialogs_[index];
    ArkUINodeApiAdapter::DialogDispose(activeDialog.handle);
    if (activeDialog.dismissContext != nullptr) {
        delete activeDialog.dismissContext;
    }
    activeDialogs_.erase(activeDialogs_.begin() + static_cast<std::ptrdiff_t>(index));
    dialogCloseInProgress_ = false;
}

void ModalCoordinator::RetireDialogDismissContext(DialogDismissContext* context)
{
    if (context == nullptr) {
        return;
    }
    for (DialogDismissContext* retiredContext : retiredDialogDismissContexts_) {
        if (retiredContext == context) {
            return;
        }
    }
    retiredDialogDismissContexts_.push_back(context);
}

bool ModalCoordinator::ReleaseRetiredDialogDismissContext(DialogDismissContext* context)
{
    if (context == nullptr) {
        return false;
    }
    for (auto iter = retiredDialogDismissContexts_.begin(); iter != retiredDialogDismissContexts_.end(); ++iter) {
        if (*iter != context) {
            continue;
        }
        retiredDialogDismissContexts_.erase(iter);
        delete context;
        return true;
    }
    return false;
}

void ModalCoordinator::DetachRetiredDialogDismissContexts()
{
    for (DialogDismissContext* context : retiredDialogDismissContexts_) {
        DetachDialogDismissContext(context);
    }
    retiredDialogDismissContexts_.clear();
}

void ModalCoordinator::DetachDialogDismissContext(DialogDismissContext* context)
{
    if (context == nullptr) {
        return;
    }
    auto& contexts = GetDetachedDialogDismissContexts();
    for (const auto& detachedContext : contexts) {
        if (detachedContext.get() == context) {
            return;
        }
    }
    contexts.emplace_back(context);
}

bool ModalCoordinator::ReleaseDetachedDialogDismissContext(DialogDismissContext* context)
{
    if (context == nullptr) {
        return false;
    }
    auto& contexts = GetDetachedDialogDismissContexts();
    for (auto iter = contexts.begin(); iter != contexts.end(); ++iter) {
        if (iter->get() != context) {
            continue;
        }
        contexts.erase(iter);
        return true;
    }
    return false;
}

std::vector<std::unique_ptr<ModalCoordinator::DialogDismissContext>>&
ModalCoordinator::GetDetachedDialogDismissContexts()
{
    static std::vector<std::unique_ptr<DialogDismissContext>> contexts;
    return contexts;
}

void ModalCoordinator::HandleNativeDialogDismiss(DialogDismissContext* context)
{
    if (context == nullptr) {
        return;
    }

    auto activeDialogIt = std::find_if(activeDialogs_.begin(), activeDialogs_.end(),
        [context](const ActiveDialogState& activeDialog) { return activeDialog.dismissContext == context; });
    if (activeDialogIt == activeDialogs_.end()) {
        if (!ReleaseRetiredDialogDismissContext(context) && !ReleaseDetachedDialogDismissContext(context)) {
            delete context;
        }
        return;
    }

    size_t dismissedIndex = static_cast<size_t>(std::distance(activeDialogs_.begin(), activeDialogIt));
    std::string dismissedModalId = activeDialogIt->modalId;
    RemoveActiveDialogState(dismissedIndex);

    auto desiredDialogIt = std::find(openModalStack_.begin(), openModalStack_.end(), dismissedModalId);
    if (desiredDialogIt != openModalStack_.end()) {
        openModalStack_.erase(desiredDialogIt, openModalStack_.end());
    }

    UpdateModalPresentation();
}

void ModalCoordinator::RegisterOwner()
{
    if (!HasOwnerContext()) {
        return;
    }
    GetModalCoordinatorRegistry()[BuildRegistryKey(renderId_, surfaceId_)] = this;
}

void ModalCoordinator::UnregisterOwner()
{
    if (!HasOwnerContext()) {
        return;
    }

    auto& registry = GetModalCoordinatorRegistry();
    auto iter = registry.find(BuildRegistryKey(renderId_, surfaceId_));
    if (iter != registry.end() && iter->second == this) {
        registry.erase(iter);
    }
}

bool ModalCoordinator::HasOwnerContext() const
{
    return renderId_ >= 0 && !surfaceId_.empty();
}

std::string ModalCoordinator::BuildRegistryKey(int32_t renderId, const std::string& surfaceId)
{
    return std::to_string(renderId) + ":" + surfaceId;
}

ModalCoordinator* ModalCoordinator::FindCoordinator(int32_t renderId, const std::string& surfaceId)
{
    auto& registry = GetModalCoordinatorRegistry();
    auto iter = registry.find(BuildRegistryKey(renderId, surfaceId));
    if (iter == registry.end()) {
        return nullptr;
    }
    return iter->second;
}

void ModalCoordinator::OnNativeDialogWillDismiss(A2UIDialogDismissEvent* event)
{
    // Static callback path back into the owning coordinator instance.
    if (event == nullptr) {
        return;
    }

    ArkUIOHApiAdapter::DialogDismissEventSetShouldBlockDismiss(event, false);
    auto* context = static_cast<DialogDismissContext*>(ArkUIOHApiAdapter::DialogDismissEventGetUserData(event));
    if (context == nullptr) {
        return;
    }

    ModalCoordinator* coordinator = FindCoordinator(context->renderId, context->surfaceId);
    if (coordinator == nullptr) {
        if (!ReleaseDetachedDialogDismissContext(context)) {
            delete context;
        }
        return;
    }
    coordinator->HandleNativeDialogDismiss(context);
}

} // namespace NativeModule
