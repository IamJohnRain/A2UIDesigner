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

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/list/ListComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedGridComponent.h"
#include "components/extended/ExtendedListComponent.h"
#include "components/extended/ExtendedRowComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "theme/ThemeManager.h"

#include "A2UIComponentTddTestHelper.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

constexpr char SCHEMA_ERROR_CODE_REQUIRED_MISS[] = "ERROR_CODE_REQUIRED_MISS";
constexpr char SCHEMA_ERROR_CODE_INVALID_VALUE[] = "ERROR_CODE_INVALID_VALUE";
constexpr char SCHEMA_ERROR_CODE_TYPE_MISMATCH[] = "ERROR_CODE_TYPE_MISMATCH";

std::map<ArkUI_NodeHandle, ArkUI_NodeEventCallback> g_breakpointNodeEventReceivers;

int32_t CaptureBreakpointNodeEventReceiver(ArkUI_NodeHandle node, ArkUI_NodeEventCallback callback)
{
    TrackAddNodeEventReceiver(node, callback);
    g_breakpointNodeEventReceivers[node] = callback;
    return 0;
}

struct DispatchCallbacks {
    napi_env env = nullptr;
    napi_value warningCallback = nullptr;
};

DispatchCallbacks RegisterDispatchCallbacks(MockNapiProvider* mockNapi)
{
    DispatchCallbacks callbacks;
    if (mockNapi == nullptr) {
        return callbacks;
    }

    callbacks.env = reinterpret_cast<napi_env>(0x1300);
    mockNapi->CreateFunction(
        callbacks.env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &callbacks.warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(callbacks.env, callbacks.warningCallback);

    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionRecv_ = nullptr;
    mockNapi->lastCallFunctionFunc_ = nullptr;
    mockNapi->lastCallFunctionArgs_.clear();
    mockNapi->callFunctionArgsHistory_.clear();
    return callbacks;
}

void RegisterRuntimeErrorCallback(MockNapiProvider* mockNapi)
{
    if (mockNapi == nullptr) {
        return;
    }

    napi_env env = reinterpret_cast<napi_env>(0x1301);
    napi_value callback = nullptr;
    mockNapi->CreateFunction(env, "dispatchRuntimeError", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env, callback);
}

napi_value GetRequestProperty(const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    if (mockNapi == nullptr || request == nullptr) {
        return nullptr;
    }
    auto objectIt = mockNapi->objectProperties_.find(request);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return nullptr;
    }
    auto propIt = objectIt->second.find(key);
    if (propIt == objectIt->second.end()) {
        return nullptr;
    }
    return propIt->second;
}

std::string GetStringValue(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto it = mockNapi->stringValues_.find(value);
    return it == mockNapi->stringValues_.end() ? "" : it->second;
}

int32_t GetInt32Value(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return 0;
    }
    auto it = mockNapi->numberValues_.find(value);
    return it == mockNapi->numberValues_.end() ? 0 : static_cast<int32_t>(it->second);
}

size_t CountWarningRequests(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    if (mockNapi == nullptr) {
        return 0U;
    }

    size_t count = 0U;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }

        napi_value request = args.front();
        std::string warningCode = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "code"));
        std::string warningPath = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "path"));
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

std::shared_ptr<Catalog> CreateNativeCatalog(const std::vector<std::string>& componentNames)
{
    auto catalog = std::make_shared<Catalog>("surface-slot-component-integration-tdd");
    for (const auto& componentName : componentNames) {
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetInnerNative(true);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::shared_ptr<Catalog> CreateExtendedProtocolCatalog(const std::vector<std::string>& componentNames)
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const auto& componentName : componentNames) {
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::shared_ptr<Catalog> CreateCatalogWithFlags(
    const std::string& catalogId, std::initializer_list<std::pair<const char*, bool>> components)
{
    auto catalog = std::make_shared<Catalog>(catalogId);
    for (const auto& entry : components) {
        if (entry.first == nullptr || entry.first[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(entry.first, CatalogItemType::COMPONENT);
        item->SetInnerNative(entry.second);
        catalog->AddComponent(item);
    }
    return catalog;
}

bool HasAdapterAttribute(ArkUI_NodeHandle node, int32_t adapterAttribute, ArkUI_NodeAdapterHandle adapterHandle)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, adapterAttribute);
    return call != nullptr && call->adapterHandle == adapterHandle;
}

int32_t CountInsertChildAtCalls(ArkUI_NodeHandle parentNode, ArkUI_NodeHandle childNode)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.insertChildAtCalls) {
        if (std::get<0>(call) == parentNode && std::get<1>(call) == childNode) {
            ++count;
        }
    }
    return count;
}

int32_t CountRemoveChildCalls(ArkUI_NodeHandle parentNode, ArkUI_NodeHandle childNode)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.removeChildCalls) {
        if (call.first == parentNode && call.second == childNode) {
            ++count;
        }
    }
    return count;
}

class SurfaceSlotComponentIntegrationTddTest : public A2UIComponentTddTest {};

} // namespace

TEST_F(SurfaceSlotComponentIntegrationTddTest, L1_should_build_bound_tree_and_lazy_list_through_surface_slot)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Button", "Text", "List" }));

    auto dataModel = ParseJson(R"({"value":{"title":"Hello from model","items":[{"name":"Alpha"},{"name":"Beta"}]}})");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataModel->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":["title","button","items"],"align":"start"},)"
                  R"({"id":"title","component":"Text","text":{"path":"/title"},"variant":"h3"},)"
                  R"({"id":"button","component":"Button","child":"buttonText","action":{"event":{"name":"tap"}}},)"
                  R"({"id":"buttonText","component":"Text","text":"Tap"},)"
                  R"({"id":"items","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.GetRootComponent();
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Column");

    std::shared_ptr<Component> title = surfaceSlot.FindComponentById("title");
    std::shared_ptr<Component> button = surfaceSlot.FindComponentById("button");
    std::shared_ptr<Component> buttonText = surfaceSlot.FindComponentById("buttonText");
    std::shared_ptr<Component> items = surfaceSlot.FindComponentById("items");
    ASSERT_NE(title, nullptr);
    ASSERT_NE(button, nullptr);
    ASSERT_NE(buttonText, nullptr);
    ASSERT_NE(items, nullptr);
    EXPECT_EQ(title->GetType(), "Text");
    EXPECT_EQ(button->GetType(), "Button");
    EXPECT_EQ(buttonText->GetType(), "Text");
    EXPECT_EQ(items->GetType(), "List");

    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), title->GetNativeView(), 0));
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), button->GetNativeView(), 1));
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), items->GetNativeView(), 2));
    EXPECT_TRUE(HasInsertChildAtCall(button->GetNativeView(), buttonText->GetNativeView(), 0));
    ExpectStringAttribute(title->GetNativeView(), NODE_TEXT_CONTENT, "Hello from model");

    auto list = std::dynamic_pointer_cast<ListComponent>(items);
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(list->IsLazyMode());
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_auto_load_standard_list_template_when_data_arrives_after_components)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "List", "Text" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(list->IsLazyMode());
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 0U);

    auto updatedModel =
        ParseJson(R"({"path":"/items","value":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"}]})");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedModel->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
}

/**
 * @tc.name: L0_should_auto_load_extended_list_template_when_data_arrives_after_components
 * @tc.desc: 验证扩展 List 先创建空适配器并在后续数据到达时复用适配器加载模板项
 * @tc.type: FUNC
 */
TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_auto_load_extended_list_template_when_data_arrives_after_components)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto listComponent = ParseJson(R"({"components":[{"id":"root","component":"List",)"
                                   R"("children":{"componentId":"rowTemplate","path":"/items"}}]})");
    ASSERT_NE(listComponent, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(listComponent->GetRoot()));

    auto templateComponent =
        ParseJson(R"({"components":[{"id":"rowTemplate","component":"Text","content":{"path":"name"}}]})");
    ASSERT_NE(templateComponent, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(templateComponent->GetRoot()));

    auto list = std::dynamic_pointer_cast<ExtendedListComponent>(surfaceSlot.FindComponentById("root"));
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(list->IsLazyMode());
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    std::shared_ptr<ListAdapterNode> initialAdapter = list->GetAdapterNode();
    EXPECT_EQ(initialAdapter->GetTemplateId(), "rowTemplate");
    EXPECT_EQ(initialAdapter->GetDataPath(), "/items");
    EXPECT_NE(initialAdapter->GetDataModel(), nullptr);
    ArkUI_NodeAdapterHandle initialAdapterHandle = initialAdapter->GetHandle();
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[initialAdapterHandle], 0U);

    auto updatedModel = ParseJson(R"({"path":"/items","value":[{"name":"gamma"},{"name":"delta"}]})");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedModel->GetRoot()));

    list = std::dynamic_pointer_cast<ExtendedListComponent>(surfaceSlot.FindComponentById("root"));
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(list->IsLazyMode());
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_EQ(adapterHandle, initialAdapterHandle);
    EXPECT_TRUE(HasAdapterAttribute(list->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x3003);
    mockArkUIPtr_->SetNodeAdapterEventType(event, NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);
    ASSERT_TRUE(mockArkUIPtr_->DispatchNodeAdapterEvent(adapterHandle, event));

    ASSERT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 1U);
    std::shared_ptr<Component> firstItem = surfaceSlot.FindComponentById("/itemsrowTemplate:0:rowTemplate");
    ASSERT_NE(firstItem, nullptr);
    ExpectStringAttribute(firstItem->GetNativeView(), NODE_TEXT_CONTENT, "gamma");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, should_isolate_list_and_grid_component_breakpoint_state)
{
    api_->addNodeEventReceiver = CaptureBreakpointNodeEventReceiver;
    g_breakpointNodeEventReceivers.clear();
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    surfaceManager->SetApiVersion(21);
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "List", "Grid" }));
    surfaceManager->UpdateBreakpoint(Breakpoint::XL);

    auto message = ParseJson(R"({"components":[
        {"id":"root","component":"Column","children":["list","grid"]},
        {"id":"list","component":"List"},
        {"id":"grid","component":"Grid","styles":{
            "columnsTemplate":{"xs":"1fr","md":"1fr 1fr"}
        }}
    ]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto list = std::dynamic_pointer_cast<ExtendedListComponent>(surfaceSlot.FindComponentById("list"));
    auto grid = std::dynamic_pointer_cast<ExtendedGridComponent>(surfaceSlot.FindComponentById("grid"));
    ASSERT_NE(list, nullptr);
    ASSERT_NE(grid, nullptr);
    ASSERT_NE(surfaceSlot.GetThemeManager(), nullptr);
    EXPECT_EQ(surfaceSlot.GetThemeManager()->GetContext().breakpoint, Breakpoint::XL);
    ASSERT_NE(g_breakpointNodeEventReceivers[list->GetNativeView()], nullptr);
    ASSERT_NE(g_breakpointNodeEventReceivers[grid->GetNativeView()], nullptr);

    const int32_t gridColumnsBefore = CountAttributeCall(grid->GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    ArkUI_NodeEvent listEvent {};
    ArkUI_NodeComponentEvent listArea {};
    listArea.data[0].f32 = 0.0F;
    listArea.data[2].f32 = 360.0F;
    mockArkUIPtr_->SetNodeEventHandle(&listEvent, list->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&listEvent, NODE_ON_SIZE_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&listEvent, &listArea);
    g_breakpointNodeEventReceivers[list->GetNativeView()](&listEvent);

    EXPECT_EQ(CountAttributeCall(grid->GetNativeView(), NODE_GRID_COLUMN_TEMPLATE), gridColumnsBefore);
    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_LANES, 1);
    list->OnConfigChange(surfaceSlot.GetThemeManager()->GetContext());
    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_LANES, 1);

    const int32_t listLanesBefore = CountAttributeCall(list->GetNativeView(), NODE_LIST_LANES);
    ArkUI_NodeEvent gridEvent {};
    ArkUI_NodeComponentEvent gridArea {};
    gridArea.data[0].f32 = 500.0F;
    gridArea.data[2].f32 = 800.0F;
    mockArkUIPtr_->SetNodeEventHandle(&gridEvent, grid->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&gridEvent, NODE_ON_SIZE_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&gridEvent, &gridArea);
    g_breakpointNodeEventReceivers[grid->GetNativeView()](&gridEvent);

    EXPECT_EQ(CountAttributeCall(list->GetNativeView(), NODE_LIST_LANES), listLanesBefore);
    ExpectStringAttribute(grid->GetNativeView(), NODE_GRID_COLUMN_TEMPLATE, "1fr 1fr");
    grid->OnConfigChange(surfaceSlot.GetThemeManager()->GetContext());
    ExpectStringAttribute(grid->GetNativeView(), NODE_GRID_COLUMN_TEMPLATE, "1fr 1fr");
    EXPECT_EQ(surfaceSlot.GetThemeManager()->GetContext().breakpoint, Breakpoint::XL);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_standard_list_when_update_data_model_path_is_root)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);

    auto updateRoot =
        ParseJson(R"({"path":"/","value":{"items":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"}]}})");
    ASSERT_NE(updateRoot, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updateRoot->GetRoot()));

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_standard_list_when_delete_data_model_path_removes_array)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);

    auto deleteItems = ParseJson(R"({"path":"/items"})");
    ASSERT_NE(deleteItems, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(deleteItems->GetRoot()));

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 0U);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_standard_list_when_replace_all_data_model_changes_array)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);

    auto replaceAll =
        ParseJson(R"({"value":{"items":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"},{"name":"zeta"}]}})");
    ASSERT_NE(replaceAll, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(replaceAll->GetRoot()));

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 4U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_skip_standard_list_refresh_when_changed_path_is_unrelated)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto list = std::dynamic_pointer_cast<ListComponent>(root);
    ASSERT_NE(list, nullptr);
    ASSERT_NE(list->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = list->GetAdapterNode()->GetHandle();
    list->GetAdapterNode()->UpdateItemCount(5);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 5U);

    auto unrelatedUpdate = ParseJson(R"({"path":"/other","value":[{"name":"gamma"}]})");
    ASSERT_NE(unrelatedUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(unrelatedUpdate->GetRoot()));

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 5U);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L1_should_route_extended_short_component_name_to_prefixed_custom_component)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateCatalogWithFlags(
        A2UI_EXTENDED_CATALOG_ID, { { "Column", true }, { "Text", true }, { "NavContainer", false } }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["nav"]},)"
                             R"({"id":"nav","component":"NavContainer","children":["page1"],"currentIndex":0},)"
                             R"({"id":"page1","component":"Text","text":"Page 1"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> nav = surfaceSlot.FindComponentById("nav");
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(nav, nullptr);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(nav->GetType(), "NavContainer");
    ASSERT_EQ(root->GetChildren().size(), 1U);
    EXPECT_EQ(root->GetChildren().front()->GetComponentId(), "nav");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_apply_initial_dynamic_current_index_for_nav_container_after_data_model_is_ready)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "NavContainer", "Text" }));

    auto dataModel = ParseJson(R"({"value":{"navModel":{"activeIndex":1}}})");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataModel->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"NavContainer","children":["pageOne","pageTwo"],)"
                             R"("currentIndex":{"path":"/navModel/activeIndex"}},)"
                             R"({"id":"pageOne","component":"Text","content":"Page One"},)"
                             R"({"id":"pageTwo","component":"Text","content":"Page Two"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> pageOne = surfaceSlot.FindComponentById("pageOne");
    std::shared_ptr<Component> pageTwo = surfaceSlot.FindComponentById("pageTwo");
    ASSERT_NE(pageOne, nullptr);
    ASSERT_NE(pageTwo, nullptr);

    ExpectI32Attribute(pageOne->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(pageTwo->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);

    auto outOfRangeUpdate = ParseJson(R"({"path":"/navModel/activeIndex","value":2})");
    ASSERT_NE(outOfRangeUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(outOfRangeUpdate->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.currentIndex"), 1U);
    ExpectI32Attribute(pageOne->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(pageTwo->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_report_invalid_current_index_when_nav_container_children_is_empty)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "NavContainer", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["navMain"]},)"
                             R"({"id":"navMain","component":"NavContainer","children":[],"currentIndex":1},)"
                             R"({"id":"pageOne","component":"Text","content":"Page One"},)"
                             R"({"id":"pageTwo","component":"Text","content":"Page Two"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "navMain.currentIndex"), 1U);
    auto nav = surfaceSlot.FindComponentById("navMain");
    ASSERT_NE(nav, nullptr);
    EXPECT_TRUE(nav->GetChildren().empty());
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_type_mismatch_warning_when_nav_container_children_is_not_array)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "NavContainer", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["navMain"]},)"
                             R"({"id":"navMain","component":"NavContainer","children":123,"currentIndex":1},)"
                             R"({"id":"pageOne","component":"Text","content":"Page One"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "navMain.children"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_accept_template_children_object_for_nav_container_without_schema_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "NavContainer", "Text" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":["navMain"]},)"
                  R"({"id":"navMain","component":"NavContainer","children":{"componentId":"pageTpl","path":"/pages"}},)"
                  R"({"id":"pageTpl","component":"Text","content":"Template Page"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "navMain.children"), 0U);
    auto nav = surfaceSlot.FindComponentById("navMain");
    ASSERT_NE(nav, nullptr);
    EXPECT_TRUE(nav->GetChildren().empty());
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_required_warning_when_nav_container_current_index_is_missing)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "NavContainer", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"NavContainer","children":["pageOne","pageTwo"]},)"
                             R"({"id":"pageOne","component":"Text","content":"Page One"},)"
                             R"({"id":"pageTwo","component":"Text","content":"Page Two"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "root.currentIndex"), 0U);
    auto pageOne = surfaceSlot.FindComponentById("pageOne");
    auto pageTwo = surfaceSlot.FindComponentById("pageTwo");
    ASSERT_NE(pageOne, nullptr);
    ASSERT_NE(pageTwo, nullptr);
    ExpectI32Attribute(pageOne->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
    ExpectI32Attribute(pageTwo->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L1_should_not_route_extended_short_component_name_under_standard_protocol)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateCatalogWithFlags(A2UI_BASIC_CATALOG_ID, { { "Column", true }, { "Text", true } }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["nav"]},)"
                             R"({"id":"nav","component":"NavContainer","children":["page1"],"currentIndex":0},)"
                             R"({"id":"page1","component":"Text","text":"Page 1"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> nav = surfaceSlot.FindComponentById("nav");
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(nav, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L1_should_attach_child_once_when_child_descriptor_is_before_parent_descriptor)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"aChild","component":"Text","text":"Alpha"},)"
                             R"({"id":"root","component":"Column","children":["aChild"]})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> child = surfaceSlot.FindComponentById("aChild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);

    EXPECT_EQ(CountInsertChildAtCalls(root->GetNativeView(), child->GetNativeView()), 1);
    EXPECT_EQ(CountRemoveChildCalls(root->GetNativeView(), child->GetNativeView()), 0);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_skip_unreferenced_descriptor_when_building_root_component_tree)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["header","componentTarget","footer"]},)"
                             R"({"id":"header","component":"Text","content":"Header"},)"
                             R"({"id":"componentTarget","component":"Column","children":["itemA","itemB","itemC"]},)"
                             R"({"id":"itemA","component":"Text","content":"Item A"},)"
                             R"({"id":"itemB","component":"Text","content":"Item B"},)"
                             R"({"id":"itemC","component":"Text","content":"Item C"},)"
                             R"({"id":"footer","component":"Text","content":"Footer"},)"
                             R"({"id":"itemTemplate","component":"Text","content":{"path":"/title"}})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> header = surfaceSlot.FindComponentById("header");
    std::shared_ptr<Component> componentTarget = surfaceSlot.FindComponentById("componentTarget");
    std::shared_ptr<Component> footer = surfaceSlot.FindComponentById("footer");
    std::shared_ptr<Component> itemTemplate = surfaceSlot.FindComponentById("itemTemplate");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(header, nullptr);
    ASSERT_NE(componentTarget, nullptr);
    ASSERT_NE(footer, nullptr);
    EXPECT_EQ(itemTemplate, nullptr);

    ASSERT_EQ(root->GetChildren().size(), 3U);
    auto childIt = root->GetChildren().begin();
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "header");
    ++childIt;
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "componentTarget");
    ++childIt;
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "footer");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_expand_template_children_for_extended_tabs_custom_component)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Tabs", "Extended.TabContent", "Text" }));

    auto dataModel = ParseJson(R"({"value":{"pages":[)"
                               R"({"title":"首页","body":"欢迎使用 GenUI"},)"
                               R"({"title":"设置","body":"在此配置偏好"})"
                               R"(]}})");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataModel->GetRoot()));
    const std::string firstTabId = "/pagestabContentTpl:0:tabContentTpl";
    const std::string secondTabId = "/pagestabContentTpl:1:tabContentTpl";

    auto message = ParseJson(
        R"({"components":[)"
        R"({"id":"root","component":"Tabs","children":{"componentId":"tabContentTpl","path":"/pages"}},)"
        R"({"id":"tabContentTpl","component":"Extended.TabContent","title":{"path":"title"},"children":["pageContent"]},)"
        R"({"id":"pageContent","component":"Text","content":{"path":"body"}})"
        R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> firstTab = surfaceSlot.FindComponentById(firstTabId);
    std::shared_ptr<Component> secondTab = surfaceSlot.FindComponentById(secondTabId);
    ASSERT_NE(root, nullptr);
    ASSERT_NE(firstTab, nullptr);
    ASSERT_NE(secondTab, nullptr);

    ASSERT_EQ(root->GetChildren().size(), 2U);
    auto childIt = root->GetChildren().begin();
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), firstTabId);
    ++childIt;
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), secondTabId);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_expand_static_children_for_extended_tabs_custom_component_when_root_descriptor_precedes_children)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Tabs", "Extended.TabContent", "Text" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Tabs","children":["tabHome","tabOrders"]},)"
                  R"({"id":"tabHome","component":"Extended.TabContent","title":"Home","children":["homePage"]},)"
                  R"({"id":"tabOrders","component":"Extended.TabContent","title":"Orders","children":["ordersPage"]},)"
                  R"({"id":"homePage","component":"Text","content":"欢迎来到首页"},)"
                  R"({"id":"ordersPage","component":"Text","content":"这里展示订单列表"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> tabHome = surfaceSlot.FindComponentById("tabHome");
    std::shared_ptr<Component> tabOrders = surfaceSlot.FindComponentById("tabOrders");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(tabHome, nullptr);
    ASSERT_NE(tabOrders, nullptr);

    ASSERT_EQ(root->GetChildren().size(), 2U);
    auto childIt = root->GetChildren().begin();
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "tabHome");
    ++childIt;
    ASSERT_NE(childIt, root->GetChildren().end());
    EXPECT_EQ((*childIt)->GetComponentId(), "tabOrders");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L1_should_resolve_build_depth_after_all_parent_relations_are_collected)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"aLeaf","component":"Text","text":"leaf"},)"
                             R"({"id":"bMid","component":"Column","children":["aLeaf"]},)"
                             R"({"id":"root","component":"Column","children":["bMid"]})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> mid = surfaceSlot.FindComponentById("bMid");
    std::shared_ptr<Component> leaf = surfaceSlot.FindComponentById("aLeaf");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(mid, nullptr);
    ASSERT_NE(leaf, nullptr);

    EXPECT_EQ(root->GetBuildDepth(), 0);
    EXPECT_EQ(mid->GetBuildDepth(), 1);
    EXPECT_EQ(leaf->GetBuildDepth(), 2);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_skip_invalid_direct_descriptors_without_processing_nodes)
{
    SurfaceSlot surfaceSlot;
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Text" }));
    std::map<std::string, JsonValue> descriptors;
    auto emptyIdDescriptor = ParseJson(R"({"id":"","component":"Text","text":"skip"})");
    auto missingComponentDescriptor = ParseJson(R"({"id":"missingComponent","text":"skip"})");
    ASSERT_NE(emptyIdDescriptor, nullptr);
    ASSERT_NE(missingComponentDescriptor, nullptr);
    descriptors.emplace("", emptyIdDescriptor->GetRoot());
    descriptors.emplace("badValue", JsonValue());
    descriptors.emplace("missingComponent", missingComponentDescriptor->GetRoot());

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> root =
        surfaceSlot.BuildRootFromComponents("root", descriptors, hasProcessedNode, sawRootDescriptor);

    EXPECT_EQ(root, nullptr);
    EXPECT_FALSE(hasProcessedNode);
    EXPECT_FALSE(sawRootDescriptor);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L1_should_resolve_empty_parent_and_cycle_parent_relations_without_blocking_build)
{
    SurfaceSlot surfaceSlot;
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));
    surfaceSlot.GetParentsRelations()["orphan"] = "";
    surfaceSlot.GetParentsRelations()["cycleA"] = "cycleB";
    surfaceSlot.GetParentsRelations()["cycleB"] = "cycleA";

    auto rootDescriptor = ParseJson(R"({"id":"root","component":"Column"})");
    auto orphanDescriptor = ParseJson(R"({"id":"orphan","component":"Text","text":"orphan"})");
    auto cycleADescriptor = ParseJson(R"({"id":"cycleA","component":"Text","text":"a"})");
    auto cycleBDescriptor = ParseJson(R"({"id":"cycleB","component":"Text","text":"b"})");
    ASSERT_NE(rootDescriptor, nullptr);
    ASSERT_NE(orphanDescriptor, nullptr);
    ASSERT_NE(cycleADescriptor, nullptr);
    ASSERT_NE(cycleBDescriptor, nullptr);

    std::map<std::string, JsonValue> descriptors { { "cycleA", cycleADescriptor->GetRoot() },
        { "cycleB", cycleBDescriptor->GetRoot() }, { "orphan", orphanDescriptor->GetRoot() },
        { "root", rootDescriptor->GetRoot() } };
    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> root =
        surfaceSlot.BuildRootFromComponents("root", descriptors, hasProcessedNode, sawRootDescriptor);

    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);
    EXPECT_EQ(surfaceSlot.ResolveBuildDepthForTest("orphan"), 0);
    EXPECT_GT(
        surfaceSlot.ResolveBuildDepthForTest("cycleA"), static_cast<int32_t>(surfaceSlot.GetParentsRelations().size()));
    EXPECT_GT(
        surfaceSlot.ResolveBuildDepthForTest("cycleB"), static_cast<int32_t>(surfaceSlot.GetParentsRelations().size()));
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L1_should_not_reattach_child_when_parent_descriptor_no_longer_declares_child)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto initialMessage = ParseJson(R"({"components":[)"
                                    R"({"id":"root","component":"Column","children":["aChild"]},)"
                                    R"({"id":"aChild","component":"Text","text":"Alpha"})"
                                    R"(]})");
    ASSERT_NE(initialMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(initialMessage->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    std::shared_ptr<Component> child = surfaceSlot.FindComponentById("aChild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(CountInsertChildAtCalls(root->GetNativeView(), child->GetNativeView()), 1);

    auto parentOnlyMessage = ParseJson(R"({"components":[{"id":"root","component":"Column"}]})");
    ASSERT_NE(parentOnlyMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(parentOnlyMessage->GetRoot()));

    EXPECT_EQ(root->GetChildren().size(), 0U);
    EXPECT_EQ(child->GetParent(), nullptr);

    auto childOnlyMessage = ParseJson(R"({"components":[{"id":"aChild","component":"Text","text":"Beta"}]})");
    ASSERT_NE(childOnlyMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(childOnlyMessage->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    child = surfaceSlot.FindComponentById("aChild");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 0U);
    EXPECT_EQ(child->GetParent(), nullptr);
    EXPECT_EQ(CountInsertChildAtCalls(root->GetNativeView(), child->GetNativeView()), 1);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_dispatch_schema_warning_when_container_children_is_empty_array)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message = ParseJson(R"({"components":[{"id":"root","component":"Column","children":[]}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "root");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "Column");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "root.children");
    std::string warningMessage = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(warningMessage.find("children cannot be an empty array"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warning_when_children_template_component_id_is_empty)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message = ParseJson(
        R"({"components":[{"id":"root","component":"Column","children":{"componentId":"","path":"/items"}}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "root");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "Column");
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "root.children.componentId");
    std::string warningMessage = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(warningMessage.find("children.componentId"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_dispatch_schema_warning_when_children_template_path_is_empty)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message = ParseJson(
        R"({"components":[{"id":"root","component":"Column","children":{"componentId":"itemTemplate","path":""}}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "root");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "Column");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "root.children.path");
    std::string warningMessage = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(warningMessage.find("children.path"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warning_when_extended_tabs_children_template_path_is_empty)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Tabs", "Extended.TabContent", "Text" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":["tabsMain"]},)"
                  R"({"id":"tabsMain","component":"Tabs","children":{"componentId":"tabTemplate","path":""}},)"
                  R"({"id":"tabTemplate","component":"Extended.TabContent","title":"Home","children":["pageHome"]},)"
                  R"({"id":"pageHome","component":"Text","content":"alpha"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "tabsMain.children.path"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warning_when_extended_tab_content_child_is_invalid)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Extended.TabContent", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["tabHome","tabOrders"]},)"
                             R"({"id":"tabHome","component":"Extended.TabContent","title":"Home","child":123},)"
                             R"({"id":"tabOrders","component":"Extended.TabContent","title":"Orders","child":""},)"
                             R"({"id":"pageHome","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "tabHome.child"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "tabOrders.child"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warning_when_extended_tab_content_children_shape_is_invalid)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Extended.TabContent", "Text" }));

    auto message = ParseJson(
        R"({"components":[)"
        R"({"id":"root","component":"Column","children":["tabHome","tabOrders","tabMore"]},)"
        R"({"id":"tabHome","component":"Extended.TabContent","title":"Home","children":123},)"
        R"({"id":"tabOrders","component":"Extended.TabContent","title":"Orders","children":{"componentId":"","path":"/items"}},)"
        R"({"id":"tabMore","component":"Extended.TabContent","title":"More","children":{"componentId":"itemTemplate","path":""}},)"
        R"({"id":"itemTemplate","component":"Text","content":"alpha"})"
        R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "tabHome.children"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "tabOrders.children.componentId"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "tabMore.children.path"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_dispatch_type_mismatch_when_component_id_is_not_string)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["title"]},)"
                             R"({"id":"title","component":"Text","text":"alpha"},)"
                             R"({"id":123,"component":"Column","children":["title"]})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "id"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "id"), 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_schema_warning_when_children_template_component_id_is_empty_and_render_id_is_non_positive)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetRenderId(-1);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message = ParseJson(
        R"({"components":[{"id":"root","component":"Column","children":{"componentId":"","path":"/items"}}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_schema_warning_when_children_template_path_does_not_start_with_slash)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"items"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warning_when_children_template_path_target_is_not_array)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto model = ParseJson(R"({"value":{"items":{"name":"alpha"}}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":"row"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")),
        SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "root");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "itemName")), "Column");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "root.children.path");
    std::string warningMessage = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"));
    EXPECT_NE(warningMessage.find("expects array"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_schema_warning_when_container_children_is_empty_array_and_render_id_is_non_positive)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetRenderId(-1);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column" }));

    auto message = ParseJson(R"({"components":[{"id":"root","component":"Column","children":[]}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_missing_path_error_when_template_children_path_is_missing_after_data_update)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RegisterRuntimeErrorCallback(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetRenderId(COMPONENT_TDD_RENDER_ID);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto model = ParseJson(R"({"value":{"other":[{"name":"alpha"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":"row"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 0U);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_schema_warning_when_template_children_target_is_not_array_and_render_id_is_non_positive)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetRenderId(-1);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto model = ParseJson(R"({"value":{"items":{"name":"alpha"}}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":"row"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_build_column_template_children_when_template_children_path_is_array)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Text","text":"row"})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 2U);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_keep_surface_root_and_mount_recursive_template_children_when_template_path_read)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Column","children":["labelTemplate"]},)"
                  R"({"id":"labelTemplate","component":"Text","text":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(surfaceSlot.GetRootComponent(), root);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    std::shared_ptr<Component> row0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:rowTemplate");
    std::shared_ptr<Component> row1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:rowTemplate");
    std::shared_ptr<Component> text0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:labelTemplate");
    std::shared_ptr<Component> text1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:labelTemplate");
    ASSERT_NE(row0, nullptr);
    ASSERT_NE(row1, nullptr);
    ASSERT_NE(text0, nullptr);
    ASSERT_NE(text1, nullptr);

    EXPECT_EQ(row0->GetParentId(), "root");
    EXPECT_EQ(row1->GetParentId(), "root");
    EXPECT_EQ(text0->GetParentId(), row0->GetComponentId());
    EXPECT_EQ(text1->GetParentId(), row1->GetComponentId());
    ASSERT_EQ(row0->GetChildren().size(), 1U);
    ASSERT_EQ(row1->GetChildren().size(), 1U);
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), row0->GetNativeView(), 0));
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), row1->GetNativeView(), 1));
    EXPECT_TRUE(HasInsertChildAtCall(row0->GetNativeView(), text0->GetNativeView(), 0));
    EXPECT_TRUE(HasInsertChildAtCall(row1->GetNativeView(), text1->GetNativeView(), 0));
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_align_extended_container_template_refresh_with_standard_components)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Row", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}},)"
                  R"({"id":"rowTemplate","component":"Row","children":["labelTemplate"]},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedColumnComponent>(root), nullptr);
    ASSERT_EQ(surfaceSlot.GetRootComponent(), root);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    std::shared_ptr<Component> row0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:rowTemplate");
    std::shared_ptr<Component> row1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:rowTemplate");
    std::shared_ptr<Component> text0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:labelTemplate");
    std::shared_ptr<Component> text1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:labelTemplate");
    ASSERT_NE(row0, nullptr);
    ASSERT_NE(row1, nullptr);
    ASSERT_NE(text0, nullptr);
    ASSERT_NE(text1, nullptr);
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedRowComponent>(row0), nullptr);
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedTextComponent>(text0), nullptr);

    EXPECT_EQ(row0->GetParentId(), "root");
    EXPECT_EQ(row1->GetParentId(), "root");
    EXPECT_EQ(text0->GetParentId(), row0->GetComponentId());
    EXPECT_EQ(text1->GetParentId(), row1->GetComponentId());
    ASSERT_EQ(row0->GetChildren().size(), 1U);
    ASSERT_EQ(row1->GetChildren().size(), 1U);
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), row0->GetNativeView(), 0));
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), row1->GetNativeView(), 1));
    EXPECT_TRUE(HasInsertChildAtCall(row0->GetNativeView(), text0->GetNativeView(), 0));
    EXPECT_TRUE(HasInsertChildAtCall(row1->GetNativeView(), text1->GetNativeView(), 0));

    auto updatedModel =
        ParseJson(R"({"path":"/items","value":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"}]})");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedModel->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(surfaceSlot.GetRootComponent(), root);
    ASSERT_EQ(root->GetChildren().size(), 2U);
    ExpectStringAttribute(text0->GetNativeView(), NODE_TEXT_CONTENT, "gamma");
    ExpectStringAttribute(text1->GetNativeView(), NODE_TEXT_CONTENT, "delta");

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(surfaceSlot.GetRootComponent(), root);
    ASSERT_EQ(root->GetChildren().size(), 3U);

    text0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:labelTemplate");
    text1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:labelTemplate");
    std::shared_ptr<Component> row2 = surfaceSlot.FindComponentById("/itemsrowTemplate:2:rowTemplate");
    std::shared_ptr<Component> text2 = surfaceSlot.FindComponentById("/itemsrowTemplate:2:labelTemplate");
    ASSERT_NE(text0, nullptr);
    ASSERT_NE(text1, nullptr);
    ASSERT_NE(row2, nullptr);
    ASSERT_NE(text2, nullptr);
    EXPECT_EQ(row2->GetParentId(), "root");
    EXPECT_EQ(text2->GetParentId(), row2->GetComponentId());
    EXPECT_TRUE(HasInsertChildAtCall(root->GetNativeView(), row2->GetNativeView(), 2));
    ExpectStringAttribute(text0->GetNativeView(), NODE_TEXT_CONTENT, "gamma");
    ExpectStringAttribute(text1->GetNativeView(), NODE_TEXT_CONTENT, "delta");
    ExpectStringAttribute(text2->GetNativeView(), NODE_TEXT_CONTENT, "epsilon");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_build_extended_list_template_children_in_lazy_mode)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"labelTemplate","path":"/items"}},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(root);
    ASSERT_NE(extendedList, nullptr);
    EXPECT_TRUE(extendedList->IsLazyMode());
    ASSERT_NE(extendedList->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = extendedList->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);
    EXPECT_EQ(root->GetChildren().size(), 0U);
    EXPECT_EQ(surfaceSlot.FindComponentById("/itemslabelTemplate:0:labelTemplate"), nullptr);
    EXPECT_EQ(surfaceSlot.FindComponentById("/itemslabelTemplate:1:labelTemplate"), nullptr);

    auto updatedModel =
        ParseJson(R"({"path":"/items","value":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"}]})");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedModel->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(root);
    ASSERT_NE(extendedList, nullptr);
    ASSERT_NE(extendedList->GetAdapterNode(), nullptr);
    adapterHandle = extendedList->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
    EXPECT_EQ(root->GetChildren().size(), 0U);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(root);
    ASSERT_NE(extendedList, nullptr);
    ASSERT_NE(extendedList->GetAdapterNode(), nullptr);
    adapterHandle = extendedList->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
    EXPECT_EQ(root->GetChildren().size(), 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_build_empty_extended_list_lazy_adapter_when_template_path_is_missing)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"labelTemplate","path":"/missing"}},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(surfaceSlot.FindComponentById("root"));
    ASSERT_NE(extendedList, nullptr);
    ASSERT_NE(extendedList->GetAdapterNode(), nullptr);
    EXPECT_TRUE(extendedList->IsLazyMode());
    EXPECT_EQ(extendedList->GetAdapterNode()->GetTemplateId(), "labelTemplate");
    EXPECT_EQ(extendedList->GetAdapterNode()->GetDataPath(), "/missing");
    EXPECT_NE(extendedList->GetAdapterNode()->GetDataModel(), nullptr);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[extendedList->GetAdapterNode()->GetHandle()], 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_build_extended_grid_template_children_in_lazy_mode)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Grid", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Grid","children":{"componentId":"labelTemplate","path":"/items"}},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto extendedGrid = std::dynamic_pointer_cast<ExtendedGridComponent>(root);
    ASSERT_NE(extendedGrid, nullptr);
    EXPECT_TRUE(extendedGrid->IsLazyMode());
    ASSERT_NE(extendedGrid->GetAdapterNode(), nullptr);
    ArkUI_NodeAdapterHandle adapterHandle = extendedGrid->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_GRID_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);
    EXPECT_EQ(root->GetChildren().size(), 0U);
    EXPECT_EQ(surfaceSlot.FindComponentById("/itemslabelTemplate:0:labelTemplate"), nullptr);
    EXPECT_EQ(surfaceSlot.FindComponentById("/itemslabelTemplate:1:labelTemplate"), nullptr);

    auto updatedModel =
        ParseJson(R"({"path":"/items","value":[{"name":"gamma"},{"name":"delta"},{"name":"epsilon"}]})");
    ASSERT_NE(updatedModel, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(updatedModel->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    extendedGrid = std::dynamic_pointer_cast<ExtendedGridComponent>(root);
    ASSERT_NE(extendedGrid, nullptr);
    ASSERT_NE(extendedGrid->GetAdapterNode(), nullptr);
    adapterHandle = extendedGrid->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_GRID_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
    EXPECT_EQ(root->GetChildren().size(), 0U);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    extendedGrid = std::dynamic_pointer_cast<ExtendedGridComponent>(root);
    ASSERT_NE(extendedGrid, nullptr);
    ASSERT_NE(extendedGrid->GetAdapterNode(), nullptr);
    adapterHandle = extendedGrid->GetAdapterNode()->GetHandle();
    EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_GRID_NODE_ADAPTER, adapterHandle));
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 3U);
    EXPECT_EQ(root->GetChildren().size(), 0U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_wrap_lazy_extended_grid_items_with_grid_item_node)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Grid", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Grid","children":{"componentId":"labelTemplate","path":"/items"}},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto extendedGrid = std::dynamic_pointer_cast<ExtendedGridComponent>(root);
    ASSERT_NE(extendedGrid, nullptr);
    ASSERT_TRUE(extendedGrid->IsLazyMode());
    ASSERT_NE(extendedGrid->GetAdapterNode(), nullptr);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x3001);
    mockArkUIPtr_->SetNodeAdapterEventType(event, NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);

    ASSERT_TRUE(mockArkUIPtr_->DispatchNodeAdapterEvent(extendedGrid->GetAdapterNode()->GetHandle(), event));

    ASSERT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 1U);
    ArkUI_NodeHandle wrapperNode = mockArkUIPtr_->nodeAdapterEventItems_[event];
    ASSERT_NE(wrapperNode, nullptr);

    ArkUI_NodeHandle gridItemNode = FindCreatedNode(ARKUI_NODE_GRID_ITEM, 0);
    ASSERT_NE(gridItemNode, nullptr);
    EXPECT_EQ(wrapperNode, gridItemNode);

    std::shared_ptr<Component> itemComponent = surfaceSlot.FindComponentById("itemslabelTemplate:0:labelTemplate");
    ASSERT_NE(itemComponent, nullptr);
    EXPECT_TRUE(HasAddChildCall(gridItemNode, itemComponent->GetNativeView()));
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_wrap_lazy_extended_list_items_with_list_item_node)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","children":{"componentId":"labelTemplate","path":"/items"}},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                  R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    auto extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(root);
    ASSERT_NE(extendedList, nullptr);
    ASSERT_TRUE(extendedList->IsLazyMode());
    ASSERT_NE(extendedList->GetAdapterNode(), nullptr);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x3002);
    mockArkUIPtr_->SetNodeAdapterEventType(event, NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);

    ASSERT_TRUE(mockArkUIPtr_->DispatchNodeAdapterEvent(extendedList->GetAdapterNode()->GetHandle(), event));

    ASSERT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 1U);
    ArkUI_NodeHandle wrapperNode = mockArkUIPtr_->nodeAdapterEventItems_[event];
    ASSERT_NE(wrapperNode, nullptr);

    ArkUI_NodeHandle listItemNode = FindCreatedNode(ARKUI_NODE_LIST_ITEM, 0);
    ASSERT_NE(listItemNode, nullptr);
    EXPECT_EQ(wrapperNode, listItemNode);

    std::shared_ptr<Component> itemComponent = surfaceSlot.FindComponentById("itemslabelTemplate:0:labelTemplate");
    ASSERT_NE(itemComponent, nullptr);
    EXPECT_TRUE(HasAddChildCall(listItemNode, itemComponent->GetNativeView()));
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_accept_extended_list_component_specific_styles_without_undefined_field_warnings)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"List","children":["first","second"],"styles":{)"
                             R"("listDirection":"horizontal",)"
                             R"("scrollBar":"on",)"
                             R"("nestedScroll":{"scrollForward":"parentFirst","scrollBackward":"selfFirst"}}},)"
                             R"({"id":"first","component":"Text","content":"first"},)"
                             R"({"id":"second","component":"Text","content":"second"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "root.styles.listDirection"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "root.styles.scrollBar"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "root.styles.nestedScroll"), 0U);
}

/**
 * @tc.name: L0_should_accept_expression_values_for_extended_container_specific_styles
 * @tc.desc: Verify expression values are accepted by extended container-specific styles without schema warnings.
 * @tc.type: FUNC
 */
TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_accept_expression_values_for_extended_container_specific_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(
        CreateExtendedProtocolCatalog({ "Column", "Row", "Stack", "Grid", "List", "NavContainer", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"column","component":"Column","styles":{)"
                             R"("justifyContent":"{{ 'spaceBetween' }}","alignItems":"{{ 'center' }}"}},)"
                             R"({"id":"row","component":"Row","styles":{)"
                             R"("wrap":"{{ 'wrap' }}","justifyContent":"{{ 'center' }}",)"
                             R"("alignItems":"{{ 'bottom' }}"}},)"
                             R"({"id":"stack","component":"Stack","styles":{"alignContent":"{{ 'bottomEnd' }}"}},)"
                             R"({"id":"grid","component":"Grid","styles":{)"
                             R"("columnsTemplate":"{{ '1fr 1fr' }}",)"
                             R"("rowsTemplate":{"xs":"{{ '1fr' }}","md":"{{ '1fr 1fr' }}"},)"
                             R"("columnsGap":"{{ 6 }}","rowsGap":"{{ 8 }}" }},)"
                             R"({"id":"list","component":"List","styles":{)"
                             R"("listDirection":"{{ 'horizontal' }}","scrollBar":"{{ 'on' }}",)"
                             R"("nestedScroll":{"scrollForward":"{{ 'parentFirst' }}",)"
                             R"("scrollBackward":"{{ 'selfFirst' }}"}}},)"
                             R"({"id":"nav","component":"NavContainer","children":["page0","page1"],)"
                             R"("currentIndex":"{{ 1 }}"},)"
                             R"({"id":"page0","component":"Text","content":"page0"},)"
                             R"({"id":"page1","component":"Text","content":"page1"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "column.styles.justifyContent"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "column.styles.alignItems"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.wrap"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.justifyContent"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.alignItems"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "stack.styles.alignContent"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.columnsTemplate"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.rowsTemplate"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.columnsGap"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.rowsGap"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.listDirection"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.scrollBar"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.nestedScroll"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "nav.currentIndex"), 0U);

    std::shared_ptr<Component> grid = surfaceSlot.FindComponentById("grid");
    ASSERT_NE(grid, nullptr);
    ExpectStringAttribute(grid->GetNativeView(), NODE_GRID_COLUMN_TEMPLATE, "1fr 1fr");
    const NativeAttributeCall* rowsTemplate = FindLastAttributeCall(grid->GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(rowsTemplate, nullptr);
    EXPECT_TRUE(rowsTemplate->stringValue == "1fr" || rowsTemplate->stringValue == "1fr 1fr");
    ExpectF32Attribute(grid->GetNativeView(), NODE_GRID_COLUMN_GAP, 6.0F);
    ExpectF32Attribute(grid->GetNativeView(), NODE_GRID_ROW_GAP, 8.0F);

    std::shared_ptr<Component> list = surfaceSlot.FindComponentById("list");
    ASSERT_NE(list, nullptr);
    ExpectI32Attribute(list->GetNativeView(), NODE_LIST_DIRECTION, ARKUI_AXIS_HORIZONTAL);
    ExpectI32Attribute(list->GetNativeView(), NODE_SCROLL_BAR_DISPLAY_MODE, ARKUI_SCROLL_BAR_DISPLAY_MODE_ON);
    const NativeAttributeCall* nestedScroll = FindLastAttributeCall(list->GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_recover_private_style_expression_when_missing_dependency_arrives)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Stack", "Text" }));

    auto initialData = ParseJson(R"({"value":{"stack":{}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Stack","children":["content"],)"
                             R"("styles":{"alignContent":)"
                             R"("{{ (10 / $__dataModel.stack.divisor) > 1 ? 'bottomEnd' : 'center' }}"}},)"
                             R"({"id":"content","component":"Text","content":"content"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> stack = surfaceSlot.FindComponentById("root");
    ASSERT_NE(stack, nullptr);
    const auto& bindings = stack->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "styles.alignContent");
    EXPECT_EQ(bindings[0].type_, BindingType::EXPRESSION);
    EXPECT_EQ(bindings[0].dataPath_, "/stack/divisor");

    auto dataUpdate = ParseJson(R"({"path":"/stack/divisor","value":2})");
    ASSERT_NE(dataUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataUpdate->GetRoot()));

    const NativeAttributeCall* align = FindLastAttributeCall(stack->GetNativeView(), NODE_STACK_ALIGN_CONTENT);
    ASSERT_NE(align, nullptr);
    ASSERT_FALSE(align->values.empty());
    EXPECT_EQ(align->values[0].i32, ARKUI_ALIGNMENT_BOTTOM_END);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_column_item_margin_expression_when_data_model_changes)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Text" }));

    auto initialData = ParseJson(R"({"value":{"spacing":{"column":8}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["first","second"],)"
                             R"("itemMargin":"{{ $__dataModel.spacing.column }}"},)"
                             R"({"id":"first","component":"Text","content":"first"},)"
                             R"({"id":"second","component":"Text","content":"second"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> first = surfaceSlot.FindComponentById("first");
    ASSERT_NE(first, nullptr);
    const NativeAttributeCall* initialMargin = FindLastAttributeCall(first->GetNativeView(), NODE_MARGIN);
    ASSERT_NE(initialMargin, nullptr);
    ASSERT_EQ(initialMargin->values.size(), 4U);
    EXPECT_FLOAT_EQ(initialMargin->values[2].f32, 4.0F);

    auto dataUpdate = ParseJson(R"({"path":"/spacing/column","value":20})");
    ASSERT_NE(dataUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataUpdate->GetRoot()));

    const NativeAttributeCall* updatedMargin = FindLastAttributeCall(first->GetNativeView(), NODE_MARGIN);
    ASSERT_NE(updatedMargin, nullptr);
    ASSERT_EQ(updatedMargin->values.size(), 4U);
    EXPECT_FLOAT_EQ(updatedMargin->values[2].f32, 10.0F);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_validate_negative_container_spacing_expression_results)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "List", "Text" }));

    auto initialData = ParseJson(R"({"value":{"spacing":{"column":-2,"list":-4}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["items","tail"],)"
                             R"("itemMargin":"{{ $__dataModel.spacing.column }}"},)"
                             R"({"id":"items","component":"List","children":["content"],)"
                             R"("space":"{{ $__dataModel.spacing.list }}"},)"
                             R"({"id":"content","component":"Text","content":"content"},)"
                             R"({"id":"tail","component":"Text","content":"tail"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.itemMargin"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "items.space"), 1U);

    std::shared_ptr<Component> list = surfaceSlot.FindComponentById("items");
    ASSERT_NE(list, nullptr);
    const NativeAttributeCall* margin = FindLastAttributeCall(list->GetNativeView(), NODE_MARGIN);
    ASSERT_NE(margin, nullptr);
    ASSERT_EQ(margin->values.size(), 4U);
    EXPECT_FLOAT_EQ(margin->values[2].f32, 4.0F);
    const NativeAttributeCall* space = FindLastAttributeCall(list->GetNativeView(), NODE_LIST_SPACE);
    ASSERT_NE(space, nullptr);
    ASSERT_FALSE(space->values.empty());
    EXPECT_FLOAT_EQ(space->values[0].f32, 0.0F);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_list_space_expression_when_data_model_changes)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto initialData = ParseJson(R"({"value":{"spacing":{"list":4}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"List","children":["content"],)"
                             R"("space":"{{ $__dataModel.spacing.list }}"},)"
                             R"({"id":"content","component":"Text","content":"content"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> list = surfaceSlot.FindComponentById("root");
    ASSERT_NE(list, nullptr);
    const NativeAttributeCall* initialSpace = FindLastAttributeCall(list->GetNativeView(), NODE_LIST_SPACE);
    ASSERT_NE(initialSpace, nullptr);
    ASSERT_FALSE(initialSpace->values.empty());
    EXPECT_FLOAT_EQ(initialSpace->values[0].f32, 4.0F);

    auto dataUpdate = ParseJson(R"({"path":"/spacing/list","value":14})");
    ASSERT_NE(dataUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataUpdate->GetRoot()));

    const NativeAttributeCall* updatedSpace = FindLastAttributeCall(list->GetNativeView(), NODE_LIST_SPACE);
    ASSERT_NE(updatedSpace, nullptr);
    ASSERT_FALSE(updatedSpace->values.empty());
    EXPECT_FLOAT_EQ(updatedSpace->values[0].f32, 14.0F);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_resolve_row_layout_fields_from_top_level_and_prefer_styles)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Row" }));

    auto initialData = ParseJson(R"({"value":{"row":{"justify":"end","align":"top","wrap":"noWrap"}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto topLevelMessage = ParseJson(R"({"components":[)"
                                     R"({"id":"root","component":"Row","children":[],)"
                                     R"("justifyContent":"{{ $__dataModel.row.justify }}",)"
                                     R"("alignItems":"{{ $__dataModel.row.align }}",)"
                                     R"("wrap":"{{ $__dataModel.row.wrap }}"})"
                                     R"(]})");
    ASSERT_NE(topLevelMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(topLevelMessage->GetRoot()));

    std::shared_ptr<Component> row = surfaceSlot.FindComponentById("root");
    ASSERT_NE(row, nullptr);
    const NativeAttributeCall* topLevelOption = FindLastAttributeCall(row->GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(topLevelOption, nullptr);
    ASSERT_GE(topLevelOption->values.size(), 4U);
    EXPECT_EQ(topLevelOption->values[1].i32, ARKUI_FLEX_WRAP_NO_WRAP);
    EXPECT_EQ(topLevelOption->values[2].i32, ARKUI_FLEX_ALIGNMENT_END);
    EXPECT_EQ(topLevelOption->values[3].i32, ARKUI_ITEM_ALIGNMENT_START);

    auto styleMessage = ParseJson(R"({"components":[)"
                                  R"({"id":"root","component":"Row","children":[],)"
                                  R"("justifyContent":"{{ $__dataModel.row.justify }}",)"
                                  R"("alignItems":"{{ $__dataModel.row.align }}",)"
                                  R"("wrap":"{{ $__dataModel.row.wrap }}",)"
                                  R"("styles":{"justifyContent":"center","alignItems":"bottom","wrap":"wrap"}})"
                                  R"(]})");
    ASSERT_NE(styleMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(styleMessage->GetRoot()));

    const NativeAttributeCall* styleOption = FindLastAttributeCall(row->GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(styleOption, nullptr);
    ASSERT_GE(styleOption->values.size(), 4U);
    EXPECT_EQ(styleOption->values[1].i32, ARKUI_FLEX_WRAP_WRAP);
    EXPECT_EQ(styleOption->values[2].i32, ARKUI_FLEX_ALIGNMENT_CENTER);
    EXPECT_EQ(styleOption->values[3].i32, ARKUI_ITEM_ALIGNMENT_END);

    auto topLevelUpdate = ParseJson(R"({"path":"/row","value":{"justify":"start","align":"center","wrap":"noWrap"}})");
    ASSERT_NE(topLevelUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(topLevelUpdate->GetRoot()));

    const NativeAttributeCall* afterTopLevelUpdate = FindLastAttributeCall(row->GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(afterTopLevelUpdate, nullptr);
    ASSERT_GE(afterTopLevelUpdate->values.size(), 4U);
    EXPECT_EQ(afterTopLevelUpdate->values[1].i32, ARKUI_FLEX_WRAP_WRAP);
    EXPECT_EQ(afterTopLevelUpdate->values[2].i32, ARKUI_FLEX_ALIGNMENT_CENTER);
    EXPECT_EQ(afterTopLevelUpdate->values[3].i32, ARKUI_ITEM_ALIGNMENT_END);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_refresh_row_style_wrap_expression_when_data_model_changes)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Row" }));

    auto initialData = ParseJson(R"({"value":{"row":{"wrap":"noWrap"}}})");
    ASSERT_NE(initialData, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(initialData->GetRoot()));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Row","children":[],"wrap":"noWrap",)"
                             R"("styles":{"wrap":"{{ $__dataModel.row.wrap }}"}})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> row = surfaceSlot.FindComponentById("root");
    ASSERT_NE(row, nullptr);
    const NativeAttributeCall* initialOption = FindLastAttributeCall(row->GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(initialOption, nullptr);
    ASSERT_GE(initialOption->values.size(), 2U);
    EXPECT_EQ(initialOption->values[1].i32, ARKUI_FLEX_WRAP_NO_WRAP);

    auto dataUpdate = ParseJson(R"({"path":"/row/wrap","value":"wrap"})");
    ASSERT_NE(dataUpdate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(dataUpdate->GetRoot()));

    const NativeAttributeCall* updatedOption = FindLastAttributeCall(row->GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(updatedOption, nullptr);
    ASSERT_GE(updatedOption->values.size(), 2U);
    EXPECT_EQ(updatedOption->values[1].i32, ARKUI_FLEX_WRAP_WRAP);
}

/**
 * @tc.name: L0_should_dispatch_schema_warnings_for_invalid_expression_values_on_extended_container_styles
 * @tc.desc: Verify resolved expression values are still validated for extended container-specific schema warnings.
 * @tc.type: FUNC
 */
TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warnings_for_invalid_expression_values_on_extended_container_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(
        CreateExtendedProtocolCatalog({ "Column", "Row", "Stack", "Grid", "List", "NavContainer", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"column","component":"Column","styles":{)"
                             R"("justifyContent":"{{ 'diagonal' }}","alignItems":"{{ 1 }}"}},)"
                             R"({"id":"row","component":"Row","styles":{)"
                             R"("wrap":"{{ 'reverse' }}","justifyContent":"{{ '' }}",)"
                             R"("alignItems":"{{ 1 }}"}},)"
                             R"({"id":"stack","component":"Stack","styles":{"alignContent":"{{ 'middle' }}"}},)"
                             R"({"id":"grid","component":"Grid","styles":{)"
                             R"("columnsTemplate":"{{ '' }}","rowsTemplate":"{{ 2 }}",)"
                             R"("columnsGap":"{{ 'wide' }}","rowsGap":"{{ 0 - 1 }}" }},)"
                             R"({"id":"list","component":"List","styles":{)"
                             R"("listDirection":"{{ 'sideways' }}","scrollBar":"{{ 3 }}",)"
                             R"("nestedScroll":"{{ 'sideways' }}" }},)"
                             R"({"id":"nav","component":"NavContainer","children":["page0"],)"
                             R"("currentIndex":"{{ 'first' }}"},)"
                             R"({"id":"page0","component":"Text","content":"page0"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "column.styles.justifyContent"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "column.styles.alignItems"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.wrap"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "row.styles.justifyContent"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "row.styles.alignItems"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "stack.styles.alignContent"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.columnsTemplate"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.rowsTemplate"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "grid.styles.columnsGap"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "grid.styles.rowsGap"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.listDirection"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "list.styles.scrollBar"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.styles.nestedScroll"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "nav.currentIndex"), 1U);
}

/**
 * @tc.name: L0_should_validate_dynamic_object_values_for_extended_grid_and_list_styles
 * @tc.desc: Verify dynamic object values for Grid templates and List nestedScroll keep schema warning behavior.
 * @tc.type: FUNC
 */
TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_validate_dynamic_object_values_for_extended_grid_and_list_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Grid", "List", "Text" }));

    auto model = ParseJson(R"({"value":{)"
                           R"("gridValid":{"xs":"1fr","md":"2fr 1fr"},)"
                           R"("gridType":{"xs":2},)"
                           R"("gridEmpty":{"xs":""},)"
                           R"("gridNoBreakpoints":{"xx":"1fr"},)"
                           R"("nestedValid":{"scrollForward":"parentFirst","scrollBackward":"selfFirst"},)"
                           R"("nestedNumber":1,)"
                           R"("nestedType":{"scrollForward":1},)"
                           R"("nestedInvalid":{"scrollBackward":"sideways"})"
                           R"(}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto message = ParseJson(R"({
        "components": [
            {"id":"gridValid","component":"Grid","styles":{"columnsTemplate":{"path":"/gridValid"}}},
            {"id":"gridType","component":"Grid","styles":{"columnsTemplate":{"path":"/gridType"}}},
            {"id":"gridEmpty","component":"Grid","styles":{"columnsTemplate":{"path":"/gridEmpty"}}},
            {"id":"gridNoBreakpoints","component":"Grid","styles":{"columnsTemplate":{"path":"/gridNoBreakpoints"}}},
            {"id":"listValid","component":"List","styles":{"nestedScroll":{"path":"/nestedValid"}}},
            {"id":"listNumber","component":"List","styles":{"nestedScroll":{"path":"/nestedNumber"}}},
            {"id":"listType","component":"List","styles":{"nestedScroll":{"path":"/nestedType"}}},
            {"id":"listInvalid","component":"List","styles":{"nestedScroll":{"path":"/nestedInvalid"}}}
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "gridValid.styles.columnsTemplate"), 0U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "gridType.styles.columnsTemplate.xs"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "gridEmpty.styles.columnsTemplate.xs"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "gridNoBreakpoints.styles.columnsTemplate"),
        1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "listValid.styles.nestedScroll"), 0U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "listNumber.styles.nestedScroll"), 1U);
    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "listType.styles.nestedScroll.scrollForward"),
        1U);
    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "listInvalid.styles.nestedScroll.scrollBackward"),
        1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_invalid_value_warning_for_extended_row_empty_justify_content)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Row", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Row","children":["title"],"styles":{"justifyContent":""}},)"
                             R"({"id":"title","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.justifyContent"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warnings_for_extended_column_invalid_item_margin_children_and_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","itemMargin":-2,"children":["title","",1],)"
                             R"("styles":{"justifyContent":"diagonal","alignItems":1}},)"
                             R"({"id":"title","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.itemMargin"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "root.children[1]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.children[2]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.justifyContent"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.alignItems"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warnings_for_extended_row_invalid_item_margin_children_and_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Row" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Row","itemMargin":-4,"children":[],)"
                             R"("styles":{"justifyContent":"diagonal","alignItems":0,"wrap":"spiral"}})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.itemMargin"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.children"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.justifyContent"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.alignItems"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.wrap"), 1U);
}

TEST_F(
    SurfaceSlotComponentIntegrationTddTest, L0_should_dispatch_invalid_value_warning_for_negative_extended_list_space)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"List","children":["title"],"space":-1},)"
                             R"({"id":"title","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.space"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warnings_for_extended_list_invalid_children_and_nested_scroll_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "List", "Text" }));

    auto dynamicMessage =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"List","space":-8,"children":["title","",1],)"
                  R"("styles":{"listDirection":"diagonal","scrollBar":1,"nestedScroll":{"path":"/mode"}}},)"
                  R"({"id":"title","component":"Text","content":"alpha"})"
                  R"(]})");
    ASSERT_NE(dynamicMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(dynamicMessage->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.space"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "root.children[1]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.children[2]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.listDirection"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.scrollBar"), 1U);
    size_t nestedScrollInvalidBaseline =
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.nestedScroll");

    auto invalidStringMessage = ParseJson(R"({"components":[)"
                                          R"({"id":"root","component":"List","styles":{"nestedScroll":"sideways"}})"
                                          R"(]})");
    ASSERT_NE(invalidStringMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(invalidStringMessage->GetRoot()));
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.nestedScroll"),
        nestedScrollInvalidBaseline + 1U);

    auto invalidTypeMessage = ParseJson(R"({"components":[)"
                                        R"({"id":"root","component":"List","styles":{"nestedScroll":1}})"
                                        R"(]})");
    ASSERT_NE(invalidTypeMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(invalidTypeMessage->GetRoot()));

    auto invalidObjectMessage = ParseJson(
        R"({"components":[)"
        R"({"id":"root","component":"List","styles":{"nestedScroll":{"scrollForward":1,"scrollBackward":"sideways"}}})"
        R"(]})");
    ASSERT_NE(invalidObjectMessage, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(invalidObjectMessage->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.nestedScroll"), 2U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.nestedScroll.scrollForward"),
        1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.nestedScroll.scrollBackward"),
        1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_type_mismatch_warning_for_extended_grid_columns_gap_string)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Grid", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Grid","children":["title"],"styles":{)"
                             R"("columnsTemplate":"1fr","rowsTemplate":"1fr","columnsGap":"","rowsGap":8}},)"
                             R"({"id":"title","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.styles.columnsGap"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_dispatch_schema_warnings_for_extended_grid_invalid_children_entries)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Grid", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Grid","children":["title","",1],)"
                             R"("styles":{"columnsTemplate":"1fr 1fr","rowsTemplate":""}},)"
                             R"({"id":"title","component":"Text","content":"alpha"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Grid");
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "root.children[1]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "root.children[2]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.styles.rowsTemplate"), 1U);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_build_template_children_for_all_extended_container_types)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);

    const std::vector<std::string> containerTypes = { "Column", "Row", "List", "Stack", "Grid" };
    for (const auto& containerType : containerTypes) {
        SCOPED_TRACE(containerType);
        SurfaceSlot& surfaceSlot =
            surfaceManager->CreateSurface(std::string(COMPONENT_TDD_SURFACE_ID) + "-" + containerType, nullptr);
        surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ containerType, "Text" }));

        auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
        ASSERT_NE(model, nullptr);
        ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

        std::string messageJson = R"({"components":[)"
                                  R"({"id":"root","component":")" +
                                  containerType +
                                  R"(","children":{"componentId":"labelTemplate","path":"/items"}},)"
                                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                                  R"(]})";
        auto message = ParseJson(messageJson);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

        std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
        ASSERT_NE(root, nullptr);
        EXPECT_EQ(root->GetType(), containerType);
        if (containerType == "List") {
            auto extendedList = std::dynamic_pointer_cast<ExtendedListComponent>(root);
            ASSERT_NE(extendedList, nullptr);
            EXPECT_TRUE(extendedList->IsLazyMode());
            ASSERT_NE(extendedList->GetAdapterNode(), nullptr);
            ArkUI_NodeAdapterHandle adapterHandle = extendedList->GetAdapterNode()->GetHandle();
            EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_LIST_NODE_ADAPTER, adapterHandle));
            EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);
            EXPECT_EQ(root->GetChildren().size(), 0U);
            continue;
        }
        if (containerType == "Grid") {
            auto extendedGrid = std::dynamic_pointer_cast<ExtendedGridComponent>(root);
            ASSERT_NE(extendedGrid, nullptr);
            EXPECT_TRUE(extendedGrid->IsLazyMode());
            ASSERT_NE(extendedGrid->GetAdapterNode(), nullptr);
            ArkUI_NodeAdapterHandle adapterHandle = extendedGrid->GetAdapterNode()->GetHandle();
            EXPECT_TRUE(HasAdapterAttribute(root->GetNativeView(), NODE_GRID_NODE_ADAPTER, adapterHandle));
            EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[adapterHandle], 2U);
            EXPECT_EQ(root->GetChildren().size(), 0U);
            continue;
        }
        ASSERT_EQ(root->GetChildren().size(), 2U);

        std::shared_ptr<Component> text0 = surfaceSlot.FindComponentById("/itemslabelTemplate:0:labelTemplate");
        std::shared_ptr<Component> text1 = surfaceSlot.FindComponentById("/itemslabelTemplate:1:labelTemplate");
        ASSERT_NE(text0, nullptr);
        ASSERT_NE(text1, nullptr);
        EXPECT_EQ(text0->GetParentId(), "root");
        EXPECT_EQ(text1->GetParentId(), "root");
        ExpectStringAttribute(text0->GetNativeView(), NODE_TEXT_CONTENT, "alpha");
        ExpectStringAttribute(text1->GetNativeView(), NODE_TEXT_CONTENT, "beta");
    }
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_keep_dispatch_warning_when_button_child_is_empty)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Button" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["btn"]},)"
                             R"({"id":"btn","component":"Button","child":"","action":{"event":{"name":"tap"}}})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")), "ERROR_CODE_REQUIRED_MISS");
    std::string warningPath = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path"));
    EXPECT_NE(warningPath.find("btn.child"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest, L0_should_keep_dispatch_warning_when_button_action_is_missing)
{
    DispatchCallbacks callbacks = RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateNativeCatalog({ "Column", "Button", "Text" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["btn","txt"]},)"
                             R"({"id":"btn","component":"Button","child":"txt"},)"
                             R"({"id":"txt","component":"Text","text":"ok"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(mockNapiPtr_->lastCallFunctionFunc_, callbacks.warningCallback);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")), "ERROR_CODE_REQUIRED_MISS");
    std::string warningPath = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path"));
    EXPECT_NE(warningPath.find("btn.action"), std::string::npos);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_standard_required_warning_for_extended_button_without_action)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Button" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["btn"]},)"
                             R"({"id":"btn","component":"Button","text":"ok"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_REQUIRED_MISS", "btn.action"), 0U);
    std::shared_ptr<Component> button = surfaceSlot.FindComponentById("btn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetType(), "Button");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_dispatch_standard_required_warning_for_extended_button_without_child)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Button" }));

    auto message = ParseJson(R"({"components":[)"
                             R"({"id":"root","component":"Column","children":["btn"]},)"
                             R"({"id":"btn","component":"Button","text":"ok"})"
                             R"(]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_REQUIRED_MISS", "btn.child"), 0U);
    std::shared_ptr<Component> button = surfaceSlot.FindComponentById("btn");
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetType(), "Button");
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_create_template_instances_when_template_descriptor_sent_in_separate_update_components)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Row", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto templateMsg = ParseJson(R"({"components":[)"
                                 R"({"id":"rowTemplate","component":"Row","children":["labelTemplate"]},)"
                                 R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                                 R"(]})");
    ASSERT_NE(templateMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(templateMsg->GetRoot()));

    auto containerMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}})"
                  R"(]})");
    ASSERT_NE(containerMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(containerMsg->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    std::shared_ptr<Component> row0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:rowTemplate");
    std::shared_ptr<Component> row1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:rowTemplate");
    std::shared_ptr<Component> text0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:labelTemplate");
    std::shared_ptr<Component> text1 = surfaceSlot.FindComponentById("/itemsrowTemplate:1:labelTemplate");
    ASSERT_NE(row0, nullptr);
    ASSERT_NE(row1, nullptr);
    ASSERT_NE(text0, nullptr);
    ASSERT_NE(text1, nullptr);

    EXPECT_EQ(row0->GetParentId(), "root");
    EXPECT_EQ(row1->GetParentId(), "root");
    EXPECT_EQ(text0->GetParentId(), row0->GetComponentId());
    EXPECT_EQ(text1->GetParentId(), row1->GetComponentId());
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_use_updated_template_descriptor_when_template_sent_again_in_separate_message)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Row", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha","age":10},{"name":"beta","age":20}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto templateMsg = ParseJson(R"({"components":[)"
                                 R"({"id":"rowTemplate","component":"Row","children":["labelTemplate"]},)"
                                 R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                                 R"(]})");
    ASSERT_NE(templateMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(templateMsg->GetRoot()));

    auto containerMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}})"
                  R"(]})");
    ASSERT_NE(containerMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(containerMsg->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    auto updatedTemplateMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"rowTemplate","component":"Row","children":["labelTemplate","ageTemplate"]},)"
                  R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}},)"
                  R"({"id":"ageTemplate","component":"Text","content":{"path":"age"}})"
                  R"(]})");
    ASSERT_NE(updatedTemplateMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(updatedTemplateMsg->GetRoot()));

    auto refreshMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}})"
                  R"(]})");
    ASSERT_NE(refreshMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(refreshMsg->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    std::shared_ptr<Component> row0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:rowTemplate");
    ASSERT_NE(row0, nullptr);
    ASSERT_EQ(row0->GetChildren().size(), 2U);

    std::shared_ptr<Component> age0 = surfaceSlot.FindComponentById("/itemsrowTemplate:0:ageTemplate");
    ASSERT_NE(age0, nullptr);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_create_template_instances_with_deep_nested_children_across_messages)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Row", "Text" }));

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    auto outerTemplate = ParseJson(R"({"components":[)"
                                   R"({"id":"cardTemplate","component":"Column","children":["rowTemplate"]})"
                                   R"(]})");
    ASSERT_NE(outerTemplate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(outerTemplate->GetRoot()));

    auto innerTemplate = ParseJson(R"({"components":[)"
                                   R"({"id":"rowTemplate","component":"Row","children":["labelTemplate"]},)"
                                   R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                                   R"(]})");
    ASSERT_NE(innerTemplate, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(innerTemplate->GetRoot()));

    auto containerMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"cardTemplate","path":"/items"}})"
                  R"(]})");
    ASSERT_NE(containerMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(containerMsg->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2U);

    std::shared_ptr<Component> card0 = surfaceSlot.FindComponentById("/itemscardTemplate:0:cardTemplate");
    ASSERT_NE(card0, nullptr);
    ASSERT_EQ(card0->GetChildren().size(), 1U);

    std::shared_ptr<Component> row0 = surfaceSlot.FindComponentById("/itemscardTemplate:0:rowTemplate");
    ASSERT_NE(row0, nullptr);
    ASSERT_EQ(row0->GetChildren().size(), 1U);

    std::shared_ptr<Component> text0 = surfaceSlot.FindComponentById("/itemscardTemplate:0:labelTemplate");
    ASSERT_NE(text0, nullptr);
}

TEST_F(SurfaceSlotComponentIntegrationTddTest,
    L0_should_not_create_template_instances_when_data_model_sent_after_container_in_separate_messages)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);
    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
    surfaceSlot.SetCatalog(CreateExtendedProtocolCatalog({ "Column", "Row", "Text" }));

    auto templateMsg = ParseJson(R"({"components":[)"
                                 R"({"id":"rowTemplate","component":"Row","children":["labelTemplate"]},)"
                                 R"({"id":"labelTemplate","component":"Text","content":{"path":"name"}})"
                                 R"(]})");
    ASSERT_NE(templateMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(templateMsg->GetRoot()));

    auto containerMsg =
        ParseJson(R"({"components":[)"
                  R"({"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}})"
                  R"(]})");
    ASSERT_NE(containerMsg, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(containerMsg->GetRoot()));

    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 0U);

    auto model = ParseJson(R"({"value":{"items":[{"name":"alpha"},{"name":"beta"}]}})");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 0U);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(containerMsg->GetRoot()));

    root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    ASSERT_EQ(root->GetChildren().size(), 2U);
}
