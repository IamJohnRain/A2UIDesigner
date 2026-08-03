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

#include "components/Component.h"
#include "components/extended/ExtendedGridComponent.h"
#include "composition/GridAdapterNode.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

#include "A2UIArkUITypeConverter.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

class GridAdapterNodeBranchTest : public A2UITest {};

class ExposedGridAdapterNode : public GridAdapterNode {
public:
    using GridAdapterNode::BuildItemWrapper;
    using GridAdapterNode::OnNestedAdapterUpdate;
    using GridAdapterNode::SetupNestedAdapter;
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
    return std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7100), false);
}

TEST_F(GridAdapterNodeBranchTest, should_return_early_when_grid_nested_update_input_is_null)
{
    ExposedGridAdapterNode adapter;
    EXPECT_NO_FATAL_FAILURE(adapter.OnNestedAdapterUpdate(nullptr, "/users/0"));
}

TEST_F(GridAdapterNodeBranchTest, should_skip_grid_nested_update_when_data_model_is_null)
{
    ExposedGridAdapterNode parentAdapter;

    auto nestedGrid = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapter = std::make_shared<GridAdapterNode>();
    nestedAdapter->Initialize("tmpl", "/items", 0);
    nestedGrid->SetLazyMode(true);
    nestedGrid->SetAdapterNode(nestedAdapter);

    EXPECT_NO_FATAL_FAILURE(parentAdapter.OnNestedAdapterUpdate(nestedGrid, "/users/0"));
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(nestedAdapter->GetHandle(), 0), 0);
}

TEST_F(GridAdapterNodeBranchTest, should_update_nested_grid_item_count_for_relative_path)
{
    ExposedGridAdapterNode parentAdapter;

    auto modelData = ParseJson(R"({"users":[{"friends":[1,2,3]}]})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(modelData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto root = CreatePlainComponent();
    auto nestedGrid = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapter = std::make_shared<GridAdapterNode>();
    nestedAdapter->Initialize("tmpl", "friends", 0);
    nestedGrid->SetLazyMode(true);
    nestedGrid->SetAdapterNode(nestedAdapter);
    root->AddChild(nestedGrid);

    parentAdapter.OnNestedAdapterUpdate(root, "/users/0");

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 3U);
}

TEST_F(GridAdapterNodeBranchTest, should_update_nested_grid_item_count_for_absolute_path)
{
    ExposedGridAdapterNode parentAdapter;

    auto modelData = ParseJson(R"({"global":{"items":[{"id":1},{"id":2}]}})");
    ASSERT_NE(modelData, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(modelData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto nestedGrid = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapter = std::make_shared<GridAdapterNode>();
    nestedAdapter->Initialize("tmpl", "/global/items", 0);
    nestedGrid->SetLazyMode(true);
    nestedGrid->SetAdapterNode(nestedAdapter);

    parentAdapter.OnNestedAdapterUpdate(nestedGrid, "/users/0");

    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 2U);
}

TEST_F(GridAdapterNodeBranchTest, should_skip_grid_update_when_node_is_missing_or_not_array)
{
    ExposedGridAdapterNode parentAdapter;

    auto dataModel = std::make_shared<DataModel>("surface");
    auto nonArrayData = ParseJson(R"({"users":[{"friends":{}}]})");
    ASSERT_NE(nonArrayData, nullptr);
    dataModel->ReplaceAll(nonArrayData->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto nestedGridMissing = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapterMissing = std::make_shared<GridAdapterNode>();
    nestedAdapterMissing->Initialize("tmpl", "missing", 1);
    nestedGridMissing->SetLazyMode(true);
    nestedGridMissing->SetAdapterNode(nestedAdapterMissing);

    parentAdapter.OnNestedAdapterUpdate(nestedGridMissing, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapterMissing->GetHandle()], 1U);

    auto nestedGridNonArray = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapterNonArray = std::make_shared<GridAdapterNode>();
    nestedAdapterNonArray->Initialize("tmpl", "friends", 1);
    nestedGridNonArray->SetLazyMode(true);
    nestedGridNonArray->SetAdapterNode(nestedAdapterNonArray);

    parentAdapter.OnNestedAdapterUpdate(nestedGridNonArray, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapterNonArray->GetHandle()], 1U);
}

TEST_F(GridAdapterNodeBranchTest, should_skip_grid_update_when_grid_is_not_lazy_or_adapter_missing)
{
    ExposedGridAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto eagerGrid = std::make_shared<ExtendedGridComponent>();
    auto eagerAdapter = std::make_shared<GridAdapterNode>();
    eagerAdapter->Initialize("tmpl", "friends", 0);
    eagerGrid->SetAdapterNode(eagerAdapter);
    eagerGrid->SetLazyMode(false);
    parentAdapter.OnNestedAdapterUpdate(eagerGrid, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[eagerAdapter->GetHandle()], 0U);

    auto lazyWithoutAdapter = std::make_shared<ExtendedGridComponent>();
    lazyWithoutAdapter->SetLazyMode(true);
    EXPECT_NO_FATAL_FAILURE(parentAdapter.OnNestedAdapterUpdate(lazyWithoutAdapter, "/users/0"));
}

TEST_F(GridAdapterNodeBranchTest, should_skip_grid_update_when_adapter_path_is_empty)
{
    ExposedGridAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1,2]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto lazyGrid = std::make_shared<ExtendedGridComponent>();
    auto nestedAdapter = std::make_shared<GridAdapterNode>();
    nestedAdapter->Initialize("tmpl", "", 2);
    lazyGrid->SetLazyMode(true);
    lazyGrid->SetAdapterNode(nestedAdapter);

    parentAdapter.OnNestedAdapterUpdate(lazyGrid, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[nestedAdapter->GetHandle()], 2U);
}

TEST_F(GridAdapterNodeBranchTest, should_skip_grid_update_when_data_path_not_found_but_continue_children)
{
    ExposedGridAdapterNode parentAdapter;

    auto data = ParseJson(R"({"users":[{"friends":[1,2,3]}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = std::make_shared<DataModel>("surface");
    dataModel->ReplaceAll(data->GetRoot());
    parentAdapter.SetDataModel(dataModel);

    auto root = CreatePlainComponent();
    auto missingPathGrid = std::make_shared<ExtendedGridComponent>();
    auto missingPathAdapter = std::make_shared<GridAdapterNode>();
    missingPathAdapter->Initialize("tmpl", "missingPath", 1);
    missingPathGrid->SetLazyMode(true);
    missingPathGrid->SetAdapterNode(missingPathAdapter);

    auto validPathGrid = std::make_shared<ExtendedGridComponent>();
    auto validPathAdapter = std::make_shared<GridAdapterNode>();
    validPathAdapter->Initialize("tmpl", "friends", 0);
    validPathGrid->SetLazyMode(true);
    validPathGrid->SetAdapterNode(validPathAdapter);

    root->AddChild(missingPathGrid);
    root->AddChild(validPathGrid);

    parentAdapter.OnNestedAdapterUpdate(root, "/users/0");
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[missingPathAdapter->GetHandle()], 1U);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[validPathAdapter->GetHandle()], 3U);
}

TEST_F(GridAdapterNodeBranchTest, should_setup_nested_adapter_only_for_grid_component_and_existing_template)
{
    ExposedGridAdapterNode parentAdapter;
    parentAdapter.SetSurfaceInfo("surface_a", 777);
    parentAdapter.SetDataModel(std::make_shared<DataModel>("surface_a"));

    auto templateDescriptor = ParseJson(R"({"component":"Text"})");
    ASSERT_NE(templateDescriptor, nullptr);

    std::map<std::string, JsonValue> descriptors;
    descriptors["tmpl"] = templateDescriptor->GetRoot();

    auto gridComp = std::make_shared<ExtendedGridComponent>();
    parentAdapter.SetupNestedAdapter(gridComp, "Grid", "missing", "/items", descriptors);
    EXPECT_FALSE(gridComp->IsLazyMode());
    EXPECT_EQ(gridComp->GetAdapterNode(), nullptr);

    auto nonGridComp = CreatePlainComponent();
    parentAdapter.SetupNestedAdapter(nonGridComp, "Grid", "tmpl", "/items", descriptors);

    parentAdapter.SetupNestedAdapter(gridComp, "Column", "tmpl", "/items", descriptors);
    EXPECT_FALSE(gridComp->IsLazyMode());

    parentAdapter.SetupNestedAdapter(gridComp, "Grid", "tmpl", "/items", descriptors);
    EXPECT_TRUE(gridComp->IsLazyMode());
    ASSERT_NE(gridComp->GetAdapterNode(), nullptr);
    EXPECT_EQ(gridComp->GetAdapterNode()->GetTemplateId(), "tmpl");
    EXPECT_EQ(gridComp->GetAdapterNode()->GetDataPath(), "/items");
    EXPECT_EQ(gridComp->GetAdapterNode()->GetSurfaceId(), "surface_a");
    EXPECT_EQ(gridComp->GetAdapterNode()->GetRenderId(), 777);
}

TEST_F(GridAdapterNodeBranchTest, should_build_grid_item_wrapper_only_when_component_and_item_node_are_available)
{
    ExposedGridAdapterNode adapter;

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
        auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7300), false);
        TemplateAdapterNode::ItemWrapperInfo nullWrapperNode = adapter.BuildItemWrapper(component);
        EXPECT_EQ(nullWrapperNode.rootNode, nullptr);
        EXPECT_EQ(nullWrapperNode.contentParentNode, nullptr);
    }

    auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7301), false);
    TemplateAdapterNode::ItemWrapperInfo wrapper = adapter.BuildItemWrapper(component);
    EXPECT_NE(wrapper.rootNode, nullptr);
    EXPECT_EQ(wrapper.rootNode, wrapper.contentParentNode);

    const auto* heightPolicy = [&]() -> const MockArkUINativeProvider::SetAttributeRecord* {
        for (auto iter = mockArkUIPtr_->setAttributeRecords_.rbegin();
             iter != mockArkUIPtr_->setAttributeRecords_.rend(); ++iter) {
            if (iter->nodeHandle == wrapper.rootNode && iter->attribute == NODE_HEIGHT_LAYOUTPOLICY) {
                return &(*iter);
            }
        }
        return nullptr;
    }();
    ASSERT_NE(heightPolicy, nullptr);
    ASSERT_FALSE(heightPolicy->values.empty());
    EXPECT_EQ(heightPolicy->values[0].i32, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT));

    mockArkUIPtr_->resetAttributeRecords_.clear();
    adapter.SetGridItemHeightWrapContent(false);
    auto explicitRowsComponent = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7302), false);
    TemplateAdapterNode::ItemWrapperInfo explicitRowsWrapper = adapter.BuildItemWrapper(explicitRowsComponent);
    EXPECT_NE(explicitRowsWrapper.rootNode, nullptr);

    bool resetExplicitRowsWrapperHeightPolicy = false;
    for (const auto& record : mockArkUIPtr_->resetAttributeRecords_) {
        if (record.nodeHandle == explicitRowsWrapper.rootNode && record.attribute == NODE_HEIGHT_LAYOUTPOLICY) {
            resetExplicitRowsWrapperHeightPolicy = true;
        }
    }
    EXPECT_TRUE(resetExplicitRowsWrapperHeightPolicy);
}

} // namespace
