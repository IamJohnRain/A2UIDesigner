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

#include "components/A2UI/modal/ModalCoordinator.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/A2UI/button/ButtonComponent.h"
#include "components/A2UI/column/ColumnComponent.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

constexpr int32_t MODAL_TEST_RENDER_ID = 700100;
constexpr char MODAL_TEST_SURFACE_ID[] = "modal-dynamic-binding-test";

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

std::shared_ptr<ButtonComponent> CreateButton(const std::string& id)
{
    auto button = std::make_shared<ButtonComponent>();
    button->SetComponentId(id);
    button->SetRenderId(MODAL_TEST_RENDER_ID);
    button->SetSurfaceId(MODAL_TEST_SURFACE_ID);
    return button;
}

std::shared_ptr<ColumnComponent> CreateColumn(const std::string& id)
{
    auto column = std::make_shared<ColumnComponent>();
    column->SetComponentId(id);
    column->SetRenderId(MODAL_TEST_RENDER_ID);
    column->SetSurfaceId(MODAL_TEST_SURFACE_ID);
    return column;
}

} // namespace

class ModalCoordinatorTest : public A2UITest {
protected:
    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(MODAL_TEST_RENDER_ID);
        A2UITest::TearDown();
    }
};

/**
 * @tc.name: ModalCoordinatorTest001
 * @tc.desc: Verify the following ModalCoordinator behavior: store dynamic trigger and content descriptors.
 * @tc.type: FUNC
 */
TEST_F(ModalCoordinatorTest, should_store_dynamic_trigger_and_content_descriptors)
{
    /**
     * @tc.steps: step1. Build a modal descriptor with dynamic trigger and content paths and invoke the descriptor
     * creation interface.
     * @tc.expected: The descriptor stores modal id and dynamic json literals while direct trigger and content ids
     * remain empty.
     */

    auto adapter = ParseJson(
        R"({"id":"dynamicModal","component":"Modal","trigger":{"path":"/modal/trigger"},"content":{"path":"/modal/content"}})");
    ASSERT_NE(adapter, nullptr);

    ModalCoordinator creator;
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(creator.TryCreateDescriptor(adapter->GetRoot(), descriptor));

    EXPECT_EQ(descriptor.modalId, "dynamicModal");
    EXPECT_TRUE(descriptor.triggerId.empty());
    EXPECT_TRUE(descriptor.contentId.empty());
    EXPECT_FALSE(descriptor.triggerJsonLiteral.empty());
    EXPECT_FALSE(descriptor.contentJsonLiteral.empty());
}

/**
 * @tc.name: ModalCoordinatorTest002
 * @tc.desc: Verify the following ModalCoordinator behavior: refresh dynamic trigger and content when data model
 * changes.
 * @tc.type: FUNC
 */
TEST_F(ModalCoordinatorTest, should_refresh_dynamic_trigger_and_content_when_data_model_changes)
{
    /**
     * @tc.steps: step1. Create the render and surface context, bind a modal descriptor, update the data model, and
     * refresh modal bindings.
     * @tc.expected: The modal binding count remains correct and the trigger and content ids are updated to the latest
     * data model values.
     */

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(MODAL_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(MODAL_TEST_SURFACE_ID, nullptr);

    auto initialData = ParseJson(R"({"value":{"modal":{"trigger":"openButton","content":"dialogCard"}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto descriptorAdapter = ParseJson(
        R"({"id":"dynamicModal","component":"Modal","trigger":{"path":"/modal/trigger"},"content":{"path":"/modal/content"}})");
    ASSERT_NE(descriptorAdapter, nullptr);

    ModalCoordinator creator2;
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(creator2.TryCreateDescriptor(descriptorAdapter->GetRoot(), descriptor));

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["openButton"] = CreateButton("openButton");
    allComponents["dialogCard"] = CreateColumn("dialogCard");
    allComponents["otherButton"] = CreateButton("otherButton");
    allComponents["otherCard"] = CreateColumn("otherCard");
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TEST_RENDER_ID, MODAL_TEST_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("dynamicModal"), "openButton");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dynamicModal"), "dialogCard");

    auto updatedData = ParseJson(R"({"value":{"modal":{"trigger":"otherButton","content":"otherCard"}}})");
    ASSERT_NE(updatedData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedData->GetRoot()));

    coordinator.RefreshModalBindings();

    EXPECT_EQ(coordinator.GetBindingCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetBindingTriggerIdForTest("dynamicModal"), "otherButton");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dynamicModal"), "otherCard");

    coordinator.Dispose();
}

/**
 * @tc.name: ModalCoordinatorTest003
 * @tc.desc: Verify the following ModalCoordinator behavior: stack nested modals and dismiss the top layer only.
 * @tc.type: FUNC
 */
TEST_F(ModalCoordinatorTest, should_stack_nested_modals_and_restore_previous_layer_when_top_closes)
{
    /**
     * @tc.steps: step1. Bind two valid modal descriptors and open them in sequence.
     *            step2. Toggle the top modal again to dismiss only the top layer.
     *            step3. Dismiss all active modals through the coordinator lifecycle entry.
     * @tc.expected: The inner modal overlays the outer modal, closing the top layer restores the outer modal,
     *               and lifecycle cleanup clears the full stack.
     */

    auto outerDescriptorAdapter =
        ParseJson(R"({"id":"outerModal","component":"Modal","trigger":"outerButton","content":"outerCard"})");
    auto innerDescriptorAdapter =
        ParseJson(R"({"id":"innerModal","component":"Modal","trigger":"innerButton","content":"innerCard"})");
    ASSERT_NE(outerDescriptorAdapter, nullptr);
    ASSERT_NE(innerDescriptorAdapter, nullptr);

    ModalCoordinator creator;
    ModalCoordinator::ModalDescriptor outerDescriptor;
    ModalCoordinator::ModalDescriptor innerDescriptor;
    ASSERT_TRUE(creator.TryCreateDescriptor(outerDescriptorAdapter->GetRoot(), outerDescriptor));
    ASSERT_TRUE(creator.TryCreateDescriptor(innerDescriptorAdapter->GetRoot(), innerDescriptor));

    auto outerButton = CreateButton("outerButton");
    auto innerButton = CreateButton("innerButton");
    auto outerCard = CreateColumn("outerCard");
    auto innerCard = CreateColumn("innerCard");

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["outerButton"] = outerButton;
    allComponents["innerButton"] = innerButton;
    allComponents["outerCard"] = outerCard;
    allComponents["innerCard"] = innerCard;

    std::map<std::string, std::string> parentsRelations;
    parentsRelations["innerButton"] = "outerCard";

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TEST_RENDER_ID, MODAL_TEST_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ outerDescriptor, innerDescriptor }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("outerModal");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "outerModal");

    coordinator.RequestOpenModalForTest("innerModal");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 2U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 2U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "innerModal");

    coordinator.RequestOpenModalForTest("innerModal");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "outerModal");

    coordinator.DismissActiveModal();
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 0U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 0U);

    coordinator.Dispose();
}

/**
 * @tc.name: ModalCoordinatorTest004
 * @tc.desc: Verify the following ModalCoordinator behavior: keep the desired modal open when a synchronous dismiss
 *           callback fires during a content refresh transition.
 * @tc.type: FUNC
 */
TEST_F(ModalCoordinatorTest, should_reopen_same_modal_after_sync_dismiss_when_refresh_updates_content_binding)
{
    /**
     * @tc.steps: step1. Bind a modal descriptor whose content id resolves from the data model and open the modal.
     *            step2. Configure the native dialog mock to fire dismiss callbacks synchronously on close.
     *            step3. Refresh the modal binding to a new content component while keeping the same modal id open.
     * @tc.expected: The coordinator closes the stale dialog and immediately reopens the same modal with the new content
     *               binding instead of clearing the desired modal stack.
     */

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(MODAL_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(MODAL_TEST_SURFACE_ID, nullptr);

    auto initialData = ParseJson(R"({"value":{"modal":{"trigger":"openButton","content":"dialogCard"}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto descriptorAdapter = ParseJson(
        R"({"id":"dynamicModal","component":"Modal","trigger":{"path":"/modal/trigger"},"content":{"path":"/modal/content"}})");
    ASSERT_NE(descriptorAdapter, nullptr);

    ModalCoordinator creator;
    ModalCoordinator::ModalDescriptor descriptor;
    ASSERT_TRUE(creator.TryCreateDescriptor(descriptorAdapter->GetRoot(), descriptor));

    std::map<std::string, std::shared_ptr<Component>> allComponents;
    allComponents["openButton"] = CreateButton("openButton");
    allComponents["dialogCard"] = CreateColumn("dialogCard");
    allComponents["otherCard"] = CreateColumn("otherCard");
    std::map<std::string, std::string> parentsRelations;

    ModalCoordinator coordinator;
    coordinator.SetOwnerContext(MODAL_TEST_RENDER_ID, MODAL_TEST_SURFACE_ID);
    coordinator.HandlePendingModalDescriptors({ descriptor }, allComponents, parentsRelations);

    coordinator.RequestOpenModalForTest("dynamicModal");
    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dynamicModal"), "dialogCard");
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);

    mockArkUIPtr_->SetDialogCloseTriggersDismissCallback(true);

    auto updatedData = ParseJson(R"({"value":{"modal":{"trigger":"openButton","content":"otherCard"}}})");
    ASSERT_NE(updatedData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedData->GetRoot()));

    coordinator.RefreshModalBindings();

    EXPECT_EQ(coordinator.GetBindingContentIdForTest("dynamicModal"), "otherCard");
    EXPECT_EQ(mockArkUIPtr_->closedDialogs_.size(), 1U);
    EXPECT_EQ(coordinator.GetOpenModalCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogCountForTest(), 1U);
    EXPECT_EQ(coordinator.GetActiveDialogModalIdForTest(), "dynamicModal");

    coordinator.Dispose();
}
