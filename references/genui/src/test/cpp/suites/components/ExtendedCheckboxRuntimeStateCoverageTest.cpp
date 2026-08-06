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
#include <memory>
#include <string>

#include "TestFixture.h"

#define private public
#define protected public
#include "components/Component.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "components/extended/ExtendedCheckboxGroupComponent.h"
#include "composition/TemplateAdapterNode.h"

#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#undef protected
#undef private

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"

using namespace NativeModule;

namespace {

constexpr int32_t RUNTIME_RENDER_ID = 910200;
constexpr const char* RUNTIME_SURFACE_ID = "checkbox-runtime-state-surface";

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

std::shared_ptr<Catalog> BuildCheckboxCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Checkbox", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

void DispatchCheckboxChange(MockArkUINativeProvider* mockArkUI, ExtendedCheckboxComponent& component, bool checked)
{
    ASSERT_NE(mockArkUI, nullptr);
    ASSERT_NE(component.GetCheckboxNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = checked ? 1 : 0;
    mockArkUI->SetNodeEventHandle(&fakeEvent, component.GetCheckboxNodeForTest());
    mockArkUI->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUI->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    EXPECT_TRUE(mockArkUI->DispatchNodeEvent(component.GetCheckboxNodeForTest(), &fakeEvent));
}

class RuntimeStateComponent : public Component {
public:
    RuntimeStateComponent(const std::string& scope, const std::string& key, const JsonValue& state)
        : Component(reinterpret_cast<ArkUI_NodeHandle>(0x7100), false), scope_(scope), key_(key), state_(state)
    {}

    std::string GetRuntimeStateScope() const override
    {
        return scope_;
    }

    std::string GetRuntimeStateKey() const override
    {
        return key_;
    }

    JsonValue CaptureRuntimeState() const override
    {
        return state_;
    }

    void RestoreRuntimeState(const JsonValue& state) override
    {
        ++restoreCount_;
        restoredState_ = state;
    }

    int restoreCount_ = 0;
    JsonValue restoredState_;

private:
    std::string scope_;
    std::string key_;
    JsonValue state_;
};

class RuntimeStateTemplateAdapterNode : public TemplateAdapterNode {
public:
    using TemplateAdapterNode::OnAdapterEvent;
    using TemplateAdapterNode::OnNewItemAttached;

protected:
    void OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath) override
    {
        ++nestedUpdateCount_;
        lastComponent_ = component;
        lastParentPath_ = parentPath;
    }

    void SetupNestedAdapter(const std::shared_ptr<Component>&, const std::string&, const std::string&,
        const std::string&, const std::map<std::string, JsonValue>&) override
    {}

public:
    int32_t nestedUpdateCount_ = 0;
    std::string lastParentPath_;
    std::shared_ptr<Component> lastComponent_;
};

class ExtendedCheckboxRuntimeStateCoverageTest : public A2UITest {
protected:
    void TearDown() override
    {
        auto& renderManager = RenderManager::GetInstance();
        if (renderManager.HasRenderSlot(RUNTIME_RENDER_ID)) {
            renderManager.RemoveRenderSlot(RUNTIME_RENDER_ID);
        }
        A2UITest::TearDown();
    }

    SurfaceSlot& CreateManagedSurface()
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(RUNTIME_RENDER_ID);
        SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(RUNTIME_SURFACE_ID);
        surface.SetCatalog(BuildCheckboxCatalog());
        return surface;
    }
};

} // namespace

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_checkbox_private_properties_and_null_node_branches)
{
    ExtendedCheckboxComponent checkbox;
    EXPECT_EQ(checkbox.GetLabel(), "");
    EXPECT_EQ(checkbox.GetValue(), "");
    EXPECT_EQ(checkbox.GetGroup(), "");
    EXPECT_FALSE(checkbox.HasExplicitShape());
    EXPECT_TRUE(checkbox.GetRuntimeStateKey().empty());
    EXPECT_FALSE(checkbox.CaptureRuntimeState().IsValid());

    auto labelValue = JsonAdapter::CreateString("label-a");
    auto selectValue = JsonAdapter::CreateBool(true);
    auto valueValue = JsonAdapter::CreateString("value-a");
    auto groupValue = JsonAdapter::CreateString("group-a");
    ASSERT_NE(labelValue, nullptr);
    ASSERT_NE(selectValue, nullptr);
    ASSERT_NE(valueValue, nullptr);
    ASSERT_NE(groupValue, nullptr);

    auto labelDecl = checkbox.GetPrivatePropertyDeclaration("label");
    auto selectDecl = checkbox.GetPrivatePropertyDeclaration("select");
    auto valueDecl = checkbox.GetPrivatePropertyDeclaration("value");
    auto groupDecl = checkbox.GetPrivatePropertyDeclaration("group");
    ASSERT_TRUE(labelDecl.applyValue);
    ASSERT_TRUE(selectDecl.applyValue);
    ASSERT_TRUE(valueDecl.applyValue);
    ASSERT_TRUE(groupDecl.applyValue);
    labelDecl.applyValue(labelValue->GetRoot());
    selectDecl.applyValue(selectValue->GetRoot());
    valueDecl.applyValue(valueValue->GetRoot());
    groupDecl.applyValue(groupValue->GetRoot());
    EXPECT_EQ(checkbox.GetLabel(), "label-a");
    EXPECT_TRUE(checkbox.GetSelect());
    EXPECT_EQ(checkbox.GetValue(), "value-a");
    EXPECT_EQ(checkbox.GetGroup(), "group-a");

    ArkUI_NodeHandle checkboxNode = checkbox.checkboxNode_;
    ArkUI_NodeHandle textNode = checkbox.textNode_;
    checkbox.checkboxNode_ = nullptr;
    checkbox.textNode_ = nullptr;
    checkbox.UpdateChangeEventRegistration();
    checkbox.SetSelect(false);
    checkbox.SetSelectedColor(0xFF010203U);
    checkbox.SetUnselectedColor(0xFF040506U);
    checkbox.SetMark(0xFF070809U, 0.0F, 1.0F);
    checkbox.SetShape(A2UICheckboxShape::ROUNDED_SQUARE);
    checkbox.SetLabel("no-node-label");
    checkbox.SetGroup("no-node-group");
    checkbox.checkboxNode_ = checkboxNode;
    checkbox.textNode_ = textNode;

    checkbox.OnPropertyRemoved("select");
    checkbox.OnPropertyRemoved("label");
    checkbox.OnPropertyRemoved("value");
    checkbox.OnPropertyRemoved("group");
    EXPECT_FALSE(checkbox.GetSelect());
    EXPECT_EQ(checkbox.GetLabel(), "");
    EXPECT_EQ(checkbox.GetValue(), "");
    EXPECT_EQ(checkbox.GetGroup(), "");
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_not_set_layout_weight_on_checkbox_text_node)
{
    ExtendedCheckboxComponent checkbox;
    ASSERT_NE(checkbox.textNode_, nullptr);

    bool layoutWeightApplied = false;
    for (const auto& record : mockArkUIPtr_->setAttributeRecords_) {
        if (record.nodeHandle == checkbox.textNode_ && record.attribute == NODE_LAYOUT_WEIGHT) {
            layoutWeightApplied = true;
            break;
        }
    }
    EXPECT_FALSE(layoutWeightApplied);
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_checkbox_events_style_alias_and_binding_guards)
{
    ExtendedCheckboxComponent checkbox;
    checkbox.NodeEventReceiver(nullptr);
    checkbox.HandleNodeEvent(nullptr);

    ArkUI_NodeEvent clickEvent = {};
    mockArkUIPtr_->SetNodeEventType(&clickEvent, NODE_ON_CLICK);
    checkbox.HandleNodeEvent(&clickEvent);

    ArkUI_NodeEvent changeEvent = {};
    mockArkUIPtr_->SetNodeEventType(&changeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);
    mockArkUIPtr_->SetNodeEventComponentEvent(&changeEvent, nullptr);
    checkbox.HandleNodeEvent(&changeEvent);

    auto styles = ParseJson(R"({"aliasColor":"#102030"})");
    ASSERT_NE(styles, nullptr);
    bool overridden = false;
    checkbox.ApplyStyleColor(styles->GetRoot(), "missingColor", "aliasColor", 0xFF000000U, overridden,
        &ExtendedCheckboxComponent::SetSelectedColor);
    EXPECT_TRUE(overridden);
    EXPECT_EQ(checkbox.GetSelectedColorForTest(), 0xFF102030U);

    checkbox.SetApplyingStyleDeltaUpdateForTest(true);
    auto emptyStyles = ParseJson(R"({})");
    ASSERT_NE(emptyStyles, nullptr);
    checkbox.ApplyStyleColor(emptyStyles->GetRoot(), "missingColor", nullptr, 0xFF998877U, overridden,
        &ExtendedCheckboxComponent::SetSelectedColor);
    EXPECT_EQ(checkbox.GetSelectedColorForTest(), 0xFF102030U);
    checkbox.SetApplyingStyleDeltaUpdateForTest(false);

    checkbox.ValidateResolvedMarkDfx(JsonValue());
    auto invalidStroke = ParseJson(R"({"strokeColor":"not-a-color","size":"bad","strokeWidth":0})");
    ASSERT_NE(invalidStroke, nullptr);
    checkbox.ValidateResolvedMarkDfx(invalidStroke->GetRoot());

    checkbox.AddBinding("select", "/checked");
    checkbox.renderContext_.surfaceId = "surface-binding";
    checkbox.renderContext_.bindingEngine = nullptr;
    checkbox.SyncSelectToBoundDataModel(true);
    checkbox.renderContext_.bindingEngine = BindingEngine::Create();
    checkbox.renderContext_.surfaceId = "";
    checkbox.SyncSelectToBoundDataModel(false);
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_resolve_dynamic_mark_members_and_theme_cache)
{
    SurfaceSlot& surface = CreateManagedSurface();
    auto data = ParseJson(R"({"mark":{"strokeColor":"#445566","size":24}})");
    ASSERT_NE(data, nullptr);
    auto dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJson(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "styles": {
                "mark": {
                    "strokeColor": {"path": "/mark/strokeColor"},
                    "size": {"path": "/mark/size"},
                    "strokeWidth": {"call": "length", "args": {"value": "abcd", "min": 1}, "returnType": "boolean"}
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    auto checkbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(surface.FindComponentById("root"));
    ASSERT_NE(checkbox, nullptr);
    EXPECT_EQ(checkbox->GetMarkStrokeColorForTest(), 0xFF445566U);
    EXPECT_FLOAT_EQ(checkbox->GetMarkSizeForTest(), 24.0F);

    auto firstTheme = checkbox->GetTheme();
    auto secondTheme = checkbox->GetTheme();
    EXPECT_NE(firstTheme, nullptr);
    EXPECT_EQ(firstTheme, secondTheme);
    checkbox->OnConfigChange(ThemeContext {});

    auto nonObjectMark = ParseJson(R"("bad")");
    ASSERT_NE(nonObjectMark, nullptr);
    EXPECT_EQ(checkbox->ResolveMarkDynamicMembers(nonObjectMark->GetRoot()), nullptr);

    auto callMark = ParseJson(R"({"strokeWidth":{"call":"not","args":{"value":false},"returnType":"boolean"}})");
    ASSERT_NE(callMark, nullptr);
    auto resolvedCallMark = checkbox->ResolveMarkDynamicMembers(callMark->GetRoot());
    ASSERT_NE(resolvedCallMark, nullptr);
    EXPECT_TRUE(resolvedCallMark->GetRoot().Has("strokeWidth"));

    auto plainMessage = ParseJson(R"({
        "components": [{"id": "plain", "component": "Checkbox"}]
    })");
    ASSERT_NE(plainMessage, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(plainMessage->GetRoot()));
    auto plainCheckbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(surface.FindComponentById("plain"));
    ASSERT_NE(plainCheckbox, nullptr);
    plainCheckbox->OnConfigChange(ThemeContext {});
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_surface_manager_recreate_and_latest_transfer_branches)
{
    SurfaceManager manager;
    manager.SetRenderId(920300);
    auto contentHandle = reinterpret_cast<A2UINodeContentHandle>(0x920301);

    SurfaceSlot& first = manager.CreateSurface("first", contentHandle);
    auto state = ParseJson(R"({"select":true})");
    ASSERT_NE(state, nullptr);
    first.StoreRuntimeState("scope", "key", state->GetRoot());
    JsonValue output;
    ASSERT_TRUE(first.GetRuntimeState("scope", "key", output));

    SurfaceSlot& recreated = manager.CreateSurface("first");
    EXPECT_FALSE(recreated.GetRuntimeState("scope", "key", output));

    manager.CreateSurface("second");
    manager.SetRootFillMode(true);
    manager.SetFontSizeScale(1.25F);
    manager.SetApiVersion(23);
    EXPECT_FLOAT_EQ(manager.FindSurface("first")->GetFontSizeScale(), 1.25F);
    EXPECT_EQ(manager.FindSurface("second")->GetApiVersion(), 23);

    manager.SetContentHandle(contentHandle);
    manager.RemoveSurface("second");
    EXPECT_EQ(manager.GetLatestSurfaceId(), "first");
    EXPECT_EQ(manager.FindSurface("first")->GetContentHandle(), contentHandle);

    manager.latestSurfaceId_ = "missing-latest";
    manager.SetContentHandle(contentHandle);
    EXPECT_EQ(manager.GetLatestSurface(), nullptr);

    manager.surfaceOrder_.push_back("missing-order");
    manager.UpdateThemeMode(ThemeMode::DARK);
    manager.UpdateBreakpoint(Breakpoint::LG);

    manager.surfaces_["no-theme"].SetSurfaceId("no-theme");
    manager.surfaceOrder_.push_back("no-theme");
    manager.UpdateThemeMode(ThemeMode::LIGHT);
    manager.UpdateBreakpoint(Breakpoint::MD);
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_surface_slot_pending_templates_and_group_inheritance)
{
    SurfaceSlot surface;
    auto emptyComponents = ParseJson(R"({"components":[]})");
    ASSERT_NE(emptyComponents, nullptr);
    EXPECT_FALSE(surface.UpdateComponents(emptyComponents->GetRoot()));

    surface.pendingTemplateContainers_.insert("missing-container");
    surface.ProcessPendingTemplateContainers();
    EXPECT_TRUE(surface.pendingTemplateContainers_.empty());

    auto nonTemplateContainer = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x930001), false);
    surface.allComponents_["non-template"] = nonTemplateContainer;
    surface.pendingTemplateContainers_.insert("non-template");
    surface.ProcessPendingTemplateContainers();
    EXPECT_TRUE(surface.pendingTemplateContainers_.empty());

    auto templateContainer = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x930002), false);
    templateContainer->childListDescriptor_.type = ChildListType::TEMPLATE_PATH;
    templateContainer->childListDescriptor_.templateComponentId = "missing-template";
    surface.allComponents_["template"] = templateContainer;
    surface.pendingTemplateContainers_.insert("template");
    surface.ProcessPendingTemplateContainers();
    EXPECT_EQ(surface.pendingTemplateContainers_.count("template"), 1U);

    templateContainer->childListDescriptor_.templateComponentId = "template-root";
    auto templateRoot = ParseJson(R"({"id":"template-root","component":"Column","child":"missing-child"})");
    ASSERT_NE(templateRoot, nullptr);
    surface.allComponentDescriptorStore_["template-root"] = templateRoot->GetRoot();
    surface.ProcessPendingTemplateContainers();
    EXPECT_EQ(surface.pendingTemplateContainers_.count("template"), 1U);

    auto group = std::make_shared<ExtendedCheckboxGroupComponent>();
    group->SetGroup("groupA");
    group->SetSelectAll(true);
    auto sameGroupCheckbox = std::make_shared<ExtendedCheckboxComponent>();
    sameGroupCheckbox->SetGroup("groupA");
    sameGroupCheckbox->SetSelect(false);
    auto otherGroupCheckbox = std::make_shared<ExtendedCheckboxComponent>();
    otherGroupCheckbox->SetGroup("groupB");
    otherGroupCheckbox->SetSelect(false);

    surface.allComponents_["group"] = group;
    surface.allComponents_["same"] = sameGroupCheckbox;
    surface.allComponents_["other"] = otherGroupCheckbox;
    surface.SyncExtendedCheckboxGroupState();
    EXPECT_TRUE(sameGroupCheckbox->GetSelectForTest());
    EXPECT_FALSE(otherGroupCheckbox->GetSelectForTest());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_template_adapter_rewrite_and_default_event)
{
    auto rewrite = ParseJson(R"({"escaped":"\\${name}","open":"${name","nested":[{"value":"${title}"}]})");
    ASSERT_NE(rewrite, nullptr);
    JsonValue root = rewrite->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/items", 4);
    EXPECT_EQ(root.GetString("escaped", ""), "${name}");
    EXPECT_EQ(root.GetString("open", ""), "${name");
    EXPECT_EQ(root.GetItem("nested").GetArrayItem(0).GetString("value", ""), "${/items/4/title}");

    std::map<std::string, JsonValue> descriptors;
    auto emptyIdDescriptor = ParseJson(R"("not-an-object-descriptor")");
    ASSERT_NE(emptyIdDescriptor, nullptr);
    descriptors["root"] = emptyIdDescriptor->GetRoot();
    std::string id = "root";
    std::map<std::string, JsonValue> generated;
    TemplateAdapterNode::TemplateInstanceBuildContext context = {
        .templateComponentId = "tmpl",
        .arrayPath = "/items",
        .itemIndex = 0,
        .allDescriptors = &descriptors,
        .generatedDescriptors = &generated,
    };
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context).empty());

    RuntimeStateTemplateAdapterNode node;
    auto unknownEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x930100);
    mockArkUIPtr_->SetNodeAdapterEventType(unknownEvent, 123456);
    node.OnAdapterEvent(unknownEvent);
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_runtime_state_miss_empty_sync_and_template_move)
{
    SurfaceSlot& surface = CreateManagedSurface();
    auto message = ParseJson(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "group": "groupA",
            "value": "valueA"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));
    auto checkbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(surface.FindComponentById("root"));
    ASSERT_NE(checkbox, nullptr);
    checkbox->ApplyInheritedSelect(true);
    EXPECT_TRUE(checkbox->GetSelectForTest());

    ExtendedCheckboxComponent emptyKeyCheckbox;
    emptyKeyCheckbox.SyncRuntimeStateToSurface();

    RuntimeStateTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo(RUNTIME_SURFACE_ID, RUNTIME_RENDER_ID);
    auto dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    auto modelData = ParseJson(R"({"items":[{"name":"first"}]})");
    ASSERT_NE(modelData, nullptr);
    dataModel->ReplaceAll(modelData->GetRoot());
    node.SetDataModel(dataModel);

    auto templateRoot = ParseJson(R"({"component":"Checkbox","group":"groupA","value":{"path":"name"}})");
    ASSERT_NE(templateRoot, nullptr);
    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    auto firstAttach = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x930200);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(firstAttach, 0);
    node.OnNewItemAttached(firstAttach);
    ASSERT_EQ(node.items_.size(), 1U);

    auto secondAttach = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x930201);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(secondAttach, 0);
    node.OnNewItemAttached(secondAttach);
    EXPECT_FALSE(node.movedWrappers_.empty());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_cover_base_component_runtime_state_defaults)
{
    Component component(reinterpret_cast<ArkUI_NodeHandle>(0x7200), false);
    EXPECT_TRUE(component.GetRuntimeStateScope().empty());
    EXPECT_TRUE(component.GetRuntimeStateKey().empty());
    EXPECT_FALSE(component.CaptureRuntimeState().IsValid());

    auto state = ParseJson(R"({"select":true})");
    ASSERT_NE(state, nullptr);
    component.RestoreRuntimeState(state->GetRoot());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_capture_and_restore_checkbox_runtime_state)
{
    ExtendedCheckboxComponent checkbox;
    checkbox.SetComponentId("cb1");
    checkbox.SetGroup("groupA");
    checkbox.SetValue("");
    checkbox.SetSelect(true);

    EXPECT_EQ(checkbox.GetRuntimeStateScope(), "ExtendedCheckbox.select");
    EXPECT_EQ(checkbox.GetRuntimeStateKey(), "groupA\ncb1");

    JsonValue captured = checkbox.CaptureRuntimeState();
    ASSERT_TRUE(captured.IsObject());
    EXPECT_EQ(captured.GetString("group", ""), "groupA");
    EXPECT_EQ(captured.GetString("key", ""), "cb1");
    EXPECT_EQ(captured.GetString("value", "fallback"), "");
    EXPECT_TRUE(captured.GetBool("select", false));

    checkbox.SetSelect(false);
    auto validState = ParseJson(R"({"group":"groupA","key":"cb1","value":"ignored","select":true})");
    ASSERT_NE(validState, nullptr);
    checkbox.RestoreRuntimeState(validState->GetRoot());
    EXPECT_TRUE(checkbox.GetSelectForTest());

    auto legacyValueState = ParseJson(R"({"group":"groupA","value":"cb1","select":false})");
    ASSERT_NE(legacyValueState, nullptr);
    checkbox.RestoreRuntimeState(legacyValueState->GetRoot());
    EXPECT_TRUE(checkbox.GetSelectForTest());

    checkbox.SetValue("valueA");
    EXPECT_EQ(checkbox.GetRuntimeStateKey(), "groupA\ncb1");
    captured = checkbox.CaptureRuntimeState();
    ASSERT_TRUE(captured.IsObject());
    EXPECT_EQ(captured.GetString("key", ""), "cb1");
    EXPECT_EQ(captured.GetString("value", ""), "valueA");
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_keep_duplicate_checkbox_values_separate_by_component_id)
{
    ExtendedCheckboxComponent selectedCheckbox;
    selectedCheckbox.SetComponentId("cb1");
    selectedCheckbox.SetGroup("groupA");
    selectedCheckbox.SetValue("same-value");
    selectedCheckbox.SetSelect(true);

    ExtendedCheckboxComponent duplicateValueCheckbox;
    duplicateValueCheckbox.SetComponentId("cb2");
    duplicateValueCheckbox.SetGroup("groupA");
    duplicateValueCheckbox.SetValue("same-value");
    duplicateValueCheckbox.SetSelect(false);

    EXPECT_EQ(selectedCheckbox.GetRuntimeStateKey(), "groupA\ncb1");
    EXPECT_EQ(duplicateValueCheckbox.GetRuntimeStateKey(), "groupA\ncb2");

    JsonValue selectedState = selectedCheckbox.CaptureRuntimeState();
    ASSERT_TRUE(selectedState.IsObject());

    duplicateValueCheckbox.RestoreRuntimeState(selectedState);
    EXPECT_FALSE(duplicateValueCheckbox.GetSelectForTest());

    selectedCheckbox.SetSelect(false);
    selectedCheckbox.RestoreRuntimeState(selectedState);
    EXPECT_TRUE(selectedCheckbox.GetSelectForTest());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_ignore_invalid_or_explicit_checkbox_runtime_state)
{
    ExtendedCheckboxComponent checkbox;
    checkbox.SetComponentId("cb2");
    checkbox.SetGroup("groupA");
    checkbox.SetValue("valueA");
    checkbox.SetSelect(false);

    auto nonObject = ParseJson(R"([true])");
    auto missingSelect = ParseJson(R"({"group":"groupA","key":"cb2"})");
    auto mismatchedGroup = ParseJson(R"({"group":"other","key":"cb2","select":true})");
    auto mismatchedValue = ParseJson(R"({"group":"groupA","key":"other","select":true})");
    ASSERT_NE(nonObject, nullptr);
    ASSERT_NE(missingSelect, nullptr);
    ASSERT_NE(mismatchedGroup, nullptr);
    ASSERT_NE(mismatchedValue, nullptr);

    checkbox.RestoreRuntimeState(nonObject->GetRoot());
    checkbox.RestoreRuntimeState(missingSelect->GetRoot());
    checkbox.RestoreRuntimeState(mismatchedGroup->GetRoot());
    checkbox.RestoreRuntimeState(mismatchedValue->GetRoot());
    EXPECT_FALSE(checkbox.GetSelectForTest());

    checkbox.hasExplicitSelect_ = true;
    auto validState = ParseJson(R"({"group":"groupA","key":"cb2","select":true})");
    ASSERT_NE(validState, nullptr);
    checkbox.RestoreRuntimeState(validState->GetRoot());
    EXPECT_FALSE(checkbox.GetSelectForTest());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_apply_inherited_select_from_runtime_state_or_fallback)
{
    ExtendedCheckboxComponent detachedCheckbox;
    detachedCheckbox.SetComponentId("detached");
    detachedCheckbox.SetGroup("groupA");
    detachedCheckbox.ApplyInheritedSelect(true);
    EXPECT_TRUE(detachedCheckbox.GetSelectForTest());

    detachedCheckbox.hasExplicitSelect_ = true;
    detachedCheckbox.ApplyInheritedSelect(false);
    EXPECT_TRUE(detachedCheckbox.GetSelectForTest());

    SurfaceSlot& surface = CreateManagedSurface();
    auto message = ParseJson(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "group": "groupA",
            "value": "valueA"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    auto checkbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(surface.FindComponentById("root"));
    ASSERT_NE(checkbox, nullptr);
    EXPECT_FALSE(checkbox->HasExplicitSelect());

    auto state = ParseJson(R"({"group":"groupA","key":"root","value":"valueA","select":true})");
    ASSERT_NE(state, nullptr);
    surface.StoreRuntimeState(checkbox->GetRuntimeStateScope(), checkbox->GetRuntimeStateKey(), state->GetRoot());

    checkbox->ApplyInheritedSelect(false);
    EXPECT_TRUE(checkbox->GetSelectForTest());
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_sync_checkbox_runtime_state_to_managed_surface_on_change)
{
    SurfaceSlot& surface = CreateManagedSurface();
    auto message = ParseJson(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "group": "groupA",
            "value": "valueA",
            "select": false,
            "listeners": {
                "onChange": {"type": "event", "name": "changed"}
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    auto checkbox = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(surface.FindComponentById("root"));
    ASSERT_NE(checkbox, nullptr);

    DispatchCheckboxChange(mockArkUIPtr_, *checkbox, true);

    JsonValue storedState;
    ASSERT_TRUE(surface.GetRuntimeState(checkbox->GetRuntimeStateScope(), checkbox->GetRuntimeStateKey(), storedState));
    EXPECT_EQ(storedState.GetString("group", ""), "groupA");
    EXPECT_EQ(storedState.GetString("value", ""), "valueA");
    EXPECT_TRUE(storedState.GetBool("select", false));
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_store_iterate_clear_and_clone_runtime_state)
{
    SurfaceSlot surface;
    JsonValue output;
    EXPECT_FALSE(surface.GetRuntimeState("", "key", output));
    EXPECT_FALSE(surface.GetRuntimeState("scope", "", output));
    surface.ForEachRuntimeState("", [](const std::string&, const JsonValue&) { FAIL(); });
    surface.ForEachRuntimeState("missing", [](const std::string&, const JsonValue&) { FAIL(); });
    surface.ClearRuntimeStateStore();

    auto state = ParseJson(R"({"select":true,"value":"A"})");
    ASSERT_NE(state, nullptr);
    surface.StoreRuntimeState("", "key", state->GetRoot());
    surface.StoreRuntimeState("scope", "", state->GetRoot());
    surface.StoreRuntimeState("scope", "invalid", JsonValue());
    EXPECT_FALSE(surface.GetRuntimeState("scope", "key", output));

    surface.StoreRuntimeState("scope", "key", state->GetRoot());
    ASSERT_TRUE(surface.GetRuntimeState("scope", "key", output));
    EXPECT_TRUE(output.GetBool("select", false));

    int visitCount = 0;
    surface.ForEachRuntimeState("scope", [&visitCount](const std::string& key, const JsonValue& value) {
        EXPECT_EQ(key, "key");
        EXPECT_TRUE(value.GetBool("select", false));
        ++visitCount;
    });
    EXPECT_EQ(visitCount, 1);

    surface.ClearRuntimeStateStore();
    EXPECT_FALSE(surface.GetRuntimeState("scope", "key", output));
}

TEST_F(ExtendedCheckboxRuntimeStateCoverageTest, should_capture_and_restore_runtime_state_tree)
{
    SurfaceSlot surface;
    auto parentState = ParseJson(R"({"select":true,"value":"parent"})");
    auto childState = ParseJson(R"({"select":false,"value":"child"})");
    ASSERT_NE(parentState, nullptr);
    ASSERT_NE(childState, nullptr);

    auto parent = std::make_shared<RuntimeStateComponent>("scope", "parent", parentState->GetRoot());
    auto child = std::make_shared<RuntimeStateComponent>("scope", "child", childState->GetRoot());
    parent->AddChild(child);

    surface.CaptureRuntimeStateTree(nullptr);
    surface.CaptureRuntimeStateTree(parent);

    JsonValue output;
    ASSERT_TRUE(surface.GetRuntimeState("scope", "parent", output));
    EXPECT_EQ(output.GetString("value", ""), "parent");
    ASSERT_TRUE(surface.GetRuntimeState("scope", "child", output));
    EXPECT_EQ(output.GetString("value", ""), "child");

    auto restoreParent = std::make_shared<RuntimeStateComponent>("scope", "parent", JsonValue());
    auto restoreChild = std::make_shared<RuntimeStateComponent>("scope", "child", JsonValue());
    restoreParent->AddChild(restoreChild);

    surface.RestoreRuntimeStateTree(nullptr);
    surface.RestoreRuntimeStateTree(restoreParent);
    EXPECT_EQ(restoreParent->restoreCount_, 1);
    EXPECT_EQ(restoreParent->restoredState_.GetString("value", ""), "parent");
    EXPECT_EQ(restoreChild->restoreCount_, 1);
    EXPECT_EQ(restoreChild->restoredState_.GetString("value", ""), "child");
}
