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

#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "utils/JsonAdapter.h"

using namespace NativeModule;

namespace {

JsonValue ParseJsonOrInvalid(const std::string& json)
{
    auto adapter = JsonAdapter::Parse(json);
    if (adapter == nullptr) {
        return JsonValue();
    }
    return adapter->GetRoot();
}

std::string BuildNestedJson(int32_t depth)
{
    std::string json = R"({"a":)";
    for (int32_t i = 1; i < depth; ++i) {
        json += R"({"a":)";
    }
    json += "1";
    for (int32_t i = 1; i < depth; ++i) {
        json += "}";
    }
    json += "}";
    return json;
}

class BindingUpdateProbe : public Component {
public:
    explicit BindingUpdateProbe(ArkUI_NodeHandle nativeView) : Component(nativeView, false) {}

    using Component::AddBinding;
    using Component::RemoveBindingsForProperty;

    std::string GetType() const override
    {
        return "BindingUpdateProbe";
    }

    int updateCount = 0;
    std::map<std::string, JsonValue> storedValues;
    JsonValue lastUpdateValue;

    void OnDataUpdate(const std::string& property, const JsonValue& value) override
    {
        ++updateCount;
        lastUpdateValue = value;
        Component::OnDataUpdate(property, value);
    }

protected:
    void OnPropertyApplied(const std::string& propertyName, const JsonValue& value) override
    {
        storedValues[propertyName] = value;
    }
};

class ExpressionRefreshProbe : public Component {
public:
    explicit ExpressionRefreshProbe(ArkUI_NodeHandle nativeView) : Component(nativeView, false) {}

    std::string GetType() const override
    {
        return "ExpressionRefreshProbe";
    }

    void AddExpressionBinding(const std::string& propertyName, const std::string& expression,
        const std::vector<std::string>& globalVarDeps, const std::string& dataPath = "")
    {
        auto& bindings = const_cast<std::vector<DataBinding>&>(GetDataBindings());
        DataBinding binding(propertyName, expression, globalVarDeps);
        binding.dataPath_ = dataPath;
        bindings.push_back(std::move(binding));
    }

    int updateCount = 0;
    std::vector<std::string> updatedProperties;

    void OnDataUpdate(const std::string& property, const JsonValue& value) override
    {
        ++updateCount;
        updatedProperties.push_back(property);
        lastValues[property] = value;
    }

    std::map<std::string, JsonValue> lastValues;
};

} // namespace

class BindingEngineDepthTddTest : public ::testing::Test {
protected:
    std::shared_ptr<BindingEngine> engine_;

    void SetUp() override
    {
        engine_ = BindingEngine::Create();
        ASSERT_NE(engine_, nullptr);
    }

    void TearDown() override
    {
        engine_.reset();
    }
};

TEST_F(BindingEngineDepthTddTest, L0_should_update_by_path_when_flat_value)
{
    JsonValue flat = ParseJsonOrInvalid(R"({"x":42})");
    ASSERT_TRUE(flat.IsValid());

    engine_->UpdateDataModelByPath("surf-1", "/test", flat);
    auto model = engine_->GetOrCreateDataModel("surf-1");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/test/x");
    ASSERT_TRUE(node.has_value());
    EXPECT_DOUBLE_EQ(node->GetNumberValue(0.0), 42.0);
}

TEST_F(BindingEngineDepthTddTest, L1_should_continue_update_by_path_when_depth_exceeds_limit)
{
    std::string json21 = BuildNestedJson(21);
    JsonValue deep = ParseJsonOrInvalid(json21);
    ASSERT_TRUE(deep.IsValid());
    ASSERT_EQ(DataModel::MeasureJsonDepth(deep), 21);

    engine_->UpdateDataModelByPath("surf-reject", "/deep", deep);
    auto model = engine_->GetOrCreateDataModel("surf-reject");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/deep");
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(DataModel::MeasureJsonDepth(node.value()), 21);
}

TEST_F(BindingEngineDepthTddTest, L2_should_accept_update_by_path_at_exactly_max_depth)
{
    std::string json20 = BuildNestedJson(20);
    JsonValue atLimit = ParseJsonOrInvalid(json20);
    ASSERT_TRUE(atLimit.IsValid());
    ASSERT_EQ(DataModel::MeasureJsonDepth(atLimit), 20);

    engine_->UpdateDataModelByPath("surf-exact", "/limit", atLimit);
    auto model = engine_->GetOrCreateDataModel("surf-exact");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/limit");
    EXPECT_TRUE(node.has_value());
}

TEST_F(BindingEngineDepthTddTest, L0_should_replace_all_when_flat_value)
{
    JsonValue flat = ParseJsonOrInvalid(R"({"name":"test"})");
    ASSERT_TRUE(flat.IsValid());

    engine_->ReplaceDataModel("surf-replace", flat);
    auto model = engine_->GetOrCreateDataModel("surf-replace");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/name");
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->GetStringValue(""), "test");
}

TEST_F(BindingEngineDepthTddTest, L1_should_continue_replace_all_when_depth_exceeds_limit)
{
    std::string json21 = BuildNestedJson(21);
    JsonValue deep = ParseJsonOrInvalid(json21);
    ASSERT_TRUE(deep.IsValid());

    engine_->ReplaceDataModel("surf-replace-reject", deep);
    auto model = engine_->GetOrCreateDataModel("surf-replace-reject");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/");
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(DataModel::MeasureJsonDepth(node.value()), 21);
}

TEST_F(BindingEngineDepthTddTest, L2_should_accept_replace_all_at_exactly_max_depth)
{
    std::string json20 = BuildNestedJson(20);
    JsonValue atLimit = ParseJsonOrInvalid(json20);
    ASSERT_TRUE(atLimit.IsValid());
    ASSERT_EQ(DataModel::MeasureJsonDepth(atLimit), 20);

    engine_->ReplaceDataModel("surf-replace-exact", atLimit);
    auto model = engine_->GetOrCreateDataModel("surf-replace-exact");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("/");
    EXPECT_TRUE(node.has_value());
}

TEST_F(BindingEngineDepthTddTest, L1_should_skip_update_by_path_when_invalid_path)
{
    JsonValue flat = ParseJsonOrInvalid(R"({"x":1})");
    ASSERT_TRUE(flat.IsValid());

    engine_->UpdateDataModelByPath("surf-invalid", "", flat);
    auto model = engine_->GetOrCreateDataModel("surf-invalid");
    ASSERT_NE(model, nullptr);

    auto node = model->GetNode("");
    EXPECT_FALSE(node.has_value());
}

TEST_F(BindingEngineDepthTddTest, L0_should_delete_by_path_and_ignore_invalid_delete_requests)
{
    JsonValue initial = ParseJsonOrInvalid(R"({"user":{"name":"Alice","age":18}})");
    ASSERT_TRUE(initial.IsValid());
    engine_->ReplaceDataModel("surf-delete", initial);

    auto model = engine_->GetOrCreateDataModel("surf-delete");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(model->GetNode("/user/name").has_value());

    engine_->DeleteDataModelByPath("surf-delete", "");
    EXPECT_TRUE(model->GetNode("/user/name").has_value());

    engine_->DeleteDataModelByPath("missing-surface", "/user/name");
    EXPECT_TRUE(model->GetNode("/user/name").has_value());

    engine_->DeleteDataModelByPath("surf-delete", "/user/name");
    EXPECT_FALSE(model->GetNode("/user/name").has_value());
    EXPECT_TRUE(model->GetNode("/user/age").has_value());
}

TEST_F(BindingEngineDepthTddTest, L0_should_process_update_requests_for_replace_update_and_delete)
{
    JsonValue initial = ParseJsonOrInvalid(R"({"user":{"name":"Alice"}})");
    ASSERT_TRUE(initial.IsValid());

    DataModelUpdate replaceRequest { .surfaceId = "surf-process", .value = initial };
    engine_->ProcessUpdate(replaceRequest);

    auto model = engine_->GetOrCreateDataModel("surf-process");
    ASSERT_NE(model, nullptr);
    ASSERT_TRUE(model->GetNode("/user/name").has_value());
    EXPECT_EQ(model->GetNode("/user/name")->GetStringValue(""), "Alice");

    auto score = JsonAdapter::CreateNumber(9.0);
    ASSERT_NE(score, nullptr);
    DataModelUpdate updateRequest { .surfaceId = "surf-process", .path = "/user/score", .value = score->GetRoot() };
    engine_->ProcessUpdate(updateRequest);

    ASSERT_TRUE(model->GetNode("/user/score").has_value());
    EXPECT_DOUBLE_EQ(model->GetNode("/user/score")->GetNumberValue(0.0), 9.0);

    DataModelUpdate deleteRequest { .surfaceId = "surf-process", .path = "/user/name" };
    engine_->ProcessUpdate(deleteRequest);
    EXPECT_FALSE(model->GetNode("/user/name").has_value());
}

TEST_F(BindingEngineDepthTddTest, L0_should_handle_null_sync_component_lookup_and_stats_smoke_paths)
{
    engine_->SyncComponentBindings(nullptr);
    EXPECT_EQ(engine_->GetComponent("missing"), nullptr);

    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3100));
    comp->SetComponentId("lookup_component");
    comp->AddBinding("label", "/name");
    engine_->RegisterComponent(comp);

    EXPECT_EQ(engine_->GetComponent("lookup_component"), comp);

    JsonValue readyData = ParseJsonOrInvalid(R"({"name":"Alice"})");
    ASSERT_TRUE(readyData.IsValid());
    engine_->ReplaceDataModel("default", readyData);

    engine_->RenderAll();
    engine_->PrintBindingStats();
}

TEST_F(BindingEngineDepthTddTest, L0_should_not_duplicate_pending_component_when_syncing_same_bindings_twice)
{
    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3101));
    comp->SetComponentId("pending_default_component");
    comp->AddBinding("label", "/name");

    engine_->SyncComponentBindings(comp);
    engine_->SyncComponentBindings(comp);

    engine_->UpdateDataModel({ { "name", "Alice" } });

    EXPECT_EQ(comp->updateCount, 2);
}

TEST_F(BindingEngineDepthTddTest, L0_should_not_queue_component_without_bindings_when_syncing_before_data_ready)
{
    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3104));
    comp->SetComponentId("empty_sync_component");

    engine_->SyncComponentBindings(comp);
    engine_->UpdateDataModel({ { "name", "Alice" } });

    EXPECT_EQ(comp->updateCount, 0);
}

/**
 * @tc.name: BindingEngineDepthTdd_should_bind_immediately_when_registering_after_data_model_ready
 * @tc.desc: Verify RegisterComponent binds the default-surface path immediately once the data model is already ready.
 * @tc.type: FUNC
 */
TEST_F(BindingEngineDepthTddTest, L0_should_bind_immediately_when_registering_after_data_model_ready)
{
    JsonValue readyData = ParseJsonOrInvalid(R"({"name":"Alice"})");
    ASSERT_TRUE(readyData.IsValid());
    engine_->ReplaceDataModel("default", readyData);

    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3102));
    comp->SetComponentId("ready_default_component");
    comp->AddBinding("label", "/name");

    engine_->RegisterComponent(comp);

    ASSERT_EQ(comp->updateCount, 1);
    ASSERT_TRUE(comp->storedValues.count("label") > 0);
    EXPECT_EQ(comp->storedValues["label"].GetStringValue(""), "Alice");
}

/**
 * @tc.name: BindingEngineDepthTdd_should_rebind_immediately_when_syncing_new_path_after_data_model_ready
 * @tc.desc: Verify SyncComponentBindings re-applies the current binding path immediately after the data model is ready.
 * @tc.type: FUNC
 */
TEST_F(BindingEngineDepthTddTest, L0_should_rebind_immediately_when_syncing_new_path_after_data_model_ready)
{
    JsonValue readyData = ParseJsonOrInvalid(R"({"name":"Alice","city":"Shenzhen"})");
    ASSERT_TRUE(readyData.IsValid());
    engine_->ReplaceDataModel("default", readyData);

    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3103));
    comp->SetComponentId("ready_sync_component");
    comp->AddBinding("label", "/name");
    engine_->RegisterComponent(comp);
    ASSERT_EQ(comp->updateCount, 1);

    comp->RemoveBindingsForProperty("label");
    comp->AddBinding("label", "/city");
    engine_->SyncComponentBindings(comp);

    ASSERT_EQ(comp->updateCount, 2);
    ASSERT_TRUE(comp->storedValues.count("label") > 0);
    EXPECT_EQ(comp->storedValues["label"].GetStringValue(""), "Shenzhen");
}

TEST_F(BindingEngineDepthTddTest, L0_should_not_refresh_expression_binding_when_registering_after_data_ready)
{
    JsonValue readyData = ParseJsonOrInvalid(R"({"name":"Alice"})");
    ASSERT_TRUE(readyData.IsValid());
    engine_->ReplaceDataModel("default", readyData);

    auto comp = std::make_shared<ExpressionRefreshProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3105));
    comp->SetComponentId("ready_expression_component");
    comp->AddExpressionBinding("text", "$__colorMode", { "__colorMode" });

    engine_->RegisterComponent(comp);

    EXPECT_EQ(comp->updateCount, 0);
}

TEST_F(BindingEngineDepthTddTest, L0_should_skip_expression_refresh_when_binding_has_no_data_path)
{
    auto comp = std::make_shared<ExpressionRefreshProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3106));
    comp->SetComponentId("expression_without_path");
    comp->AddExpressionBinding("text", "$__colorMode", { "__colorMode" });

    engine_->BindComponentImmediate(comp, "default", true);

    EXPECT_EQ(comp->updateCount, 0);
}

TEST_F(BindingEngineDepthTddTest, L0_should_refresh_expression_binding_once_for_duplicate_property_dependencies)
{
    auto comp = std::make_shared<ExpressionRefreshProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3107));
    comp->SetComponentId("expression_dedup_component");
    comp->AddExpressionBinding("text", "$__dataModel.user.first", { "__dataModel" }, "/user/first");
    comp->AddExpressionBinding("text", "$__dataModel.user.last", { "__dataModel" }, "/user/last");

    engine_->BindComponentImmediate(comp, "default", true);

    ASSERT_EQ(comp->updateCount, 1);
    ASSERT_EQ(comp->updatedProperties.size(), 1U);
    EXPECT_EQ(comp->updatedProperties[0], "text");
}

TEST_F(BindingEngineDepthTddTest, L0_should_apply_invalid_fallback_when_binding_immediate_cannot_resolve_value)
{
    JsonValue readyData = ParseJsonOrInvalid(R"({"name":"Alice"})");
    ASSERT_TRUE(readyData.IsValid());
    engine_->ReplaceDataModel("default", readyData);

    auto comp = std::make_shared<BindingUpdateProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3108));
    comp->SetComponentId("missing_path_component");
    comp->AddBinding("label", "/missing");

    engine_->RegisterComponent(comp);

    ASSERT_EQ(comp->updateCount, 1);
    EXPECT_FALSE(comp->lastUpdateValue.IsValid());
}
