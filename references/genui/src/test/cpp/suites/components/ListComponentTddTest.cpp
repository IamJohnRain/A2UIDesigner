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

#include "components/A2UI/list/ListComponent.h"
#include "data/DataModel.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

class ListComponentTddTest : public A2UIComponentTddTest {};

namespace {

class ListComponentProbe : public ListComponent {
public:
    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }

    bool InvokeExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildren(childList, surfaceSlot, childIds);
    }
};

bool HasAdapterAttribute(ArkUI_NodeHandle node, ArkUI_NodeAdapterHandle adapterHandle)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, NODE_LIST_NODE_ADAPTER);
    return call != nullptr && call->adapterHandle == adapterHandle;
}

} // namespace

TEST_F(ListComponentTddTest, L0_list_should_create_list_node_and_report_type)
{
    auto list = std::make_shared<ListComponent>();
    ASSERT_NE(list, nullptr);

    EXPECT_EQ(list->GetType(), "List");
    EXPECT_EQ(list->GetNativeView(), FindCreatedNode(ARKUI_NODE_LIST));
}

TEST_F(ListComponentTddTest, L0_list_should_apply_direction_and_align_descriptor_values)
{
    auto list = std::make_shared<ListComponent>();
    auto descriptor = ParseJson(R"({"id":"items","component":"List","direction":"horizontal","align":"end"})");
    ASSERT_NE(descriptor, nullptr);

    list->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_DIRECTION, ARKUI_AXIS_HORIZONTAL);
    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_ALIGN_LIST_ITEM, ARKUI_LIST_ITEM_ALIGNMENT_END);
}

TEST_F(ListComponentTddTest, L0_list_should_fallback_invalid_direction_and_align_tokens)
{
    auto list = std::make_shared<ListComponent>();
    auto descriptor = ParseJson(R"({"id":"items","component":"List","direction":"bad","align":"bad"})");
    ASSERT_NE(descriptor, nullptr);

    list->ApplyDescriptor(descriptor->GetRoot());

    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_DIRECTION, ARKUI_AXIS_VERTICAL);
    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_ALIGN_LIST_ITEM, ARKUI_LIST_ITEM_ALIGNMENT_START);
}

TEST_F(ListComponentTddTest, L0_list_should_wrap_eager_children_in_list_item_nodes)
{
    auto list = std::make_shared<ListComponent>();
    auto child = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x8300));

    list->AddChild(child);

    ArkUI_NodeHandle listItemNode = FindCreatedNode(ARKUI_NODE_LIST_ITEM);
    ASSERT_NE(listItemNode, nullptr);
    EXPECT_TRUE(HasAddChildCall(listItemNode, child->GetNativeView()));
    EXPECT_TRUE(HasAddChildCall(list->GetNativeView(), listItemNode));
}

TEST_F(ListComponentTddTest, L0_list_should_remove_wrapped_items_when_children_are_removed)
{
    auto list = std::make_shared<ListComponent>();
    auto child = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x8301));
    list->AddChild(child);
    ArkUI_NodeHandle listItemNode = FindCreatedNode(ARKUI_NODE_LIST_ITEM);
    ASSERT_NE(listItemNode, nullptr);

    list->RemoveAllChildren();

    ASSERT_FALSE(g_tracker.removeChildCalls.empty());
    EXPECT_EQ(g_tracker.removeChildCalls.back().first, list->GetNativeView());
    EXPECT_EQ(g_tracker.removeChildCalls.back().second, listItemNode);
    EXPECT_GE(g_tracker.disposeNodeCount, 1);
}

TEST_F(ListComponentTddTest, L0_list_should_skip_eager_child_wrapping_when_lazy_mode_is_enabled)
{
    auto list = std::make_shared<ListComponent>();
    auto child = std::make_shared<BasicLeafComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x8302));

    list->SetLazyMode(true);
    list->AddChild(child);

    EXPECT_EQ(FindCreatedNode(ARKUI_NODE_LIST_ITEM), nullptr);
    EXPECT_FALSE(HasAddChildCall(list->GetNativeView(), child->GetNativeView()));
}

TEST_F(ListComponentTddTest, L0_list_should_apply_adapter_when_lazy_mode_and_adapter_are_both_available)
{
    auto list = std::make_shared<ListComponent>();
    auto adapter = std::make_shared<ListAdapterNode>();
    adapter->Initialize("rowTemplate", "/items", 3);

    list->SetAdapterNode(adapter);
    list->SetLazyMode(true);

    EXPECT_TRUE(list->IsLazyMode());
    EXPECT_EQ(list->GetAdapterNode(), adapter);
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), adapter->GetHandle()));
}

TEST_F(ListComponentTddTest, L0_list_should_configure_lazy_adapter_from_absolute_data_path)
{
    auto list = std::make_shared<ListComponent>();
    auto adapter = ParseJson(R"({"items":[{"name":"A"},{"name":"B"}]})");
    ASSERT_NE(adapter, nullptr);
    auto dataModel = std::make_shared<DataModel>(COMPONENT_TDD_SURFACE_ID);
    dataModel->ReplaceAll(adapter->GetRoot());
    auto templateAdapter = ParseJson(R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})");
    ASSERT_NE(templateAdapter, nullptr);

    LazyAdapterConfig config;
    config.templateComponentId = "rowTemplate";
    config.templatePath = "/items";
    config.dataModel = dataModel;
    config.templateDescriptor = templateAdapter->GetRoot();
    config.allDescriptors["rowTemplate"] = templateAdapter->GetRoot();
    config.surfaceId = COMPONENT_TDD_SURFACE_ID;
    config.renderId = COMPONENT_TDD_RENDER_ID;

    list->SetupLazyAdapter(config);

    ASSERT_NE(list->GetAdapterNode(), nullptr);
    EXPECT_TRUE(list->IsLazyMode());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[list->GetAdapterNode()->GetHandle()], 2U);
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), list->GetAdapterNode()->GetHandle()));
}

TEST_F(ListComponentTddTest, L0_list_should_create_empty_lazy_adapter_when_absolute_data_source_is_missing)
{
    auto list = std::make_shared<ListComponent>();

    LazyAdapterConfig nullModelConfig;
    nullModelConfig.templateComponentId = "rowTemplate";
    nullModelConfig.templatePath = "/items";
    list->SetupLazyAdapter(nullModelConfig);
    EXPECT_EQ(list->GetAdapterNode(), nullptr);

    auto dataModel = std::make_shared<DataModel>(COMPONENT_TDD_SURFACE_ID);
    LazyAdapterConfig missingPathConfig;
    missingPathConfig.templateComponentId = "rowTemplate";
    missingPathConfig.templatePath = "/items";
    missingPathConfig.dataModel = dataModel;
    list->SetupLazyAdapter(missingPathConfig);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    EXPECT_TRUE(list->IsLazyMode());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[list->GetAdapterNode()->GetHandle()], 0U);
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), list->GetAdapterNode()->GetHandle()));
}

TEST_F(ListComponentTddTest, L0_list_should_create_empty_lazy_adapter_when_absolute_data_source_is_not_array)
{
    auto list = std::make_shared<ListComponent>();
    auto adapter = ParseJson(R"({"items":{"name":"A"}})");
    ASSERT_NE(adapter, nullptr);
    auto dataModel = std::make_shared<DataModel>(COMPONENT_TDD_SURFACE_ID);
    dataModel->ReplaceAll(adapter->GetRoot());

    LazyAdapterConfig config;
    config.templateComponentId = "rowTemplate";
    config.templatePath = "/items";
    config.dataModel = dataModel;

    list->SetupLazyAdapter(config);

    ASSERT_NE(list->GetAdapterNode(), nullptr);
    EXPECT_TRUE(list->IsLazyMode());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[list->GetAdapterNode()->GetHandle()], 0U);
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), list->GetAdapterNode()->GetHandle()));
}

TEST_F(ListComponentTddTest, L0_list_should_return_false_when_refresh_lazy_adapter_preconditions_are_not_met)
{
    auto list = std::make_shared<ListComponent>();
    EXPECT_FALSE(list->RefreshLazyAdapterFromDataModel());

    auto adapterWithoutModel = std::make_shared<ListAdapterNode>();
    adapterWithoutModel->Initialize("rowTemplate", "/items", 2);
    list->SetAdapterNode(adapterWithoutModel);
    list->SetLazyMode(true);
    EXPECT_FALSE(list->RefreshLazyAdapterFromDataModel());

    auto modelData = ParseJson(R"({"items":[{"name":"A"}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>(COMPONENT_TDD_SURFACE_ID);
    dataModel->ReplaceAll(modelData->GetRoot());

    auto relativePathAdapter = std::make_shared<ListAdapterNode>();
    relativePathAdapter->Initialize("rowTemplate", "items", 2);
    relativePathAdapter->SetDataModel(dataModel);
    list->SetAdapterNode(relativePathAdapter);
    EXPECT_FALSE(list->RefreshLazyAdapterFromDataModel());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[relativePathAdapter->GetHandle()], 2U);
}

TEST_F(ListComponentTddTest, L0_list_should_refresh_lazy_adapter_item_count_for_missing_non_array_and_array_targets)
{
    auto modelData = ParseJson(R"({"other":[{"name":"X"}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>(COMPONENT_TDD_SURFACE_ID);
    dataModel->ReplaceAll(modelData->GetRoot());

    auto list = std::make_shared<ListComponent>();
    auto adapter = std::make_shared<ListAdapterNode>();
    adapter->Initialize("rowTemplate", "/items", 2);
    adapter->SetDataModel(dataModel);
    list->SetAdapterNode(adapter);
    list->SetLazyMode(true);

    EXPECT_TRUE(list->RefreshLazyAdapterFromDataModel());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapter->GetHandle()], 0U);

    auto nonArrayData = ParseJson(R"({"items":{"name":"A"}})");
    ASSERT_NE(nonArrayData, nullptr);
    dataModel->ReplaceAll(nonArrayData->GetRoot());
    adapter->UpdateItemCount(3);
    EXPECT_TRUE(list->RefreshLazyAdapterFromDataModel());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapter->GetHandle()], 0U);

    auto arrayData = ParseJson(R"({"items":[{"name":"A"},{"name":"B"},{"name":"C"}]})");
    ASSERT_NE(arrayData, nullptr);
    dataModel->ReplaceAll(arrayData->GetRoot());
    EXPECT_TRUE(list->RefreshLazyAdapterFromDataModel());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapter->GetHandle()], 3U);
}

TEST_F(ListComponentTddTest, L0_list_should_collect_and_expand_template_children_into_lazy_adapter)
{
    ListComponentProbe list;
    auto descriptor =
        ParseJson(R"({"id":"list","component":"List","children":{"componentId":"rowTemplate","path":"/items"}})");
    ASSERT_NE(descriptor, nullptr);
    list.InvokeCollectChildListDescriptor(descriptor->GetRoot());

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    auto templateAdapter = ParseJson(R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})");
    ASSERT_NE(templateAdapter, nullptr);
    surfaceSlot.GetDescriptorsById()["rowTemplate"] = templateAdapter->GetRoot();
    const_cast<std::map<std::string, JsonValue>&>(surfaceSlot.GetAllComponentDescriptorStore())["rowTemplate"] =
        templateAdapter->GetRoot();
    auto modelAdapter = ParseJson(R"({"value":{"items":[{"name":"A"}]}})");
    ASSERT_NE(modelAdapter, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(modelAdapter->GetRoot()));

    std::list<std::string> childIds = { "stale" };
    EXPECT_FALSE(list.InvokeExpandTemplateChildren(list.GetChildListDescriptor(), surfaceSlot, childIds));

    EXPECT_TRUE(childIds.empty());
    ASSERT_NE(list.GetAdapterNode(), nullptr);
    EXPECT_TRUE(list.IsLazyMode());
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[list.GetAdapterNode()->GetHandle()], 1U);
}

TEST_F(ListComponentTddTest, L0_list_should_return_false_when_template_child_descriptor_is_missing)
{
    ListComponentProbe list;
    auto descriptor =
        ParseJson(R"({"id":"list","component":"List","children":{"componentId":"missingTemplate","path":"/items"}})");
    ASSERT_NE(descriptor, nullptr);
    list.InvokeCollectChildListDescriptor(descriptor->GetRoot());
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);

    std::list<std::string> childIds = { "stale" };
    EXPECT_FALSE(list.InvokeExpandTemplateChildren(list.GetChildListDescriptor(), surfaceSlot, childIds));

    EXPECT_TRUE(childIds.empty());
    EXPECT_EQ(list.GetAdapterNode(), nullptr);
}

TEST_F(ListComponentTddTest, L0_list_should_return_theme_when_surface_context_is_available)
{
    auto list = std::make_shared<ListComponent>();
    PrepareThemeContext(*list);

    EXPECT_NE(list->GetTheme(), nullptr);
}
