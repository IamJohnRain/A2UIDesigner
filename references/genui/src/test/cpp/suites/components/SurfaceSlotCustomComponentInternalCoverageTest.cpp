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

#include <gtest/gtest.h>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/list/ListComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/RenderContext.h"
#include "composition/ChildListDescriptor.h"
#include "data/BindingEngine.h"
#include "utils/JsonAdapter.h"

#include "NapiResourceManager.h"
#include "TestFixture.h"

#define private public
#define protected public
#include "components/custom/CustomComponent.h"

#include "SurfaceSlot.h"
#undef protected
#undef private

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCatalog(
    const std::string& catalogId, std::initializer_list<std::pair<const char*, bool>> components)
{
    auto catalog = std::make_shared<Catalog>(catalogId);
    for (const auto& entry : components) {
        if (entry.first == nullptr || entry.first[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(entry.first, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        item->SetInnerNative(entry.second);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::vector<std::string> ToVector(const std::list<std::string>& values)
{
    return std::vector<std::string>(values.begin(), values.end());
}

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

std::string BuildNestedValueMessage(int depth)
{
    std::string value = "0";
    for (int i = 0; i < depth; ++i) {
        value = std::string("{\"level\":") + value + "}";
    }
    return std::string("{\"value\":") + value + "}";
}

class FakeComponent : public Component {
public:
    explicit FakeComponent(const std::string& type, ArkUI_NodeHandle nativeView = nullptr)
        : Component(nativeView, false), type_(type)
    {}
    std::string GetType() const override
    {
        return type_;
    }

private:
    std::string type_;
};

class TemplateProbeComponent : public FakeComponent {
public:
    explicit TemplateProbeComponent(const std::string& type, ArkUI_NodeHandle nativeView = nullptr)
        : FakeComponent(type, nativeView)
    {}

    bool InvokeExpandTemplateChildrenEager(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
    }
};

class RenderSlotCleanupGuard {
public:
    explicit RenderSlotCleanupGuard(int32_t renderId) : renderId_(renderId) {}
    ~RenderSlotCleanupGuard()
    {
        RenderManager::GetInstance().RemoveRenderSlot(renderId_);
    }

private:
    int32_t renderId_;
};

struct NapiResourceManagerMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref createCustomComponentRef_ = nullptr;
    napi_ref updateCustomComponentRef_ = nullptr;
};

void ResetNapiResourceManagerRefs()
{
    NapiResourceManager* resourceManager = RenderManager::GetInstance().GetNapiResourceManager();
    if (resourceManager == nullptr) {
        return;
    }
    auto* mirror = reinterpret_cast<NapiResourceManagerMirror*>(resourceManager);
    mirror->napiEnv_ = nullptr;
    mirror->createCustomComponentRef_ = nullptr;
    mirror->updateCustomComponentRef_ = nullptr;
}

} // namespace

class SurfaceSlotCustomComponentInternalCoverageTest : public A2UITest {};

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_initialize_native_dialog_api_when_surface_slot_is_created)
{
    ASSERT_NE(mockArkUIPtr_, nullptr);
    EXPECT_EQ(mockArkUIPtr_->nativeDialogApiCallCount_, 0);

    SurfaceSlot slot;

    EXPECT_EQ(mockArkUIPtr_->nativeDialogApiCallCount_, 1);
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_prepare_descriptor_and_component_reuse_branches)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("prepare_surface");
    slot.SetRenderId(1201);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true }, { "Button", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        1,
        {"id":"","component":"Text"},
        {"id":"same","component":"Text","text":"first"},
        {"id":"same","component":"Text","text":"second"},
        {"id":"same","component":"Button","text":"ignored"},
        {"id":"same"},
        {"id":"replace","component":"Text","text":"keep"},
        {"id":"replace","text":"ignored-missing-type"},
        {"id":"modal1","component":"Modal","trigger":"btn","content":"card"}
    ])");
    ASSERT_NE(descriptors, nullptr);
    slot.PrepareDescriptorById(descriptors->GetRoot());

    ASSERT_EQ(slot.descriptorsById_.size(), 3u);
    EXPECT_EQ(slot.descriptorsById_["same"].GetString("component", ""), "Text");
    EXPECT_EQ(slot.descriptorsById_["same"].GetString("text", ""), "second");
    EXPECT_EQ(slot.descriptorsById_["replace"].GetString("component", ""), "Text");

    bool isNewNode = false;
    std::shared_ptr<Component> createdNode =
        slot.GetOrCreateComponentNode(slot.descriptorsById_["same"], "same", "Text", isNewNode);
    ASSERT_NE(createdNode, nullptr);
    EXPECT_TRUE(isNewNode);

    std::shared_ptr<Component> reusedNode =
        slot.GetOrCreateComponentNode(slot.descriptorsById_["same"], "same", "Text", isNewNode);
    EXPECT_EQ(reusedNode, createdNode);
    EXPECT_FALSE(isNewNode);

    std::shared_ptr<Component> typeMismatchNode =
        slot.GetOrCreateComponentNode(slot.descriptorsById_["same"], "same", "Button", isNewNode);
    EXPECT_EQ(typeMismatchNode, nullptr);

    slot.allComponents_["ghost"] = nullptr;
    std::shared_ptr<Component> recreatedNode =
        slot.GetOrCreateComponentNode(slot.descriptorsById_["same"], "ghost", "Text", isNewNode);
    ASSERT_NE(recreatedNode, nullptr);
    EXPECT_TRUE(isNewNode);
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_normalize_extended_component_type_by_catalog_lookup)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("extended_normalize_surface");
    slot.SetRenderId(1301);
    slot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "NavContainer", false }, { "Text", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {"id":"nav_exact","component":"NavContainer"},
        {"id":"text_short","component":"Text"},
        {"id":"empty_component","component":""},
        {"id":"dot_type","component":"Custom.NavContainer"},
        {"id":"unknown","component":"UnknownType"}
    ])");
    ASSERT_NE(descriptors, nullptr);
    slot.PrepareDescriptorById(descriptors->GetRoot());

    ASSERT_EQ(slot.descriptorsById_.size(), 5u);
    EXPECT_EQ(slot.descriptorsById_["nav_exact"].GetString("component", ""), "NavContainer");
    EXPECT_EQ(slot.descriptorsById_["text_short"].GetString("component", ""), "Text");
    EXPECT_EQ(slot.descriptorsById_["empty_component"].GetString("component", ""), "");
    EXPECT_EQ(slot.descriptorsById_["dot_type"].GetString("component", ""), "Custom.NavContainer");
    EXPECT_EQ(slot.descriptorsById_["unknown"].GetString("component", ""), "UnknownType");
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_skip_extended_component_type_normalization_when_inactive)
{
    SurfaceSlot standardSlot;
    standardSlot.SetSurfaceId("standard_surface");
    standardSlot.SetRenderId(1302);
    standardSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "NavContainer", false } }));

    std::unique_ptr<JsonAdapter> standardDescriptors = ParseJson(R"([
        {"id":"nav","component":"NavContainer"}
    ])");
    ASSERT_NE(standardDescriptors, nullptr);
    standardSlot.PrepareDescriptorById(standardDescriptors->GetRoot());
    ASSERT_EQ(standardSlot.descriptorsById_.size(), 1u);
    EXPECT_EQ(standardSlot.descriptorsById_["nav"].GetString("component", ""), "NavContainer");

    SurfaceSlot fallbackSlot;
    fallbackSlot.SetSurfaceId("fallback_surface");
    fallbackSlot.SetRenderId(1303);
    fallbackSlot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);

    std::unique_ptr<JsonAdapter> fallbackDescriptors = ParseJson(R"([
        {"id":"nav","component":"NavContainer"}
    ])");
    ASSERT_NE(fallbackDescriptors, nullptr);
    fallbackSlot.PrepareDescriptorById(fallbackDescriptors->GetRoot());
    ASSERT_EQ(fallbackSlot.descriptorsById_.size(), 1u);
    EXPECT_EQ(fallbackSlot.descriptorsById_["nav"].GetString("component", ""), "NavContainer");
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest,
    should_normalize_extended_prefixed_component_to_short_type_only_when_catalog_has_short_name)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("extended_prefixed_surface");
    slot.SetRenderId(1304);
    slot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {"id":"short_match","component":"Extended.Text"},
        {"id":"keep_prefixed","component":"Extended.Unknown"}
    ])");
    ASSERT_NE(descriptors, nullptr);

    slot.PrepareDescriptorById(descriptors->GetRoot());

    ASSERT_EQ(slot.descriptorsById_.size(), 2u);
    EXPECT_EQ(slot.descriptorsById_["short_match"].GetString("component", ""), "Text");
    EXPECT_EQ(slot.descriptorsById_["keep_prefixed"].GetString("component", ""), "Extended.Unknown");
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest,
    should_collect_children_if_children_else_and_tabs_when_building_explicit_root)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("reachable_checkbox_surface");
    slot.SetRenderId(1305);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID,
        { { "Column", true }, { "Text", true }, { "Checkbox", true }, { "CheckboxGroup", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {"id":"root","component":"Column","children":["branch","tabsHost","groupHost"]},
        {"id":"branch","component":"Text","childrenIf":["ifChild"],"childrenElse":["elseChild","missingReachable"]},
        {"id":"ifChild","component":"Text","text":"if"},
        {"id":"elseChild","component":"Text","text":"else"},
        {"id":"tabsHost","component":"Text","tabs":[{"child":"tabChild"},1,{"child":""}]},
        {"id":"tabChild","component":"Text","text":"tab"},
        {"id":"groupHost","component":"CheckboxGroup","group":"group-a"},
        {"id":"checkboxReachable","component":"Checkbox","group":"group-a"},
        {"id":"checkboxIgnored","component":"Checkbox","group":"group-b"},
        {"id":"checkboxNoGroup","component":"Checkbox"}
    ])");
    ASSERT_NE(descriptors, nullptr);

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> root =
        slot.BuildRootFromComponents(descriptors->GetRoot(), hasProcessedNode, sawRootDescriptor);

    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);
    EXPECT_NE(slot.FindComponentById("ifChild"), nullptr);
    EXPECT_NE(slot.FindComponentById("elseChild"), nullptr);
    EXPECT_NE(slot.FindComponentById("tabChild"), nullptr);
    EXPECT_EQ(slot.FindComponentById("checkboxIgnored"), nullptr);
    EXPECT_EQ(slot.FindComponentById("checkboxNoGroup"), nullptr);
}

TEST_F(
    SurfaceSlotCustomComponentInternalCoverageTest, should_ignore_non_array_tabs_when_collecting_reachable_descriptors)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("reachable_non_array_tabs_surface");
    slot.SetRenderId(1306);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Column", true }, { "Text", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {"id":"root","component":"Column","children":["tabsHost"]},
        {"id":"tabsHost","component":"Text","tabs":{"child":"tabChild"}},
        {"id":"tabChild","component":"Text","text":"tab"}
    ])");
    ASSERT_NE(descriptors, nullptr);

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> root =
        slot.BuildRootFromComponents(descriptors->GetRoot(), hasProcessedNode, sawRootDescriptor);

    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);
    EXPECT_EQ(slot.FindComponentById("tabChild"), nullptr);
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest,
    should_resolve_extended_tabs_template_child_ids_from_surface_data_model)
{
    const int32_t renderId = 1307;
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);

    SurfaceSlot& surfaceSlot = surfaceManager->CreateSurface("extended_tabs_template_surface", nullptr);
    surfaceSlot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", true } }));

    std::unique_ptr<JsonAdapter> model = ParseJson(R"({
        "value": {
            "tabs": [
                { "title": "Home" },
                { "title": "Orders" }
            ]
        }
    })");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(model->GetRoot()));

    CustomComponent component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetSurfaceId("extended_tabs_template_surface");
    component.SetRenderId(renderId);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(R"({
        "children": {
            "componentId": "tabTemplate",
            "path": "/tabs"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    std::vector<std::string> childIds = ToVector(component.ResolveTabsChildIds(descriptor->GetRoot()));
    ASSERT_EQ(childIds.size(), 2U);
    EXPECT_EQ(childIds[0], "/tabstabTemplate:0:tabTemplate");
    EXPECT_EQ(childIds[1], "/tabstabTemplate:1:tabTemplate");
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_custom_component_child_slot_branches)
{
    auto tabs = std::make_shared<CustomComponent>("Tabs");
    auto childA = std::make_shared<FakeComponent>("Text", reinterpret_cast<ArkUI_NodeHandle>(0x101));
    auto childB = std::make_shared<FakeComponent>("Text", reinterpret_cast<ArkUI_NodeHandle>(0x102));
    childA->SetComponentId("tabA");
    childB->SetComponentId("tabB");

    tabs->OnAddChild(nullptr, 0);
    tabs->OnMoveChild(nullptr, 0, 1);

    tabs->childSlotHandle_ = reinterpret_cast<ArkUI_NodeContentHandle>(0x201);
    tabs->OnAddChild(childA, 0);
    tabs->OnMoveChild(childA, 0, 1);

    tabs->childToSlotMapping_["tabA"] = "tab-0";
    tabs->childSlotHandles_["tab-0"] = reinterpret_cast<ArkUI_NodeContentHandle>(0x301);
    tabs->OnAddChild(childA, 1);
    tabs->OnMoveChild(childA, 1, 0);

    tabs->AddChild(childA);
    tabs->AddChild(childB);
    tabs->RemoveAllChildren();
    EXPECT_TRUE(tabs->GetChildren().empty());

    tabs->childSlotHandle_ = reinterpret_cast<ArkUI_NodeContentHandle>(0x401);
    tabs->OnRemoveChild(childA);
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_custom_component_validation_and_property_paths)
{
    CustomComponent component("Tabs");
    std::unique_ptr<JsonAdapter> checksDescriptor = ParseJson(R"({
        "checks": [
            {"condition": {"call": "unsupported"}, "message": "skip"}
        ]
    })");
    ASSERT_NE(checksDescriptor, nullptr);
    component.ParseChecks(checksDescriptor->GetRoot());

    std::string failedMessage;
    EXPECT_TRUE(component.ValidateChecks(R"(["text-target"])", &failedMessage));
    EXPECT_TRUE(component.ValidateChecks(R"({"value":"object-target"})", &failedMessage));
    EXPECT_FALSE(component.currentCheckTargetValue_.IsValid());

    std::unique_ptr<JsonAdapter> labelValue = JsonAdapter::CreateString("label-value");
    std::unique_ptr<JsonAdapter> descValue = JsonAdapter::CreateString("desc-value");
    std::unique_ptr<JsonAdapter> styleValue = JsonAdapter::Parse(R"({"width": 12})");
    ASSERT_NE(labelValue, nullptr);
    ASSERT_NE(descValue, nullptr);
    ASSERT_NE(styleValue, nullptr);

    component.OnPropertyApplied("style", styleValue->GetRoot());
    EXPECT_TRUE(component.properties_.find("style") != component.properties_.end());
    component.OnPropertyApplied("style", JsonValue());
    EXPECT_TRUE(component.properties_.find("style") == component.properties_.end());
    component.OnPropertyRemoved("style");

    component.hasCreatedCustomComponent_ = true;

    component.OnDataUpdate("accessibility.label", labelValue->GetRoot());
    component.OnDataUpdate("accessibility.description", descValue->GetRoot());
    component.OnDataUpdate("unknown", styleValue->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityDescription);
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_surface_slot_content_and_model_branches)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("surface_content");
    auto rootColumn = std::make_shared<FakeComponent>("Column", reinterpret_cast<ArkUI_NodeHandle>(0x601));
    slot.SetRootComponent(rootColumn);

    ArkUI_NodeContentHandle content1 = reinterpret_cast<ArkUI_NodeContentHandle>(0x701);
    ArkUI_NodeContentHandle content2 = reinterpret_cast<ArkUI_NodeContentHandle>(0x702);
    slot.SetContentHandle(content1);
    EXPECT_EQ(slot.GetContentHandle(), content1);
    EXPECT_EQ(slot.GetRootComponent(), rootColumn);
    slot.SetContentHandle(content2);
    slot.SetContentHandle(content2);

    slot.SetForceRootFill(true);
    slot.SetForceRootFill(true);
    slot.SetRootComponent(nullptr);
    slot.SetContentHandle(nullptr);

    EXPECT_FALSE(slot.UpdateComponents(JsonValue()));
    std::unique_ptr<JsonAdapter> nonArray = ParseJson(R"({"k":"v"})");
    ASSERT_NE(nonArray, nullptr);
    EXPECT_FALSE(slot.UpdateComponentsArray(nonArray->GetRoot()));

    EXPECT_FALSE(slot.UpdateDataModel(JsonValue()));
    std::unique_ptr<JsonAdapter> arrayRoot = ParseJson(R"([1,2,3])");
    ASSERT_NE(arrayRoot, nullptr);
    EXPECT_FALSE(slot.UpdateDataModel(arrayRoot->GetRoot()));

    std::unique_ptr<JsonAdapter> updateMsg = ParseJson(R"({"path":"/name","value":"alice"})");
    std::unique_ptr<JsonAdapter> deleteMsg = ParseJson(R"({"path":"/name"})");
    std::unique_ptr<JsonAdapter> replaceMsg = ParseJson(R"({"value":{"name":"bob"}})");
    std::unique_ptr<JsonAdapter> invalidMsg = ParseJson(R"({})");
    ASSERT_NE(updateMsg, nullptr);
    ASSERT_NE(deleteMsg, nullptr);
    ASSERT_NE(replaceMsg, nullptr);
    ASSERT_NE(invalidMsg, nullptr);
    EXPECT_TRUE(slot.UpdateDataModel(updateMsg->GetRoot()));
    EXPECT_TRUE(slot.UpdateDataModel(deleteMsg->GetRoot()));
    EXPECT_TRUE(slot.UpdateDataModel(replaceMsg->GetRoot()));
    EXPECT_FALSE(slot.UpdateDataModel(invalidMsg->GetRoot()));

    slot.bindingEngine_.reset();
    EXPECT_FALSE(slot.UpdateDataModel(replaceMsg->GetRoot()));

    SurfaceSlot catalogSlot;
    catalogSlot.SetCatalog(nullptr);
    EXPECT_EQ(catalogSlot.GetCatalog(), nullptr);
    catalogSlot.UpdateSurfaceProtocolMode();
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_component_template_expansion_eager_paths)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("template_surface");
    slot.SetRenderId(90301);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Column", true }, { "Text", true } }));

    TemplateProbeComponent parent("Column", reinterpret_cast<ArkUI_NodeHandle>(0x901));
    parent.SetComponentId("parent");

    ChildListDescriptor childList;
    childList.type = ChildListType::TEMPLATE_PATH;
    childList.templateComponentId = "tpl";
    childList.templatePath = "/items";

    std::list<std::string> childIds;
    EXPECT_FALSE(parent.InvokeExpandTemplateChildrenEager(childList, slot, childIds));

    std::unique_ptr<JsonAdapter> templateDescriptor = ParseJson(R"({
        "id": "tpl",
        "component": "Text",
        "text": {"path": "/name"}
    })");
    ASSERT_NE(templateDescriptor, nullptr);
    slot.descriptorsById_["tpl"] = templateDescriptor->GetRoot();
    slot.allComponentDescriptorStore_["tpl"] = templateDescriptor->GetRoot();

    slot.bindingEngine_.reset();
    EXPECT_FALSE(parent.InvokeExpandTemplateChildrenEager(childList, slot, childIds));

    slot.bindingEngine_ = BindingEngine::Create();
    std::unique_ptr<JsonAdapter> notArrayData = ParseJson(R"({"value":{"items":{"name":"x"}}})");
    ASSERT_NE(notArrayData, nullptr);
    ASSERT_TRUE(slot.UpdateDataModel(notArrayData->GetRoot()));
    EXPECT_FALSE(parent.InvokeExpandTemplateChildrenEager(childList, slot, childIds));

    std::unique_ptr<JsonAdapter> arrayData = ParseJson(R"({"value":{"items":[{"name":"a"},{"name":"b"}]}})");
    ASSERT_NE(arrayData, nullptr);
    ASSERT_TRUE(slot.UpdateDataModel(arrayData->GetRoot()));
    ASSERT_TRUE(parent.InvokeExpandTemplateChildrenEager(childList, slot, childIds));
    EXPECT_EQ(childIds.size(), 2u);
    EXPECT_NE(slot.FindComponentById(childIds.front()), nullptr);
}

/**
 * @tc.name: SurfaceSlotCustomComponentInternalCoverageTest001
 * @tc.desc: Verify SurfaceSlot component routing branches and extended protocol helper paths.
 * @tc.type: FUNC
 */
TEST_F(SurfaceSlotCustomComponentInternalCoverageTest,
    should_cover_surface_slot_component_routing_and_protocol_helper_branches)
{
    SurfaceSlot standardSlot;
    standardSlot.SetSurfaceId("route_standard");
    EXPECT_EQ(standardSlot.BuildComponent("Text"), nullptr);

    standardSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Fancy", false }, { "Button", false } }));
    EXPECT_EQ(standardSlot.BuildComponent(""), nullptr);
    std::shared_ptr<Component> customComponent = standardSlot.BuildComponent("Fancy");
    ASSERT_NE(customComponent, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<CustomComponent>(customComponent), nullptr);
    std::shared_ptr<Component> customButton = standardSlot.BuildComponent("Button");
    ASSERT_NE(customButton, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<CustomComponent>(customButton), nullptr);

    SurfaceSlot extendedSlot;
    extendedSlot.SetSurfaceId("route_extended");
    extendedSlot.SetRenderId(1502);
    extendedSlot.SetCatalog(
        BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Fancy", false }, { "PlainInner", true }, { "Text", false } }));
    extendedSlot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);
    extendedSlot.SetApiVersion(18);

    std::shared_ptr<Component> customRow = extendedSlot.BuildComponent("Custom.Row");
    ASSERT_NE(customRow, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<CustomComponent>(customRow), nullptr);

    EXPECT_EQ(extendedSlot.BuildComponent("MissingType"), nullptr);
    std::shared_ptr<Component> fancyComponent = extendedSlot.BuildComponent("Fancy");
    ASSERT_NE(fancyComponent, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<CustomComponent>(fancyComponent), nullptr);
    EXPECT_EQ(extendedSlot.BuildComponent("PlainInner"), nullptr);
    EXPECT_EQ(extendedSlot.BuildExtendedComponent("Definitely.NotExtended"), nullptr);
    EXPECT_NE(extendedSlot.BuildExtendedComponent("Text"), nullptr);
}

/**
 * @tc.name: SurfaceSlotCustomComponentInternalCoverageTest002
 * @tc.desc: Verify SurfaceSlot accessor, font scale, content handle, and dispose branches.
 * @tc.type: FUNC
 */
TEST_F(SurfaceSlotCustomComponentInternalCoverageTest, should_cover_surface_slot_accessor_and_dispose_paths)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("accessor_surface");
    slot.SetRenderId(1503);

    auto rootA = std::make_shared<FakeComponent>("Column", reinterpret_cast<ArkUI_NodeHandle>(0x1101));
    auto rootB = std::make_shared<FakeComponent>("Column", reinterpret_cast<ArkUI_NodeHandle>(0x1102));
    rootA->SetComponentId("rootA");
    rootB->SetComponentId("rootB");

    ArkUI_NodeContentHandle content = reinterpret_cast<ArkUI_NodeContentHandle>(0x1201);
    slot.SetContentHandle(content);
    slot.SetRootComponent(rootA);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content].front(), rootA->GetNativeView());

    slot.SetRootComponent(rootB);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content].front(), rootB->GetNativeView());

    slot.modalCoordinator_.reset();
    slot.DismissActiveModal();

    auto catalog = BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", false } });
    slot.SetCatalog(catalog);
    slot.SetCatalog(catalog);
    slot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);
    slot.SetApiVersion(18);
    EXPECT_EQ(slot.GetApiVersion(), 18);

    std::shared_ptr<Component> extendedText = slot.BuildComponent("Text");
    ASSERT_NE(extendedText, nullptr);
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedComponent>(extendedText), nullptr);
    slot.allComponents_["extended"] = extendedText;
    slot.allComponents_["null"] = nullptr;

    slot.RegisterComponentIfNeeded(nullptr, false);

    SurfaceSlot nullBindingSlot;
    nullBindingSlot.bindingEngine_.reset();
    nullBindingSlot.RegisterComponentIfNeeded(extendedText, true);

    const SurfaceSlot& constSlot = slot;
    std::vector<std::shared_ptr<Component>> components = constSlot.GetAllComponents();
    ASSERT_EQ(components.size(), 1u);
    EXPECT_EQ(components.front(), extendedText);

    slot.SetFontSizeScale(0.0F);
    EXPECT_FLOAT_EQ(slot.GetFontSizeScale(), 1.0F);
    slot.SetFontSizeScale(1.25F);
    EXPECT_FLOAT_EQ(slot.GetFontSizeScale(), 1.25F);

    slot.Dispose();
    EXPECT_EQ(slot.GetContentHandle(), nullptr);
    EXPECT_EQ(slot.GetRootComponent(), nullptr);
    EXPECT_EQ(slot.GetCatalog(), nullptr);
    EXPECT_EQ(slot.bindingEngine_, nullptr);
}

/**
 * @tc.name: SurfaceSlotCustomComponentInternalCoverageTest003
 * @tc.desc: Verify SurfaceSlot fallback data model update and root build failure branches.
 * @tc.type: FUNC
 */
TEST_F(
    SurfaceSlotCustomComponentInternalCoverageTest, should_cover_surface_slot_default_data_model_and_root_failure_paths)
{
    SurfaceSlot slot;

    std::unique_ptr<JsonAdapter> replaceMsg = ParseJson(R"({"value":{"name":"bob"}})");
    ASSERT_NE(replaceMsg, nullptr);
    EXPECT_TRUE(slot.UpdateDataModel(replaceMsg->GetRoot()));

    std::unique_ptr<JsonAdapter> orphanMessage = ParseJson(R"({
        "components": [
            {"id":"orphan","component":"Text","text":"orphan"}
        ]
    })");
    ASSERT_NE(orphanMessage, nullptr);
    EXPECT_TRUE(slot.UpdateComponents(orphanMessage->GetRoot()));
    EXPECT_EQ(slot.GetRootComponent(), nullptr);

    SurfaceSlot directBuildSlot;
    directBuildSlot.SetSurfaceId("direct_build_surface");
    directBuildSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true } }));
    std::unique_ptr<JsonAdapter> componentArray = ParseJson(R"([
        {"id":"root","component":"Text","text":"hello"}
    ])");
    ASSERT_NE(componentArray, nullptr);
    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> directRoot =
        directBuildSlot.BuildRootFromComponents(componentArray->GetRoot(), hasProcessedNode, sawRootDescriptor);
    ASSERT_NE(directRoot, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);

    std::set<std::shared_ptr<Component>, SurfaceSlot::BuildNodeDepthComparator> buildNodes;
    buildNodes.insert(nullptr);
    auto nameless = std::make_shared<FakeComponent>("Text", reinterpret_cast<ArkUI_NodeHandle>(0x1301));
    buildNodes.insert(nameless);
    directBuildSlot.BuildComponentTree(buildNodes);
}

TEST_F(
    SurfaceSlotCustomComponentInternalCoverageTest, should_cover_surface_slot_schema_modal_and_negative_render_branches)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("schema_modal_surface");
    slot.SetRenderId(1601);
    slot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true }, { "Card", true }, { "Column", true } }));

    std::unique_ptr<JsonAdapter> missingIdDescriptor = ParseJson(R"({"component":"Text","text":"root"})");
    ASSERT_NE(missingIdDescriptor, nullptr);
    std::map<std::string, JsonValue> directDescriptors;
    directDescriptors.emplace("root", missingIdDescriptor->GetRoot());

    bool hasProcessedNode = false;
    bool sawRootDescriptor = false;
    std::shared_ptr<Component> directRoot =
        slot.BuildRootFromComponents("root", directDescriptors, hasProcessedNode, sawRootDescriptor);
    ASSERT_NE(directRoot, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);

    std::unique_ptr<JsonAdapter> componentArray = ParseJson(R"([
        {"id":"root","component":"Text","text":"root"},
        {"id":"card_missing_child","component":"Card"},
        {"id":"column_bad_children","component":"Column","children":1},
        {"id":"modal_ok","component":"Modal","trigger":"root","content":"card_missing_child"}
    ])");
    ASSERT_NE(componentArray, nullptr);
    hasProcessedNode = false;
    sawRootDescriptor = false;
    std::shared_ptr<Component> builtRoot =
        slot.BuildRootFromComponents(componentArray->GetRoot(), hasProcessedNode, sawRootDescriptor);
    ASSERT_NE(builtRoot, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);

    SurfaceSlot noCoordinatorSlot;
    noCoordinatorSlot.SetSurfaceId("schema_modal_no_coordinator");
    noCoordinatorSlot.SetRenderId(1602);
    noCoordinatorSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true } }));
    noCoordinatorSlot.modalCoordinator_.reset();

    std::unique_ptr<JsonAdapter> noCoordinatorArray = ParseJson(R"([
        {"id":"root","component":"Text","text":"root"},
        {"id":"modal_skip","component":"Modal","trigger":"root","content":"root"}
    ])");
    ASSERT_NE(noCoordinatorArray, nullptr);
    hasProcessedNode = false;
    sawRootDescriptor = false;
    builtRoot =
        noCoordinatorSlot.BuildRootFromComponents(noCoordinatorArray->GetRoot(), hasProcessedNode, sawRootDescriptor);
    ASSERT_NE(builtRoot, nullptr);
    EXPECT_TRUE(hasProcessedNode);
    EXPECT_TRUE(sawRootDescriptor);

    SurfaceSlot negativeRenderSlot;
    negativeRenderSlot.SetSurfaceId("negative_render_surface");
    std::unique_ptr<JsonAdapter> deepValueMessage = ParseJson(BuildNestedValueMessage(21));
    ASSERT_NE(deepValueMessage, nullptr);
    EXPECT_TRUE(negativeRenderSlot.UpdateDataModel(deepValueMessage->GetRoot()));
}

TEST_F(SurfaceSlotCustomComponentInternalCoverageTest,
    should_cover_surface_slot_misc_defensive_and_extended_update_branches)
{
    SurfaceSlot::BuildNodeDepthComparator comparator;
    auto sameNode = std::make_shared<FakeComponent>("Text");
    EXPECT_FALSE(comparator(sameNode, sameNode));

    auto lhs = std::make_shared<FakeComponent>("Text");
    auto rhs = std::make_shared<FakeComponent>("Text");
    lhs->SetBuildDepth(3);
    rhs->SetBuildDepth(3);
    lhs->SetComponentId("same");
    rhs->SetComponentId("same");
    EXPECT_EQ(comparator(lhs, rhs), lhs.get() < rhs.get());

    SurfaceSlot modalSlot;
    modalSlot.DismissActiveModal();

    SurfaceSlot noRootFillSlot;
    noRootFillSlot.SetForceRootFill(true);

    SurfaceSlot nullNativeRootSlot;
    nullNativeRootSlot.SetRootComponent(std::make_shared<FakeComponent>("Column"));
    nullNativeRootSlot.SetForceRootFill(true);

    SurfaceSlot extendedSlot;
    extendedSlot.SetSurfaceId("extended_update_surface");
    extendedSlot.SetRenderId(1603);
    extendedSlot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", false }, { "Button", false } }));
    extendedSlot.SetSurfaceCatalogId(A2UI_EXTENDED_CATALOG_ID);

    std::shared_ptr<Component> customButton = extendedSlot.BuildComponent("Button");
    ASSERT_NE(customButton, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedComponent>(customButton), nullptr);

    std::unique_ptr<JsonAdapter> extendedDescriptor = ParseJson(R"({
        "id":"extended_text",
        "component":"Text",
        "content":"first",
        "styles":{"fontSize":16}
    })");
    ASSERT_NE(extendedDescriptor, nullptr);
    std::shared_ptr<Component> extendedText =
        extendedSlot.CreateOrUpdateComponentNode(extendedDescriptor->GetRoot(), "extended_text", "Text");
    ASSERT_NE(extendedText, nullptr);

    std::unique_ptr<JsonAdapter> updatedDescriptor = ParseJson(R"({
        "id":"extended_text",
        "component":"Text",
        "content":"second",
        "styles":{"fontSize":18}
    })");
    ASSERT_NE(updatedDescriptor, nullptr);
    std::shared_ptr<Component> updatedText =
        extendedSlot.CreateOrUpdateComponentNode(updatedDescriptor->GetRoot(), "extended_text", "Text");
    EXPECT_EQ(updatedText, extendedText);

    RenderContext renderContext =
        RenderContext::Create(extendedSlot.GetRenderId(), extendedSlot.GetSurfaceId(), extendedSlot.bindingEngine_,
            extendedSlot.GetCatalog(), extendedSlot.GetFontSizeScale(), extendedSlot.GetApiVersion());
    extendedSlot.ApplyExtendedComponentDescriptor(updatedDescriptor->GetRoot(), nullptr, true, renderContext);
}
