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
#include <map>
#include <memory>
#include <string>

#include "components/A2UI/list/ListComponent.h"
#include "components/Component.h"
#include "composition/ListAdapterNode.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

class ListAdapterNodeBranchTest : public A2UITest {};

class ExposedListAdapterNode : public ListAdapterNode {
public:
    using ListAdapterNode::BuildItemWrapper;
    using ListAdapterNode::OnNestedAdapterUpdate;
    using ListAdapterNode::SetupNestedAdapter;
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

std::shared_ptr<Component> CreatePlainComponent()
{
    return std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7000), false);
}

TEST_F(ListAdapterNodeBranchTest, should_return_early_when_nested_update_input_is_null)
{
    ExposedListAdapterNode adapter;
    EXPECT_NO_FATAL_FAILURE(adapter.OnNestedAdapterUpdate(nullptr, "/users/0"));
}

TEST_F(ListAdapterNodeBranchTest, should_skip_nested_update_when_data_model_is_null)
{
    ExposedListAdapterNode parentAdapter;

    auto nestedList = std::make_shared<ListComponent>();
    auto nestedAdapter = std::make_shared<ListAdapterNode>();
    nestedAdapter->Initialize("tmpl", "/items", 0);
    nestedList->SetLazyMode(true);
    nestedList->SetAdapterNode(nestedAdapter);

    EXPECT_NO_FATAL_FAILURE(parentAdapter.OnNestedAdapterUpdate(nestedList, "/users/0"));
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(nestedAdapter->GetHandle(), 0), 0);
}

TEST_F(ListAdapterNodeBranchTest, should_update_nested_item_count_for_relative_path)
{
    ExposedListAdapterNode parentAdapter;

    auto modelData = ParseJson(R"({"users":[{"friends":[1,2,3]}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(modelData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto root = CreatePlainComponent();
    auto nestedList = std::make_shared<ListComponent>();
    auto nestedAdapter = std::make_shared<ListAdapterNode>();
    nestedAdapter->Initialize("tmpl", "friends", 0);
    nestedList->SetLazyMode(true);
    nestedList->SetAdapterNode(nestedAdapter);
    root->AddChild(nestedList);

    parentAdapter.OnNestedAdapterUpdate(root, "/users/0");

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 3U);
}

TEST_F(ListAdapterNodeBranchTest, should_update_nested_item_count_for_absolute_path)
{
    ExposedListAdapterNode parentAdapter;

    auto modelData = ParseJson(R"({"global":{"items":[{"id":1},{"id":2}]}})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(modelData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto nestedList = std::make_shared<ListComponent>();
    auto nestedAdapter = std::make_shared<ListAdapterNode>();
    nestedAdapter->Initialize("tmpl", "/global/items", 0);
    nestedList->SetLazyMode(true);
    nestedList->SetAdapterNode(nestedAdapter);

    parentAdapter.OnNestedAdapterUpdate(nestedList, "/users/0");

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 2U);
}

TEST_F(ListAdapterNodeBranchTest, should_skip_update_when_node_is_missing_or_not_array)
{
    ExposedListAdapterNode parentAdapter;

    auto dataModel = std::make_shared<DataModel>("surface");
    auto nonArrayData = ParseJson(R"({"users":[{"friends":{}}]})");
    ASSERT_NE(nonArrayData, nullptr);
    dataModel->ReplaceAll(nonArrayData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto nestedListMissing = std::make_shared<ListComponent>();
    auto nestedAdapterMissing = std::make_shared<ListAdapterNode>();
    nestedAdapterMissing->Initialize("tmpl", "missing", 1);
    nestedListMissing->SetLazyMode(true);
    nestedListMissing->SetAdapterNode(nestedAdapterMissing);

    parentAdapter.OnNestedAdapterUpdate(nestedListMissing, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapterMissing->GetHandle()], 1U);

    auto nestedListNonArray = std::make_shared<ListComponent>();
    auto nestedAdapterNonArray = std::make_shared<ListAdapterNode>();
    nestedAdapterNonArray->Initialize("tmpl", "friends", 1);
    nestedListNonArray->SetLazyMode(true);
    nestedListNonArray->SetAdapterNode(nestedAdapterNonArray);

    parentAdapter.OnNestedAdapterUpdate(nestedListNonArray, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapterNonArray->GetHandle()], 1U);
}

TEST_F(ListAdapterNodeBranchTest, should_skip_update_when_list_is_not_lazy_or_adapter_missing)
{
    ExposedListAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto eagerList = std::make_shared<ListComponent>();
    auto eagerAdapter = std::make_shared<ListAdapterNode>();
    eagerAdapter->Initialize("tmpl", "friends", 0);
    eagerList->SetAdapterNode(eagerAdapter);
    eagerList->SetLazyMode(false);
    parentAdapter.OnNestedAdapterUpdate(eagerList, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[eagerAdapter->GetHandle()], 0U);

    auto lazyWithoutAdapter = std::make_shared<ListComponent>();
    lazyWithoutAdapter->SetLazyMode(true);
    EXPECT_NO_FATAL_FAILURE(parentAdapter.OnNestedAdapterUpdate(lazyWithoutAdapter, "/users/0"));
}

TEST_F(ListAdapterNodeBranchTest, should_skip_update_when_adapter_path_is_empty)
{
    ExposedListAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1,2]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto lazyList = std::make_shared<ListComponent>();
    auto nestedAdapter = std::make_shared<ListAdapterNode>();
    nestedAdapter->Initialize("tmpl", "", 2);
    lazyList->SetLazyMode(true);
    lazyList->SetAdapterNode(nestedAdapter);

    parentAdapter.OnNestedAdapterUpdate(lazyList, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 2U);
}

TEST_F(ListAdapterNodeBranchTest, should_skip_update_when_data_path_not_found_but_continue_children)
{
    ExposedListAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1,2,3]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto root = CreatePlainComponent();
    auto missingPathList = std::make_shared<ListComponent>();
    auto missingPathAdapter = std::make_shared<ListAdapterNode>();
    missingPathAdapter->Initialize("tmpl", "missingPath", 1);
    missingPathList->SetLazyMode(true);
    missingPathList->SetAdapterNode(missingPathAdapter);

    auto validPathList = std::make_shared<ListComponent>();
    auto validPathAdapter = std::make_shared<ListAdapterNode>();
    validPathAdapter->Initialize("tmpl", "friends", 0);
    validPathList->SetLazyMode(true);
    validPathList->SetAdapterNode(validPathAdapter);

    root->AddChild(missingPathList);
    root->AddChild(validPathList);

    parentAdapter.OnNestedAdapterUpdate(root, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[missingPathAdapter->GetHandle()], 1U);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[validPathAdapter->GetHandle()], 3U);
}

TEST_F(ListAdapterNodeBranchTest, should_setup_nested_adapter_only_for_list_component_and_existing_template)
{
    ExposedListAdapterNode parentAdapter;
    parentAdapter.SetSurfaceInfo("surface_a", 777);
    parentAdapter.SetDataModel(std::make_shared<DataModel>("surface_a"));

    auto templateDescriptor = ParseJson(R"({"component":"Text"})");
    ASSERT_NE(templateDescriptor, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateDescriptor->GetRoot();

    auto listComp = std::make_shared<ListComponent>();
    parentAdapter.SetupNestedAdapter(listComp, "List", "missing", "/items", descriptors);
    EXPECT_FALSE(listComp->IsLazyMode());
    EXPECT_EQ(listComp->GetAdapterNode(), nullptr);

    auto nonListComp = CreatePlainComponent();
    parentAdapter.SetupNestedAdapter(nonListComp, "List", "tmpl", "/items", descriptors);

    parentAdapter.SetupNestedAdapter(listComp, "Column", "tmpl", "/items", descriptors);
    EXPECT_FALSE(listComp->IsLazyMode());

    parentAdapter.SetupNestedAdapter(listComp, "List", "tmpl", "/items", descriptors);
    EXPECT_TRUE(listComp->IsLazyMode());
    ASSERT_NE(listComp->GetAdapterNode(), nullptr);
    EXPECT_EQ(listComp->GetAdapterNode()->GetTemplateId(), "tmpl");
    EXPECT_EQ(listComp->GetAdapterNode()->GetDataPath(), "/items");
    EXPECT_EQ(listComp->GetAdapterNode()->GetSurfaceId(), "surface_a");
    EXPECT_EQ(listComp->GetAdapterNode()->GetRenderId(), 777);
}

TEST_F(ListAdapterNodeBranchTest, should_build_list_item_wrapper_only_when_component_and_item_node_are_available)
{
    ExposedListAdapterNode adapter;

    TemplateAdapterNode::ItemWrapperInfo nullComponentWrapper = adapter.BuildItemWrapper(nullptr);
    EXPECT_EQ(nullComponentWrapper.rootNode, nullptr);
    EXPECT_EQ(nullComponentWrapper.contentParentNode, nullptr);

    auto nullNativeViewComponent = std::make_shared<Component>(nullptr, false);
    TemplateAdapterNode::ItemWrapperInfo nullNativeViewWrapper = adapter.BuildItemWrapper(nullNativeViewComponent);
    EXPECT_EQ(nullNativeViewWrapper.rootNode, nullptr);
    EXPECT_EQ(nullNativeViewWrapper.contentParentNode, nullptr);

    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    {
        ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, ReturnNullNodeHandle);
        auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7200), false);
        TemplateAdapterNode::ItemWrapperInfo nullWrapperNode = adapter.BuildItemWrapper(component);
        EXPECT_EQ(nullWrapperNode.rootNode, nullptr);
        EXPECT_EQ(nullWrapperNode.contentParentNode, nullptr);
    }

    auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7201), false);
    TemplateAdapterNode::ItemWrapperInfo wrapper = adapter.BuildItemWrapper(component);
    EXPECT_NE(wrapper.rootNode, nullptr);
    EXPECT_EQ(wrapper.rootNode, wrapper.contentParentNode);
}

} // namespace
