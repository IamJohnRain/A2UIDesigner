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

#include <cstdint>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "TestFixture.h"
#define private public
#define protected public
#include "composition/TemplateAdapterNode.h"
#undef protected
#undef private

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "components/Component.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

const std::string TEST_TEMPLATE_COMPONENT_ID = "tmpl";
const std::string TEST_ARRAY_PATH = "/items";

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

TemplateAdapterNode::TemplateInstanceBuildContext BuildTemplateContext(const std::string& templateComponentId,
    const std::string& arrayPath, int32_t itemIndex, const std::map<std::string, JsonValue>* allDescriptors,
    std::map<std::string, JsonValue>* generatedDescriptors)
{
    return {
        .templateComponentId = templateComponentId,
        .arrayPath = arrayPath,
        .itemIndex = itemIndex,
        .allDescriptors = allDescriptors,
        .generatedDescriptors = generatedDescriptors,
    };
}

class ExposedTemplateAdapterNode : public TemplateAdapterNode {
public:
    using TemplateAdapterNode::BuildItemWrapper;

    int nestedUpdateCount_ = 0;
    int setupNestedAdapterCount_ = 0;
    std::string lastParentPath_;
    std::shared_ptr<Component> lastNestedComponent_;

protected:
    void OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath) override
    {
        ++nestedUpdateCount_;
        lastNestedComponent_ = component;
        lastParentPath_ = parentPath;
    }

    void SetupNestedAdapter(const std::shared_ptr<Component>&, const std::string&, const std::string&,
        const std::string&, const std::map<std::string, JsonValue>&) override
    {
        ++setupNestedAdapterCount_;
    }
};

using CreateNodeCallback = ArkUI_NodeHandle (*)(ArkUI_NodeType);

class ScopedCreateNodeOverride {
public:
    ScopedCreateNodeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, CreateNodeCallback createNodeCallback)
        : nativeNodeApi_(nativeNodeApi),
          originalCreateNode_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->createNode)
    {
        if (nativeNodeApi_ != nullptr && createNodeCallback != nullptr) {
            nativeNodeApi_->createNode = createNodeCallback;
        }
    }

    ~ScopedCreateNodeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->createNode = originalCreateNode_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    CreateNodeCallback originalCreateNode_ = nullptr;
};

ArkUI_NodeHandle ReturnNullNodeHandle(ArkUI_NodeType type)
{
    static_cast<void>(type);
    return nullptr;
}

class TemplateAdapterNodeBranchTest : public A2UITest {
protected:
    std::set<int32_t> renderIds_;

    void TearDown() override
    {
        RenderManager& renderManager = RenderManager::GetInstance();
        for (int32_t renderId : renderIds_) {
            if (renderManager.HasRenderSlot(renderId)) {
                renderManager.RemoveRenderSlot(renderId);
            }
        }
        renderIds_.clear();
        A2UITest::TearDown();
    }

    SurfaceSlot& CreateSurfaceWithComponentCatalog(
        int32_t renderId, const std::string& surfaceId, const std::vector<std::string>& componentTypes)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
        renderIds_.insert(renderId);
        SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);

        auto catalog = std::make_shared<Catalog>("catalog_" + surfaceId);
        for (const auto& type : componentTypes) {
            auto item = std::make_shared<CatalogItem>(type, CatalogItemType::COMPONENT);
            item->SetInnerNative(true);
            catalog->AddComponent(item);
        }
        surfaceSlot.SetCatalog(catalog);
        return surfaceSlot;
    }
};

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_paths_in_object_and_array_nodes)
{
    auto adapter = ParseJson(R"({"path":"/name","child":{"path":"age"},"arr":[{"path":"title"}]})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/users", 2);

    EXPECT_EQ(root.GetString("path", ""), "/users/2/name");
    EXPECT_EQ(root.GetItem("child").GetString("path", ""), "/users/2/age");
    EXPECT_EQ(root.GetItem("arr").GetArrayItem(0).GetString("path", ""), "/users/2/title");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_absolute_path_in_template_string_value)
{
    auto adapter = ParseJson(R"({"value":"${/startAt}"})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/events", 0);

    EXPECT_EQ(root.GetString("value", ""), "${/events/0/startAt}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_multiple_template_paths_in_string_value)
{
    auto adapter = ParseJson(R"({"value":"${/startAt} - ${/endAt}."})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/events", 1);

    EXPECT_EQ(root.GetString("value", ""), "${/events/1/startAt} - ${/events/1/endAt}.");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_relative_path_in_template_string_value)
{
    auto adapter = ParseJson(R"({"text":"${name}"})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/items", 3);

    EXPECT_EQ(root.GetString("text", ""), "${/items/3/name}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_skip_function_calls_in_template_string_values)
{
    auto adapter = ParseJson(R"({"value":"${email(value:'a@b.cd')}"})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/events", 0);

    EXPECT_EQ(root.GetString("value", ""), "${email(value:'a@b.cd')}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_not_modify_plain_strings_without_template_syntax)
{
    auto adapter = ParseJson(R"({"text":"hello world","path":"/name"})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/users", 0);

    EXPECT_EQ(root.GetString("text", ""), "hello world");
    EXPECT_EQ(root.GetString("path", ""), "/users/0/name");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_nested_path_inside_function_call)
{
    auto adapter = ParseJson(R"TEST({"value":"${format(value:'${startAt}')}"})TEST");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/events", 0);

    EXPECT_EQ(root.GetString("value", ""), "${format(value:'${/events/0/startAt}')}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_multiple_nested_paths_inside_function_call)
{
    auto adapter = ParseJson(R"TEST({"value":"${format(a:'${x}', b:'${y}')}"})TEST");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/items", 2);

    EXPECT_EQ(root.GetString("value", ""), "${format(a:'${/items/2/x}', b:'${/items/2/y}')}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_rewrite_deeply_nested_paths_in_function_calls)
{
    auto adapter = ParseJson(R"TEST({"value":"${outer(arg:${inner(arg:${/deep})})}"})TEST");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/data", 1);

    EXPECT_EQ(root.GetString("value", ""), "${outer(arg:${inner(arg:${/data/1/deep})})}");
}

TEST_F(TemplateAdapterNodeBranchTest, should_ignore_invalid_json_when_rewriting_paths)
{
    JsonValue invalid;
    TemplateAdapterNode::RewriteDataPaths(invalid, "/users", 0);
    EXPECT_FALSE(invalid.IsValid());
}

TEST_F(TemplateAdapterNodeBranchTest, should_not_rewrite_empty_or_non_string_path_values)
{
    auto adapter = ParseJson(R"({"path":"","nested":{"path":1},"arr":[{"path":""},{"path":"ok"}]})");
    ASSERT_NE(adapter, nullptr);

    JsonValue root = adapter->GetRoot();
    TemplateAdapterNode::RewriteDataPaths(root, "/users", 5);

    EXPECT_EQ(root.GetString("path", "fallback"), "");
    EXPECT_EQ(root.GetItem("nested").GetItem("path").GetNumberValue(-1), 1);
    EXPECT_EQ(root.GetItem("arr").GetArrayItem(0).GetString("path", "fallback"), "");
    EXPECT_EQ(root.GetItem("arr").GetArrayItem(1).GetString("path", ""), "/users/5/ok");
}

TEST_F(TemplateAdapterNodeBranchTest, should_collect_referenced_descriptor_ids_across_children_and_child_fields)
{
    auto descriptorsAdapter = ParseJson(R"({
        "root": {
            "component": "Column",
            "children": ["childA", "templateHost"]
        },
        "childA": {
            "component": "Stack",
            "child": "leaf"
        },
        "leaf": {
            "component": "Text"
        },
        "templateHost": {
            "component": "Grid",
            "children": {
                "componentId": "itemTemplate",
                "path": "/items"
            }
        },
        "itemTemplate": {
            "component": "Text",
            "child": "templateLeaf"
        },
        "templateLeaf": {
            "component": "Text"
        }
    })");
    ASSERT_NE(descriptorsAdapter, nullptr);

    std::map<std::string, JsonValue> descriptors;
    for (JsonValue item = descriptorsAdapter->GetRoot().GetChild(); item.IsValid(); item = item.GetNext()) {
        descriptors[item.GetKey()] = item;
    }

    std::set<std::string> ids = TemplateAdapterNode::CollectReferencedDescriptorIds("root", descriptors);
    EXPECT_EQ(
        ids, (std::set<std::string> { "root", "childA", "leaf", "templateHost", "itemTemplate", "templateLeaf" }));
}

TEST_F(TemplateAdapterNodeBranchTest, should_handle_empty_missing_and_non_object_descriptors_when_collecting_ids)
{
    auto descriptorsAdapter = ParseJson(R"({
        "root": {
            "component": "Column",
            "children": ["missing", "nonObject", "root"]
        },
        "nonObject": 1
    })");
    ASSERT_NE(descriptorsAdapter, nullptr);

    std::map<std::string, JsonValue> descriptors;
    for (JsonValue item = descriptorsAdapter->GetRoot().GetChild(); item.IsValid(); item = item.GetNext()) {
        descriptors[item.GetKey()] = item;
    }

    EXPECT_TRUE(TemplateAdapterNode::CollectReferencedDescriptorIds("", descriptors).empty());

    std::set<std::string> ids = TemplateAdapterNode::CollectReferencedDescriptorIds("root", descriptors);
    EXPECT_EQ(ids, (std::set<std::string> { "root", "missing", "nonObject" }));
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_empty_when_build_tree_descriptor_input_is_invalid)
{
    std::string id = "root";
    std::map<std::string, JsonValue> generated;
    std::map<std::string, JsonValue> allDescriptors;

    auto nullDescriptorsContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 1, nullptr, &generated);
    auto nullGeneratedContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 1, &allDescriptors, nullptr);
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, nullDescriptorsContext).empty());
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, nullGeneratedContext).empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_null_when_build_descriptor_input_is_invalid)
{
    std::string id = "root";
    std::map<std::string, JsonValue> generated;
    std::map<std::string, JsonValue> allDescriptors;

    auto nullDescriptorsContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 0, nullptr, &generated);
    auto nullGeneratedContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 0, &allDescriptors, nullptr);
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, nullDescriptorsContext).empty());
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, nullGeneratedContext).empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_empty_when_descriptor_id_is_missing)
{
    std::string id = "missing";
    std::map<std::string, JsonValue> generated;
    std::map<std::string, JsonValue> allDescriptors;

    auto context = BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 1, &allDescriptors, &generated);
    EXPECT_TRUE(TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context).empty());
    EXPECT_TRUE(generated.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_build_template_descriptor_and_generate_id_when_source_has_no_id)
{
    auto rootDescriptor = ParseJson(R"({"component":"Column","path":"itemName"})");
    ASSERT_NE(rootDescriptor, nullptr);

    std::string id = "rootTemplate";
    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors[id] = rootDescriptor->GetRoot();
    std::map<std::string, JsonValue> generated;

    auto context = BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 3, &allDescriptors, &generated);
    std::string generatedId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context);
    ASSERT_FALSE(generatedId.empty());

    JsonValue instanceRoot = generated[generatedId];
    EXPECT_EQ(instanceRoot.GetString("id", ""), "/itemstmpl:3:rootTemplate");
    EXPECT_EQ(instanceRoot.GetString("path", ""), "/items/3/itemName");
    EXPECT_EQ(generated.count("/itemstmpl:3:rootTemplate"), 1U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_replace_existing_id_when_building_template_descriptor)
{
    auto rootDescriptor = ParseJson(R"({"id":"old-id","component":"Column","path":"/name"})");
    ASSERT_NE(rootDescriptor, nullptr);

    std::string id = "rootTemplate";
    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors[id] = rootDescriptor->GetRoot();
    std::map<std::string, JsonValue> generated;

    auto context = BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 2, &allDescriptors, &generated);
    std::string generatedId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context);
    ASSERT_FALSE(generatedId.empty());

    JsonValue instanceRoot = generated[generatedId];
    EXPECT_EQ(instanceRoot.GetString("id", ""), "/itemstmpl:2:rootTemplate");
    EXPECT_EQ(instanceRoot.GetString("path", ""), "/items/2/name");
}

TEST_F(TemplateAdapterNodeBranchTest, should_build_children_and_child_references_for_generated_tree)
{
    auto rootDescriptor =
        ParseJson(R"({"component":"Column","children":["childA","missingChild",1,""],"child":"childB"})");
    auto childA = ParseJson(R"({"component":"Text","text":"A"})");
    auto childB = ParseJson(R"({"component":"Text","text":"B"})");
    ASSERT_NE(rootDescriptor, nullptr);
    ASSERT_NE(childA, nullptr);
    ASSERT_NE(childB, nullptr);

    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors["root"] = rootDescriptor->GetRoot();
    allDescriptors["childA"] = childA->GetRoot();
    allDescriptors["childB"] = childB->GetRoot();

    std::string id = "root";
    std::map<std::string, JsonValue> generated;
    auto context = BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 0, &allDescriptors, &generated);
    std::string generatedRootId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context);

    ASSERT_FALSE(generatedRootId.empty());
    EXPECT_EQ(generatedRootId, "/itemstmpl:0:root");
    ASSERT_EQ(generated.count(generatedRootId), 1U);

    JsonValue generatedRoot = generated[generatedRootId];
    ASSERT_TRUE(generatedRoot.GetItem("children").IsArray());
    EXPECT_EQ(generatedRoot.GetItem("children").GetArraySize(), 1);
    EXPECT_EQ(generatedRoot.GetString("child", ""), "/itemstmpl:0:childB");
}

TEST_F(TemplateAdapterNodeBranchTest, should_build_if_branch_references_for_generated_template_tree)
{
    auto rootDescriptor =
        ParseJson(R"({"component":"If","childrenIf":["ifChild",1,"missing"],"childrenElse":["elseChild",""]})");
    auto ifChildDescriptor = ParseJson(R"({"component":"Text","content":{"path":"content"}})");
    auto elseChildDescriptor = ParseJson(R"({"component":"Text","content":{"path":"fallback"}})");
    ASSERT_NE(rootDescriptor, nullptr);
    ASSERT_NE(ifChildDescriptor, nullptr);
    ASSERT_NE(elseChildDescriptor, nullptr);

    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors["root"] = rootDescriptor->GetRoot();
    allDescriptors["ifChild"] = ifChildDescriptor->GetRoot();
    allDescriptors["elseChild"] = elseChildDescriptor->GetRoot();

    std::set<std::string> referencedIds = TemplateAdapterNode::CollectReferencedDescriptorIds("root", allDescriptors);
    EXPECT_EQ(referencedIds, (std::set<std::string> { "root", "ifChild", "elseChild", "missing" }));

    std::string id = "root";
    std::map<std::string, JsonValue> generated;
    auto context = BuildTemplateContext("msgItem", "/messages", 1, &allDescriptors, &generated);
    std::string generatedRootId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context);

    ASSERT_FALSE(generatedRootId.empty());
    JsonValue generatedRoot = generated[generatedRootId];
    JsonValue generatedIf = generatedRoot.GetItem("childrenIf");
    JsonValue generatedElse = generatedRoot.GetItem("childrenElse");
    ASSERT_TRUE(generatedIf.IsArray());
    ASSERT_TRUE(generatedElse.IsArray());
    ASSERT_EQ(generatedIf.GetArraySize(), 1);
    ASSERT_EQ(generatedElse.GetArraySize(), 1);

    std::string generatedIfId = generatedIf.GetArrayItem(0).GetStringValue("");
    std::string generatedElseId = generatedElse.GetArrayItem(0).GetStringValue("");
    EXPECT_EQ(generatedIfId, "/messagesmsgItem:1:ifChild");
    EXPECT_EQ(generatedElseId, "/messagesmsgItem:1:elseChild");
    ASSERT_EQ(generated.count(generatedIfId), 1U);
    ASSERT_EQ(generated.count(generatedElseId), 1U);
    EXPECT_EQ(generated[generatedIfId].GetItem("content").GetString("path", ""), "/messages/1/content");
    EXPECT_EQ(generated[generatedElseId].GetItem("content").GetString("path", ""), "/messages/1/fallback");
}

TEST_F(TemplateAdapterNodeBranchTest, should_keep_generated_id_when_children_or_child_are_not_strings)
{
    auto rootDescriptor = ParseJson(R"({"component":"Column","children":{},"child":{}})");
    ASSERT_NE(rootDescriptor, nullptr);

    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors["root"] = rootDescriptor->GetRoot();
    std::string id = "root";
    std::map<std::string, JsonValue> generated;

    auto context = BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 1, &allDescriptors, &generated);
    std::string generatedRootId = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(id, context);

    ASSERT_FALSE(generatedRootId.empty());
    EXPECT_EQ(generatedRootId, "/itemstmpl:1:root");
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_generated_id_when_child_is_non_string_or_empty)
{
    auto childObjectDescriptor = ParseJson(R"({"component":"Column","child":{}})");
    auto childEmptyDescriptor = ParseJson(R"({"component":"Column","child":""})");
    ASSERT_NE(childObjectDescriptor, nullptr);
    ASSERT_NE(childEmptyDescriptor, nullptr);

    std::map<std::string, JsonValue> generated;
    std::map<std::string, JsonValue> allDescriptors;
    allDescriptors["rootObj"] = childObjectDescriptor->GetRoot();
    allDescriptors["rootEmpty"] = childEmptyDescriptor->GetRoot();

    std::string idObj = "rootObj";
    std::string idEmpty = "rootEmpty";
    auto objectContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 4, &allDescriptors, &generated);
    auto emptyContext =
        BuildTemplateContext(TEST_TEMPLATE_COMPONENT_ID, TEST_ARRAY_PATH, 5, &allDescriptors, &generated);
    std::string generatedObj = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(idObj, objectContext);
    std::string generatedEmpty = TemplateAdapterNode::BuildTemplateInstanceTreeDescriptors(idEmpty, emptyContext);

    EXPECT_EQ(generatedObj, "/itemstmpl:4:rootObj");
    EXPECT_EQ(generatedEmpty, "/itemstmpl:5:rootEmpty");
}

TEST_F(TemplateAdapterNodeBranchTest, should_create_initialize_update_and_dispose_adapter_handle)
{
    ArkUI_NodeAdapterHandle handle = nullptr;
    {
        ExposedTemplateAdapterNode node;
        handle = node.GetHandle();
        ASSERT_NE(handle, nullptr);

        node.Initialize("tmpl", "/items", 2);
        EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[handle], 2U);
        EXPECT_EQ(mockArkUIPtr_->registeredReceivers_.count(handle), 1U);

        node.UpdateItemCount(5);
        EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[handle], 5U);

        node.ReloadAllItems();
    }

    EXPECT_EQ(mockArkUIPtr_->disposedAdapters_.size(), 1U);
    EXPECT_EQ(mockArkUIPtr_->disposedAdapters_.front(), handle);
}

TEST_F(TemplateAdapterNodeBranchTest, should_build_item_wrapper_and_cover_create_node_failure_branch)
{
    ExposedTemplateAdapterNode node;

    TemplateAdapterNode::ItemWrapperInfo nullComponentWrapper = node.BuildItemWrapper(nullptr);
    EXPECT_EQ(nullComponentWrapper.rootNode, nullptr);
    EXPECT_EQ(nullComponentWrapper.contentParentNode, nullptr);

    auto nullNativeViewComponent = std::make_shared<Component>(nullptr, false);
    TemplateAdapterNode::ItemWrapperInfo nullNativeViewWrapper = node.BuildItemWrapper(nullNativeViewComponent);
    EXPECT_EQ(nullNativeViewWrapper.rootNode, nullptr);
    EXPECT_EQ(nullNativeViewWrapper.contentParentNode, nullptr);

    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7100), false);
    {
        ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, ReturnNullNodeHandle);
        TemplateAdapterNode::ItemWrapperInfo failedWrapper = node.BuildItemWrapper(component);
        EXPECT_EQ(failedWrapper.rootNode, nullptr);
        EXPECT_EQ(failedWrapper.contentParentNode, nullptr);
    }

    TemplateAdapterNode::ItemWrapperInfo wrapper = node.BuildItemWrapper(component);
    EXPECT_NE(wrapper.rootNode, nullptr);
    EXPECT_EQ(wrapper.contentParentNode, wrapper.rootNode);
}

TEST_F(TemplateAdapterNodeBranchTest, should_skip_dispose_when_handle_is_null_on_destruction)
{
    ExposedTemplateAdapterNode* node = new ExposedTemplateAdapterNode();
    ASSERT_NE(node->GetHandle(), nullptr);
    node->handle_ = nullptr;
    delete node;
    SUCCEED();
}

TEST_F(TemplateAdapterNodeBranchTest, should_dispatch_static_event_and_set_node_id)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1000);
    mockArkUIPtr_->SetNodeAdapterEventUserData(event, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(event, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 3);

    TemplateAdapterNode::OnStaticAdapterEvent(event);

    ASSERT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_.count(event), 1U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_ignore_will_attach_and_will_detach_events)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);

    auto attachEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1001);
    mockArkUIPtr_->SetNodeAdapterEventType(attachEvent, NODE_ADAPTER_EVENT_WILL_ATTACH_TO_NODE);
    EXPECT_NO_FATAL_FAILURE(node.OnAdapterEvent(attachEvent));

    auto detachEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1002);
    mockArkUIPtr_->SetNodeAdapterEventType(detachEvent, NODE_ADAPTER_EVENT_WILL_DETACH_FROM_NODE);
    EXPECT_NO_FATAL_FAILURE(node.OnAdapterEvent(detachEvent));

    EXPECT_TRUE(node.items_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_ignore_static_event_when_userdata_is_null)
{
    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1001);
    mockArkUIPtr_->SetNodeAdapterEventUserData(event, nullptr);
    mockArkUIPtr_->SetNodeAdapterEventType(event, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);

    TemplateAdapterNode::OnStaticAdapterEvent(event);

    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_.count(event), 0U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_handle_add_event_successfully_and_track_item)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo("surface_ok", 401);

    auto modelData = ParseJson(R"({"items":[{"name":"first"}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface_ok");
    dataModel->ReplaceAll(modelData->GetRoot());
    node.SetDataModel(dataModel);

    auto templateRoot = ParseJson(R"({"component":"Column"})");
    ASSERT_NE(templateRoot, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    CreateSurfaceWithComponentCatalog(401, "surface_ok", { "Column" });

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1002);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);

    node.OnNewItemAttached(event);

    EXPECT_EQ(node.nestedUpdateCount_, 1);
    EXPECT_EQ(node.lastParentPath_, "/items/0");
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 1U);
    EXPECT_EQ(node.items_.size(), 1U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_early_on_add_event_when_generated_descriptor_missing)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("missing", "/items", 1);
    node.SetSurfaceInfo("surface_x", 402);

    CreateSurfaceWithComponentCatalog(402, "surface_x", { "Column" });

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1003);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);

    node.OnNewItemAttached(event);

    EXPECT_EQ(node.nestedUpdateCount_, 0);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 0U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_early_on_add_event_when_surface_not_found)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo("surface_missing", 403);

    auto templateRoot = ParseJson(R"({"component":"Column"})");
    ASSERT_NE(templateRoot, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1004);
    node.OnNewItemAttached(event);

    EXPECT_EQ(node.nestedUpdateCount_, 0);
    EXPECT_TRUE(node.items_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_continue_when_data_model_path_missing)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo("surface_model_missing", 406);

    auto modelData = ParseJson(R"({"other":[{"name":"x"}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface_model_missing");
    dataModel->ReplaceAll(modelData->GetRoot());
    node.SetDataModel(dataModel);

    auto templateRoot = ParseJson(R"({"component":"Column"})");
    ASSERT_NE(templateRoot, nullptr);
    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    CreateSurfaceWithComponentCatalog(406, "surface_model_missing", { "Column" });

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1011);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);
    node.OnNewItemAttached(event);

    EXPECT_EQ(node.nestedUpdateCount_, 1);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 1U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_early_on_add_event_when_component_build_fails)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo("surface_build_fail", 404);

    auto templateRoot = ParseJson(R"({"component":"UnknownType"})");
    ASSERT_NE(templateRoot, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    CreateSurfaceWithComponentCatalog(404, "surface_build_fail", { "Column" });

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1005);
    node.OnNewItemAttached(event);

    EXPECT_EQ(node.nestedUpdateCount_, 0);
    EXPECT_TRUE(node.items_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_return_early_when_wrapper_build_fails_during_attach)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);
    node.SetSurfaceInfo("surface_wrapper_fail", 407);

    auto modelData = ParseJson(R"({"items":[{"name":"first"}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface_wrapper_fail");
    dataModel->ReplaceAll(modelData->GetRoot());
    node.SetDataModel(dataModel);

    auto templateRoot = ParseJson(R"({"component":"Column"})");
    ASSERT_NE(templateRoot, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);

    CreateSurfaceWithComponentCatalog(407, "surface_wrapper_fail", { "Column" });

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1012);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 0);

    {
        ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, ReturnNullNodeHandle);
        node.OnNewItemAttached(event);
    }

    EXPECT_EQ(node.nestedUpdateCount_, 1);
    EXPECT_TRUE(node.items_.empty());
    EXPECT_TRUE(node.itemContentParents_.empty());
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 0U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_remove_item_when_detach_event_matches_existing_handle)
{
    ExposedTemplateAdapterNode node;

    auto handle = reinterpret_cast<ArkUI_NodeHandle>(0x9000);
    node.items_[handle] = std::make_shared<Component>(handle, false);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1006);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, handle);

    node.OnItemDetached(event);
    EXPECT_TRUE(node.items_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_ignore_detach_event_for_unknown_item)
{
    ExposedTemplateAdapterNode node;
    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1007);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, reinterpret_cast<ArkUI_NodeHandle>(0x9999));

    node.OnItemDetached(event);

    EXPECT_TRUE(node.items_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_clear_tracked_content_parent_when_detaching_item)
{
    ExposedTemplateAdapterNode node;

    auto nativeView = reinterpret_cast<ArkUI_NodeHandle>(0x8801);
    auto wrapperHandle = reinterpret_cast<ArkUI_NodeHandle>(0x8802);
    auto contentParent = reinterpret_cast<ArkUI_NodeHandle>(0x8803);
    node.items_[wrapperHandle] = std::make_shared<Component>(nativeView, false);
    node.itemContentParents_[wrapperHandle] = contentParent;

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x8804);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, wrapperHandle);

    node.OnItemDetached(event);

    EXPECT_TRUE(node.items_.empty());
    EXPECT_TRUE(node.itemContentParents_.empty());
}

TEST_F(TemplateAdapterNodeBranchTest, should_dispatch_adapter_events_for_all_supported_types)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 1);

    auto templateRoot = ParseJson(R"({"component":"Column"})");
    ASSERT_NE(templateRoot, nullptr);
    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateRoot->GetRoot();
    node.SetAllDescriptors(descriptors);
    node.SetSurfaceInfo("surface_dispatch", 405);
    CreateSurfaceWithComponentCatalog(405, "surface_dispatch", { "Column" });

    auto addEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1008);
    mockArkUIPtr_->SetNodeAdapterEventType(addEvent, NODE_ADAPTER_EVENT_ON_ADD_NODE_TO_ADAPTER);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(addEvent, 0);
    node.OnAdapterEvent(addEvent);
    ASSERT_EQ(node.items_.size(), 1U);
    ArkUI_NodeHandle addedHandle = node.items_.begin()->first;

    auto removeEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1009);
    mockArkUIPtr_->SetNodeAdapterEventType(removeEvent, NODE_ADAPTER_EVENT_ON_REMOVE_NODE_FROM_ADAPTER);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(removeEvent, addedHandle);
    node.OnAdapterEvent(removeEvent);
    EXPECT_TRUE(node.items_.empty());

    auto unknownEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x1010);
    mockArkUIPtr_->SetNodeAdapterEventType(unknownEvent, 999);
    EXPECT_NO_FATAL_FAILURE(node.OnAdapterEvent(unknownEvent));
}

TEST_F(TemplateAdapterNodeBranchTest, should_increment_template_version)
{
    ExposedTemplateAdapterNode node;
    EXPECT_EQ(node.templateVersion_, 0U);
    node.IncrementTemplateVersion();
    EXPECT_EQ(node.templateVersion_, 1U);
    node.IncrementTemplateVersion();
    EXPECT_EQ(node.templateVersion_, 2U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_produce_different_node_ids_after_version_increment)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 2);

    auto eventV0 = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2000);
    mockArkUIPtr_->SetNodeAdapterEventUserData(eventV0, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(eventV0, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(eventV0, 0);

    TemplateAdapterNode::OnStaticAdapterEvent(eventV0);
    int64_t idV0 = mockArkUIPtr_->nodeAdapterEventNodeIds_[eventV0];

    node.IncrementTemplateVersion();

    auto eventV1 = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2001);
    mockArkUIPtr_->SetNodeAdapterEventUserData(eventV1, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(eventV1, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(eventV1, 0);

    TemplateAdapterNode::OnStaticAdapterEvent(eventV1);
    int64_t idV1 = mockArkUIPtr_->nodeAdapterEventNodeIds_[eventV1];

    EXPECT_NE(idV0, idV1);
}

TEST_F(TemplateAdapterNodeBranchTest, should_produce_same_node_id_for_same_version_and_index)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 2);

    auto eventA = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2010);
    mockArkUIPtr_->SetNodeAdapterEventUserData(eventA, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(eventA, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(eventA, 1);

    TemplateAdapterNode::OnStaticAdapterEvent(eventA);
    int64_t idA = mockArkUIPtr_->nodeAdapterEventNodeIds_[eventA];

    auto eventB = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2011);
    mockArkUIPtr_->SetNodeAdapterEventUserData(eventB, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(eventB, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(eventB, 1);

    TemplateAdapterNode::OnStaticAdapterEvent(eventB);
    int64_t idB = mockArkUIPtr_->nodeAdapterEventNodeIds_[eventB];

    EXPECT_EQ(idA, idB);
}

TEST_F(TemplateAdapterNodeBranchTest, should_produce_different_node_ids_for_different_indices)
{
    ExposedTemplateAdapterNode node;
    node.Initialize("tmpl", "/items", 3);

    auto event0 = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2020);
    mockArkUIPtr_->SetNodeAdapterEventUserData(event0, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(event0, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event0, 0);
    TemplateAdapterNode::OnStaticAdapterEvent(event0);
    int64_t id0 = mockArkUIPtr_->nodeAdapterEventNodeIds_[event0];

    auto event1 = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x2021);
    mockArkUIPtr_->SetNodeAdapterEventUserData(event1, &node);
    mockArkUIPtr_->SetNodeAdapterEventType(event1, NODE_ADAPTER_EVENT_ON_GET_NODE_ID);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event1, 1);
    TemplateAdapterNode::OnStaticAdapterEvent(event1);
    int64_t id1 = mockArkUIPtr_->nodeAdapterEventNodeIds_[event1];

    EXPECT_NE(id0, id1);
}

TEST_F(TemplateAdapterNodeBranchTest, should_keep_components_in_surface_on_detach_when_surface_exists)
{
    ExposedTemplateAdapterNode node;
    node.SetSurfaceInfo("surface_detach", 500);
    SurfaceSlot& surfaceSlot = CreateSurfaceWithComponentCatalog(500, "surface_detach", { "Column" });

    auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x5000), false);
    component->SetComponentId("detach_item");
    surfaceSlot.GetAllComponents()["detach_item"] = component;

    auto childComponent = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x5001), false);
    childComponent->SetComponentId("detach_child");
    component->AddChild(childComponent);
    surfaceSlot.GetAllComponents()["detach_child"] = childComponent;

    auto handle = reinterpret_cast<ArkUI_NodeHandle>(0x5000);
    node.items_[handle] = component;

    EXPECT_EQ(surfaceSlot.GetAllComponents().size(), 2U);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x5000);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, handle);

    node.OnItemDetached(event);

    EXPECT_TRUE(node.items_.empty());
    EXPECT_EQ(surfaceSlot.GetAllComponents().size(), 2U);
}

TEST_F(TemplateAdapterNodeBranchTest, should_still_remove_from_items_when_surface_not_found_on_detach)
{
    ExposedTemplateAdapterNode node;

    auto handle = reinterpret_cast<ArkUI_NodeHandle>(0x6000);
    node.items_[handle] = std::make_shared<Component>(handle, false);

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x6001);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, handle);

    node.OnItemDetached(event);

    EXPECT_TRUE(node.items_.empty());
}

} // namespace
