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
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "TestFixture.h"
#define private public
#include "checks/ChecksEngine.h"
#undef private

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "functions/FunctionBridge.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

class ChecksEngineTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    std::set<int32_t> renderIds_;
    intptr_t manualValueId_ = 0x400000;

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetFunctionBridgeState();
    }

    void TearDown() override
    {
        RenderManager& renderManager = RenderManager::GetInstance();
        for (int32_t renderId : renderIds_) {
            if (renderManager.HasRenderSlot(renderId)) {
                renderManager.RemoveRenderSlot(renderId);
            }
        }
        renderIds_.clear();
        ResetFunctionBridgeState();
        A2UITest::TearDown();
    }

    void ResetFunctionBridgeState()
    {
        mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
        FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
        mockNapiPtr_->ResetCreateReferenceStatus();
    }

    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, "callback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        return callback;
    }

    SurfaceSlot& CreateSurface(
        int32_t renderId, const std::string& surfaceId, const std::vector<std::string>& functions)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
        renderIds_.insert(renderId);
        SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);

        auto catalog = std::make_shared<Catalog>("catalog_" + surfaceId);
        for (const auto& name : functions) {
            catalog->AddFunction(std::make_shared<CatalogItem>(name, CatalogItemType::LOCAL_FUNCTION));
        }
        surfaceSlot.SetCatalog(catalog);
        return surfaceSlot;
    }

    napi_value RawValue(intptr_t id) const
    {
        return reinterpret_cast<napi_value>(id);
    }

    napi_value NewManualTypedValue(napi_valuetype type)
    {
        napi_value value = RawValue(manualValueId_++);
        mockNapiPtr_->valueTypes_[value] = type;
        if (type == napi_object) {
            mockNapiPtr_->objectProperties_[value] = {};
        }
        return value;
    }

    napi_value NewManualBool(bool value)
    {
        napi_value napiValue = NewManualTypedValue(napi_boolean);
        mockNapiPtr_->boolValues_[napiValue] = value;
        return napiValue;
    }

    napi_value NewManualString(const std::string& value)
    {
        napi_value napiValue = NewManualTypedValue(napi_string);
        mockNapiPtr_->stringValues_[napiValue] = value;
        return napiValue;
    }

    napi_value NewManualObject()
    {
        return NewManualTypedValue(napi_object);
    }

    void SetObjectProperty(napi_value object, const std::string& key, napi_value value)
    {
        mockNapiPtr_->objectProperties_[object][key] = value;
    }

    napi_value PredictInvokeResultObject() const
    {
        return RawValue(static_cast<intptr_t>(mockNapiPtr_->nextValueId_ + 7));
    }

    void SetInvokeResponse(napi_value resultObject, bool includeSuccess, bool success)
    {
        mockNapiPtr_->valueTypes_[resultObject] = napi_object;
        mockNapiPtr_->objectProperties_[resultObject] = {};
        if (includeSuccess) {
            SetObjectProperty(resultObject, "success", NewManualBool(success));
        }
    }

    void PrepareNormalizeSuccessResponse(const std::string& value)
    {
        for (int offset = 5; offset <= 20; ++offset) {
            napi_value resultObject = RawValue(static_cast<intptr_t>(mockNapiPtr_->nextValueId_ + offset));
            SetInvokeResponse(resultObject, true, true);

            napi_value normalizedArgs = NewManualObject();
            SetObjectProperty(normalizedArgs, "value", NewManualString(value));
            SetObjectProperty(resultObject, "normalizedArgs", normalizedArgs);
            SetObjectProperty(resultObject, "normalizedReturnType", NewManualString("boolean"));
        }
    }
};

TEST_F(ChecksEngineTest, should_parse_checks_and_collect_binding_paths)
{
    auto descriptor = ParseJson(
        R"({
            "checks": [
                {
                    "condition": {"call": "required", "args": {"value": {"path": "/user/name"}}},
                    "message": "name required"
                },
                {
                    "condition": {"call": "numeric", "args": {"value": "18"}},
                    "message": "age numeric"
                }
            ]
        })");
    ASSERT_NE(descriptor, nullptr);

    ChecksEngine engine(nullptr);
    engine.ParseChecks(descriptor->GetRoot());

    const auto& bindingPaths = engine.GetBindingPaths();
    EXPECT_EQ(bindingPaths.size(), 1U);
    EXPECT_EQ(bindingPaths.count("/user/name"), 1U);
    EXPECT_EQ(engine.checks_.size(), 2U);
}

TEST_F(ChecksEngineTest, should_clear_state_and_skip_invalid_check_items)
{
    auto validDescriptor =
        ParseJson(R"({"checks":[{"condition":{"call":"required","args":{"value":"a"}},"message":"ok"}]})");
    ASSERT_NE(validDescriptor, nullptr);

    ChecksEngine engine(nullptr);
    engine.ParseChecks(validDescriptor->GetRoot());
    EXPECT_EQ(engine.checks_.size(), 1U);

    auto invalidDescriptor = ParseJson(
        R"({
            "checks": [
                1,
                {"message": "missing condition"},
                {"condition": {"call": "formatNumber", "args": {"value": 1}}},
                {"condition": {"call": "required", "args": {"value": {"path": ""}}}, "message": "keep"}
            ]
        })");
    ASSERT_NE(invalidDescriptor, nullptr);

    engine.ParseChecks(invalidDescriptor->GetRoot());

    EXPECT_EQ(engine.checks_.size(), 1U);
    EXPECT_TRUE(engine.GetBindingPaths().empty());
}

TEST_F(ChecksEngineTest, should_report_default_message_for_invalid_condition_rule)
{
    ChecksEngine engine(nullptr);
    engine.checks_.push_back(ChecksEngine::CheckRule { JsonValue(), "" });

    std::string failedMessage = "preset";
    EXPECT_FALSE(engine.Validate(&failedMessage));
    EXPECT_EQ(failedMessage, "Invalid value");
}

TEST_F(ChecksEngineTest, should_evaluate_literal_condition_variants)
{
    ChecksEngine engine(nullptr);

    std::unique_ptr<JsonAdapter> boolTrue = JsonAdapter::CreateBool(true);
    std::unique_ptr<JsonAdapter> boolFalse = JsonAdapter::CreateBool(false);
    std::unique_ptr<JsonAdapter> nullValue = JsonAdapter::CreateNull();
    std::unique_ptr<JsonAdapter> numberValue = JsonAdapter::CreateNumber(3.14);

    ASSERT_NE(boolTrue, nullptr);
    ASSERT_NE(boolFalse, nullptr);
    ASSERT_NE(nullValue, nullptr);
    ASSERT_NE(numberValue, nullptr);

    EXPECT_TRUE(engine.EvaluateCondition(boolTrue->GetRoot()));
    EXPECT_FALSE(engine.EvaluateCondition(boolFalse->GetRoot()));
    EXPECT_FALSE(engine.EvaluateCondition(nullValue->GetRoot()));
    EXPECT_TRUE(engine.EvaluateCondition(numberValue->GetRoot()));

    EXPECT_FALSE(engine.EvaluateCondition(JsonValue()));
}

TEST_F(ChecksEngineTest, should_build_default_context_when_provider_is_null)
{
    ChecksEngine engine(nullptr);
    ChecksResolveContext context = engine.BuildContext();
    EXPECT_EQ(context.renderId, -1);
    EXPECT_TRUE(context.surfaceId.empty());
    EXPECT_TRUE(context.componentId.empty());
}

TEST_F(ChecksEngineTest, should_validate_required_check_with_default_target_value)
{
    const int32_t renderId = 301;
    const std::string surfaceId = "checks-surface";
    const std::string componentId = "text-input";
    CreateSurface(renderId, surfaceId, { "required" });

    mockNapiPtr_->nextValueId_ = 5000;
    PrepareNormalizeSuccessResponse("hello");
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());

    bool defaultTargetCalled = false;
    ChecksEngine engine(
        [renderId, surfaceId, componentId]() { return ChecksResolveContext { renderId, surfaceId, componentId }; },
        [&defaultTargetCalled](JsonValue& targetValue) {
            defaultTargetCalled = true;
            std::unique_ptr<JsonAdapter> value = JsonAdapter::CreateString("hello");
            if (value == nullptr) {
                return false;
            }
            targetValue = value->GetRoot();
            return targetValue.IsValid();
        });

    auto descriptor = ParseJson(R"({"checks":[{"condition":{"call":"required"},"message":"required failed"}]})");
    ASSERT_NE(descriptor, nullptr);

    engine.ParseChecks(descriptor->GetRoot());

    std::string failedMessage;
    EXPECT_TRUE(engine.Validate(&failedMessage));
    EXPECT_TRUE(defaultTargetCalled);
    EXPECT_TRUE(failedMessage.empty());
}

TEST_F(ChecksEngineTest, should_fail_when_function_normalization_fails)
{
    const int32_t renderId = 302;
    const std::string surfaceId = "checks-surface-fail";
    const std::string componentId = "text-input";
    CreateSurface(renderId, surfaceId, { "required" });

    mockNapiPtr_->nextValueId_ = 6000;
    napi_value resultObject = PredictInvokeResultObject();
    SetInvokeResponse(resultObject, true, false);
    SetObjectProperty(resultObject, "errorCode", NewManualString("E_FAIL"));
    SetObjectProperty(resultObject, "errorMessage", NewManualString("normalize failed"));
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());

    ChecksEngine engine(
        [renderId, surfaceId, componentId]() { return ChecksResolveContext { renderId, surfaceId, componentId }; });

    auto descriptor =
        ParseJson(R"({"checks":[{"condition":{"call":"required","args":{"value":"x"}},"message":"required failed"}]})");
    ASSERT_NE(descriptor, nullptr);

    engine.ParseChecks(descriptor->GetRoot());

    std::string failedMessage;
    EXPECT_FALSE(engine.Validate(&failedMessage));
    EXPECT_EQ(failedMessage, "required failed");
}

TEST_F(ChecksEngineTest, should_fail_when_no_check_array_is_present)
{
    ChecksEngine engine(nullptr);

    auto descriptor = ParseJson(R"({"checks":{"condition":{"call":"required"}}})");
    ASSERT_NE(descriptor, nullptr);

    engine.ParseChecks(descriptor->GetRoot());
    std::string failedMessage = "will be cleared";
    EXPECT_TRUE(engine.Validate(&failedMessage));
    EXPECT_TRUE(failedMessage.empty());
    EXPECT_TRUE(engine.checks_.empty());
}

TEST_F(ChecksEngineTest, should_collect_binding_paths_from_array_and_invalid_value)
{
    ChecksEngine engine(nullptr);
    std::unordered_set<std::string> paths;
    JsonValue invalid;
    engine.CollectCheckBindingPaths(invalid, paths);
    EXPECT_TRUE(paths.empty());

    auto value = ParseJson(
        R"([
            {"path": "/root"},
            {"nested": {"path": "/deep"}},
            {"path": ""},
            {"path": 1},
            [{"path": "/array/item"}]
        ])");
    ASSERT_NE(value, nullptr);

    engine.CollectCheckBindingPaths(value->GetRoot(), paths);
    EXPECT_EQ(paths.size(), 3U);
    EXPECT_EQ(paths.count("/root"), 1U);
    EXPECT_EQ(paths.count("/deep"), 1U);
    EXPECT_EQ(paths.count("/array/item"), 1U);
}

TEST_F(ChecksEngineTest, should_accept_all_legacy_check_function_names)
{
    ChecksEngine engine(nullptr);
    auto descriptor = ParseJson(
        R"({
            "checks": [
                {"condition": {"call": "required", "args": {"value": "x"}}},
                {"condition": {"call": "regex", "args": {"value": "x", "pattern": "x"}}},
                {"condition": {"call": "length", "args": {"value": "x", "min": 1}}},
                {"condition": {"call": "numeric", "args": {"value": "1"}}},
                {"condition": {"call": "email", "args": {"value": "a@b.com"}}},
                {"condition": {"call": "formatNumber", "args": {"value": 1}}}
            ]
        })");
    ASSERT_NE(descriptor, nullptr);

    engine.ParseChecks(descriptor->GetRoot());
    EXPECT_EQ(engine.checks_.size(), 5U);
}

TEST_F(ChecksEngineTest, should_validate_with_null_message_buffer_and_fail)
{
    ChecksEngine engine(nullptr);
    auto falseValue = JsonAdapter::CreateBool(false);
    ASSERT_NE(falseValue, nullptr);
    engine.checks_.push_back(ChecksEngine::CheckRule { falseValue->GetRoot(), "" });

    EXPECT_FALSE(engine.Validate(nullptr));
}

TEST_F(ChecksEngineTest, should_use_default_message_when_check_message_is_empty_on_false_result)
{
    ChecksEngine engine(nullptr);
    auto falseValue = JsonAdapter::CreateBool(false);
    ASSERT_NE(falseValue, nullptr);
    engine.checks_.push_back(ChecksEngine::CheckRule { falseValue->GetRoot(), "" });

    std::string failedMessage = "preset";
    EXPECT_FALSE(engine.Validate(&failedMessage));
    EXPECT_EQ(failedMessage, "Invalid value");
}

TEST_F(ChecksEngineTest, should_handle_default_target_provider_returning_false)
{
    const int32_t renderId = 303;
    const std::string surfaceId = "checks-surface-provider-false";
    const std::string componentId = "text-input";
    CreateSurface(renderId, surfaceId, { "required" });

    ChecksEngine engine(
        [renderId, surfaceId, componentId]() { return ChecksResolveContext { renderId, surfaceId, componentId }; },
        [](JsonValue&) { return false; });

    auto descriptor = ParseJson(R"({"checks":[{"condition":{"call":"required"},"message":"required failed"}]})");
    ASSERT_NE(descriptor, nullptr);
    engine.ParseChecks(descriptor->GetRoot());

    std::string failedMessage;
    EXPECT_FALSE(engine.Validate(&failedMessage));
    EXPECT_EQ(failedMessage, "required failed");
}

TEST_F(ChecksEngineTest, should_skip_default_target_injection_when_args_already_present)
{
    const int32_t renderId = 304;
    const std::string surfaceId = "checks-surface-args";
    const std::string componentId = "text-input";
    CreateSurface(renderId, surfaceId, { "required" });

    mockNapiPtr_->nextValueId_ = 7000;
    PrepareNormalizeSuccessResponse("hello");
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());

    bool providerCalled = false;
    ChecksEngine engine(
        [renderId, surfaceId, componentId]() { return ChecksResolveContext { renderId, surfaceId, componentId }; },
        [&providerCalled](JsonValue&) {
            providerCalled = true;
            return true;
        });

    auto descriptor = ParseJson(
        R"({"checks":[{"condition":{"call":"required","args":"invalid-object"},"message":"required failed"}]})");
    ASSERT_NE(descriptor, nullptr);
    engine.ParseChecks(descriptor->GetRoot());

    std::string failedMessage;
    EXPECT_TRUE(engine.Validate(&failedMessage));
    EXPECT_TRUE(failedMessage.empty());
    EXPECT_FALSE(providerCalled);
}

} // namespace
