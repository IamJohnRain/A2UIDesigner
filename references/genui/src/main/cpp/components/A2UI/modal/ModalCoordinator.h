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

#ifndef A2UI_MODAL_COORDINATOR_H
#define A2UI_MODAL_COORDINATOR_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../Component.h"
#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

class A2UIComponent;

class ModalCoordinator {
public:
    // Lightweight protocol description for a modal binding.
    struct ModalDescriptor {
        std::string modalId;
        std::string triggerId;
        std::string contentId;
        std::string triggerJsonLiteral;
        std::string contentJsonLiteral;
        bool hasWeight = false;
        float weight = 0.0F;
        bool hasAccessibilityLabel = false;
        std::string accessibilityLabelJson;
        bool hasAccessibilityDescription = false;
        std::string accessibilityDescriptionJson;
    };

    ModalCoordinator() = default;
    ~ModalCoordinator();

    // Parse a Modal protocol node into a descriptor and emit schema warnings when needed.
    bool TryCreateDescriptor(const JsonValue& nodeValue, ModalDescriptor& descriptor) const;

    // Set the owner identity used by the global registry and dismiss callbacks.
    void SetOwnerContext(int32_t renderId, const std::string& surfaceId);
    // Close the currently active modal, if any.
    void DismissActiveModal();
    // Release all bindings, dialog state, and owner registration.
    void Dispose();
    // Rebuild modal bindings from the latest component graph.
    void HandlePendingModalDescriptors(const std::vector<ModalDescriptor>& modalDescriptors,
        const std::map<std::string, std::shared_ptr<Component>>& allComponents,
        const std::map<std::string, std::string>& parentsRelations);
    // Re-resolve the latest modal descriptors against the current data model.
    void RefreshModalBindings();

#ifdef TDD_BUILD
    size_t GetBindingCountForTest() const
    {
        return modalBindings_.size();
    }
    std::string GetBindingTriggerIdForTest(const std::string& modalId) const
    {
        auto iter = modalBindings_.find(modalId);
        return iter == modalBindings_.end() ? "" : iter->second.triggerId;
    }
    std::string GetBindingContentIdForTest(const std::string& modalId) const
    {
        auto iter = modalBindings_.find(modalId);
        return iter == modalBindings_.end() ? "" : iter->second.contentId;
    }
    std::string GetActiveDialogModalIdForTest() const
    {
        return activeDialogs_.empty() ? "" : activeDialogs_.back().modalId;
    }
    size_t GetOpenModalCountForTest() const
    {
        return openModalStack_.size();
    }
    size_t GetActiveDialogCountForTest() const
    {
        return activeDialogs_.size();
    }
    bool IsDialogCloseInProgressForTest() const
    {
        return dialogCloseInProgress_;
    }
    void RequestOpenModalForTest(const std::string& modalId);
#endif

private:
    // Runtime binding from modal id to trigger/content components.
    struct ModalBinding {
        std::string triggerId;
        std::string contentId;
        std::shared_ptr<Component> contentComponent;
        bool hasForwardedWeight = false;
        bool hasForwardedAccessibilityLabel = false;
        bool hasForwardedAccessibilityDescription = false;
    };

    struct ResolvedModalBinding {
        const ModalDescriptor* descriptor = nullptr;
        ModalDescriptor resolvedDescriptor;
        std::vector<ModalDescriptor>* retainedModalDescriptors = nullptr;
        std::shared_ptr<Component> triggerComponent;
        std::shared_ptr<A2UIComponent> clickableTrigger;
        std::shared_ptr<Component> contentComponent;
    };

    // Owner data passed through native dismiss callbacks.
    struct DialogDismissContext {
        int32_t renderId = -1;
        std::string surfaceId;
    };

    struct ActiveDialogState {
        A2UINativeDialogHandle handle = nullptr;
        DialogDismissContext* dismissContext = nullptr;
        std::string modalId;
        std::string contentId;
    };

    struct DesiredDialogState {
        std::string modalId;
        std::string contentId;
        const ModalBinding* binding = nullptr;
    };

    // Parse the optional accessibility object (label/description) into the descriptor.
    void ParseAccessibility(const JsonValue& nodeValue, ModalDescriptor& descriptor) const;

    // Remove all auxiliary click bindings used for modal triggers.
    void ClearModalTriggerBindings();
    // Remove modal-forwarded common attributes from an old trigger.
    void ResetTriggerCommonAttributes(const ModalBinding& binding, const std::shared_ptr<Component>& triggerComponent);
    // Validate descriptors and attach runtime trigger callbacks.
    void ApplyModalBindings(const std::vector<ModalDescriptor>& modalDescriptors,
        std::vector<ModalDescriptor>* retainedModalDescriptors = nullptr);
    void RetainModalDescriptor(
        const ModalDescriptor& descriptor, std::vector<ModalDescriptor>* retainedModalDescriptors) const;
    bool ValidateModalDescriptorForBinding(const ModalDescriptor& descriptor, ModalDescriptor& resolvedDescriptor,
        std::vector<ModalDescriptor>* retainedModalDescriptors) const;
    bool ResolveModalBindingComponents(ResolvedModalBinding& binding) const;
    bool ReserveModalBindingIds(const ModalDescriptor& descriptor, const ModalDescriptor& resolvedDescriptor,
        std::vector<ModalDescriptor>* retainedModalDescriptors, std::set<std::string>& boundTriggerIds,
        std::set<std::string>& boundContentIds) const;
    void ApplyResolvedModalBinding(const ResolvedModalBinding& binding);
    // Resolve dynamic trigger/content ids in a descriptor.
    bool ResolveModalDescriptorIds(ModalDescriptor& descriptor) const;
    // Apply forwarded modal common attributes onto the trigger component.
    void ApplyTriggerCommonAttributes(
        const ModalDescriptor& descriptor, const std::shared_ptr<Component>& triggerComponent);
    // Update the desired modal stack in response to a trigger click.
    void HandleModalTriggerClick(const std::string& modalId);
    // Build the desired dialog stack from active modal ids and current bindings.
    void BuildDesiredDialogStack(std::vector<DesiredDialogState>& desiredDialogs);
    // Reconcile the desired modal state with the active native dialog.
    void UpdateModalPresentation();
    // Create and show the native dialog for the target modal.
    bool PresentNativeDialog(const ModalBinding& binding, const std::string& modalId);
    // Apply dialog content/style/mode/cancel/dismiss/show settings; returns false on the first failure.
    bool ConfigureNativeDialog(A2UINativeDialogHandle dialogHandle, const ModalBinding& binding,
        const std::string& modalId, DialogDismissContext* dismissContext) const;
    // Close and release the top-most native dialog before presenting another modal.
    bool CloseTopActiveDialogForTransition();
    // Force close every active dialog in the stack.
    void ForceCloseAllActiveDialogs();
    // Dispose dialog resources and remove the active dialog at the given index.
    void RemoveActiveDialogState(size_t index);
    // Keep old dismiss contexts alive when native close does not synchronously call back.
    void RetireDialogDismissContext(DialogDismissContext* context);
    // Release a retired dismiss context if a late native callback arrives.
    bool ReleaseRetiredDialogDismissContext(DialogDismissContext* context);
    // Detach retired contexts that may still be referenced by native callbacks.
    void DetachRetiredDialogDismissContexts();
    // Keep a detached dismiss context alive after this coordinator is disposed.
    static void DetachDialogDismissContext(DialogDismissContext* context);
    // Release a detached dismiss context when its late native callback arrives.
    static bool ReleaseDetachedDialogDismissContext(DialogDismissContext* context);
    // Store detached contexts that are still owned by native dialog callbacks.
    static std::vector<std::unique_ptr<DialogDismissContext>>& GetDetachedDialogDismissContexts();
    // Handle native dismiss completion and continue pending transitions.
    void HandleNativeDialogDismiss(DialogDismissContext* context);
    // Register this coordinator in the global registry.
    void RegisterOwner();
    // Remove this coordinator from the global registry.
    void UnregisterOwner();
    // Return whether owner identity is complete enough for registration.
    bool HasOwnerContext() const;

    // Build the registry key from render and surface identity.
    static std::string BuildRegistryKey(int32_t renderId, const std::string& surfaceId);
    // Find a coordinator instance from the global registry.
    static ModalCoordinator* FindCoordinator(int32_t renderId, const std::string& surfaceId);
    // Static native dismiss callback entry point.
    static void OnNativeDialogWillDismiss(A2UIDialogDismissEvent* event);

    const std::map<std::string, std::shared_ptr<Component>>* allComponents_ = nullptr;
    const std::map<std::string, std::string>* parentsRelations_ = nullptr;
    std::vector<ModalDescriptor> modalDescriptors_;
    std::unordered_map<std::string, ModalBinding> modalBindings_;
    int32_t renderId_ = -1;
    std::string surfaceId_;
    std::vector<std::string> openModalStack_;
    std::vector<ActiveDialogState> activeDialogs_;
    bool dialogCloseInProgress_ = false;
    std::vector<DialogDismissContext*> retiredDialogDismissContexts_;
};

} // namespace NativeModule

#endif // A2UI_MODAL_COORDINATOR_H
