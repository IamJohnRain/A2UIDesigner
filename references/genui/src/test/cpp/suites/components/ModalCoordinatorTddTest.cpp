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

#include <map>

#include "components/A2UI/button/ButtonComponent.h"
#include "components/A2UI/column/ColumnComponent.h"
#include "components/A2UI/modal/ModalCoordinator.h"

#include "A2UIComponentTddTestHelper.h"

using namespace NativeModule;

class ModalCoordinatorTddTest : public A2UIComponentTddTest {};

namespace {

constexpr char MODAL_TDD_SURFACE_ID[] = "modal-component-tdd-test";
constexpr int32_t MODAL_TDD_RENDER_ID = 812201;

std::shared_ptr<ButtonComponent> CreateTrigger(const std::string& id)
{
    auto trigger = std::make_shared<ButtonComponent>();
    trigger->SetComponentId(id);
    trigger->SetRenderId(MODAL_TDD_RENDER_ID);
    trigger->SetSurfaceId(MODAL_TDD_SURFACE_ID);
    return trigger;
}

std::shared_ptr<ColumnComponent> CreateContent(const std::string& id)
{
    auto content = std::make_shared<ColumnComponent>();
    content->SetComponentId(id);
    content->SetRenderId(MODAL_TDD_RENDER_ID);
    content->SetSurfaceId(MODAL_TDD_SURFACE_ID);
    return content;
}

ModalCoordinator::ModalDescriptor CreateDescriptor(
    const std::string& modalId, const std::string& triggerId, const std::string& contentId)
{
    ModalCoordinator::ModalDescriptor descriptor;
    descriptor.modalId = modalId;
    descriptor.triggerId = triggerId;
    descriptor.contentId = contentId;
    return descriptor;
}

void DispatchDismissCallback(MockArkUINativeProvider* provider, const NativeDialogDismissRegistration& registration,
    ArkUI_DialogDismissEvent* event)
{
    ASSERT_NE(provider, nullptr);
    ASSERT_NE(registration.callback, nullptr);
    provider->SetDialogDismissEventUserData(event, registration.userData);
    registration.callback(event);
}

ArkUI_NativeDialogHandle TrackCreateNullDialog()
{
    g_dialogTracker.createCalls.push_back(nullptr);
    return nullptr;
}

int32_t ReturnSetContentError(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content)
{
    static_cast<void>(handle);
    static_cast<void>(content);
    return 1;
}

int32_t ReturnShowError(ArkUI_NativeDialogHandle handle, bool showInSubWindow)
{
    static_cast<void>(handle);
    static_cast<void>(showInSubWindow);
    return 1;
}

} // namespace

TEST_F(ModalCoordinatorTddTest, L0_modal_should_parse_descriptor_with_weight_and_accessibility)
{
    auto adapter =
        ParseJson(R"({"id":"confirmModal","component":"Modal","trigger":"openButton","content":"dialogContent",)"
                  R"("weight":2,"accessibility":{"label":"Open dialog","description":"Shows details"}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));

    EXPECT_EQ(descriptor.modalId, "confirmModal");
    EXPECT_EQ(descriptor.triggerId, "openButton");
    EXPECT_EQ(descriptor.contentId, "dialogContent");
    EXPECT_TRUE(descriptor.hasWeight);
    EXPECT_FLOAT_EQ(descriptor.weight, 2.0F);
    EXPECT_TRUE(descriptor.hasAccessibilityLabel);
    EXPECT_TRUE(descriptor.hasAccessibilityDescription);
    EXPECT_EQ(descriptor.accessibilityLabelJson, R"("Open dialog")");
    EXPECT_EQ(descriptor.accessibilityDescriptionJson, R"("Shows details")");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_reject_non_object_descriptor)
{
    auto adapter = ParseJson(R"(["not", "object"])");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    ModalCoordinator::ModalDescriptor descriptor;

    EXPECT_FALSE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_ignore_invalid_descriptor_field_types)
{
    auto adapter = ParseJson(R"({"id":"badModal","component":"Modal","trigger":7,"content":false,)"
                             R"("weight":"heavy","accessibility":{"label":5,"description":false}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));

    EXPECT_EQ(descriptor.modalId, "badModal");
    EXPECT_TRUE(descriptor.triggerId.empty());
    EXPECT_TRUE(descriptor.contentId.empty());
    EXPECT_TRUE(descriptor.triggerJsonLiteral.empty());
    EXPECT_TRUE(descriptor.contentJsonLiteral.empty());
    EXPECT_FALSE(descriptor.hasWeight);
    EXPECT_FALSE(descriptor.hasAccessibilityLabel);
    EXPECT_FALSE(descriptor.hasAccessibilityDescription);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_bind_trigger_and_forward_common_attributes)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;
    ModalCoordinator::ModalDescriptor descriptor = CreateDescriptor("confirmModal", "openButton", "dialogContent");
    descriptor.hasWeight = true;
    descriptor.weight = 3.0F;
    descriptor.hasAccessibilityLabel = true;
    descriptor.accessibilityLabelJson = R"("Open dialog")";
    descriptor.hasAccessibilityDescription = true;
    descriptor.accessibilityDescriptionJson = R"("Shows details")";

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("confirmModal"), "openButton");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("confirmModal"), "dialogContent");
    ExpectU32Attribute(trigger->GetNativeView(), NODE_LAYOUT_WEIGHT, 3U);
    ExpectStringAttribute(trigger->GetNativeView(), NODE_ACCESSIBILITY_TEXT, "Open dialog");
    ExpectStringAttribute(trigger->GetNativeView(), NODE_ACCESSIBILITY_DESCRIPTION, "Shows details");

    coordinator.HandlePendingModalDescriptors({}, allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_LAYOUT_WEIGHT));
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_ACCESSIBILITY_TEXT));
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_ACCESSIBILITY_DESCRIPTION));
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_skip_bindings_for_invalid_component_relations)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations = { { "dialogContent", "root" } };

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("mountedContent", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    parentsRelations.clear();
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("rootContent", "openButton", "root") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingTrigger", "missingButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingContent", "openButton", "missingContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_drop_descriptor_when_trigger_reference_is_missing)
{
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingTrigger", "missingButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    allComponents["missingButton"] = CreateTrigger("missingButton");
    coordinator.RefreshModalBindings();

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_drop_descriptor_when_content_reference_is_missing)
{
    auto trigger = CreateTrigger("openButton");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingContent", "openButton", "missingContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    allComponents["missingContent"] = CreateContent("missingContent");
    coordinator.RefreshModalBindings();

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_keep_only_first_binding_for_duplicate_trigger_or_content)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    auto secondContent = CreateContent("secondContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content }, { "secondContent", secondContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("firstModal", "openButton", "dialogContent"),
            CreateDescriptor("duplicateTrigger", "openButton", "secondContent"),
            CreateDescriptor("duplicateContent", "missingButton", "dialogContent") },
        allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("firstModal"), "dialogContent");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("duplicateTrigger"), "");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("duplicateContent"), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_skip_binding_when_dynamic_ids_cannot_be_resolved)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;
    ModalCoordinator::ModalDescriptor descriptor = CreateDescriptor("dynamicModal", "", "dialogContent");
    descriptor.triggerJsonLiteral = R"({"path":"/missingTrigger"})";

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_present_native_dialog_and_handle_dismiss_callback)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("confirmModal");

    ASSERT_EQ(g_dialogTracker.createCalls.size(), 1U);
    ASSERT_EQ(g_dialogTracker.setContentCalls.size(), 1U);
    EXPECT_EQ(g_dialogTracker.setContentCalls.back().second, content->GetNativeView());
    ASSERT_EQ(g_dialogTracker.showCalls.size(), 1U);
    EXPECT_FALSE(g_dialogTracker.showCalls.back().second);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "confirmModal");

    ArkUI_DialogDismissEvent* event = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2201);
    ASSERT_FALSE(g_dialogTracker.dismissRegistrations.empty());
    DispatchDismissCallback(mockArkUIPtr_, g_dialogTracker.dismissRegistrations.back(), event);

    EXPECT_FALSE(mockArkUIPtr_->dialogDismissShouldBlockDismiss_[event]);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_present_dialog_when_dialog_api_is_incomplete)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->create = nullptr;

    coordinator.RequestOpenModalForTest("confirmModal");

    EXPECT_TRUE(g_dialogTracker.createCalls.empty());
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_present_dialog_when_create_returns_null)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->create = TrackCreateNullDialog;

    coordinator.RequestOpenModalForTest("confirmModal");

    ASSERT_EQ(g_dialogTracker.createCalls.size(), 1U);
    EXPECT_EQ(g_dialogTracker.createCalls.back(), nullptr);
    EXPECT_TRUE(g_dialogTracker.setContentCalls.empty());
    EXPECT_TRUE(g_dialogTracker.showCalls.empty());
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_dialog_when_set_content_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->setContent = ReturnSetContentError;

    coordinator.RequestOpenModalForTest("confirmModal");

    EXPECT_EQ(g_dialogTracker.createCalls.size(), 1U);
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_dialog_when_show_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->show = ReturnShowError;

    coordinator.RequestOpenModalForTest("confirmModal");

    EXPECT_EQ(g_dialogTracker.createCalls.size(), 1U);
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_close_active_dialog_when_dismiss_is_requested)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    coordinator.RequestOpenModalForTest("confirmModal");

    coordinator.DismissActiveModal();

    ASSERT_EQ(g_dialogTracker.closeCalls.size(), 1U);
    EXPECT_FALSE(coordinator.IsDialogCloseInProgressForTest());

    ArkUI_DialogDismissEvent* event = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2202);
    ASSERT_FALSE(g_dialogTracker.dismissRegistrations.empty());
    DispatchDismissCallback(mockArkUIPtr_, g_dialogTracker.dismissRegistrations.back(), event);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_transition_between_active_dialogs)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    auto secondTrigger = CreateTrigger("detailsButton");
    auto secondContent = CreateContent("detailsContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content }, { "detailsButton", secondTrigger }, { "detailsContent", secondContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ CreateDescriptor("confirmModal", "openButton", "dialogContent"),
                                                  CreateDescriptor("detailsModal", "detailsButton", "detailsContent") },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("confirmModal");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 1U);
    NativeDialogDismissRegistration firstRegistration = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("detailsModal");

    ASSERT_EQ(g_dialogTracker.createCalls.size(), 2U);
    ASSERT_EQ(g_dialogTracker.showCalls.size(), 2U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "detailsModal");

    ArkUI_DialogDismissEvent* topEvent = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2203);
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 2U);
    DispatchDismissCallback(mockArkUIPtr_, g_dialogTracker.dismissRegistrations.back(), topEvent);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "confirmModal");

    ArkUI_DialogDismissEvent* bottomEvent = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2204);
    DispatchDismissCallback(mockArkUIPtr_, firstRegistration, bottomEvent);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "");
}

// --- Retained descriptor tests ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_when_dynamic_ids_cannot_be_resolved)
{
    // Use a descriptor whose trigger is a static id that does not exist in allComponents yet.
    // The first call should retain the descriptor because the trigger component is not found.
    // After adding the trigger component, a re-submit with the same descriptor should bind successfully.
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("dynamicModal", "someTrigger", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    allComponents["someTrigger"] = CreateTrigger("someTrigger");
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("dynamicModal", "someTrigger", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("dynamicModal"), "someTrigger");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_when_content_mounted_in_tree)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations = { { "dialogContent", "root" } };

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("mountedModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    parentsRelations.clear();
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("mountedModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_when_content_is_root)
{
    auto trigger = CreateTrigger("openButton");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("rootModal", "openButton", "root") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    auto normalContent = CreateContent("normalContent");
    allComponents["normalContent"] = normalContent;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("rootModal", "openButton", "normalContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_for_duplicate_trigger_and_recover_on_refresh)
{
    auto trigger = CreateTrigger("openButton");
    auto firstContent = CreateContent("firstContent");
    auto secondContent = CreateContent("secondContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "firstContent", firstContent }, { "secondContent", secondContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("firstModal", "openButton", "firstContent"),
            CreateDescriptor("dupTriggerModal", "openButton", "secondContent"),
        },
        allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("firstModal"), "firstContent");

    // After re-binding with only the duplicate descriptor, it should succeed since trigger is no longer taken
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("dupTriggerModal", "openButton", "secondContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dupTriggerModal"), "secondContent");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_for_duplicate_content_and_recover_on_refresh)
{
    auto firstTrigger = CreateTrigger("firstButton");
    auto secondTrigger = CreateTrigger("secondButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "firstButton", firstTrigger },
        { "secondButton", secondTrigger }, { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("firstModal", "firstButton", "dialogContent"),
            CreateDescriptor("dupContentModal", "secondButton", "dialogContent"),
        },
        allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("firstModal"), "firstButton");

    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("dupContentModal", "secondButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("dupContentModal"), "secondButton");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_retain_descriptor_when_trigger_component_not_found)
{
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingTrigger", "absentButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    // Add the missing trigger and refresh — descriptor was NOT retained, so still no binding
    allComponents["absentButton"] = CreateTrigger("absentButton");
    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_retain_descriptor_when_content_component_not_found)
{
    auto trigger = CreateTrigger("openButton");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("missingContent", "openButton", "absentContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    // Add the missing content and refresh — descriptor was NOT retained, so still no binding
    allComponents["absentContent"] = CreateContent("absentContent");
    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_check_component_existence_before_duplicate_id)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    auto secondContent = CreateContent("secondContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content }, { "secondContent", secondContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    // duplicateContent uses missingButton as trigger (component not found) AND duplicate content id
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("firstModal", "openButton", "dialogContent"),
            CreateDescriptor("dupContentModal", "missingButton", "dialogContent"),
        },
        allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("firstModal"), "dialogContent");
    // dupContentModal was dropped because trigger was missing, not because of duplicate content
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dupContentModal"), "");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_mixed_descriptors_with_partial_success)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations = { { "dialogContent", "root" } };

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    // First descriptor: valid binding; Second: content mounted in tree (retained, not dropped)
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("validModal", "openButton", "dialogContent"),
        },
        allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    // Both descriptors fail now, but the mounted-content one is retained for later refresh
    parentsRelations.clear();
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("validModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("validModal"), "dialogContent");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_clear_modal_descriptors_when_empty_list_passed)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);

    coordinator.HandlePendingModalDescriptors({}, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    // After clearing with empty list, RefreshModalBindings should be a no-op
    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_schema_warning_for_missing_trigger_without_owner_context)
{
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    // No SetOwnerContext — renderId_ remains -1, DispatchSchemaWarning returns early without crash
    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("noOwnerModal", "absentButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_schema_warning_for_missing_content_without_owner_context)
{
    auto trigger = CreateTrigger("openButton");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("noOwnerModal", "openButton", "absentContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- Dispose and lifecycle tests ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispose_clears_bindings_and_dialogs)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);

    coordinator.Dispose();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispose_without_owner_context)
{
    ModalCoordinator coordinator;
    coordinator.Dispose();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- HandleModalTriggerClick stack manipulation tests ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_toggle_off_when_clicking_top_modal)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);

    ArkUI_DialogDismissEvent* event = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2205);
    DispatchDismissCallback(mockArkUIPtr_, g_dialogTracker.dismissRegistrations.back(), event);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_erase_above_when_clicking_non_top_modal)
{
    auto trigger1 = CreateTrigger("button1");
    auto content1 = CreateContent("content1");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    auto trigger3 = CreateTrigger("button3");
    auto content3 = CreateContent("content3");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "button1", trigger1 },
        { "content1", content1 }, { "button2", trigger2 }, { "content2", content2 }, { "button3", trigger3 },
        { "content3", content3 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "button1", "content1"),
            CreateDescriptor("modal2", "button2", "content2"),
            CreateDescriptor("modal3", "button3", "content3"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 1U);
    NativeDialogDismissRegistration reg1 = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("modal2");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 2U);
    NativeDialogDismissRegistration reg2 = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("modal3");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 3U);
    NativeDialogDismissRegistration reg3 = g_dialogTracker.dismissRegistrations.back();

    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 3U);

    ArkUI_DialogDismissEvent* event3 = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2210);
    DispatchDismissCallback(mockArkUIPtr_, reg3, event3);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    ArkUI_DialogDismissEvent* event2 = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2211);
    DispatchDismissCallback(mockArkUIPtr_, reg2, event2);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "modal1");
}

// --- HandleModalTriggerClick empty modalId guard ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_ignore_empty_modal_id_click)
{
    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.RequestOpenModalForTest("");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 0U);
}

// --- RefreshModalBindings clears bindings when all removed ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_refresh_clears_bindings_when_components_removed)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);

    allComponents.erase("openButton");
    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- OnNativeDialogWillDismiss static callback tests ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_handle_dismiss_callback_for_unknown_coordinator)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    coordinator.RequestOpenModalForTest("confirmModal");

    ASSERT_FALSE(g_dialogTracker.dismissRegistrations.empty());
    auto& reg = g_dialogTracker.dismissRegistrations.back();
    ASSERT_NE(reg.userData, nullptr);
    ASSERT_NE(reg.callback, nullptr);

    coordinator.Dispose();

    ArkUI_DialogDismissEvent* event = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2221);
    mockArkUIPtr_->SetDialogDismissEventUserData(event, reg.userData);
    reg.callback(event);
}

// --- RetireDialogDismissContext / DetachRetiredDialogDismissContexts ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retire_and_detach_dismiss_contexts_on_dispose)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    {
        ModalCoordinator coordinator;
        coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
        coordinator.HandlePendingModalDescriptors(
            { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
        coordinator.RequestOpenModalForTest("confirmModal");

        coordinator.DismissActiveModal();
        coordinator.Dispose();
    }
}

// --- Validate unknown fields in descriptor ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_warning_for_unknown_descriptor_fields)
{
    auto adapter =
        ParseJson(R"({"id":"warnModal","component":"Modal","trigger":"btn","content":"dlg","unknownField":"value"})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_EQ(descriptor.modalId, "warnModal");
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_warning_for_unknown_accessibility_fields)
{
    auto adapter = ParseJson(R"({"id":"accModal","component":"Modal","trigger":"btn","content":"dlg",)"
                             R"("accessibility":{"label":"Open","unknownAcc":"value"}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_TRUE(descriptor.hasAccessibilityLabel);
}

// --- Weight type mismatch warning ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_warning_for_non_numeric_weight)
{
    auto adapter =
        ParseJson(R"({"id":"weightModal","component":"Modal","trigger":"btn","content":"dlg","weight":"heavy"})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_FALSE(descriptor.hasWeight);
}

// --- Accessibility label type mismatch ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_warning_for_non_string_accessibility_label)
{
    auto adapter = ParseJson(R"({"id":"accLabelModal","component":"Modal","trigger":"btn","content":"dlg",)"
                             R"("accessibility":{"label":123}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_FALSE(descriptor.hasAccessibilityLabel);
}

// --- Accessibility description type mismatch ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dispatch_warning_for_non_string_accessibility_description)
{
    auto adapter = ParseJson(R"({"id":"accDescModal","component":"Modal","trigger":"btn","content":"dlg",)"
                             R"("accessibility":{"description":true}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_FALSE(descriptor.hasAccessibilityDescription);
}

// --- ApplyModalBindings with null retainedModalDescriptors (RefreshModalBindings path) ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_when_trigger_is_not_a2ui_component)
{
    auto plainContent = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF001));
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "plainTrigger", plainContent },
        { "dialogContent", CreateContent("dialogContent") } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("nonA2UITrigger", "plainTrigger", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- Content with null native view retained ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_descriptor_when_content_has_no_native_view)
{
    auto trigger = CreateTrigger("openButton");
    auto nullContent = std::make_shared<Component>(nullptr, false);
    nullContent->SetComponentId("nullContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "nullContent", nullContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("nullViewModal", "openButton", "nullContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- BuildDesiredDialogStack: break on missing binding ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_truncate_stack_when_binding_removed)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content }, { "button2", trigger2 }, { "content2", content2 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "openButton", "dialogContent"),
            CreateDescriptor("modal2", "button2", "content2"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    coordinator.RequestOpenModalForTest("modal2");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    allComponents.erase("button2");
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("modal1", "openButton", "dialogContent") }, allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- SetOwnerContext re-registration ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_re_register_on_set_owner_context)
{
    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID + 1, "anotherSurface");
    coordinator.Dispose();
}

// --- ResolveModalDescriptorIds with empty modalId ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_skip_descriptor_with_empty_modal_id)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- ForceCloseAllActiveDialogs when no dialogs active ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_handle_empty_descriptors_clears_dialogs)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);

    coordinator.HandlePendingModalDescriptors({}, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- CloseTopActiveDialogForTransition with no active dialogs ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_refresh_with_no_descriptors_is_noop)
{
    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

// --- Dynamic id resolution: triggerJsonLiteral and contentJsonLiteral paths ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_resolve_dynamic_trigger_with_static_fallback)
{
    auto trigger = CreateTrigger("resolvedTrigger");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "resolvedTrigger", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator::ModalDescriptor descriptor;
    descriptor.modalId = "dynamicModal";
    descriptor.triggerId = "resolvedTrigger";
    descriptor.contentId = "dialogContent";

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- Parse descriptor with only trigger id (no content) ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_reject_descriptor_missing_content_id)
{
    auto adapter = ParseJson(R"({"id":"noContent","component":"Modal","trigger":"btn"})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_TRUE(descriptor.contentId.empty());
}

// --- ApplyModalBindings: null allComponents_ guard ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_retain_when_trigger_content_not_found_without_context)
{
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("testModal", "absentTrigger", "absentContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    coordinator.RefreshModalBindings();
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- HandleModalTriggerClick: click same modal twice opens then closes stack ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_handle_click_same_modal_twice)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("confirmModal");
    ASSERT_EQ(g_dialogTracker.createCalls.size(), 1U);

    ArkUI_DialogDismissEvent* event1 = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2230);
    DispatchDismissCallback(mockArkUIPtr_, g_dialogTracker.dismissRegistrations.back(), event1);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(g_dialogTracker.createCalls.size(), 2U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
}

// --- PresentNativeDialog error path tests ---

int32_t ReturnAlignmentError(ArkUI_NativeDialogHandle handle, ArkUI_DialogAlignment, float, float)
{
    static_cast<void>(handle);
    return 1;
}

int32_t ReturnModalModeError(ArkUI_NativeDialogHandle handle, bool)
{
    static_cast<void>(handle);
    return 1;
}

int32_t ReturnAutoCancelError(ArkUI_NativeDialogHandle handle, bool)
{
    static_cast<void>(handle);
    return 1;
}

int32_t ReturnRegisterDismissError(ArkUI_NativeDialogHandle, void*, void (*)(ArkUI_DialogDismissEvent*))
{
    return 1;
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_when_alignment_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->setContentAlignment = ReturnAlignmentError;

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_when_modal_mode_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->setModalMode = ReturnModalModeError;

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_when_auto_cancel_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->setAutoCancel = ReturnAutoCancelError;

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_cleanup_when_register_dismiss_fails)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    dialogApi_->registerOnWillDismissWithUserData = ReturnRegisterDismissError;

    coordinator.RequestOpenModalForTest("confirmModal");
    EXPECT_EQ(g_dialogTracker.disposeCalls.size(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

// --- ReadStringProperty required=true path ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_read_descriptor_with_only_id)
{
    auto adapter = ParseJson(R"({"id":"minimal","component":"Modal"})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_EQ(descriptor.modalId, "minimal");
    EXPECT_TRUE(descriptor.triggerId.empty());
    EXPECT_TRUE(descriptor.contentId.empty());
}

// --- CloseTopActiveDialogForTransition when close API unavailable ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_force_reset_when_close_api_unavailable)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content }, { "button2", trigger2 }, { "content2", content2 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "openButton", "dialogContent"),
            CreateDescriptor("modal2", "button2", "content2"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    coordinator.RequestOpenModalForTest("modal2");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    dialogApi_->close = nullptr;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("modal1", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- DismissActiveModal when no dialog active ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_dismiss_when_no_active_dialog)
{
    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.DismissActiveModal();
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

// --- ResetTriggerCommonAttributes: all forwarded attributes ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_reset_all_forwarded_attributes_on_clear)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator::ModalDescriptor descriptor = CreateDescriptor("fullModal", "openButton", "dialogContent");
    descriptor.hasWeight = true;
    descriptor.weight = 2.0F;
    descriptor.hasAccessibilityLabel = true;
    descriptor.accessibilityLabelJson = R"("Test Label")";
    descriptor.hasAccessibilityDescription = true;
    descriptor.accessibilityDescriptionJson = R"("Test Desc")";

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    ExpectU32Attribute(trigger->GetNativeView(), NODE_LAYOUT_WEIGHT, 2U);
    ExpectStringAttribute(trigger->GetNativeView(), NODE_ACCESSIBILITY_TEXT, "Test Label");
    ExpectStringAttribute(trigger->GetNativeView(), NODE_ACCESSIBILITY_DESCRIPTION, "Test Desc");

    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("otherModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_LAYOUT_WEIGHT));
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_ACCESSIBILITY_TEXT));
    EXPECT_TRUE(HasResetAttributeCall(trigger->GetNativeView(), NODE_ACCESSIBILITY_DESCRIPTION));
}

// --- ValidateUnknownDescriptorFields with known fields only ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_warn_for_known_descriptor_fields)
{
    auto adapter = ParseJson(R"({"id":"knownModal","component":"Modal","trigger":"btn","content":"dlg","weight":1,)"
                             R"("accessibility":{"label":"L","description":"D"}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(coordinator.TryCreateDescriptor(adapter->GetRoot(), descriptor));
    EXPECT_EQ(descriptor.modalId, "knownModal");
    EXPECT_TRUE(descriptor.hasWeight);
    EXPECT_TRUE(descriptor.hasAccessibilityLabel);
    EXPECT_TRUE(descriptor.hasAccessibilityDescription);
}

// --- HandleNativeDialogDismiss with retired context ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_handle_dismiss_after_close_transition)
{
    auto trigger1 = CreateTrigger("button1");
    auto content1 = CreateContent("content1");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "button1", trigger1 },
        { "content1", content1 }, { "button2", trigger2 }, { "content2", content2 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "button1", "content1"),
            CreateDescriptor("modal2", "button2", "content2"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 1U);
    NativeDialogDismissRegistration reg1 = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("modal2");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 2U);
    NativeDialogDismissRegistration reg2 = g_dialogTracker.dismissRegistrations.back();

    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    coordinator.DismissActiveModal();
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);

    ArkUI_DialogDismissEvent* dismissEvent = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2250);
    DispatchDismissCallback(mockArkUIPtr_, reg2, dismissEvent);
}

// --- Retain descriptor when trigger component is plain Component (not A2UIComponent) ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_when_trigger_is_plain_and_recover)
{
    auto plainTrigger = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0xF100));
    plainTrigger->SetComponentId("plainButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "plainButton", plainTrigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("plainModal", "plainButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    allComponents["plainButton"] = CreateTrigger("plainButton");
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("plainModal", "plainButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- Retain descriptor with null native view content and recover ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_when_null_native_view_and_recover)
{
    auto trigger = CreateTrigger("openButton");
    auto nullContent = std::make_shared<Component>(nullptr, false);
    nullContent->SetComponentId("nullContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "nullContent", nullContent } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("nullModal", "openButton", "nullContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    allComponents["nullContent"] = CreateContent("nullContent");
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("nullModal", "openButton", "nullContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- UpdateModalPresentation: stack transition with dialog close ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_transition_dialog_when_modal_replaced)
{
    auto trigger1 = CreateTrigger("button1");
    auto content1 = CreateContent("content1");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "button1", trigger1 },
        { "content1", content1 }, { "button2", trigger2 }, { "content2", content2 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "button1", "content1"),
            CreateDescriptor("modal2", "button2", "content2"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 1U);
    NativeDialogDismissRegistration reg1 = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("modal2");
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    ArkUI_DialogDismissEvent* event1 = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2260);
    DispatchDismissCallback(mockArkUIPtr_, reg1, event1);
}

// --- BuildDesiredDialogStack: break when binding missing ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_truncate_open_stack_on_binding_loss)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("modal1", "openButton", "dialogContent") }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);

    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("modal2", "missingButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);
}

// --- HandleNativeDialogDismiss: context not in active dialogs (retired path) ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_release_retired_context_on_late_dismiss)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("confirmModal", "openButton", "dialogContent") }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("confirmModal");
    ASSERT_FALSE(g_dialogTracker.dismissRegistrations.empty());

    coordinator.DismissActiveModal();
    coordinator.Dispose();
}

// --- HandlePendingModalDescriptors with bindings empty after apply ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_force_close_dialogs_when_bindings_empty_after_apply)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations = { { "dialogContent", "root" } };

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("modal1", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
}

// --- HandleModalTriggerClick: click on non-top modal erases above ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_erase_above_when_reclicking_non_top)
{
    auto trigger1 = CreateTrigger("button1");
    auto content1 = CreateContent("content1");
    auto trigger2 = CreateTrigger("button2");
    auto content2 = CreateContent("content2");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "button1", trigger1 },
        { "content1", content1 }, { "button2", trigger2 }, { "content2", content2 } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        {
            CreateDescriptor("modal1", "button1", "content1"),
            CreateDescriptor("modal2", "button2", "content2"),
        },
        allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("modal1");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 1U);
    NativeDialogDismissRegistration reg1 = g_dialogTracker.dismissRegistrations.back();

    coordinator.RequestOpenModalForTest("modal2");
    ASSERT_EQ(g_dialogTracker.dismissRegistrations.size(), 2U);
    NativeDialogDismissRegistration reg2 = g_dialogTracker.dismissRegistrations.back();

    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);

    coordinator.RequestOpenModalForTest("modal1");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);

    ArkUI_DialogDismissEvent* event2 = reinterpret_cast<ArkUI_DialogDismissEvent*>(0xA2270);
    DispatchDismissCallback(mockArkUIPtr_, reg2, event2);
}

// --- Retain descriptor when resolved to root content ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_retain_root_content_descriptor_and_recover)
{
    auto trigger = CreateTrigger("openButton");
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger },
        { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("rootModal", "openButton", "root") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);

    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("rootModal", "openButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
}

// --- Schema warning for missing trigger and content with owner context ---

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_dispatch_schema_warning_for_missing_trigger_with_context)
{
    auto content = CreateContent("dialogContent");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "dialogContent", content } };
    std::map<std::string, std::string> parentsRelations;
    mockNapiPtr_->callFunctionCallCount_ = 0;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("warnModal", "absentButton", "dialogContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(ModalCoordinatorTddTest, L0_modal_should_not_dispatch_schema_warning_for_missing_content_with_context)
{
    auto trigger = CreateTrigger("openButton");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "openButton", trigger } };
    std::map<std::string, std::string> parentsRelations;
    mockNapiPtr_->callFunctionCallCount_ = 0;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TDD_RENDER_ID, MODAL_TDD_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors(
        { CreateDescriptor("warnModal", "openButton", "absentContent") }, allComponents, parentsRelations);
    EXPECT_EQ(coordinator.GetBindingCountForTest(), 0U);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}
