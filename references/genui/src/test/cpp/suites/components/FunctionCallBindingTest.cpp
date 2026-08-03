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

#include <algorithm>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "components/Component.h"
#include "data/BindingEngine.h"
#include "data/DataBinding.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "data/ResolvedValue.h"
#include "utils/JsonAdapter.h"

#include "ArkUINativeAPI.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

struct NativeAttributeCall {
    ArkUI_NodeHandle node = nullptr;
    int32_t attribute = -1;
    int32_t size = 0;
    std::vector<ArkUI_NumberValue> values;
    std::string stringValue;
};

struct NativeCallTracker {
    int32_t setAttributeCount = 0;
    std::vector<NativeAttributeCall> attributeCalls;
};

NativeCallTracker g_tracker;

void ResetTracker()
{
    g_tracker = NativeCallTracker {};
}

int32_t TrackSetAttribute(ArkUI_NodeHandle node, int32_t attribute, ArkUI_AttributeItem* item)
{
    ++g_tracker.setAttributeCount;
    NativeAttributeCall call;
    call.node = node;
    call.attribute = attribute;
    if (item != nullptr) {
        call.size = item->size;
        call.stringValue = item->string == nullptr ? "" : item->string;
        for (int32_t i = 0; i < item->size; ++i) {
            call.values.push_back(item->value[i]);
        }
    }
    g_tracker.attributeCalls.push_back(call);
    return 0;
}

int32_t CountAttributeCall(int32_t attribute)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.attributeCalls) {
        if (call.attribute == attribute) {
            ++count;
        }
    }
    return count;
}

class FunctionCallBindingProbe : public Component {
public:
    explicit FunctionCallBindingProbe(ArkUI_NodeHandle nativeView, bool ownsNativeView = false)
        : Component(nativeView, ownsNativeView)
    {}

    std::string GetType() const override
    {
        return "FunctionCallBindingProbe";
    }

    void InvokeSetPropertyFromDescriptor(
        const std::string& propertyKey, const JsonValue& descriptor, const std::string& bindingKey = "")
    {
        SetPropertyFromDescriptor(propertyKey, descriptor, bindingKey);
    }

    void InvokeOnDataUpdate(const std::string& property, const JsonValue& value)
    {
        OnDataUpdate(property, value);
    }

    void DeclarePrivateProperty(const PropertyDeclaration& declaration)
    {
        privateDeclarations_[declaration.name] = declaration;
    }

    std::vector<std::string> appliedPropertyNames;
    std::vector<std::string> removedPropertyNames;
    std::vector<std::string> updatedPropertyNames;
    std::map<std::string, JsonValue> storedProperties;
    int32_t updateCount = 0;

protected:
    void OnDataUpdate(const std::string& property, const JsonValue& value) override
    {
        ++updateCount;
        updatedPropertyNames.push_back(property);
        Component::OnDataUpdate(property, value);
    }

    void OnPropertyApplied(const std::string& propertyName, const JsonValue& value) override
    {
        appliedPropertyNames.push_back(propertyName);
        storedProperties[propertyName] = value;
    }

    void OnPropertyRemoved(const std::string& propertyName) override
    {
        removedPropertyNames.push_back(propertyName);
        storedProperties.erase(propertyName);
    }

    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override
    {
        auto it = privateDeclarations_.find(propertyName);
        if (it != privateDeclarations_.end()) {
            return it->second;
        }
        return Component::GetPrivatePropertyDeclaration(propertyName);
    }

private:
    std::map<std::string, PropertyDeclaration> privateDeclarations_;
};

} // namespace

class FunctionCallBindingTest : public A2UITest {
protected:
    ArkUI_NativeNodeAPI_1* api_ = nullptr;
    ArkUI_NativeNodeAPI_1 savedApi_ {};
    std::set<int32_t> renderIds_;

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetTracker();

        api_ = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
        ASSERT_NE(api_, nullptr);
        savedApi_ = *api_;
        api_->setAttribute = TrackSetAttribute;
    }

    void TearDown() override
    {
        for (int32_t renderId : renderIds_) {
            if (RenderManager::GetInstance().HasRenderSlot(renderId)) {
                RenderManager::GetInstance().RemoveRenderSlot(renderId);
            }
        }
        renderIds_.clear();

        if (api_ != nullptr) {
            *api_ = savedApi_;
        }
        ResetTracker();
        A2UITest::TearDown();
    }
};

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

TEST_F(FunctionCallBindingTest, L0_should_extract_no_paths_from_plain_literal)
{
    auto adapter = ParseJson(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_extract_path_from_simple_descriptor)
{
    auto adapter = ParseJson(R"({"path":"/user/name"})");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/user/name");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_paths_from_function_call_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatNumber",
            "args": {
                "value": {"path": "/data/price"},
                "locale": "en-US"
            }
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/data/price");
}

TEST_F(FunctionCallBindingTest, L0_should_deduplicate_paths)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {
                "a": {"path": "/shared/value"},
                "b": {"path": "/shared/value"}
            }
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/shared/value");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_multiple_paths_from_nested_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatCurrency",
            "args": {
                "amount": {"path": "/order/total"},
                "currency": {"path": "/order/currencyCode"}
            }
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 2U);
    EXPECT_EQ(paths[0], "/order/total");
    EXPECT_EQ(paths[1], "/order/currencyCode");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_paths_from_array_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "concat",
            "args": [
                {"path": "/first/name"},
                {"path": "/last/name"}
            ]
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 2U);
}

TEST_F(FunctionCallBindingTest, L0_should_return_empty_for_invalid_value)
{
    JsonValue invalid;
    auto paths = DynamicValueResolver::ExtractDataPaths(invalid);
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_add_function_call_binding_with_correct_type)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4001), false);

    auto descriptor = ParseJson(
        R"({
            "call": "formatNumber",
            "args": {
                "value": {"path": "/price"}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->AddFunctionCallBinding("content", "/price", descriptor->GetRoot());
    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "content");
    EXPECT_EQ(bindings[0].dataPath_, "/price");
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);
    EXPECT_TRUE(bindings[0].functionCallDescriptor_.IsValid());
}

TEST_F(FunctionCallBindingTest, L0_should_register_function_call_binding_via_set_property)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4002), false);
    ResetTracker();

    auto descriptor = ParseJson(
        R"({
            "content": {
                "call": "formatNumber",
                "args": {
                    "value": {"path": "/price"}
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("content", descriptor->GetRoot());

    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "content");
    EXPECT_EQ(bindings[0].dataPath_, "/price");
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);
}

TEST_F(FunctionCallBindingTest, L0_should_re_execute_function_on_data_update)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4003), false);
    ResetTracker();

    auto descriptor = ParseJson(
        R"({
            "content": {
                "call": "formatNumber",
                "args": {
                    "value": {"path": "/price"}
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("content", descriptor->GetRoot());
    ASSERT_EQ(component->GetDataBindings().size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].type_, BindingType::FUNCTION_CALL);

    auto newValue = ParseJson(R"(99)");
    ASSERT_NE(newValue, nullptr);
    component->InvokeOnDataUpdate("content", newValue->GetRoot());

    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);
    EXPECT_TRUE(bindings[0].functionCallDescriptor_.IsValid());
}

TEST_F(FunctionCallBindingTest, L0_should_fall_through_to_apply_runtime_when_no_function_call_binding)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4004), false);

    component->AddBinding("customProp", "/some/path");
    ASSERT_EQ(component->GetDataBindings().size(), 1U);

    auto value = ParseJson(R"("updated")");
    ASSERT_NE(value, nullptr);
    component->InvokeOnDataUpdate("customProp", value->GetRoot());

    EXPECT_TRUE(std::find(component->appliedPropertyNames.begin(), component->appliedPropertyNames.end(),
                    "customProp") != component->appliedPropertyNames.end());
}

TEST_F(FunctionCallBindingTest, L0_should_not_register_binding_when_no_data_paths)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4005), false);

    auto descriptor = ParseJson(
        R"({
            "content": {
                "call": "formatNumber",
                "args": {
                    "value": 42,
                    "locale": "en-US"
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("content", descriptor->GetRoot());

    auto bindings = component->GetDataBindings();
    EXPECT_TRUE(bindings.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_not_register_binding_for_empty_property)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4006), false);

    auto descriptor = ParseJson(
        R"({
            "": {
                "call": "formatNumber",
                "args": {
                    "value": {"path": "/price"}
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("", descriptor->GetRoot());
    EXPECT_TRUE(component->GetDataBindings().empty());
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_path_when_value_is_not_string)
{
    auto adapter = ParseJson(R"({"path": 123})");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_path_when_path_is_empty_string)
{
    auto adapter = ParseJson(R"({"path": ""})");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_return_empty_paths_for_number_literal)
{
    auto adapter = ParseJson(R"(42)");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_return_empty_paths_for_boolean_literal)
{
    auto adapter = ParseJson(R"(true)");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_resolve_function_call_descriptor_should_return_null_for_non_object)
{
    DynamicResolveContext context = { .renderId = -1, .surfaceId = "", .componentId = "" };
    auto numAdapter = ParseJson(R"(42)");
    ASSERT_NE(numAdapter, nullptr);
    auto result = DynamicValueResolver::ResolveFunctionCallDescriptor(numAdapter->GetRoot(), context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(FunctionCallBindingTest, L0_resolve_function_call_descriptor_should_return_null_for_non_string_call)
{
    DynamicResolveContext context = { .renderId = -1, .surfaceId = "", .componentId = "" };
    auto adapter = ParseJson(R"({"call": 123})");
    ASSERT_NE(adapter, nullptr);
    auto result = DynamicValueResolver::ResolveFunctionCallDescriptor(adapter->GetRoot(), context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(FunctionCallBindingTest, L0_resolve_function_call_descriptor_should_return_null_for_empty_call)
{
    DynamicResolveContext context = { .renderId = -1, .surfaceId = "", .componentId = "" };
    auto adapter = ParseJson(R"({"call": ""})");
    ASSERT_NE(adapter, nullptr);
    auto result = DynamicValueResolver::ResolveFunctionCallDescriptor(adapter->GetRoot(), context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(FunctionCallBindingTest, L0_resolve_function_call_descriptor_should_succeed_without_args)
{
    DynamicResolveContext context = { .renderId = -1, .surfaceId = "", .componentId = "" };
    auto adapter = ParseJson(R"({"call": "required"})");
    ASSERT_NE(adapter, nullptr);
    auto result = DynamicValueResolver::ResolveFunctionCallDescriptor(adapter->GetRoot(), context);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetFunctionName(), "required");
}

TEST_F(FunctionCallBindingTest, L0_should_reject_function_call_when_allow_dynamic_is_false)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4010), false);
    ResetTracker();

    component->DeclarePrivateProperty({ .name = "variant",
        .type = PropertyValueType::STRING,
        .allowDynamic = false,
        .enumAllowed = { "body", "caption" },
        .enumFallback = "body" });

    auto descriptor = ParseJson(
        R"({
            "variant": {
                "call": "formatString",
                "returnType": "string",
                "args": {"value": "caption"}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("variant", descriptor->GetRoot());

    EXPECT_TRUE(component->GetDataBindings().empty());
}

TEST_F(FunctionCallBindingTest, L0_should_register_binding_even_when_function_call_resolve_fails)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4011), false);
    ResetTracker();

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(9021);
    renderIds_.insert(9021);
    SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface("9022");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    auto root = ParseJson(R"({"data":{"value":"hello"}})");
    ASSERT_NE(root, nullptr);
    dataModel->ReplaceAll(root->GetRoot());

    component->SetRenderId(9021);
    component->SetSurfaceId("9022");
    component->SetComponentId("fc-resolve-fail-test");

    auto descriptor = ParseJson(
        R"({
            "text": {
                "call": "required",
                "returnType": "boolean",
                "args": {"value": {"path": "/data/value"}}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "text");
    EXPECT_EQ(bindings[0].dataPath_, "/data/value");
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);
    EXPECT_TRUE(bindings[0].functionCallDescriptor_.IsValid());
}

TEST_F(FunctionCallBindingTest, L0_should_retain_binding_after_on_data_update_with_resolve_failure)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4012), false);
    ResetTracker();

    auto descriptor = ParseJson(
        R"({
            "text": {
                "call": "formatNumber",
                "returnType": "string",
                "args": {"value": {"path": "/data/value"}}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    ASSERT_EQ(component->GetDataBindings().size(), 1U);

    auto updateValue = ParseJson(R"("updated")");
    ASSERT_NE(updateValue, nullptr);
    component->InvokeOnDataUpdate("text", updateValue->GetRoot());

    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);
}

TEST_F(FunctionCallBindingTest, L0_should_use_binding_key_when_set_property_with_explicit_key)
{
    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4013), false);

    auto descriptor = ParseJson(
        R"({
            "text": {
                "call": "formatNumber",
                "returnType": "string",
                "args": {"value": {"path": "/data/value"}}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot(), "customBindingKey");

    auto bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "customBindingKey");
    EXPECT_EQ(bindings[0].dataPath_, "/data/value");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_paths_from_template_string)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {"value": "Hello, ${/userName}! Today is ${/currentDate}"}
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 2U);
    EXPECT_EQ(paths[0], "/userName");
    EXPECT_EQ(paths[1], "/currentDate");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_path_from_single_template_placeholder)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {"value": "${/data/name}"}
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/data/name");
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_escaped_template_placeholder)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {"value": "\\${/userName} is escaped"}
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_non_path_template_placeholder)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {"value": "${notAPath}"}
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

TEST_F(FunctionCallBindingTest, L0_should_deduplicate_paths_from_template_and_path_descriptor)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {
                "value": "${/data/name}",
                "fallback": {"path": "/data/name"}
            }
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/data/name");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_paths_from_mixed_template_and_path_descriptor)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {
                "value": "Hello ${/userName}",
                "fallback": {"path": "/data/fallback"}
            }
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 2U);
    EXPECT_EQ(paths[0], "/userName");
    EXPECT_EQ(paths[1], "/data/fallback");
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_paths_from_plain_string_without_template)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {"value": "plain text without any path reference"}
        })");
    ASSERT_NE(adapter, nullptr);
    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    EXPECT_TRUE(paths.empty());
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(FunctionCallBindingTest, L0_should_extract_expression_paths_from_allowed_function_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatNumber",
            "args": {
                "value": "{{ $__dataModel.price }}",
                "decimals": "{{ $__dataModel.precision }}",
                "grouping": "{{ $__dataModel.grouping }}"
            }
        })");
    ASSERT_NE(adapter, nullptr);

    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 3U);
    EXPECT_EQ(paths[0], "/price");
    EXPECT_EQ(paths[1], "/precision");
    EXPECT_EQ(paths[2], "/grouping");
}

TEST_F(FunctionCallBindingTest, L0_should_filter_static_expression_args_when_extracting_paths)
{
    auto adapter = ParseJson(
        R"({
            "call": "regex",
            "args": {
                "value": "{{ $__dataModel.input }}",
                "pattern": "{{ $__dataModel.pattern }}"
            }
        })");
    ASSERT_NE(adapter, nullptr);

    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/input");
}

TEST_F(FunctionCallBindingTest, L0_should_not_extract_expression_paths_from_static_action_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "setDataModel",
            "args": {
                "path": "{{ $__dataModel.targetPath }}",
                "value": "{{ $__dataModel.nextValue }}"
            }
        })");
    ASSERT_NE(adapter, nullptr);

    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 1U);
    EXPECT_EQ(paths[0], "/nextValue");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_mixed_paths_from_allowed_recursive_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "setDataModel",
            "args": {
                "path": "/target",
                "value": {
                    "fromPath": {"path": "/a"},
                    "fromTemplate": "${/b}",
                    "fromExpression": "{{ $__dataModel.c }}"
                }
            }
        })");
    ASSERT_NE(adapter, nullptr);

    auto paths = DynamicValueResolver::ExtractDataPaths(adapter->GetRoot());
    ASSERT_EQ(paths.size(), 3U);
    EXPECT_EQ(paths[0], "/a");
    EXPECT_EQ(paths[1], "/b");
    EXPECT_EQ(paths[2], "/c");
}

TEST_F(FunctionCallBindingTest, L0_should_extract_global_dependencies_from_allowed_function_args)
{
    auto adapter = ParseJson(
        R"({
            "call": "formatString",
            "args": {
                "value": "{{ $__widthBreakpoint }}"
            }
        })");
    ASSERT_NE(adapter, nullptr);

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(adapter->GetRoot());
    EXPECT_TRUE(dependencies.dataPaths.empty());
    ASSERT_EQ(dependencies.globalVariables.size(), 1U);
    EXPECT_EQ(dependencies.globalVariables[0], "__widthBreakpoint");
}

TEST_F(FunctionCallBindingTest, L0_should_refresh_function_call_binding_when_global_dependency_changes)
{
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(9023);
    renderIds_.insert(9023);
    SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface("9024");

    auto component = std::make_shared<FunctionCallBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4014), false);
    component->SetRenderId(9023);
    component->SetSurfaceId("9024");
    component->SetComponentId("fc-global-dep-test");
    component->DeclarePrivateProperty(
        { .name = "dynamicText", .type = PropertyValueType::STRING, .allowDynamic = true, .fallbackString = "" });

    auto descriptor = ParseJson(
        R"({
            "dynamicText": {
                "call": "formatString",
                "returnType": "string",
                "args": {
                    "value": "{{ $__widthBreakpoint }}"
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("dynamicText", descriptor->GetRoot());
    ASSERT_EQ(component->GetDataBindings().size(), 1U);
    ASSERT_EQ(component->GetDataBindings()[0].globalVarDeps_.size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].globalVarDeps_[0], "__widthBreakpoint");

    surfaceSlot.GetBindingEngine()->RegisterComponent(component);
    EXPECT_EQ(component->updateCount, 0);

    renderSlot.GetSurfaceManager()->UpdateBreakpoint(Breakpoint::LG);

    EXPECT_EQ(component->updateCount, 1);
    ASSERT_EQ(component->updatedPropertyNames.size(), 1U);
    EXPECT_EQ(component->updatedPropertyNames[0], "dynamicText");
}
#endif
