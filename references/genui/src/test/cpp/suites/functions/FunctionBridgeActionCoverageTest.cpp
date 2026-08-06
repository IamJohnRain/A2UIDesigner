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

#include <cJSON.h>
#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/button/ButtonComponent.h"
#include "components/Component.h"
#include "components/extended/ExtendedTextComponent.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/ActionInfo.h"
#include "functions/ActionParser.h"
#include "functions/EventContextResolver.h"
#include "functions/FunctionBridge.h"
#include "functions/FunctionCallInfo.h"
#include "functions/FunctionResult.h"
#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/JsonAdapter.h"

#include "NativeEntry.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceErrorCodes.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

#if !defined(_MSC_VER)
#define A2UI_ENABLE_STD_ALLOC_FAIL_HOOK 1
#else
#define A2UI_ENABLE_STD_ALLOC_FAIL_HOOK 0
#endif

namespace {

#if A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
thread_local int32_t gStdAllocFailCountdown = -1;

void* AllocateWithStdFailHook(std::size_t size)
{
    if (gStdAllocFailCountdown == 0) {
        throw std::bad_alloc();
    }
    if (gStdAllocFailCountdown > 0) {
        --gStdAllocFailCountdown;
    }

    void* ptr = std::malloc(size == 0 ? 1 : size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void FreeWithStdFailHook(void* ptr) noexcept
{
    std::free(ptr);
}
#endif

std::unique_ptr<JsonAdapter> ParseJson(const std::string& jsonText)
{
    return JsonAdapter::Parse(jsonText);
}

std::string BuildNestedContextJson(int32_t depth)
{
    std::string json = "{}";
    for (int32_t index = 0; index < depth; ++index) {
        json = std::string("{\"k") + std::to_string(index) + "\":" + json + "}";
    }
    return json;
}

std::string BuildNestedArgsJson(int32_t depth)
{
    std::string json = R"("{{ 'leaf' }}")";
    for (int32_t index = 0; index < depth; ++index) {
        json = std::string("{\"k") + std::to_string(index) + "\":" + json + "}";
    }
    return json;
}

int gCjsonFailCountdown = -1;

void* TestCjsonMalloc(size_t size)
{
    if (gCjsonFailCountdown == 0) {
        return nullptr;
    }
    if (gCjsonFailCountdown > 0) {
        --gCjsonFailCountdown;
    }
    return std::malloc(size);
}

void TestCjsonFree(void* ptr)
{
    std::free(ptr);
}

class ScopedCjsonAllocFail {
public:
    explicit ScopedCjsonAllocFail(int failAfter)
    {
        cJSON_Hooks hooks = { .malloc_fn = TestCjsonMalloc, .free_fn = TestCjsonFree };
        gCjsonFailCountdown = failAfter;
        cJSON_InitHooks(&hooks);
    }

    ~ScopedCjsonAllocFail()
    {
        gCjsonFailCountdown = -1;
        cJSON_InitHooks(nullptr);
    }
};

#if A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
class ScopedStdAllocFail {
public:
    explicit ScopedStdAllocFail(int32_t failAfter) : previousCountdown_(gStdAllocFailCountdown)
    {
        gStdAllocFailCountdown = failAfter;
    }

    ~ScopedStdAllocFail()
    {
        gStdAllocFailCountdown = previousCountdown_;
    }

private:
    int32_t previousCountdown_ = -1;
};

template<typename Callback>
int32_t ExerciseStdAllocationFailures(Callback callback, int32_t maxFailAfter)
{
    int32_t caughtFailures = 0;
    for (int32_t failAfter = 0; failAfter <= maxFailAfter; ++failAfter) {
        try {
            ScopedStdAllocFail fail(failAfter);
            callback();
        } catch (const std::bad_alloc&) {
            ++caughtFailures;
        }
    }
    return caughtFailures;
}
#else
template<typename Callback>
int32_t ExerciseStdAllocationFailures(Callback callback, int32_t maxFailAfter)
{
    (void)callback;
    (void)maxFailAfter;
    return 0;
}
#endif

bool ContainsString(const std::vector<std::string>& values, const std::string& expected)
{
    for (const std::string& value : values) {
        if (value == expected) {
            return true;
        }
    }
    return false;
}

size_t CountString(const std::vector<std::string>& values, const std::string& expected)
{
    size_t count = 0;
    for (const std::string& value : values) {
        if (value == expected) {
            ++count;
        }
    }
    return count;
}

std::shared_ptr<FunctionCallInfo> ResolveFunctionCallDescriptorForCoverage(
    const std::string& functionName, const std::string& argsJson, const std::string& returnType = "void")
{
    std::string descriptorJson = std::string("{\"call\":\"") + functionName + "\",\"args\":" + argsJson +
                                 ",\"returnType\":\"" + returnType + "\"}";
    auto resolverInput = ParseJson(descriptorJson);
    if (resolverInput == nullptr) {
        return nullptr;
    }

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_policy_coverage", .componentId = "button_policy_coverage"
    };
    return DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
}

} // namespace

#if A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
void* operator new(std::size_t size)
{
    return AllocateWithStdFailHook(size);
}

void* operator new[](std::size_t size)
{
    return AllocateWithStdFailHook(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return AllocateWithStdFailHook(size);
    } catch (...) {
        return nullptr;
    }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    try {
        return AllocateWithStdFailHook(size);
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void* ptr) noexcept
{
    FreeWithStdFailHook(ptr);
}

void operator delete[](void* ptr) noexcept
{
    FreeWithStdFailHook(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    FreeWithStdFailHook(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    FreeWithStdFailHook(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    FreeWithStdFailHook(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    FreeWithStdFailHook(ptr);
}
#endif

namespace {

class FunctionBridgeActionCoverageTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);
    std::set<int32_t> renderIds_;
    intptr_t manualValueId_ = 0x200000;

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetFunctionBridgeState();
        ResetActionDispatchBridgeState();
        ResetRuntimeErrorDispatchBridgeState();
        ResetWarningDispatchBridgeState();
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
        A2UITest::TearDown();
    }

    void ResetFunctionBridgeState()
    {
        mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
        FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
        mockNapiPtr_->ResetCreateReferenceStatus();
    }

    void ResetActionDispatchBridgeState()
    {
        mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
        ActionDispatchBridge::GetInstance().RegisterDispatchAction(env_, CreateCallback());
        mockNapiPtr_->ResetCreateReferenceStatus();
    }

    void ResetRuntimeErrorDispatchBridgeState()
    {
        mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
        RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
        mockNapiPtr_->ResetCreateReferenceStatus();
    }

    void ResetWarningDispatchBridgeState()
    {
        mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
        WarningDispatchBridge::GetInstance().RegisterDispatchWarning(env_, CreateCallback());
        mockNapiPtr_->ResetCreateReferenceStatus();
    }

    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, "callback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        return callback;
    }

    std::shared_ptr<FunctionCallInfo> CreateFunctionCall(
        const std::string& name, const std::string& returnType = "void", const JsonValue& args = JsonValue())
    {
        return std::make_shared<FunctionCallInfo>(name, args, returnType);
    }

    SurfaceSlot& CreateSurface(int32_t renderId, const std::string& surfaceId,
        const std::vector<std::string>& allowedFunctions, bool withCatalog = true)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
        renderIds_.insert(renderId);
        SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
        if (withCatalog) {
            auto catalog = std::make_shared<Catalog>(std::string("catalog_") + surfaceId);
            for (const std::string& functionName : allowedFunctions) {
                catalog->AddFunction(std::make_shared<CatalogItem>(functionName, CatalogItemType::LOCAL_FUNCTION));
            }
            surfaceSlot.SetCatalog(catalog);
        }
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

    napi_value NewManualNumber(double value)
    {
        napi_value napiValue = NewManualTypedValue(napi_number);
        mockNapiPtr_->numberValues_[napiValue] = value;
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

    napi_value NewManualArray(const std::vector<napi_value>& elements)
    {
        napi_value arrayValue = NewManualObject();
        mockNapiPtr_->isArrayFlags_[arrayValue] = true;
        mockNapiPtr_->arrayLengths_[arrayValue] = static_cast<uint32_t>(elements.size());
        for (size_t index = 0; index < elements.size(); ++index) {
            mockNapiPtr_->arrayElements_[arrayValue][static_cast<uint32_t>(index)] = elements[index];
        }
        return arrayValue;
    }

    void SetObjectProperty(napi_value object, const std::string& key, napi_value value)
    {
        mockNapiPtr_->objectProperties_[object][key] = value;
    }

    napi_value PredictInvokeResultObject(bool normalizeOnly = false) const
    {
        return RawValue(static_cast<intptr_t>(mockNapiPtr_->nextValueId_ + (normalizeOnly ? 9 : 8)));
    }

    void SetInvokeResponse(napi_value resultObject, bool includeSuccess, bool success, bool includeValue = false,
        napi_value value = nullptr)
    {
        mockNapiPtr_->valueTypes_[resultObject] = napi_object;
        mockNapiPtr_->objectProperties_[resultObject] = {};
        if (includeSuccess) {
            SetObjectProperty(resultObject, "success", NewManualBool(success));
        }
        if (includeValue) {
            SetObjectProperty(resultObject, "value", value);
        }
    }

    bool FindRuntimeErrorRequest(int32_t* renderId, int32_t* errorCode, std::string* errorMessage, std::string* source)
    {
        for (const auto& objectEntry : mockNapiPtr_->objectProperties_) {
            const auto& properties = objectEntry.second;
            auto renderIdIt = properties.find("renderId");
            auto errorCodeIt = properties.find("errorCode");
            auto errorMessageIt = properties.find("errorMessage");
            if (renderIdIt == properties.end() || errorCodeIt == properties.end() ||
                errorMessageIt == properties.end()) {
                continue;
            }

            if (renderId != nullptr) {
                auto renderIdValue = mockNapiPtr_->numberValues_.find(renderIdIt->second);
                *renderId = renderIdValue != mockNapiPtr_->numberValues_.end()
                                ? static_cast<int32_t>(renderIdValue->second)
                                : 0;
            }
            if (errorCode != nullptr) {
                auto errorCodeValue = mockNapiPtr_->numberValues_.find(errorCodeIt->second);
                *errorCode = errorCodeValue != mockNapiPtr_->numberValues_.end()
                                 ? static_cast<int32_t>(errorCodeValue->second)
                                 : 0;
            }
            if (errorMessage != nullptr) {
                auto errorMessageValue = mockNapiPtr_->stringValues_.find(errorMessageIt->second);
                *errorMessage = errorMessageValue != mockNapiPtr_->stringValues_.end() ? errorMessageValue->second : "";
            }
            if (source != nullptr) {
                auto sourceIt = properties.find("source");
                if (sourceIt != properties.end()) {
                    auto sourceValue = mockNapiPtr_->stringValues_.find(sourceIt->second);
                    *source = sourceValue != mockNapiPtr_->stringValues_.end() ? sourceValue->second : "";
                } else {
                    source->clear();
                }
            }
            return true;
        }
        return false;
    }

    size_t CountRuntimeErrorRequests() const
    {
        size_t count = 0;
        for (const auto& objectEntry : mockNapiPtr_->objectProperties_) {
            const auto& properties = objectEntry.second;
            if (properties.find("renderId") == properties.end() || properties.find("errorCode") == properties.end() ||
                properties.find("errorMessage") == properties.end()) {
                continue;
            }
            ++count;
        }
        return count;
    }
};

class ExposedButtonComponent : public ButtonComponent {
public:
    using ButtonComponent::HandleSpecialProperty;
};

} // namespace

TEST(ActionInfoCoverageTest, should_cover_validity_for_function_event_and_unknown)
{
    ActionInfo unknown;
    EXPECT_EQ(unknown.GetType(), ActionType::UNKNOWN);
    EXPECT_FALSE(unknown.IsValid());

    ActionInfo functionActionWithNull { std::shared_ptr<FunctionCallInfo>(), JsonValue() };
    EXPECT_EQ(functionActionWithNull.GetType(), ActionType::FUNCTION_CALL);
    EXPECT_EQ(functionActionWithNull.GetFunctionCall(), nullptr);
    EXPECT_FALSE(functionActionWithNull.IsValid());

    auto argsAdapter = ParseJson("{}");
    ASSERT_NE(argsAdapter, nullptr);
    auto emptyNameCall = std::make_shared<FunctionCallInfo>("", argsAdapter->GetRoot(), "void");
    ActionInfo functionActionWithEmptyName(emptyNameCall, argsAdapter->GetRoot());
    EXPECT_FALSE(functionActionWithEmptyName.IsValid());

    auto namedCall = std::make_shared<FunctionCallInfo>("open", argsAdapter->GetRoot(), "void");
    ActionInfo functionAction(namedCall, argsAdapter->GetRoot());
    EXPECT_TRUE(functionAction.IsValid());
    EXPECT_EQ(functionAction.GetFunctionCall()->GetFunctionName(), "open");
    EXPECT_TRUE(functionAction.GetFunctionCallDescriptor().IsObject());

    ActionInfo eventAction("", JsonValue());
    EXPECT_EQ(eventAction.GetType(), ActionType::EVENT);
    EXPECT_FALSE(eventAction.IsValid());

    auto eventContext = ParseJson("{\"id\":\"btn1\"}");
    ASSERT_NE(eventContext, nullptr);
    ActionInfo validEventAction("tap", eventContext->GetRoot());
    EXPECT_TRUE(validEventAction.IsValid());
    EXPECT_EQ(validEventAction.GetEventName(), "tap");
    EXPECT_TRUE(validEventAction.GetEventContextDescriptor().IsObject());
}

TEST(FunctionResultCoverageTest, should_cover_all_types_literals_and_string_format)
{
    FunctionResult nullResult;
    FunctionResult boolResult(true);
    FunctionResult intResult(42);
    FunctionResult doubleResult(3.5);
    FunctionResult stringResult(std::string("a\"b"));

    EXPECT_TRUE(nullResult.IsNull());
    EXPECT_TRUE(boolResult.IsBool());
    EXPECT_TRUE(intResult.IsInt());
    EXPECT_TRUE(doubleResult.IsDouble());
    EXPECT_TRUE(stringResult.IsString());

    EXPECT_TRUE(nullResult.GetBoolValue(true));
    EXPECT_EQ(boolResult.GetBoolValue(false), true);
    EXPECT_EQ(boolResult.GetIntValue(7), 7);
    EXPECT_EQ(intResult.GetIntValue(0), 42);
    EXPECT_DOUBLE_EQ(intResult.GetDoubleValue(1.25), 1.25);
    EXPECT_DOUBLE_EQ(doubleResult.GetDoubleValue(0.0), 3.5);
    EXPECT_EQ(doubleResult.GetStringValue("fallback"), "fallback");
    EXPECT_EQ(stringResult.GetStringValue("fallback"), "a\"b");

    JsonValue nullJson = nullResult.ToJsonValue();
    JsonValue boolJson = boolResult.ToJsonValue();
    JsonValue intJson = intResult.ToJsonValue();
    JsonValue doubleJson = doubleResult.ToJsonValue();
    JsonValue stringJson = stringResult.ToJsonValue();
    EXPECT_TRUE(nullJson.IsNull());
    EXPECT_TRUE(boolJson.IsBool());
    EXPECT_TRUE(intJson.IsNumber());
    EXPECT_TRUE(doubleJson.IsNumber());
    EXPECT_TRUE(stringJson.IsString());

    EXPECT_EQ(nullResult.ToJsonLiteral(), "null");
    EXPECT_EQ(boolResult.ToJsonLiteral(), "true");
    EXPECT_EQ(intResult.ToJsonLiteral(), "42");
    EXPECT_EQ(FunctionResult(8.0).ToJsonLiteral(), "8");
    EXPECT_EQ(FunctionResult(3.5).ToJsonLiteral(), "3.5");
    EXPECT_EQ(FunctionResult(std::numeric_limits<double>::infinity()).ToJsonLiteral(), "inf");
    EXPECT_EQ(stringResult.ToJsonLiteral(), "\"a\\\"b\"");

    EXPECT_EQ(nullResult.ToString(), "null");
    EXPECT_EQ(boolResult.ToString(), "true");
    EXPECT_EQ(intResult.ToString(), "42");
    EXPECT_EQ(FunctionResult(2.5).ToString(), "2.5");
    EXPECT_EQ(stringResult.ToString(), "a\"b");

    EXPECT_TRUE(FunctionResult(10).Equals(FunctionResult(10)));
    EXPECT_FALSE(FunctionResult(10).Equals(FunctionResult(11)));
    EXPECT_FALSE(FunctionResult(10).Equals(FunctionResult(10.0)));
    EXPECT_TRUE(FunctionResult(std::string("x")).Equals(FunctionResult(std::string("x"))));
}

TEST(FunctionResultCoverageTest, should_cover_remaining_equals_and_to_string_paths)
{
    EXPECT_EQ(FunctionResult(false).ToString(), "false");
    EXPECT_EQ(FunctionResult(false).ToJsonLiteral(), "false");

    EXPECT_TRUE(FunctionResult().Equals(FunctionResult()));
    EXPECT_TRUE(FunctionResult(false).Equals(FunctionResult(false)));
    EXPECT_TRUE(FunctionResult(3.25).Equals(FunctionResult(3.25)));
    EXPECT_FALSE(FunctionResult(3.25).Equals(FunctionResult(1.25)));
}

TEST(FunctionResultCoverageTest, should_support_json_object_result)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    ASSERT_NE(adapter, nullptr);
    JsonValue root = adapter->GetRoot();
    ASSERT_TRUE(root.PutBool("isOn", true));
    ASSERT_TRUE(root.PutString("label", ""));

    FunctionResult objectResult(root);
    EXPECT_TRUE(objectResult.IsJsonValue());
    EXPECT_TRUE(objectResult.ToJsonValue().IsObject());
    EXPECT_EQ(objectResult.ToJsonLiteral(), R"({"isOn":true,"label":""})");
    EXPECT_EQ(objectResult.ToString(), R"({"isOn":true,"label":""})");
    EXPECT_TRUE(objectResult.Equals(FunctionResult(root)));
}

TEST(FunctionResultCoverageTest, should_return_invalid_json_when_create_adapter_fails)
{
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(FunctionResult().ToJsonValue().IsValid());
    }
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(FunctionResult(true).ToJsonValue().IsValid());
    }
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(FunctionResult(1).ToJsonValue().IsValid());
    }
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(FunctionResult(1.0).ToJsonValue().IsValid());
    }
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(FunctionResult(std::string("x")).ToJsonValue().IsValid());
    }
}

TEST(FunctionResultCoverageTest, should_cover_default_switch_branches_with_invalid_internal_type)
{
    struct FunctionResultLayout {
        FunctionResultType type_;
        bool boolValue_;
        int32_t intValue_;
        double doubleValue_;
        std::string stringValue_;
        JsonValue jsonValue_;
    };

    FunctionResult invalidTypeResult;
    auto* invalidLayout = reinterpret_cast<FunctionResultLayout*>(&invalidTypeResult);
    invalidLayout->type_ = static_cast<FunctionResultType>(-1);

    EXPECT_FALSE(invalidTypeResult.ToJsonValue().IsValid());
    EXPECT_EQ(invalidTypeResult.ToJsonLiteral(), "null");
    EXPECT_EQ(invalidTypeResult.ToString(), "null");

    FunctionResult anotherInvalidTypeResult;
    auto* anotherInvalidLayout = reinterpret_cast<FunctionResultLayout*>(&anotherInvalidTypeResult);
    anotherInvalidLayout->type_ = static_cast<FunctionResultType>(-1);
    EXPECT_FALSE(invalidTypeResult.Equals(anotherInvalidTypeResult));
}

TEST(ActionParserCoverageTest, should_reject_invalid_descriptor_shapes)
{
    auto numberAdapter = ParseJson("1");
    ASSERT_NE(numberAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(numberAdapter->GetRoot()), nullptr);

    auto actionNotObjectAdapter = ParseJson("{\"action\":1}");
    ASSERT_NE(actionNotObjectAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(actionNotObjectAdapter->GetRoot()), nullptr);

    auto bothAdapter = ParseJson(R"({"action":{"functionCall":{"call":"x"},"event":{"name":"tap"}}})");
    ASSERT_NE(bothAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(bothAdapter->GetRoot()), nullptr);

    auto invalidEventAdapter = ParseJson(R"({"action":{"event":"tap"}})");
    ASSERT_NE(invalidEventAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(invalidEventAdapter->GetRoot()), nullptr);

    auto missingActionContentAdapter = ParseJson(R"({"action":{"name":"tap"}})");
    ASSERT_NE(missingActionContentAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(missingActionContentAdapter->GetRoot()), nullptr);
}

TEST(ActionParserCoverageTest, should_parse_function_call_with_and_without_args)
{
    auto withArgsAdapter =
        ParseJson(R"({"action":{"functionCall":{"call":"format","args":{"a":1},"returnType":"string"}}})");
    ASSERT_NE(withArgsAdapter, nullptr);
    std::shared_ptr<ActionInfo> withArgsAction = ActionParser::Parse(withArgsAdapter->GetRoot());
    ASSERT_NE(withArgsAction, nullptr);
    EXPECT_EQ(withArgsAction->GetType(), ActionType::FUNCTION_CALL);
    ASSERT_NE(withArgsAction->GetFunctionCall(), nullptr);
    EXPECT_EQ(withArgsAction->GetFunctionCall()->GetFunctionName(), "format");
    EXPECT_EQ(withArgsAction->GetFunctionCall()->GetReturnType(), "string");
    EXPECT_TRUE(withArgsAction->GetFunctionCall()->GetArgs().IsObject());

    auto defaultReturnTypeAdapter = ParseJson(R"({"functionCall":{"call":"required"}})");
    ASSERT_NE(defaultReturnTypeAdapter, nullptr);
    std::shared_ptr<ActionInfo> defaultAction = ActionParser::Parse(defaultReturnTypeAdapter->GetRoot());
    ASSERT_NE(defaultAction, nullptr);
    EXPECT_EQ(defaultAction->GetFunctionCall()->GetReturnType(), "void");

    auto emptyCallAdapter = ParseJson(R"({"action":{"functionCall":{"call":""}}})");
    ASSERT_NE(emptyCallAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(emptyCallAdapter->GetRoot()), nullptr);
}

TEST(ActionParserCoverageTest, should_parse_event_context_and_reject_invalid_depth)
{
    auto nonObjectContextAdapter = ParseJson(R"({"action":{"event":{"name":"tap","context":123}}})");
    ASSERT_NE(nonObjectContextAdapter, nullptr);
    std::shared_ptr<ActionInfo> nonObjectContextAction = ActionParser::Parse(nonObjectContextAdapter->GetRoot());
    ASSERT_NE(nonObjectContextAction, nullptr);
    EXPECT_EQ(nonObjectContextAction->GetType(), ActionType::EVENT);
    EXPECT_TRUE(nonObjectContextAction->GetEventContextDescriptor().IsNumber());

    auto validContextAdapter = ParseJson(R"({"action":{"event":{"name":"tap","context":{"a":[1,2,3],"b":"x"}}}})");
    ASSERT_NE(validContextAdapter, nullptr);
    std::shared_ptr<ActionInfo> validContextAction = ActionParser::Parse(validContextAdapter->GetRoot());
    ASSERT_NE(validContextAction, nullptr);
    EXPECT_EQ(validContextAction->GetType(), ActionType::EVENT);
    EXPECT_TRUE(validContextAction->GetEventContextDescriptor().IsObject());

    std::string deepContextJson =
        std::string("{\"action\":{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}}";
    auto deepContextAdapter = ParseJson(deepContextJson);
    ASSERT_NE(deepContextAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(deepContextAdapter->GetRoot()), nullptr);
}

TEST(ActionParserCoverageTest, should_handle_empty_event_name_and_missing_context)
{
    auto emptyNameAdapter = ParseJson(R"({"action":{"event":{"name":""}}})");
    ASSERT_NE(emptyNameAdapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(emptyNameAdapter->GetRoot()), nullptr);

    auto noContextAdapter = ParseJson(R"({"action":{"event":{"name":"tap"}}})");
    ASSERT_NE(noContextAdapter, nullptr);
    std::shared_ptr<ActionInfo> action = ActionParser::Parse(noContextAdapter->GetRoot());
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->GetType(), ActionType::EVENT);
    EXPECT_FALSE(action->GetEventContextDescriptor().IsValid());
}

TEST(ActionParserCoverageTest, should_fail_when_context_or_args_clone_fails)
{
    auto contextActionAdapter = ParseJson(R"({"action":{"event":{"name":"tap","context":{"k":"v"}}}})");
    ASSERT_NE(contextActionAdapter, nullptr);
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_EQ(ActionParser::Parse(contextActionAdapter->GetRoot()), nullptr);
    }

    auto argsActionAdapter = ParseJson(R"({"action":{"functionCall":{"call":"x","args":{"k":"v"}}}})");
    ASSERT_NE(argsActionAdapter, nullptr);
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_EQ(ActionParser::Parse(argsActionAdapter->GetRoot()), nullptr);
    }
}

TEST(ActionParserCoverageTest, should_drop_action_when_function_call_and_event_both_exist_even_if_call_invalid)
{
    auto adapter = ParseJson(R"({"action":{"functionCall":{"call":""},"event":{"name":"tap","context":{"value":1}}}})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(ActionParser::Parse(adapter->GetRoot()), nullptr);
}

TEST(ActionParserCoverageTest, should_accept_event_context_at_maximum_supported_depth)
{
    std::string descriptorJson =
        std::string(R"({"action":{"event":{"name":"tap","context":)") + BuildNestedContextJson(19) + "}}}";
    auto adapter = ParseJson(descriptorJson);
    ASSERT_NE(adapter, nullptr);
    std::shared_ptr<ActionInfo> action = ActionParser::Parse(adapter->GetRoot());
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->GetType(), ActionType::EVENT);
    EXPECT_EQ(action->GetEventName(), "tap");
}

TEST(ActionParserCoverageTest, should_keep_public_parse_behavior_for_invalid_function_or_event_nodes)
{
    auto invalidFunctionNode = ParseJson(R"({"action":{"functionCall":[]}})");
    ASSERT_NE(invalidFunctionNode, nullptr);
    EXPECT_EQ(ActionParser::Parse(invalidFunctionNode->GetRoot()), nullptr);

    auto invalidEventNode = ParseJson(R"({"action":{"event":[]}})");
    ASSERT_NE(invalidEventNode, nullptr);
    EXPECT_EQ(ActionParser::Parse(invalidEventNode->GetRoot()), nullptr);
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_cover_invalid_inputs_and_resolution_paths)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    JsonValue emptyDescriptor;
    JsonValue invalidResult = EventContextResolver::Resolve(emptyDescriptor, context);
    EXPECT_TRUE(invalidResult.IsObject());
    EXPECT_EQ(invalidResult.GetArraySize(), 0);

    auto nonObjectDescriptor = ParseJson("[]");
    ASSERT_NE(nonObjectDescriptor, nullptr);
    JsonValue nonObjectResult = EventContextResolver::Resolve(nonObjectDescriptor->GetRoot(), context);
    EXPECT_TRUE(nonObjectResult.IsObject());

    auto objectDescriptor = ParseJson(R"({"":1,"keep":"ok","bad":{"path":"//invalid"}})");
    ASSERT_NE(objectDescriptor, nullptr);
    JsonValue resolved = EventContextResolver::Resolve(objectDescriptor->GetRoot(), context);
    EXPECT_TRUE(resolved.IsObject());
    EXPECT_FALSE(resolved.Has(""));
    EXPECT_TRUE(resolved.Has("keep"));
    EXPECT_FALSE(resolved.Has("bad"));
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_resolve_nested_expression_values)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };
    auto descriptor = ParseJson(R"({
        "count": "{{ 2 + 3 }}",
        "nested": {
            "enabled": "{{ 1 < 2 }}",
            "items": ["{{ 6 * 7 }}"]
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    JsonValue resolved = EventContextResolver::Resolve(descriptor->GetRoot(), context);

    ASSERT_TRUE(resolved.IsObject());
    EXPECT_DOUBLE_EQ(resolved.GetItem("count").GetNumberValue(0.0), 5.0);
    JsonValue nested = resolved.GetItem("nested");
    ASSERT_TRUE(nested.IsObject());
    EXPECT_TRUE(nested.GetItem("enabled").GetBoolValue(false));
    JsonValue items = nested.GetItem("items");
    ASSERT_TRUE(items.IsArray());
    EXPECT_DOUBLE_EQ(items.GetArrayItem(0).GetNumberValue(0.0), 42.0);
}
#endif

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_return_invalid_when_context_object_create_fails)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    JsonValue invalidDescriptor;
    ScopedCjsonAllocFail fail(0);
    JsonValue result = EventContextResolver::Resolve(invalidDescriptor, context);
    EXPECT_FALSE(result.IsValid());
}

TEST_F(FunctionBridgeActionCoverageTest, action_dispatch_bridge_should_cover_registration_and_dispatch_branches)
{
    auto& bridge = ActionDispatchBridge::GetInstance();
    JsonValue context;

    bridge.RegisterDispatchAction(nullptr, nullptr);
    bridge.RegisterDispatchAction(env_, nullptr);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));

    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    bridge.RegisterDispatchAction(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));

    bridge.RegisterDispatchAction(env_, CreateCallback());
    ASSERT_FALSE(mockNapiPtr_->refToValue_.empty());
    napi_ref oldRef = mockNapiPtr_->refToValue_.begin()->first;
    bridge.RegisterDispatchAction(env_, CreateCallback());
    EXPECT_EQ(mockNapiPtr_->refToValue_.count(oldRef), 0u);

    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));
    mockNapiPtr_->ResetGetReferenceValueStatus();

    mockNapiPtr_->refToValue_.clear();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));

    bridge.RegisterDispatchAction(env_, CreateCallback());
    mockNapiPtr_->SetCreateObjectStatus(napi_generic_failure);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));
    mockNapiPtr_->ResetCreateObjectStatus();

    mockNapiPtr_->nextValueId_ = 0;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));
    mockNapiPtr_->nextValueId_ = 100;

    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "source", "tap", context));
    mockNapiPtr_->ResetCallFunctionStatus();

    EXPECT_TRUE(bridge.Dispatch(1, "surface", "source", "tap", context));
}

TEST_F(FunctionBridgeActionCoverageTest, warning_dispatch_bridge_should_cover_registration_and_dispatch_branches)
{
    auto& bridge = WarningDispatchBridge::GetInstance();

    bridge.RegisterDispatchWarning(nullptr, nullptr);
    bridge.RegisterDispatchWarning(env_, nullptr);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));

    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    bridge.RegisterDispatchWarning(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));

    bridge.RegisterDispatchWarning(env_, CreateCallback());
    ASSERT_FALSE(mockNapiPtr_->refToValue_.empty());
    napi_ref oldRef = mockNapiPtr_->refToValue_.begin()->first;
    bridge.RegisterDispatchWarning(env_, CreateCallback());
    EXPECT_EQ(mockNapiPtr_->refToValue_.count(oldRef), 0u);

    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));
    mockNapiPtr_->ResetGetReferenceValueStatus();

    struct WarningBridgeLayout {
        napi_env warningNapiEnv;
        napi_ref dispatchWarningRef;
    };

    auto* layout = reinterpret_cast<WarningBridgeLayout*>(&bridge);
    ASSERT_NE(layout->dispatchWarningRef, nullptr);
    mockNapiPtr_->refToValue_[layout->dispatchWarningRef] = nullptr;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));

    bridge.RegisterDispatchWarning(env_, CreateCallback());
    mockNapiPtr_->refToValue_.clear();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));

    bridge.RegisterDispatchWarning(env_, CreateCallback());
    mockNapiPtr_->SetCreateObjectStatus(napi_generic_failure);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));
    mockNapiPtr_->ResetCreateObjectStatus();

    mockNapiPtr_->nextValueId_ = 0;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));
    mockNapiPtr_->nextValueId_ = 100;

    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", "W", "m", "p", "component", "name"));
    mockNapiPtr_->ResetCallFunctionStatus();

    EXPECT_TRUE(bridge.Dispatch(11, "surface_a", "component_a", "W_11", "warning", "root", "component", "item"));
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
}

TEST_F(
    FunctionBridgeActionCoverageTest, warning_dispatch_bridge_should_register_when_previous_env_is_null_but_ref_exists)
{
    auto& bridge = WarningDispatchBridge::GetInstance();
    bridge.RegisterDispatchWarning(env_, CreateCallback());

    struct WarningBridgeLayout {
        napi_env warningNapiEnv;
        napi_ref dispatchWarningRef;
    };

    auto* layout = reinterpret_cast<WarningBridgeLayout*>(&bridge);
    ASSERT_NE(layout->dispatchWarningRef, nullptr);
    size_t oldRefCount = mockNapiPtr_->refToValue_.size();
    layout->warningNapiEnv = nullptr;

    bridge.RegisterDispatchWarning(env_, CreateCallback());
    EXPECT_GT(mockNapiPtr_->refToValue_.size(), oldRefCount);

    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(bridge.Dispatch(9, "surface_prev", "component_prev", "W_9", "ok", "root", "component", "item"));
}

TEST_F(FunctionBridgeActionCoverageTest, runtime_error_dispatch_bridge_should_cover_registration_and_dispatch_branches)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();

    bridge.RegisterDispatchRuntimeError(nullptr, nullptr);
    bridge.RegisterDispatchRuntimeError(env_, nullptr);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));

    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));

    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    ASSERT_FALSE(mockNapiPtr_->refToValue_.empty());
    napi_ref oldRef = mockNapiPtr_->refToValue_.begin()->first;
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    EXPECT_EQ(mockNapiPtr_->refToValue_.count(oldRef), 0u);

    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
    mockNapiPtr_->ResetGetReferenceValueStatus();

    mockNapiPtr_->refToValue_.clear();
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));

    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    mockNapiPtr_->SetCreateObjectStatus(napi_generic_failure);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
    mockNapiPtr_->ResetCreateObjectStatus();

    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
    mockNapiPtr_->ResetCallFunctionStatus();

    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(bridge.Dispatch(11, "surface_a", "component_a", 3201, "runtime failed", "RuntimeUnitTest"));

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 11);
    EXPECT_EQ(errorCode, 3201);
    EXPECT_EQ(errorMessage, "runtime failed");
    EXPECT_EQ(source, "RuntimeUnitTest");
}

TEST_F(FunctionBridgeActionCoverageTest, runtime_error_dispatch_bridge_should_fail_when_request_object_is_null)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    mockNapiPtr_->nextValueId_ = 0;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
    mockNapiPtr_->nextValueId_ = 100;
}

TEST_F(FunctionBridgeActionCoverageTest,
    runtime_error_dispatch_bridge_should_fail_when_reference_value_resolves_to_null_callback)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    struct RuntimeErrorBridgeLayout {
        napi_env napiEnv;
        napi_ref dispatchRef;
    };

    auto* layout = reinterpret_cast<RuntimeErrorBridgeLayout*>(&bridge);
    ASSERT_NE(layout->dispatchRef, nullptr);
    mockNapiPtr_->refToValue_[layout->dispatchRef] = nullptr;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest, runtime_error_dispatch_bridge_should_fail_when_env_is_null_before_dispatch)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    struct RuntimeErrorBridgeLayout {
        napi_env napiEnv;
        napi_ref dispatchRef;
    };

    auto* layout = reinterpret_cast<RuntimeErrorBridgeLayout*>(&bridge);
    ASSERT_NE(layout->dispatchRef, nullptr);
    layout->napiEnv = nullptr;
    EXPECT_FALSE(bridge.Dispatch(1, "surface", "component", 1002, "failed", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest,
    runtime_error_dispatch_bridge_should_keep_previous_callback_when_new_registration_invalid)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(bridge.Dispatch(9, "surface_prev", "component_prev", 2001, "ok", "unit"));

    bridge.RegisterDispatchRuntimeError(nullptr, CreateCallback());
    bridge.RegisterDispatchRuntimeError(env_, nullptr);
    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(bridge.Dispatch(9, "surface_prev", "component_prev", 2001, "ok", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest, runtime_error_dispatch_bridge_should_cover_registration_exception_edges)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    mockNapiPtr_->SetThrowOnMethod("DeleteReference");
    EXPECT_THROW(bridge.RegisterDispatchRuntimeError(env_, CreateCallback()), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();

    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    mockNapiPtr_->SetThrowOnMethod("CreateReference");
    EXPECT_THROW(bridge.RegisterDispatchRuntimeError(env_, CreateCallback()), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest, runtime_error_dispatch_bridge_should_cover_dispatch_exception_edges)
{
    struct ThrowCase {
        const char* methodName;
        int32_t callIndex;
    };

    std::vector<ThrowCase> cases = { { "GetReferenceValue", 1 }, { "CreateObject", 1 }, { "CreateInt32", 1 },
        { "CreateInt32", 2 }, { "SetNamedProperty", 1 }, { "SetNamedProperty", 2 }, { "CreateStringUtf8", 1 },
        { "CreateStringUtf8", 2 }, { "CreateStringUtf8", 3 }, { "CreateStringUtf8", 4 }, { "GetGlobal", 1 },
        { "CallFunction", 1 } };

    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    for (const ThrowCase& item : cases) {
        mockNapiPtr_->ResetThrowOnMethod();
        bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
        mockNapiPtr_->SetThrowOnMethod(item.methodName, item.callIndex);
        EXPECT_THROW(bridge.Dispatch(21, "surface_throw", "component_throw", 4001, "failed", "RuntimeThrowTest"),
            std::runtime_error);
    }
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest, native_entry_register_dispatch_runtime_error_should_validate_callback_argument)
{
    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RuntimeErrorDispatchBridge::GetInstance().Dispatch(1, "surface", "component", 1, "m", "unit"));

    mockNapiPtr_->SetCallbackArgs({ NewManualNumber(1.0) });
    EXPECT_EQ(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RuntimeErrorDispatchBridge::GetInstance().Dispatch(1, "surface", "component", 1, "m", "unit"));

    mockNapiPtr_->SetCallbackArgs({ CreateCallback() });
    EXPECT_EQ(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(RuntimeErrorDispatchBridge::GetInstance().Dispatch(1, "surface", "component", 1, "m", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest, native_entry_register_dispatch_runtime_error_should_reject_null_callback_value)
{
    mockNapiPtr_->SetCallbackArgs({ nullptr });
    EXPECT_EQ(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RuntimeErrorDispatchBridge::GetInstance().Dispatch(1, "surface", "component", 1, "m", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest, action_parser_should_dispatch_runtime_error_when_context_depth_exceeds_limit)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    std::string deepContextJson =
        std::string("{\"action\":{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}}";
    auto deepContextAdapter = ParseJson(deepContextJson);
    ASSERT_NE(deepContextAdapter, nullptr);

    ActionParseContext parseContext = { .renderId = 12, .surfaceId = "surface_a", .componentId = "button_a" };
    mockNapiPtr_->objectProperties_.clear();
    EXPECT_EQ(ActionParser::Parse(deepContextAdapter->GetRoot(), parseContext), nullptr);

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 12);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ACTION_PARSE_FAILED);
    EXPECT_NE(errorMessage.find("depth exceeds"), std::string::npos);
    EXPECT_EQ(source, "ActionParser");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_dispatch_runtime_error_when_path_missing)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(13, "surface_runtime_error", { "noop" });

    auto resolverInput = ParseJson(R"({"path":"/missing"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 13, .surfaceId = "surface_runtime_error", .componentId = "button_runtime_error"
    };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.path, "/missing");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 13);
    EXPECT_EQ(errorCode, SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED);
    EXPECT_NE(errorMessage.find("path not found"), std::string::npos);
    EXPECT_NE(errorMessage.find("/missing"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(
    FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_not_dispatch_runtime_error_when_render_id_negative)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(-1, "surface_runtime_error_zero", { "noop" });

    auto resolverInput = ParseJson(R"({"path":"/missing"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_runtime_error_negative", .componentId = "button_runtime_error_negative"
    };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.path, "/missing");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_expression_values_in_function_args)
{
    auto resolverInput = ParseJson(R"({
        "call": "setDataModel",
        "args": {
            "path": "{{ '/user/' + 'name' }}",
            "value": "{{ 'Ada' + ' Lovelace' }}"
        },
        "returnType": "void"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_expression_args", .componentId = "button_expression_args"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    EXPECT_EQ(functionCall->GetFunctionName(), "setDataModel");
    ASSERT_TRUE(functionCall->GetArgs().IsObject());
    EXPECT_EQ(functionCall->GetArgs().GetString("path", ""), "{{ '/user/' + 'name' }}");
    EXPECT_EQ(functionCall->GetArgs().GetString("value", ""), "Ada Lovelace");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_nested_array_expression_args)
{
    auto resolverInput = ParseJson(R"({
        "call": "setDataModel",
        "args": {
            "path": "/items",
            "value": [
                "{{ 'alpha' }}",
                { "value": "{{ 1 + 1 }}" },
                [ "{{ 'beta' }}", { "enabled": "{{ true }}" } ]
            ]
        },
        "returnType": "void"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_expression_array_args", .componentId = "button_expression_array_args"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);

    JsonValue items = functionCall->GetArgs().GetItem("value");
    ASSERT_TRUE(items.IsArray());
    ASSERT_EQ(items.GetArraySize(), 3);
    EXPECT_EQ(items.GetArrayItem(0).GetStringValue(""), "alpha");
    EXPECT_DOUBLE_EQ(items.GetArrayItem(1).GetItem("value").GetNumberValue(0.0), 2.0);

    JsonValue nestedItems = items.GetArrayItem(2);
    ASSERT_TRUE(nestedItems.IsArray());
    EXPECT_EQ(nestedItems.GetArrayItem(0).GetStringValue(""), "beta");
    EXPECT_TRUE(nestedItems.GetArrayItem(1).GetItem("enabled").GetBoolValue(false));
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_fail_when_function_args_exceed_max_depth)
{
    std::string resolverJson = std::string(R"({"call":"setDataModel","args":{"path":"/deep","value":)") +
                               BuildNestedArgsJson(18) + R"(},"returnType":"void"})";
    auto resolverInput = ParseJson(resolverJson);
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_expression_deep_args", .componentId = "button_expression_deep_args"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    EXPECT_EQ(functionCall, nullptr);
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_keep_static_function_args_literal)
{
    auto resolverInput = ParseJson(R"({
        "call": "regex",
        "args": {
            "value": "{{ 'abc' }}",
            "pattern": "{{ 'abc' }}"
        },
        "returnType": "boolean"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_static_args", .componentId = "button_static_args"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    ASSERT_TRUE(functionCall->GetArgs().IsObject());
    EXPECT_EQ(functionCall->GetArgs().GetString("value", ""), "abc");
    EXPECT_EQ(functionCall->GetArgs().GetString("pattern", ""), "{{ 'abc' }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_keep_numeric_bounds_literal)
{
    auto resolverInput = ParseJson(R"({
        "call": "numeric",
        "args": {
            "value": "{{ 5 }}",
            "min": "{{ 1 }}",
            "max": "{{ 10 }}"
        },
        "returnType": "boolean"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_numeric_bounds", .componentId = "button_numeric_bounds"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    ASSERT_TRUE(functionCall->GetArgs().IsObject());
    EXPECT_DOUBLE_EQ(functionCall->GetArgs().GetNumber("value", 0.0), 5.0);
    EXPECT_EQ(functionCall->GetArgs().GetString("min", ""), "{{ 1 }}");
    EXPECT_EQ(functionCall->GetArgs().GetString("max", ""), "{{ 10 }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_keep_open_url_literal)
{
    auto resolverInput = ParseJson(R"({
        "call": "openUrl",
        "args": {
            "url": "{{ 'https://example.com' }}"
        },
        "returnType": "void"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_open_url", .componentId = "button_open_url"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    ASSERT_TRUE(functionCall->GetArgs().IsObject());
    EXPECT_EQ(functionCall->GetArgs().GetString("url", ""), "{{ 'https://example.com' }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_set_attributes_allowed_args)
{
    auto resolverInput = ParseJson(R"({
        "call": "setAttributes",
        "args": {
            "componentId": "{{ 'targetText' }}",
            "value": {
                "text": "{{ 'hello' }}",
                "{{ 'key' }}": "{{ 'value' }}"
            }
        },
        "returnType": "void"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_set_attrs", .componentId = "button_set_attrs"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    ASSERT_TRUE(functionCall->GetArgs().IsObject());
    EXPECT_EQ(functionCall->GetArgs().GetString("componentId", ""), "targetText");
    JsonValue value = functionCall->GetArgs().GetItem("value");
    ASSERT_TRUE(value.IsObject());
    EXPECT_EQ(value.GetString("text", ""), "hello");
    EXPECT_EQ(value.GetString("{{ 'key' }}", ""), "value");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_standard_matrix_allowed_args)
{
    auto resolverInput = ParseJson(R"({
        "call": "formatCurrency",
        "args": {
            "value": "{{ 40 + 2 }}",
            "currency": "{{ 'USD' }}",
            "decimals": "{{ 2 }}",
            "grouping": "{{ true }}"
        },
        "returnType": "string"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_standard_matrix", .componentId = "button_standard_matrix"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    JsonValue args = functionCall->GetArgs();
    ASSERT_TRUE(args.IsObject());
    EXPECT_DOUBLE_EQ(args.GetNumber("value", 0.0), 42.0);
    EXPECT_EQ(args.GetString("currency", ""), "USD");
    EXPECT_DOUBLE_EQ(args.GetNumber("decimals", 0.0), 2.0);
    EXPECT_TRUE(args.GetBool("grouping", false));
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_boolean_array_items)
{
    auto resolverInput = ParseJson(R"({
        "call": "and",
        "args": {
            "values": ["{{ true }}", "{{ false }}"]
        },
        "returnType": "boolean"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_boolean_array", .componentId = "button_boolean_array"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    JsonValue values = functionCall->GetArgs().GetItem("values");
    ASSERT_TRUE(values.IsArray());
    ASSERT_EQ(values.GetArraySize(), 2);
    EXPECT_TRUE(values.GetArrayItem(0).GetBoolValue(false));
    EXPECT_FALSE(values.GetArrayItem(1).GetBoolValue(true));
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_pluralize_branches)
{
    auto resolverInput = ParseJson(R"({
        "call": "pluralize",
        "args": {
            "value": "{{ 2 }}",
            "one": "{{ 'item' }}",
            "other": "{{ 'items' }}"
        },
        "returnType": "string"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_pluralize", .componentId = "button_pluralize"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    JsonValue args = functionCall->GetArgs();
    ASSERT_TRUE(args.IsObject());
    EXPECT_DOUBLE_EQ(args.GetNumber("value", 0.0), 2.0);
    EXPECT_EQ(args.GetString("one", ""), "item");
    EXPECT_EQ(args.GetString("other", ""), "items");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_extended_function_args)
{
    auto selectInput = ParseJson(R"({
        "call": "getSelectValue",
        "args": {
            "componentId": "{{ 'selectA' }}"
        },
        "returnType": "string"
    })");
    ASSERT_NE(selectInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_extended_args", .componentId = "button_extended_args"
    };

    std::shared_ptr<FunctionCallInfo> selectCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(selectInput->GetRoot(), context);
    ASSERT_NE(selectCall, nullptr);
    EXPECT_EQ(selectCall->GetArgs().GetString("componentId", ""), "selectA");

    auto navigateInput = ParseJson(R"({
        "call": "navigate",
        "args": {
            "componentId": "{{ 'sourcePage' }}",
            "targetComponentId": "{{ 'targetPage' }}"
        },
        "returnType": "void"
    })");
    ASSERT_NE(navigateInput, nullptr);

    std::shared_ptr<FunctionCallInfo> navigateCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(navigateInput->GetRoot(), context);
    ASSERT_NE(navigateCall, nullptr);
    EXPECT_EQ(navigateCall->GetArgs().GetString("componentId", ""), "sourcePage");
    EXPECT_EQ(navigateCall->GetArgs().GetString("targetComponentId", ""), "targetPage");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_remaining_root_policy_args)
{
    std::shared_ptr<FunctionCallInfo> requiredCall = ResolveFunctionCallDescriptorForCoverage(
        "required", R"({"value":"  {{ 'filled' }}  ","message":"{{ 'literal' }}"})", "boolean");
    ASSERT_NE(requiredCall, nullptr);
    EXPECT_EQ(requiredCall->GetArgs().GetString("value", ""), "filled");
    EXPECT_EQ(requiredCall->GetArgs().GetString("message", ""), "{{ 'literal' }}");

    std::shared_ptr<FunctionCallInfo> lengthCall = ResolveFunctionCallDescriptorForCoverage(
        "length", R"({"value":"{{ 'abc' }}","min":"{{ 1 }}","max":"{{ 3 }}"})", "boolean");
    ASSERT_NE(lengthCall, nullptr);
    EXPECT_EQ(lengthCall->GetArgs().GetString("value", ""), "abc");
    EXPECT_EQ(lengthCall->GetArgs().GetString("min", ""), "{{ 1 }}");
    EXPECT_EQ(lengthCall->GetArgs().GetString("max", ""), "{{ 3 }}");

    std::shared_ptr<FunctionCallInfo> emailCall = ResolveFunctionCallDescriptorForCoverage(
        "email", R"({"value":"{{ 'user@example.com' }}","message":"{{ 'raw message' }}"})", "boolean");
    ASSERT_NE(emailCall, nullptr);
    EXPECT_EQ(emailCall->GetArgs().GetString("value", ""), "user@example.com");
    EXPECT_EQ(emailCall->GetArgs().GetString("message", ""), "{{ 'raw message' }}");

    std::shared_ptr<FunctionCallInfo> formatStringCall = ResolveFunctionCallDescriptorForCoverage(
        "formatString", R"({"value":"{{ 'hello' + ' world' }}","locale":"{{ 'raw locale' }}"})", "string");
    ASSERT_NE(formatStringCall, nullptr);
    EXPECT_EQ(formatStringCall->GetArgs().GetString("value", ""), "hello world");
    EXPECT_EQ(formatStringCall->GetArgs().GetString("locale", ""), "{{ 'raw locale' }}");

    std::shared_ptr<FunctionCallInfo> notCall =
        ResolveFunctionCallDescriptorForCoverage("not", R"({"value":"{{ false }}","unused":"{{ true }}"})", "boolean");
    ASSERT_NE(notCall, nullptr);
    EXPECT_TRUE(notCall->GetArgs().GetItem("value").IsBool());
    EXPECT_FALSE(notCall->GetArgs().GetItem("value").GetBoolValue(true));
    EXPECT_EQ(notCall->GetArgs().GetString("unused", ""), "{{ true }}");

    std::shared_ptr<FunctionCallInfo> numberCall = ResolveFunctionCallDescriptorForCoverage("formatNumber",
        R"({"value":"{{ 42 }}","decimals":"{{ 3 }}","grouping":"{{ true }}","suffix":"{{ 'raw' }}"})", "string");
    ASSERT_NE(numberCall, nullptr);
    EXPECT_DOUBLE_EQ(numberCall->GetArgs().GetItem("value").GetNumberValue(0.0), 42.0);
    EXPECT_DOUBLE_EQ(numberCall->GetArgs().GetItem("decimals").GetNumberValue(0.0), 3.0);
    EXPECT_TRUE(numberCall->GetArgs().GetItem("grouping").GetBoolValue(false));
    EXPECT_EQ(numberCall->GetArgs().GetString("suffix", ""), "{{ 'raw' }}");

    std::shared_ptr<FunctionCallInfo> dateCall = ResolveFunctionCallDescriptorForCoverage("formatDate",
        R"({"value":"{{ '2026-06-26' }}","format":"{{ 'yyyy-MM-dd' }}","timeZone":"{{ 'raw/tz' }}"})", "string");
    ASSERT_NE(dateCall, nullptr);
    EXPECT_EQ(dateCall->GetArgs().GetString("value", ""), "2026-06-26");
    EXPECT_EQ(dateCall->GetArgs().GetString("format", ""), "yyyy-MM-dd");
    EXPECT_EQ(dateCall->GetArgs().GetString("timeZone", ""), "{{ 'raw/tz' }}");

    std::shared_ptr<FunctionCallInfo> pluralCall = ResolveFunctionCallDescriptorForCoverage("pluralize",
        R"({"zero":"{{ 'zero' }}","two":"{{ 'two' }}","few":"{{ 'few' }}","many":"{{ 'many' }}","style":"{{ 'raw' }}"})",
        "string");
    ASSERT_NE(pluralCall, nullptr);
    EXPECT_EQ(pluralCall->GetArgs().GetString("zero", ""), "zero");
    EXPECT_EQ(pluralCall->GetArgs().GetString("two", ""), "two");
    EXPECT_EQ(pluralCall->GetArgs().GetString("few", ""), "few");
    EXPECT_EQ(pluralCall->GetArgs().GetString("many", ""), "many");
    EXPECT_EQ(pluralCall->GetArgs().GetString("style", ""), "{{ 'raw' }}");

    std::shared_ptr<FunctionCallInfo> checkboxCall = ResolveFunctionCallDescriptorForCoverage(
        "getCheckboxGroupValues", R"({"group":"{{ 'permissions' }}","componentId":"{{ 'rawComponent' }}"})", "array");
    ASSERT_NE(checkboxCall, nullptr);
    EXPECT_EQ(checkboxCall->GetArgs().GetString("group", ""), "permissions");
    EXPECT_EQ(checkboxCall->GetArgs().GetString("componentId", ""), "{{ 'rawComponent' }}");

    std::shared_ptr<FunctionCallInfo> radioCall =
        ResolveFunctionCallDescriptorForCoverage("getRadioValue", R"({"group":"{{ 'theme' }}"})", "string");
    ASSERT_NE(radioCall, nullptr);
    EXPECT_EQ(radioCall->GetArgs().GetString("group", ""), "theme");

    std::shared_ptr<FunctionCallInfo> toggleCall = ResolveFunctionCallDescriptorForCoverage(
        "getToggleValue", R"({"componentId":"{{ 'toggleA' }}","group":"{{ 'rawGroup' }}"})", "boolean");
    ASSERT_NE(toggleCall, nullptr);
    EXPECT_EQ(toggleCall->GetArgs().GetString("componentId", ""), "toggleA");
    EXPECT_EQ(toggleCall->GetArgs().GetString("group", ""), "{{ 'rawGroup' }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_or_values_and_leave_extra_arg_literal)
{
    std::shared_ptr<FunctionCallInfo> functionCall = ResolveFunctionCallDescriptorForCoverage("or",
        R"({"values":["{{ false }}",{"nested":"{{ true }}","items":["{{ false }}"]}],"fallback":"{{ 'raw' }}"})",
        "boolean");
    ASSERT_NE(functionCall, nullptr);

    JsonValue args = functionCall->GetArgs();
    JsonValue values = args.GetItem("values");
    ASSERT_TRUE(values.IsArray());
    ASSERT_EQ(values.GetArraySize(), 2);
    EXPECT_FALSE(values.GetArrayItem(0).GetBoolValue(true));

    JsonValue objectItem = values.GetArrayItem(1);
    ASSERT_TRUE(objectItem.IsObject());
    EXPECT_TRUE(objectItem.GetItem("nested").GetBoolValue(false));
    JsonValue nestedItems = objectItem.GetItem("items");
    ASSERT_TRUE(nestedItems.IsArray());
    ASSERT_EQ(nestedItems.GetArraySize(), 1);
    EXPECT_FALSE(nestedItems.GetArrayItem(0).GetBoolValue(true));
    EXPECT_EQ(args.GetString("fallback", ""), "{{ 'raw' }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_keep_custom_expression_args_literal)
{
    std::shared_ptr<FunctionCallInfo> customCall = ResolveFunctionCallDescriptorForCoverage(
        "customAction", R"({"value":"{{ 'raw' }}","nested":{"enabled":"{{ true }}"}})", "void");
    ASSERT_NE(customCall, nullptr);
    EXPECT_EQ(customCall->GetArgs().GetString("value", ""), "{{ 'raw' }}");
    EXPECT_EQ(customCall->GetArgs().GetItem("nested").GetString("enabled", ""), "{{ true }}");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_nested_function_call_args)
{
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(37, "surface_nested_function_args", { "customAction", "nestedLocal" });

    mockNapiPtr_->nextValueId_ = 5800;
    napi_value invokeResult = PredictInvokeResultObject();
    SetInvokeResponse(invokeResult, true, true, true, NewManualString("nested result"));

    auto resolverInput = ParseJson(R"({
        "call":"customAction",
        "args":{"value":{"call":"nestedLocal","returnType":"string"}},
        "returnType":"void"
    })");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 37, .surfaceId = "surface_nested_function_args", .componentId = "probe_nested_function_args"
    };

    std::shared_ptr<FunctionCallInfo> functionCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(functionCall, nullptr);
    EXPECT_EQ(functionCall->GetArgs().GetString("value", ""), "nested result");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_path_args_for_client_function)
{
    SurfaceSlot& surfaceSlot = CreateSurface(35, "surface_client_path_args", { "formatName" });
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    auto initialData = ParseJson(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto resolverInput = ParseJson(R"({
        "call":"formatName",
        "args":{"name":{"path":"/user/name"}},
        "returnType":"string"
    })");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 35, .surfaceId = "surface_client_path_args", .componentId = "probe_client_path_args"
    };

    std::shared_ptr<FunctionCallInfo> firstCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(firstCall, nullptr);
    EXPECT_EQ(firstCall->GetArgs().GetString("name", ""), "Alice");

    auto updatedName = ParseJson(R"("Bob")");
    ASSERT_NE(updatedName, nullptr);
    dataModel->UpdateByPath("/user/name", updatedName->GetRoot());
    std::shared_ptr<FunctionCallInfo> secondCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
    ASSERT_NE(secondCall, nullptr);
    EXPECT_EQ(secondCall->GetArgs().GetString("name", ""), "Bob");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_reject_invalid_function_call_descriptors)
{
    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_invalid_policy", .componentId = "button_invalid_policy"
    };

    JsonValue invalidValue;
    EXPECT_EQ(DynamicValueResolver::ResolveFunctionCallDescriptor(invalidValue, context), nullptr);

    auto arrayDescriptor = ParseJson("[]");
    ASSERT_NE(arrayDescriptor, nullptr);
    EXPECT_EQ(DynamicValueResolver::ResolveFunctionCallDescriptor(arrayDescriptor->GetRoot(), context), nullptr);

    auto missingCallDescriptor = ParseJson(R"({"args":{"value":"{{ 'x' }}"}})");
    ASSERT_NE(missingCallDescriptor, nullptr);
    EXPECT_EQ(DynamicValueResolver::ResolveFunctionCallDescriptor(missingCallDescriptor->GetRoot(), context), nullptr);

    auto nonStringCallDescriptor = ParseJson(R"({"call":3,"args":{"value":"{{ 'x' }}"}})");
    ASSERT_NE(nonStringCallDescriptor, nullptr);
    EXPECT_EQ(
        DynamicValueResolver::ResolveFunctionCallDescriptor(nonStringCallDescriptor->GetRoot(), context), nullptr);

    auto emptyCallDescriptor = ParseJson(R"({"call":"","args":{"value":"{{ 'x' }}"}})");
    ASSERT_NE(emptyCallDescriptor, nullptr);
    EXPECT_EQ(DynamicValueResolver::ResolveFunctionCallDescriptor(emptyCallDescriptor->GetRoot(), context), nullptr);

    auto noArgsDescriptor = ParseJson(R"({"call":"formatString","returnType":"string"})");
    ASSERT_NE(noArgsDescriptor, nullptr);
    std::shared_ptr<FunctionCallInfo> noArgsCall =
        DynamicValueResolver::ResolveFunctionCallDescriptor(noArgsDescriptor->GetRoot(), context);
    ASSERT_NE(noArgsCall, nullptr);
    EXPECT_EQ(noArgsCall->GetFunctionName(), "formatString");
    EXPECT_FALSE(noArgsCall->GetArgs().IsValid());
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_extract_dependencies_from_allowed_policy_args)
{
    auto descriptor = ParseJson(R"({
        "call": "setAttributes",
        "args": {
            "componentId": "{{ $__targetId }}",
            "value": {
                "primary": "{{ $__dataModel.user.name + $__suffix }}",
                "secondary": "{{ $__dataModel.user.name }}",
                "globalAgain": "{{ $__targetId }}",
                "template": "${/legacy/template}"
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor->GetRoot());
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/legacy/template"));
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/user/name"));
    EXPECT_EQ(CountString(dependencies.dataPaths, "/user/name"), 1u);
    EXPECT_TRUE(ContainsString(dependencies.globalVariables, "__targetId"));
    EXPECT_TRUE(ContainsString(dependencies.globalVariables, "__suffix"));
    EXPECT_EQ(CountString(dependencies.globalVariables, "__targetId"), 1u);
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_ignore_disallowed_and_custom_arg_dependencies)
{
    auto descriptor = ParseJson(R"([
        {
            "call": "regex",
            "args": {
                "value": "{{ $__dataModel.allowed }}",
                "pattern": "{{ $__dataModel.ignoredPattern }}",
                "message": "{{ $__ignoredMessage }}"
            }
        },
        {
            "call": "customAction",
            "args": {
                "value": "{{ $__dataModel.customIgnored }}",
                "flag": "{{ $__customFlag }}"
            }
        },
        {
            "call": "openUrl",
            "args": {
                "url": "{{ $__dataModel.urlIgnored }}"
            }
        }
    ])");
    ASSERT_NE(descriptor, nullptr);

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor->GetRoot());
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/allowed"));
    EXPECT_FALSE(ContainsString(dependencies.dataPaths, "/ignoredPattern"));
    EXPECT_FALSE(ContainsString(dependencies.dataPaths, "/customIgnored"));
    EXPECT_FALSE(ContainsString(dependencies.dataPaths, "/urlIgnored"));
    EXPECT_FALSE(ContainsString(dependencies.globalVariables, "__ignoredMessage"));
    EXPECT_FALSE(ContainsString(dependencies.globalVariables, "__customFlag"));
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_extract_dependencies_from_boolean_array_policy)
{
    auto descriptor = ParseJson(R"({
        "call": "or",
        "args": {
            "values": [
                "{{ $__dataModel.left }}",
                {
                    "call": "formatString",
                    "args": {
                        "value": "{{ $__dataModel.nested + $__nestedGlobal }}"
                    }
                },
                {
                    "nested": "{{ $__dataModel.deep }}"
                }
            ],
            "unused": "{{ $__dataModel.unused }}"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor->GetRoot());
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/left"));
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/nested"));
    EXPECT_TRUE(ContainsString(dependencies.dataPaths, "/deep"));
    EXPECT_FALSE(ContainsString(dependencies.dataPaths, "/unused"));
    EXPECT_TRUE(ContainsString(dependencies.globalVariables, "__nestedGlobal"));
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_skip_invalid_dependency_descriptors)
{
    auto descriptor = ParseJson(R"([
        {"call": 3, "args": {"value": "{{ $__dataModel.nonStringCall }}"}},
        {"call": "", "args": {"value": "{{ $__dataModel.emptyCall }}"}},
        {"call": "formatString", "args": {"value": "{{ 1 + }}"}},
        {"call": "formatString", "args": {"value": 42}},
        "literal"
    ])");
    ASSERT_NE(descriptor, nullptr);

    DynamicValueDependencies dependencies = DynamicValueResolver::ExtractDependencies(descriptor->GetRoot());
    EXPECT_TRUE(dependencies.dataPaths.empty());
    EXPECT_TRUE(dependencies.globalVariables.empty());
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_cover_policy_arg_cjson_failure_edges)
{
    auto resolverInput = ParseJson(R"({
        "call": "setAttributes",
        "args": {
            "componentId": "{{ 'target' }}",
            "value": {
                "text": "{{ 'hello' }}",
                "items": ["{{ 'a' }}", "{{ 'b' }}"],
                "flags": { "enabled": "{{ true }}" }
            }
        },
        "returnType": "void"
    })");
    ASSERT_NE(resolverInput, nullptr);

    DynamicResolveContext context = {
        .renderId = -1, .surfaceId = "surface_policy_failures", .componentId = "button_policy_failures"
    };

    int32_t failedResolutions = 0;
    int32_t successfulResolutions = 0;
    for (int32_t failAfter = 0; failAfter <= 512; ++failAfter) {
        ScopedCjsonAllocFail fail(failAfter);
        std::shared_ptr<FunctionCallInfo> functionCall =
            DynamicValueResolver::ResolveFunctionCallDescriptor(resolverInput->GetRoot(), context);
        if (functionCall == nullptr) {
            ++failedResolutions;
            continue;
        }
        ++successfulResolutions;
        EXPECT_EQ(functionCall->GetArgs().GetString("componentId", ""), "target");
    }

    EXPECT_GT(failedResolutions, 0);
    EXPECT_GT(successfulResolutions, 0);
}
#endif

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_not_evaluate_expression_inside_path_binding)
{
    auto resolverInput = ParseJson(R"({"path": "{{ '/user/' + 'name' }}"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = -1,
        .surfaceId = "surface_expression_path",
        .componentId = "text_expression_path",
        .allowExpression = true };

    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::INVALID);
}
#endif

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_dispatch_runtime_error_when_render_id_zero)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(0, "surface_runtime_error_zero", { "noop" });

    auto resolverInput = ParseJson(R"({"path":"/missing"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 0, .surfaceId = "surface_runtime_error_zero", .componentId = "button_runtime_error_zero"
    };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.path, "/missing");

    int32_t renderId = -1;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 0);
    EXPECT_EQ(errorCode, SURFACE_ERROR_DYNAMIC_VALUE_RESOLVE_FAILED);
    EXPECT_NE(errorMessage.find("path not found"), std::string::npos);
    EXPECT_NE(errorMessage.find("/missing"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_runtime_error_when_expression_uses_unknown_global_variable)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(63, "surface_unknown_global", { "noop" });

    auto descriptor = ParseJson(R"({
        "id":"text_unknown_global",
        "content":"{{ 'Mode = ' + $__unknownMode }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(63);
    text.SetSurfaceId("surface_unknown_global");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());

    EXPECT_EQ(text.GetTextValueForTest(), "Mode = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 63);
    EXPECT_EQ(errorCode, 3204);
    EXPECT_NE(errorMessage.find("no global variables"), std::string::npos);
    EXPECT_NE(errorMessage.find("__unknownMode"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_runtime_error_once_when_registering_unknown_global_expression_binding)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(70, "surface_unknown_global_binding", { "noop" });
    auto modelRoot = ParseJson(R"({})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot.GetBindingEngine();
    ASSERT_NE(bindingEngine, nullptr);
    bindingEngine->ReplaceDataModel("surface_unknown_global_binding", modelRoot->GetRoot());

    auto descriptor = ParseJson(R"({
        "id":"text_unknown_global_binding",
        "content":"{{ 'Mode = ' + $__unknownMode }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    auto text = std::make_shared<ExtendedTextComponent>();
    text->SetRenderId(70);
    text->SetSurfaceId("surface_unknown_global_binding");

    mockNapiPtr_->objectProperties_.clear();
    text->ApplyDescriptor(descriptor->GetRoot());
    EXPECT_EQ(text->GetTextValueForTest(), "Mode = ");
    EXPECT_EQ(CountRuntimeErrorRequests(), 1u);

    bindingEngine->RegisterComponent(text);
    EXPECT_EQ(CountRuntimeErrorRequests(), 1u);
    EXPECT_EQ(text->GetTextValueForTest(), "Mode = ");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_defer_missing_expression_path_error_until_data_model_update)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(64, "surface_missing_expression_path", { "noop" });
    auto modelRoot = ParseJson(R"({"user":{"name":"alice"}})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(modelRoot->GetRoot());

    auto descriptor = ParseJson(R"({
        "id":"text_missing_expression_path",
        "content":"{{ 'User name = ' + $__dataModel.user.missing }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(64);
    text.SetSurfaceId("surface_missing_expression_path");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());

    EXPECT_EQ(text.GetTextValueForTest(), "User name = ");
    EXPECT_EQ(CountRuntimeErrorRequests(), 0u);

    auto update = ParseJson(R"({"value":{"user":{"name":"alice"}}})");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateDataModel(update->GetRoot()));
    text.ApplyDescriptor(descriptor->GetRoot());

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 64);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("path not found"), std::string::npos);
    EXPECT_NE(errorMessage.find("/user/missing"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_dispatch_illegal_expression_error_when_json_pointer_template_path_missing)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(67, "surface_missing_json_pointer", { "noop" });
    auto modelRoot = ParseJson(R"({"user":{"name":"alice"}})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(modelRoot->GetRoot());

    auto resolverInput = ParseJson(R"("User name = ${/user/missing}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 67,
        .surfaceId = "surface_missing_json_pointer",
        .componentId = "text_missing_json_pointer",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.value.GetStringValue(""), "User name = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 67);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("path not found"), std::string::npos);
    EXPECT_NE(errorMessage.find("/user/missing"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_dispatch_illegal_expression_runtime_error_when_expression_uses_invalid_data_model_key)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(65, "surface_illegal_expression", { "noop" });

    auto resolverInput = ParseJson(R"("{{ 'User name = ' + $__dataModel[name] }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 65,
        .surfaceId = "surface_illegal_expression",
        .componentId = "text_illegal_expression",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(resolved.value.GetStringValue("fallback"), "User name = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 65);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(errorMessage.find("__dataModel"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_dispatch_size_non_array_runtime_error_without_illegal_prefix)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(74, "surface_size_non_array", { "noop" });

    auto resolverInput = ParseJson(R"("{{ 'Size = ' + size(42) }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 74,
        .surfaceId = "surface_size_non_array",
        .componentId = "text_size_non_array",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.value.GetStringValue("fallback"), "Size = 0");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 74);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_EQ(errorMessage, "Built-in function size() expects an array argument");
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_keep_inner_illegal_error_when_size_argument_falls_back)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(75, "surface_size_inner_illegal", { "noop" });

    auto resolverInput = ParseJson(R"("{{ 'Size = ' + size(a) }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 75,
        .surfaceId = "surface_size_inner_illegal",
        .componentId = "text_size_inner_illegal",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.value.GetStringValue("fallback"), "Size = 0");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 75);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(errorMessage.find("unquoted string: a"), std::string::npos);
    EXPECT_EQ(errorMessage.find("Built-in function size() expects an array argument"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_runtime_error_when_expression_uses_invalid_global_member_access)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(71, "surface_invalid_global_member", { "noop" });

    auto descriptor = ParseJson(R"({
        "id":"text_invalid_global_member",
        "content":"{{ 'Invalid member access = ' + $__colorMode.value }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(71);
    text.SetSurfaceId("surface_invalid_global_member");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());

    EXPECT_EQ(text.GetTextValueForTest(), "Invalid member access = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 71);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_EQ(errorMessage, "member access not supported: .value");
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_illegal_expression_runtime_error_when_expression_parse_fails)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(66, "surface_illegal_expression_parse", { "noop" });

    auto descriptor = ParseJson(R"({
        "id":"text_illegal_expression_parse",
        "content":"{{ $__dataModel[].name }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(66);
    text.SetSurfaceId("surface_illegal_expression_parse");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 66);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(errorMessage.find("unexpected token"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_preserve_prefix_for_illegal_json_pointer_bracket_operand)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(73, "surface_illegal_json_pointer_bracket", { "noop" });

    auto descriptor = ParseJson(R"({
        "id":"text_illegal_json_pointer_bracket",
        "content":"{{ 'Wrong root = ' + $__dataModel.user[/user/name] }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(73);
    text.SetSurfaceId("surface_illegal_json_pointer_bracket");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());

    EXPECT_EQ(text.GetTextValueForTest(), "Wrong root = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 73);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(errorMessage.find("__dataModel"), std::string::npos);
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_dispatch_generic_runtime_error_when_expression_result_is_null)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(67, "surface_expression_null", { "noop" });
    auto modelRoot = ParseJson(R"({"user":{"alias":null}})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(modelRoot->GetRoot());

    auto resolverInput = ParseJson(R"("{{ $__dataModel.user.alias }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 67,
        .surfaceId = "surface_expression_null",
        .componentId = "text_expression_null",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(resolved.errorMessage, "expression evaluation failed");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 67);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_EQ(errorMessage, "expression evaluation failed");
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_not_dispatch_runtime_error_when_negative_render_id_expression_has_soft_error)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    auto resolverInput = ParseJson(R"("{{ 'Mode = ' + $__unknownMode }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = -1,
        .surfaceId = "surface_negative_expression",
        .componentId = "text_negative_expression",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(resolved.value.GetStringValue("fallback"), "Mode = ");
    EXPECT_NE(resolved.errorMessage.find("no global variables"), std::string::npos);
    EXPECT_NE(resolved.errorMessage.find("__unknownMode"), std::string::npos);

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

/**
 * @tc.name: FunctionBridgeActionCoverage_should_dispatch_runtime_error_when_expression_divides_by_zero
 * @tc.desc: Verify DynamicValueResolver dispatches hard expression runtime errors when evaluation returns an invalid
 * result.
 * @tc.type: FUNC
 */
TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_dispatch_runtime_error_when_expression_divides_by_zero)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(69, "surface_expression_div_zero", { "noop" });

    auto resolverInput = ParseJson(R"("{{ 1 / 0 }}")");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 69,
        .surfaceId = "surface_expression_div_zero",
        .componentId = "text_expression_div_zero",
        .allowExpression = true };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(resolved.errorMessage, "division by zero");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 69);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_EQ(errorMessage, "division by zero");
    EXPECT_EQ(source, "DynamicValueResolver");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_not_dispatch_component_runtime_error_without_positive_render_id)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    auto descriptor = ParseJson(R"({
        "id":"text_component_negative",
        "content":"{{ 'Mode = ' + $__unknownMode }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(-1);
    text.SetSurfaceId("surface_component_negative");
    text.SetComponentId("text_component_negative");

    mockNapiPtr_->objectProperties_.clear();
    text.ApplyDescriptor(descriptor->GetRoot());
    mockNapiPtr_->objectProperties_.clear();
    static_cast<Component&>(text).OnDataUpdate("content", JsonValue());

    EXPECT_EQ(text.GetTextValueForTest(), "Mode = ");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_component_runtime_error_when_expression_refresh_becomes_invalid)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(68, "surface_component_refresh", { "noop" });
    auto descriptor = ParseJson(R"({
        "id":"text_component_refresh",
        "content":"{{ $__colorMode == 'light' ? 'a' : 1 / 0 }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    auto text = std::make_shared<ExtendedTextComponent>();
    text->SetRenderId(68);
    text->SetSurfaceId("surface_component_refresh");
    text->SetComponentId("text_component_refresh");

    mockNapiPtr_->objectProperties_.clear();
    text->ApplyDescriptor(descriptor->GetRoot());
    EXPECT_EQ(text->GetTextValueForTest(), "a");

    auto bindingEngine = surfaceSlot.GetBindingEngine();
    ASSERT_NE(bindingEngine, nullptr);
    bindingEngine->RegisterComponent(text);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(68);
    ASSERT_NE(renderSlot, nullptr);
    mockNapiPtr_->objectProperties_.clear();
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(text->GetTextValueForTest(), "a");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 68);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_NE(errorMessage.find("division by zero"), std::string::npos);
    EXPECT_EQ(source, "Component");
}

TEST_F(FunctionBridgeActionCoverageTest,
    extended_text_component_should_dispatch_component_runtime_error_when_invalid_global_member_access_refreshes)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(72, "surface_invalid_global_member_refresh", { "noop" });
    auto descriptor = ParseJson(R"({
        "id":"text_invalid_global_member_refresh",
        "content":"{{ 'Invalid member access = ' + $__colorMode.value }}"
    })");
    ASSERT_NE(descriptor, nullptr);

    ExtendedTextComponent text;
    text.SetRenderId(72);
    text.SetSurfaceId("surface_invalid_global_member_refresh");
    text.SetComponentId("text_invalid_global_member_refresh");

    text.ApplyDescriptor(descriptor->GetRoot());

    mockNapiPtr_->objectProperties_.clear();
    static_cast<Component&>(text).OnDataUpdate("content", JsonValue());

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 72);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ILLEGAL_EXPRESSION);
    EXPECT_EQ(errorMessage, "member access not supported: .value");
    EXPECT_EQ(source, "Component");
}
#endif

TEST_F(FunctionBridgeActionCoverageTest, button_component_should_pass_action_parse_context_to_action_parser)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());

    ButtonComponent button;
    button.SetRenderId(26);
    button.SetSurfaceId("surface_button_runtime_error");
    button.SetComponentId("button_runtime_error");

    std::string descriptorJson = std::string("{\"id\":\"button_runtime_error\",\"component\":\"Button\",\"action\":{"
                                             "\"event\":{\"name\":\"tap\",\"context\":") +
                                 BuildNestedContextJson(24) + "}}}";
    auto descriptor = ParseJson(descriptorJson);
    ASSERT_NE(descriptor, nullptr);

    mockNapiPtr_->objectProperties_.clear();
    button.ApplyDescriptor(descriptor->GetRoot());

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 26);
    EXPECT_EQ(errorCode, SURFACE_ERROR_ACTION_PARSE_FAILED);
    EXPECT_NE(errorMessage.find("depth exceeds"), std::string::npos);
    EXPECT_EQ(source, "ActionParser");
}

TEST_F(FunctionBridgeActionCoverageTest, action_parser_should_not_dispatch_runtime_error_when_render_id_is_non_positive)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    std::string deepContextJson =
        std::string("{\"action\":{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}}";
    auto deepContextAdapter = ParseJson(deepContextJson);
    ASSERT_NE(deepContextAdapter, nullptr);

    ActionParseContext parseContext = { .renderId = 0, .surfaceId = "surface_zero", .componentId = "button_zero" };
    mockNapiPtr_->objectProperties_.clear();
    EXPECT_EQ(ActionParser::Parse(deepContextAdapter->GetRoot(), parseContext), nullptr);

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    action_parser_should_not_dispatch_runtime_error_when_context_clone_fails_even_with_positive_render_id)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    auto contextAdapter = ParseJson(R"({"action":{"event":{"name":"tap","context":{"a":1}}}})");
    ASSERT_NE(contextAdapter, nullptr);

    ActionParseContext parseContext = {
        .renderId = 37, .surfaceId = "surface_clone_fail", .componentId = "button_clone_fail"
    };
    mockNapiPtr_->objectProperties_.clear();
    {
        ScopedCjsonAllocFail fail(0);
        EXPECT_EQ(ActionParser::Parse(contextAdapter->GetRoot(), parseContext), nullptr);
    }

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    runtime_error_dispatch_bridge_should_register_when_previous_env_is_null_but_ref_exists)
{
    auto& bridge = RuntimeErrorDispatchBridge::GetInstance();
    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());

    struct RuntimeErrorBridgeLayout {
        napi_env napiEnv;
        napi_ref dispatchRef;
    };

    auto* layout = reinterpret_cast<RuntimeErrorBridgeLayout*>(&bridge);
    ASSERT_NE(layout->dispatchRef, nullptr);
    size_t oldRefCount = mockNapiPtr_->refToValue_.size();
    layout->napiEnv = nullptr;

    bridge.RegisterDispatchRuntimeError(env_, CreateCallback());
    EXPECT_GT(mockNapiPtr_->refToValue_.size(), oldRefCount);

    mockNapiPtr_->objectProperties_.clear();
    EXPECT_TRUE(bridge.Dispatch(3, "surface_new", "component_new", 3003, "msg", "unit"));
}

TEST_F(FunctionBridgeActionCoverageTest, native_entry_register_dispatch_runtime_error_should_handle_get_cb_info_failure)
{
    mockNapiPtr_->SetGetCbInfoStatus(napi_invalid_arg);
    mockNapiPtr_->SetCallbackArgs({ CreateCallback() });
    EXPECT_EQ(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_FALSE(RuntimeErrorDispatchBridge::GetInstance().Dispatch(2, "surface", "component", 1, "m", "unit"));
    mockNapiPtr_->ResetGetCbInfoStatus();
}

TEST_F(FunctionBridgeActionCoverageTest, native_entry_register_dispatch_runtime_error_should_cover_napi_exception_edges)
{
    mockNapiPtr_->SetCallbackArgs({ CreateCallback() });
    mockNapiPtr_->SetThrowOnMethod("GetCbInfo");
    EXPECT_THROW(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();

    mockNapiPtr_->SetCallbackArgs({ CreateCallback() });
    mockNapiPtr_->SetThrowOnMethod("Typeof");
    EXPECT_THROW(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();

    mockNapiPtr_->SetCallbackArgs({ CreateCallback() });
    mockNapiPtr_->SetThrowOnMethod("CreateReference");
    EXPECT_THROW(NativeModule::RegisterDispatchRuntimeError(env_, cbInfo_), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_resolve_existing_path_and_skip_runtime_error_dispatch)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(30, "surface_path_success", { "noop" });
    auto modelRoot = ParseJson(R"({"user":{"name":"alice"}})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(modelRoot->GetRoot());

    auto resolverInput = ParseJson(R"({"path":"/user/name"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 30, .surfaceId = "surface_path_success", .componentId = "button_path_success"
    };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::PATH);
    EXPECT_EQ(resolved.path, "/user/name");
    EXPECT_EQ(resolved.value.GetStringValue(""), "alice");

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    action_parser_should_propagate_runtime_error_dispatch_exception_when_context_depth_exceeds_limit)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    std::string deepContextJson =
        std::string("{\"action\":{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}}";
    auto deepContextAdapter = ParseJson(deepContextJson);
    ASSERT_NE(deepContextAdapter, nullptr);

    ActionParseContext parseContext = {
        .renderId = 41, .surfaceId = "surface_action_throw", .componentId = "button_action_throw"
    };

    mockNapiPtr_->SetThrowOnMethod("GetReferenceValue");
    EXPECT_THROW(ActionParser::Parse(deepContextAdapter->GetRoot(), parseContext), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_not_dispatch_runtime_error_when_data_model_is_unavailable)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    auto resolverInput = ParseJson(R"({"path":"/missing"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 99, .surfaceId = "surface_missing", .componentId = "button_missing" };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.path, "/missing");
    EXPECT_NE(resolved.errorMessage.find("data model unavailable"), std::string::npos);

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_propagate_runtime_error_dispatch_exception_when_path_missing)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(42, "surface_dynamic_throw", { "noop" });

    auto resolverInput = ParseJson(R"({"path":"/missing"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 42, .surfaceId = "surface_dynamic_throw", .componentId = "button_dynamic_throw"
    };

    mockNapiPtr_->SetThrowOnMethod("GetReferenceValue");
    EXPECT_THROW(DynamicValueResolver::Resolve(resolverInput->GetRoot(), context), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest,
    dynamic_value_resolver_should_not_dispatch_runtime_error_when_path_descriptor_is_invalid)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    auto resolverInput = ParseJson(R"({"path":"bad/path/no_prefix"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 40, .surfaceId = "surface_invalid_path", .componentId = "button_invalid_path"
    };

    mockNapiPtr_->objectProperties_.clear();
    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::INVALID);
    EXPECT_NE(resolved.errorMessage.find("invalid path"), std::string::npos);

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(
    FunctionBridgeActionCoverageTest, button_component_should_propagate_action_parser_runtime_error_dispatch_exception)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());

    ButtonComponent button;
    button.SetRenderId(43);
    button.SetSurfaceId("surface_button_throw");
    button.SetComponentId("button_throw");

    std::string descriptorJson =
        std::string(
            "{\"id\":\"button_throw\",\"component\":\"Button\",\"action\":{\"event\":{\"name\":\"tap\",\"context\":") +
        BuildNestedContextJson(24) + "}}}";
    auto descriptor = ParseJson(descriptorJson);
    ASSERT_NE(descriptor, nullptr);

    mockNapiPtr_->SetThrowOnMethod("GetReferenceValue");
    EXPECT_THROW(button.ApplyDescriptor(descriptor->GetRoot()), std::runtime_error);
    mockNapiPtr_->ResetThrowOnMethod();
}

TEST_F(FunctionBridgeActionCoverageTest, action_parser_should_cover_std_allocation_failure_edges)
{
#if !A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
    GTEST_SKIP() << "std allocation failure hook is disabled on MSVC";
#else
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    std::string longName(160, 'x');
    auto functionDescriptor = ParseJson(std::string(R"({"action":{"functionCall":{"call":")") + longName +
                                        R"(","args":{"value":")" + longName + R"("},"returnType":"string"}}})");
    ASSERT_NE(functionDescriptor, nullptr);

    int32_t functionFailures = ExerciseStdAllocationFailures(
        [&functionDescriptor]() { (void)ActionParser::Parse(functionDescriptor->GetRoot()); }, 2048);

    std::string deepContextJson =
        std::string("{\"action\":{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}}";
    auto eventDescriptor = ParseJson(deepContextJson);
    ASSERT_NE(eventDescriptor, nullptr);
    ActionParseContext parseContext = { .renderId = 44,
        .surfaceId = std::string("surface_action_alloc_") + longName,
        .componentId = std::string("button_action_alloc_") + longName };

    int32_t eventFailures = ExerciseStdAllocationFailures(
        [&eventDescriptor, &parseContext]() { (void)ActionParser::Parse(eventDescriptor->GetRoot(), parseContext); },
        2048);

    EXPECT_GT(functionFailures + eventFailures, 0);
#endif
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_cover_std_allocation_failure_edges)
{
#if !A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
    GTEST_SKIP() << "std allocation failure hook is disabled on MSVC";
#else
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(45, "surface_dynamic_alloc", { "noop" });
    std::string longPathSegment(160, 'm');
    std::string path = std::string("/") + longPathSegment;
    auto resolverInput = ParseJson(std::string(R"({"path":")") + path + R"("})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = { .renderId = 45,
        .surfaceId = std::string("surface_dynamic_alloc_") + longPathSegment,
        .componentId = std::string("button_dynamic_alloc_") + longPathSegment };

    int32_t failures = ExerciseStdAllocationFailures(
        [&resolverInput, &context]() { (void)DynamicValueResolver::Resolve(resolverInput->GetRoot(), context); }, 160);

    EXPECT_GT(failures, 0);
#endif
}

TEST_F(FunctionBridgeActionCoverageTest, button_component_should_cover_std_allocation_failure_edges)
{
#if !A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
    GTEST_SKIP() << "std allocation failure hook is disabled on MSVC";
#else
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    std::string longId(160, 'b');
    ExposedButtonComponent button;
    button.SetRenderId(46);
    button.SetSurfaceId(std::string("surface_button_alloc_") + longId);
    button.SetComponentId(std::string("button_alloc_") + longId);

    std::string actionJson =
        std::string("{\"event\":{\"name\":\"tap\",\"context\":") + BuildNestedContextJson(24) + "}}";
    auto actionValue = ParseJson(actionJson);
    ASSERT_NE(actionValue, nullptr);

    int32_t failures = ExerciseStdAllocationFailures(
        [&button, &actionValue]() { (void)button.HandleSpecialProperty("action", actionValue->GetRoot()); }, 160);

    EXPECT_GT(failures, 0);
#endif
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_fail_when_builtin_normalize_fails)
{
    SurfaceSlot& surfaceSlot = CreateSurface(31, "surface_builtin_fail", { "required" });
    (void)surfaceSlot;
    auto resolverInput = ParseJson(R"({"call":"required","args":{"value":"abc"}})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 31, .surfaceId = "surface_builtin_fail", .componentId = "button_builtin_fail"
    };

    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::FUNCTION_CALL);
    EXPECT_EQ(resolved.functionName, "required");
    EXPECT_NE(resolved.errorMessage.find("normalize failed"), std::string::npos);
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_fail_when_builtin_return_type_mismatch)
{
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(32, "surface_builtin_mismatch", { "required" });
    (void)surfaceSlot;

    mockNapiPtr_->nextValueId_ = 5600;
    napi_value normalizeResult = PredictInvokeResultObject(true);
    SetInvokeResponse(normalizeResult, true, true);
    SetObjectProperty(normalizeResult, "normalizedReturnType", NewManualString("string"));

    auto resolverInput = ParseJson(R"({"call":"required"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 32, .surfaceId = "surface_builtin_mismatch", .componentId = "button_builtin_mismatch"
    };

    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::FUNCTION_CALL);
    EXPECT_EQ(resolved.functionName, "required");
    EXPECT_EQ(resolved.errorMessage, "builtin returnType mismatch");
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_resolve_local_function_call_successfully)
{
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(33, "surface_local_success", { "customLocal" });
    (void)surfaceSlot;

    mockNapiPtr_->nextValueId_ = 5700;
    napi_value invokeResult = PredictInvokeResultObject();
    SetInvokeResponse(invokeResult, true, true, true, NewManualNumber(8.0));

    auto resolverInput = ParseJson(R"({"call":"customLocal"})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 33, .surfaceId = "surface_local_success", .componentId = "button_local_success"
    };

    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_TRUE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::FUNCTION_CALL);
    EXPECT_EQ(resolved.functionName, "customLocal");
    EXPECT_DOUBLE_EQ(resolved.value.GetNumberValue(0.0), 8.0);
}

TEST_F(FunctionBridgeActionCoverageTest, dynamic_value_resolver_should_fail_when_local_function_invoke_fails)
{
    SurfaceSlot& surfaceSlot = CreateSurface(34, "surface_local_fail", { "customLocalFail" });
    (void)surfaceSlot;
    auto resolverInput = ParseJson(R"({"call":"customLocalFail","args":{"value":"x"}})");
    ASSERT_NE(resolverInput, nullptr);
    DynamicResolveContext context = {
        .renderId = 34, .surfaceId = "surface_local_fail", .componentId = "button_local_fail"
    };

    ResolvedValue resolved = DynamicValueResolver::Resolve(resolverInput->GetRoot(), context);
    EXPECT_FALSE(resolved.success);
    EXPECT_EQ(resolved.source, ResolveSource::FUNCTION_CALL);
    EXPECT_EQ(resolved.functionName, "customLocalFail");
    EXPECT_EQ(resolved.errorMessage, "local function invoke failed");
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_report_local_function_error_when_not_in_catalog)
{
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());
    CreateSurface(36, "surface_unknown_local_function", {});
    auto functionCall = CreateFunctionCall("unknownLocalFunction", "string");

    mockNapiPtr_->objectProperties_.clear();
    JsonValue returnValue;
    EXPECT_FALSE(FunctionBridge::GetInstance().InvokeForValue(
        36, "surface_unknown_local_function", "probe_unknown_local_function", functionCall, returnValue));

    int32_t renderId = -1;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_TRUE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
    EXPECT_EQ(renderId, 36);
    EXPECT_EQ(errorCode, SURFACE_ERROR_LOCAL_FUNCTION);
    EXPECT_NE(errorMessage.find("unknownLocalFunction"), std::string::npos);
    EXPECT_EQ(source, "FunctionBridge");
}

TEST_F(FunctionBridgeActionCoverageTest, button_component_should_not_dispatch_runtime_error_without_positive_render_id)
{
    RuntimeErrorDispatchBridge::GetInstance().RegisterDispatchRuntimeError(env_, CreateCallback());

    ButtonComponent button;
    button.SetSurfaceId("surface_button_runtime_error_zero");
    button.SetComponentId("button_runtime_error_zero");

    std::string descriptorJson =
        std::string(
            "{\"id\":\"button_runtime_error_zero\",\"component\":\"Button\",\"action\":{\"event\":{\"name\":\"tap\","
            "\"context\":") +
        BuildNestedContextJson(24) + "}}}";
    auto descriptor = ParseJson(descriptorJson);
    ASSERT_NE(descriptor, nullptr);

    mockNapiPtr_->objectProperties_.clear();
    button.ApplyDescriptor(descriptor->GetRoot());

    int32_t renderId = 0;
    int32_t errorCode = 0;
    std::string errorMessage;
    std::string source;
    EXPECT_FALSE(FindRuntimeErrorRequest(&renderId, &errorCode, &errorMessage, &source));
}

TEST_F(FunctionBridgeActionCoverageTest,
    button_component_handle_special_property_should_cover_checks_and_unknown_property_branches)
{
    ExposedButtonComponent button;
    auto checksValue = ParseJson(R"({"required":[{"path":"/user/name"}]})");
    ASSERT_NE(checksValue, nullptr);
    EXPECT_TRUE(button.HandleSpecialProperty("checks", checksValue->GetRoot()));

    auto rawValue = ParseJson(R"({"x":1})");
    ASSERT_NE(rawValue, nullptr);
    EXPECT_FALSE(button.HandleSpecialProperty("notSpecial", rawValue->GetRoot()));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_invalid_register_inputs)
{
    auto& bridge = FunctionBridge::GetInstance();
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed");

    bridge.RegisterInvokeLocalFunction(nullptr, nullptr);
    EXPECT_FALSE(bridge.Invoke("surface", "component", call));

    bridge.RegisterInvokeLocalFunction(env_, nullptr);
    EXPECT_FALSE(bridge.Invoke("surface", "component", call));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_registration_and_invoke_early_failures)
{
    auto& bridge = FunctionBridge::GetInstance();
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed");

    EXPECT_FALSE(bridge.Invoke("surface", "component", call));

    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();
    EXPECT_FALSE(bridge.Invoke("surface", "component", call));

    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    ASSERT_FALSE(mockNapiPtr_->refToValue_.empty());
    napi_ref oldRef = mockNapiPtr_->refToValue_.begin()->first;
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    EXPECT_EQ(mockNapiPtr_->refToValue_.count(oldRef), 0u);

    EXPECT_FALSE(bridge.Invoke("missing", "component", call));

    CreateSurface(1, "surface_no_catalog", {}, false);
    EXPECT_FALSE(bridge.Invoke("surface_no_catalog", "component", call));

    CreateSurface(2, "surface_not_allowed", { "other" });
    EXPECT_FALSE(bridge.Invoke("surface_not_allowed", "component", call));

    CreateSurface(3, "surface_allowed", { "allowed" });
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", nullptr));

    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));
    mockNapiPtr_->ResetGetReferenceValueStatus();

    mockNapiPtr_->refToValue_.clear();
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));

    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    mockNapiPtr_->SetCreateObjectStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));
    mockNapiPtr_->ResetCreateObjectStatus();

    mockNapiPtr_->nextValueId_ = 0;
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));

    mockNapiPtr_->nextValueId_ = 200;
    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));
    mockNapiPtr_->ResetCallFunctionStatus();

    mockNapiPtr_->nextValueId_ = -7;
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));

    mockNapiPtr_->nextValueId_ = 300;
    napi_value missingSuccessResult = PredictInvokeResultObject();
    mockNapiPtr_->valueTypes_[missingSuccessResult] = napi_object;
    mockNapiPtr_->objectProperties_[missingSuccessResult] = {};
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));

    mockNapiPtr_->nextValueId_ = 400;
    napi_value failedResult = PredictInvokeResultObject();
    SetInvokeResponse(failedResult, true, false);
    SetObjectProperty(failedResult, "errorCode", NewManualString("ERR"));
    SetObjectProperty(failedResult, "errorMessage", NewManualString("message"));
    EXPECT_FALSE(bridge.Invoke("surface_allowed", "component", call));

    mockNapiPtr_->nextValueId_ = 500;
    napi_value successResult = PredictInvokeResultObject();
    SetInvokeResponse(successResult, true, true);
    EXPECT_TRUE(bridge.Invoke("surface_allowed", "component", call));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_convert_return_values_for_primitives_and_absent_value)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(11, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    mockNapiPtr_->nextValueId_ = 600;
    napi_value noValueResult = PredictInvokeResultObject();
    SetInvokeResponse(noValueResult, true, true);
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsNull());

    mockNapiPtr_->nextValueId_ = 700;
    napi_value undefinedResult = PredictInvokeResultObject();
    SetInvokeResponse(undefinedResult, true, true, true, NewManualTypedValue(napi_undefined));
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsNull());

    mockNapiPtr_->nextValueId_ = 800;
    napi_value nullResult = PredictInvokeResultObject();
    SetInvokeResponse(nullResult, true, true, true, NewManualTypedValue(napi_null));
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsNull());

    mockNapiPtr_->nextValueId_ = 900;
    napi_value stringResult = PredictInvokeResultObject();
    SetInvokeResponse(stringResult, true, true, true, NewManualString("hello"));
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_EQ(returnValue.GetStringValue(""), "hello");

    mockNapiPtr_->nextValueId_ = 1000;
    napi_value numberResult = PredictInvokeResultObject();
    SetInvokeResponse(numberResult, true, true, true, NewManualNumber(12.5));
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_DOUBLE_EQ(returnValue.GetNumberValue(0.0), 12.5);

    mockNapiPtr_->nextValueId_ = 1100;
    napi_value boolResult = PredictInvokeResultObject();
    SetInvokeResponse(boolResult, true, true, true, NewManualBool(true));
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.GetBoolValue(false));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_return_value_conversion_failures)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(12, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    mockNapiPtr_->nextValueId_ = 1200;
    napi_value nullValueResult = PredictInvokeResultObject();
    SetInvokeResponse(nullValueResult, true, true, true, nullptr);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));

    mockNapiPtr_->nextValueId_ = 1300;
    napi_value typeofFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(typeofFailureResult, true, true, true, NewManualString("x"));
    mockNapiPtr_->SetTypeofStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetTypeofStatus();

    mockNapiPtr_->nextValueId_ = 1400;
    napi_value numberGetFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(numberGetFailureResult, true, true, true, NewManualNumber(1.0));
    mockNapiPtr_->SetGetValueDoubleStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetValueDoubleStatus();

    mockNapiPtr_->nextValueId_ = 1500;
    napi_value boolGetFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(boolGetFailureResult, true, true, true, NewManualBool(true));
    mockNapiPtr_->SetGetValueBoolFailOnCall(2, napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetValueBoolFailOnCall();

    mockNapiPtr_->nextValueId_ = 1600;
    napi_value unsupportedTypeResult = PredictInvokeResultObject();
    SetInvokeResponse(unsupportedTypeResult, true, true, true, NewManualTypedValue(napi_function));
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_array_conversion_branches)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(13, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    mockNapiPtr_->nextValueId_ = 1700;
    napi_value successArrayResult = PredictInvokeResultObject();
    napi_value arrayValue = NewManualArray({ NewManualString("v"), NewManualNumber(2.0) });
    SetInvokeResponse(successArrayResult, true, true, true, arrayValue);
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsArray());
    EXPECT_EQ(returnValue.GetArraySize(), 2);

    mockNapiPtr_->nextValueId_ = 1800;
    napi_value arrayLengthFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(arrayLengthFailureResult, true, true, true, NewManualArray({ NewManualString("x") }));
    mockNapiPtr_->SetGetArrayLengthStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetArrayLengthStatus();

    mockNapiPtr_->nextValueId_ = 1900;
    napi_value arrayElementFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(arrayElementFailureResult, true, true, true, NewManualArray({ NewManualString("x") }));
    mockNapiPtr_->SetGetElementStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetElementStatus();

    mockNapiPtr_->nextValueId_ = 2000;
    napi_value arrayInvalidChildResult = PredictInvokeResultObject();
    SetInvokeResponse(
        arrayInvalidChildResult, true, true, true, NewManualArray({ NewManualTypedValue(napi_function) }));
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));

    mockNapiPtr_->nextValueId_ = 2100;
    napi_value recursiveArray = NewManualObject();
    mockNapiPtr_->isArrayFlags_[recursiveArray] = true;
    mockNapiPtr_->arrayLengths_[recursiveArray] = 1;
    mockNapiPtr_->arrayElements_[recursiveArray][0] = recursiveArray;
    napi_value deepArrayResult = PredictInvokeResultObject();
    SetInvokeResponse(deepArrayResult, true, true, true, recursiveArray);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));

    mockNapiPtr_->nextValueId_ = 2150;
    napi_value appendFailureResult = PredictInvokeResultObject();
    SetInvokeResponse(appendFailureResult, true, true, true, NewManualArray({ NewManualNumber(1.0) }));
    {
        ScopedCjsonAllocFail fail(2);
        EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    }
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_object_conversion_branches)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(14, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    mockNapiPtr_->nextValueId_ = 2200;
    napi_value isArrayStatusFailureResult = PredictInvokeResultObject();
    napi_value objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(isArrayStatusFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetIsArrayStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetIsArrayStatus();

    mockNapiPtr_->nextValueId_ = 2300;
    napi_value propertyNamesFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(propertyNamesFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetPropertyNamesStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetPropertyNamesStatus();

    mockNapiPtr_->nextValueId_ = 2400;
    napi_value objectLengthFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectLengthFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetArrayLengthStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetArrayLengthStatus();

    mockNapiPtr_->nextValueId_ = 2450;
    napi_value objectPropertyNamesNullResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectPropertyNamesNullResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetPropertyNamesReturnNullOnce();
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetPropertyNamesReturnNullOnce();

    mockNapiPtr_->nextValueId_ = 2500;
    napi_value objectGetElementFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectGetElementFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetElementStatus(napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetElementStatus();

    mockNapiPtr_->nextValueId_ = 2550;
    napi_value objectNullKeyResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectNullKeyResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetElementReturnNullOnCall(1);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetElementReturnNullOnCall();

    mockNapiPtr_->nextValueId_ = 2600;
    napi_value objectGetPropertyFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectGetPropertyFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetGetNamedPropertyFailOnCall(3, napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetGetNamedPropertyFailOnCall();

    mockNapiPtr_->nextValueId_ = 2700;
    napi_value objectChildTypeFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetInvokeResponse(objectChildTypeFailureResult, true, true, true, objectValue);
    mockNapiPtr_->SetTypeofFailOnCall(2, napi_invalid_arg);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    mockNapiPtr_->ResetTypeofFailOnCall();

    mockNapiPtr_->nextValueId_ = 2800;
    napi_value objectSkipTypeResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetObjectProperty(objectValue, "fn", NewManualTypedValue(napi_function));
    SetObjectProperty(objectValue, "sym", NewManualTypedValue(napi_symbol));
    SetObjectProperty(objectValue, "ext", NewManualTypedValue(napi_external));
    SetInvokeResponse(objectSkipTypeResult, true, true, true, objectValue);
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsObject());
    EXPECT_TRUE(returnValue.Has("name"));
    EXPECT_FALSE(returnValue.Has("fn"));
    EXPECT_FALSE(returnValue.Has("sym"));
    EXPECT_FALSE(returnValue.Has("ext"));

    mockNapiPtr_->nextValueId_ = 2900;
    napi_value objectNestedInvalidResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    napi_value nestedObject = NewManualObject();
    napi_value nestedArray = NewManualArray({ NewManualTypedValue(napi_function) });
    SetObjectProperty(nestedObject, "items", nestedArray);
    SetObjectProperty(objectValue, "nested", nestedObject);
    SetInvokeResponse(objectNestedInvalidResult, true, true, true, objectValue);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));

    mockNapiPtr_->nextValueId_ = 3000;
    napi_value objectSuccessResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "name", NewManualString("A2UI"));
    SetObjectProperty(objectValue, "version", NewManualNumber(1.0));
    SetInvokeResponse(objectSuccessResult, true, true, true, objectValue);
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_TRUE(returnValue.IsObject());
    EXPECT_EQ(returnValue.GetString("name", ""), "A2UI");
    EXPECT_DOUBLE_EQ(returnValue.GetNumber("version", 0.0), 1.0);

    mockNapiPtr_->nextValueId_ = 3050;
    napi_value objectPutFailureResult = PredictInvokeResultObject();
    objectValue = NewManualObject();
    SetObjectProperty(objectValue, "value", NewManualNumber(1.0));
    SetInvokeResponse(objectPutFailureResult, true, true, true, objectValue);
    {
        ScopedCjsonAllocFail fail(2);
        EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    }
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_handle_json_create_failures_in_value_conversion)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(15, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    struct CaseInfo {
        napi_value value;
        int32_t nextValueId;
    };

    std::vector<CaseInfo> cases = { { NewManualTypedValue(napi_null), 3100 }, { NewManualString("hello"), 3200 },
        { NewManualNumber(1.0), 3300 }, { NewManualBool(true), 3400 },
        { NewManualArray({ NewManualNumber(1.0) }), 3500 }, { NewManualObject(), 3600 } };

    for (const CaseInfo& item : cases) {
        mockNapiPtr_->nextValueId_ = item.nextValueId;
        napi_value resultObject = PredictInvokeResultObject();
        SetInvokeResponse(resultObject, true, true, true, item.value);
        ScopedCjsonAllocFail fail(0);
        EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
    }

    mockNapiPtr_->nextValueId_ = 3700;
    napi_value noValueResult = PredictInvokeResultObject();
    SetInvokeResponse(noValueResult, true, true, false);
    ScopedCjsonAllocFail fail(0);
    EXPECT_TRUE(bridge.InvokeForValue("surface", "component", call, returnValue));
    EXPECT_FALSE(returnValue.IsValid());
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_fail_when_clone_json_value_fails)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(16, "surface", { "allowed" });
    std::shared_ptr<FunctionCallInfo> call = CreateFunctionCall("allowed", "json");
    JsonValue returnValue;

    mockNapiPtr_->nextValueId_ = 3800;
    napi_value resultObject = PredictInvokeResultObject();
    SetInvokeResponse(resultObject, true, true, true, NewManualNumber(9.0));
    ScopedCjsonAllocFail fail(1);
    EXPECT_FALSE(bridge.InvokeForValue("surface", "component", call, returnValue));
}

TEST(FunctionResultCoverageTest, should_cover_move_constructor_and_get_type_for_all_types)
{
    EXPECT_EQ(FunctionResult().GetType(), FunctionResultType::NULL_VALUE);
    EXPECT_EQ(FunctionResult(true).GetType(), FunctionResultType::BOOL);
    EXPECT_EQ(FunctionResult(42).GetType(), FunctionResultType::INT);
    EXPECT_EQ(FunctionResult(3.14).GetType(), FunctionResultType::DOUBLE);
    EXPECT_EQ(FunctionResult(std::string("x")).GetType(), FunctionResultType::STRING);

    std::string movable = "moved_value";
    FunctionResult movedResult(std::move(movable));
    EXPECT_TRUE(movedResult.IsString());
    EXPECT_EQ(movedResult.GetStringValue(""), "moved_value");
}

TEST(FunctionResultCoverageTest, should_verify_to_json_value_content)
{
    FunctionResult boolFalse(false);
    EXPECT_FALSE(boolFalse.ToJsonValue().GetBoolValue(true));

    FunctionResult intResult(42);
    EXPECT_DOUBLE_EQ(intResult.ToJsonValue().GetNumberValue(0.0), 42.0);

    FunctionResult doubleResult(3.14);
    EXPECT_DOUBLE_EQ(doubleResult.ToJsonValue().GetNumberValue(0.0), 3.14);

    FunctionResult stringResult(std::string("hello"));
    EXPECT_EQ(stringResult.ToJsonValue().GetStringValue(""), "hello");
}

TEST(FunctionResultCoverageTest, should_cover_negative_and_special_double_values)
{
    EXPECT_EQ(FunctionResult(-1.0).ToJsonLiteral(), "-1");
    EXPECT_EQ(FunctionResult(-3.5).ToJsonLiteral(), "-3.5");
    EXPECT_EQ(FunctionResult(-std::numeric_limits<double>::infinity()).ToJsonLiteral(), "-inf");
    EXPECT_EQ(FunctionResult(-42).ToString(), "-42");
    EXPECT_EQ(FunctionResult(-2.5).ToString(), "-2.5");
}

TEST(FunctionResultCoverageTest, should_cover_simple_string_json_literal)
{
    EXPECT_EQ(FunctionResult(std::string("hello")).ToJsonLiteral(), "\"hello\"");
    EXPECT_EQ(FunctionResult(std::string("")).ToJsonLiteral(), "\"\"");
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_detect_malformed_function_descriptors)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto callNotString = ParseJson(R"({"f1": {"call": 123}})");
    ASSERT_NE(callNotString, nullptr);
    JsonValue r1 = EventContextResolver::Resolve(callNotString->GetRoot(), context);
    EXPECT_TRUE(r1.IsObject());
    EXPECT_FALSE(r1.Has("f1"));

    auto callEmpty = ParseJson(R"({"f2": {"call": ""}})");
    ASSERT_NE(callEmpty, nullptr);
    JsonValue r2 = EventContextResolver::Resolve(callEmpty->GetRoot(), context);
    EXPECT_TRUE(r2.IsObject());
    EXPECT_FALSE(r2.Has("f2"));

    auto argsNoCall = ParseJson(R"({"f3": {"args": {"a": 1}}})");
    ASSERT_NE(argsNoCall, nullptr);
    JsonValue r3 = EventContextResolver::Resolve(argsNoCall->GetRoot(), context);
    EXPECT_TRUE(r3.IsObject());
    EXPECT_FALSE(r3.Has("f3"));

    auto returnTypeNoCall = ParseJson(R"({"f4": {"returnType": "string"}})");
    ASSERT_NE(returnTypeNoCall, nullptr);
    JsonValue r4 = EventContextResolver::Resolve(returnTypeNoCall->GetRoot(), context);
    EXPECT_TRUE(r4.IsObject());
    EXPECT_FALSE(r4.Has("f4"));

    auto argsAndReturnNoCall = ParseJson(R"({"f5": {"args": {}, "returnType": "void"}})");
    ASSERT_NE(argsAndReturnNoCall, nullptr);
    JsonValue r5 = EventContextResolver::Resolve(argsAndReturnNoCall->GetRoot(), context);
    EXPECT_TRUE(r5.IsObject());
    EXPECT_FALSE(r5.Has("f5"));
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_proceed_when_call_is_valid_string)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto validCall = ParseJson(R"({"func": {"call": "myFunc"}})");
    ASSERT_NE(validCall, nullptr);
    JsonValue result = EventContextResolver::Resolve(validCall->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_FALSE(result.Has("func"));
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_not_detect_malformed_when_extra_keys)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto extraKeys = ParseJson(R"({"data": {"args": {"a": 1}, "extra": true}})");
    ASSERT_NE(extraKeys, nullptr);
    JsonValue result = EventContextResolver::Resolve(extraKeys->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("data"));
    JsonValue dataValue = result.GetItem("data");
    EXPECT_TRUE(dataValue.IsObject());
    EXPECT_TRUE(dataValue.Has("args"));
    EXPECT_TRUE(dataValue.Has("extra"));
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_resolve_arrays_in_context)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto descriptor = ParseJson(R"({"items": [1, "hello", true, null]})");
    ASSERT_NE(descriptor, nullptr);
    JsonValue result = EventContextResolver::Resolve(descriptor->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("items"));

    JsonValue items = result.GetItem("items");
    EXPECT_TRUE(items.IsArray());
    EXPECT_EQ(items.GetArraySize(), 4);
    EXPECT_DOUBLE_EQ(items.GetArrayItem(0).GetNumberValue(0.0), 1.0);
    EXPECT_EQ(items.GetArrayItem(1).GetStringValue(""), "hello");
    EXPECT_TRUE(items.GetArrayItem(2).GetBoolValue(false));
    EXPECT_TRUE(items.GetArrayItem(3).IsNull());
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_resolve_nested_objects)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto descriptor = ParseJson(R"({"user": {"name": "Alice", "age": 30}, "nested": {"a": {"b": "deep"}}})");
    ASSERT_NE(descriptor, nullptr);
    JsonValue result = EventContextResolver::Resolve(descriptor->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("user"));
    EXPECT_EQ(result.GetItem("user").GetString("name", ""), "Alice");
    EXPECT_DOUBLE_EQ(result.GetItem("user").GetNumber("age", 0.0), 30.0);
    EXPECT_TRUE(result.Has("nested"));
    EXPECT_EQ(result.GetItem("nested").GetItem("a").GetString("b", ""), "deep");
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_handle_array_with_failing_dynamic_child)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto descriptor = ParseJson(R"({"items": [1, {"path": "//invalid"}, 3]})");
    ASSERT_NE(descriptor, nullptr);
    JsonValue result = EventContextResolver::Resolve(descriptor->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("items"));

    JsonValue items = result.GetItem("items");
    EXPECT_TRUE(items.IsArray());
    EXPECT_EQ(items.GetArraySize(), 3);
    EXPECT_DOUBLE_EQ(items.GetArrayItem(0).GetNumberValue(0.0), 1.0);
    EXPECT_TRUE(items.GetArrayItem(1).IsNull());
    EXPECT_DOUBLE_EQ(items.GetArrayItem(2).GetNumberValue(0.0), 3.0);
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_handle_scalar_context_values)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto descriptor = ParseJson(R"({"name": "Alice", "age": 25, "active": true})");
    ASSERT_NE(descriptor, nullptr);
    JsonValue result = EventContextResolver::Resolve(descriptor->GetRoot(), context);
    EXPECT_TRUE(result.IsObject());
    EXPECT_EQ(result.GetString("name", ""), "Alice");
    EXPECT_DOUBLE_EQ(result.GetNumber("age", 0.0), 25.0);
    EXPECT_TRUE(result.GetItem("active").GetBoolValue(false));
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_fail_when_create_array_or_null_fails)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    {
        auto desc = ParseJson(R"({"items": [1, 2]})");
        ASSERT_NE(desc, nullptr);
        ScopedCjsonAllocFail fail(1);
        JsonValue r = EventContextResolver::Resolve(desc->GetRoot(), context);
        EXPECT_TRUE(r.IsObject());
        EXPECT_FALSE(r.Has("items"));
    }

    {
        auto desc = ParseJson(R"({"items": [{"path": "//x"}, 2]})");
        ASSERT_NE(desc, nullptr);
        ScopedCjsonAllocFail fail(2);
        JsonValue r = EventContextResolver::Resolve(desc->GetRoot(), context);
        EXPECT_TRUE(r.IsObject());
        EXPECT_FALSE(r.Has("items"));
    }
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_fail_when_nested_create_object_fails)
{
    EventResolveContext context = { .renderId = 1, .surfaceId = "surface", .componentId = "component" };

    auto desc = ParseJson(R"({"nested": {"a": 1}})");
    ASSERT_NE(desc, nullptr);
    ScopedCjsonAllocFail fail(1);
    JsonValue r = EventContextResolver::Resolve(desc->GetRoot(), context);
    EXPECT_TRUE(r.IsObject());
    EXPECT_FALSE(r.Has("nested"));
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_cover_nested_public_resolution_edges)
{
    EventResolveContext context = {
        .renderId = 61, .surfaceId = "surface_event_ctx_internal", .componentId = "component_event_ctx_internal"
    };

    auto nestedEmptyKeyDescriptor = ParseJson(R"({"nested":{"":1,"ok":2}})");
    ASSERT_NE(nestedEmptyKeyDescriptor, nullptr);
    JsonValue nestedEmptyKeyResult = EventContextResolver::Resolve(nestedEmptyKeyDescriptor->GetRoot(), context);
    ASSERT_TRUE(nestedEmptyKeyResult.Has("nested"));
    JsonValue nestedEmptyKey = nestedEmptyKeyResult.GetItem("nested");
    EXPECT_TRUE(nestedEmptyKey.IsObject());
    EXPECT_FALSE(nestedEmptyKey.Has(""));
    EXPECT_TRUE(nestedEmptyKey.Has("ok"));

    auto nestedResolveFailDescriptor = ParseJson(R"({"nested":{"bad":{"path":"bad/path"},"ok":3}})");
    ASSERT_NE(nestedResolveFailDescriptor, nullptr);
    JsonValue nestedResolveFailResult = EventContextResolver::Resolve(nestedResolveFailDescriptor->GetRoot(), context);
    ASSERT_TRUE(nestedResolveFailResult.Has("nested"));
    JsonValue nestedResolveFail = nestedResolveFailResult.GetItem("nested");
    EXPECT_TRUE(nestedResolveFail.IsObject());
    EXPECT_FALSE(nestedResolveFail.Has("bad"));
    EXPECT_TRUE(nestedResolveFail.Has("ok"));

    auto objectForPutDescriptor = ParseJson(R"({"node":{"key":1}})");
    ASSERT_NE(objectForPutDescriptor, nullptr);
    bool observedObjectFailure = false;
    for (int failAfter = 0; failAfter <= 512; ++failAfter) {
        ScopedCjsonAllocFail fail(failAfter);
        JsonValue resolved = EventContextResolver::Resolve(objectForPutDescriptor->GetRoot(), context);
        if (!resolved.Has("node")) {
            observedObjectFailure = true;
            break;
        }
    }
    EXPECT_TRUE(observedObjectFailure);

    auto arrayForAppendDescriptor = ParseJson(R"({"items":[1]})");
    ASSERT_NE(arrayForAppendDescriptor, nullptr);
    bool observedArrayFailure = false;
    for (int failAfter = 0; failAfter <= 512; ++failAfter) {
        ScopedCjsonAllocFail fail(failAfter);
        JsonValue resolved = EventContextResolver::Resolve(arrayForAppendDescriptor->GetRoot(), context);
        if (!resolved.Has("items")) {
            observedArrayFailure = true;
            break;
        }
    }
    EXPECT_TRUE(observedArrayFailure);
}

TEST_F(FunctionBridgeActionCoverageTest, event_context_resolver_should_cover_std_allocation_failure_edges)
{
#if !A2UI_ENABLE_STD_ALLOC_FAIL_HOOK
    GTEST_SKIP() << "std allocation failure hook is disabled on MSVC";
#else
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateCallback());
    SurfaceSlot& surfaceSlot = CreateSurface(62, "surface_event_ctx_alloc", { "noop" });
    auto modelRoot = ParseJson(R"({"user":{"name":"alloc-user"}})");
    ASSERT_NE(modelRoot, nullptr);
    std::shared_ptr<DataModel> dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(modelRoot->GetRoot());

    EventResolveContext context = {
        .renderId = 62, .surfaceId = "surface_event_ctx_alloc", .componentId = "component_event_ctx_alloc"
    };
    auto descriptor =
        ParseJson(R"({"pathNode":{"path":"/user/name"},"arrayNode":[1,{"path":"bad/path"},3],"nested":{"leaf":"v"}})");
    ASSERT_NE(descriptor, nullptr);

    int32_t allocationFailures = ExerciseStdAllocationFailures(
        [&descriptor, &context]() { (void)EventContextResolver::Resolve(descriptor->GetRoot(), context); }, 4096);

    EXPECT_GT(allocationFailures, 0);
#endif
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_cover_normalize_function_call_happy_path)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(20, "surface", { "allowed" });
    auto call = CreateFunctionCall("allowed", "void");

    JsonValue normalizedArgs;
    std::string normalizedReturnType = "INITIAL";

    mockNapiPtr_->nextValueId_ = 5000;
    napi_value successResult = PredictInvokeResultObject(true);
    SetInvokeResponse(successResult, true, true);
    SetObjectProperty(successResult, "normalizedReturnType", NewManualString("string"));

    napi_value argsObject = NewManualObject();
    SetObjectProperty(argsObject, "name", NewManualString("test"));
    SetObjectProperty(successResult, "normalizedArgs", argsObject);

    EXPECT_TRUE(bridge.NormalizeFunctionCall("surface", "component", call, normalizedArgs, normalizedReturnType));
    EXPECT_EQ(normalizedReturnType, "string");
    EXPECT_TRUE(normalizedArgs.IsObject());
    EXPECT_EQ(normalizedArgs.GetString("name", ""), "test");
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_handle_normalize_without_normalized_args)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(21, "surface", { "allowed" });
    auto call = CreateFunctionCall("allowed", "void");

    JsonValue normalizedArgs;
    std::string normalizedReturnType = "INITIAL";

    mockNapiPtr_->nextValueId_ = 5100;
    napi_value successResult = PredictInvokeResultObject(true);
    SetInvokeResponse(successResult, true, true);
    SetObjectProperty(successResult, "returnType", NewManualString("number"));

    EXPECT_TRUE(bridge.NormalizeFunctionCall("surface", "component", call, normalizedArgs, normalizedReturnType));
    EXPECT_EQ(normalizedReturnType, "number");
    EXPECT_TRUE(normalizedArgs.IsNull());
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_fallback_normalized_return_type_to_function_call)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(22, "surface", { "allowed" });
    auto call = CreateFunctionCall("allowed", "myReturnType");

    JsonValue normalizedArgs;
    std::string normalizedReturnType = "INITIAL";

    mockNapiPtr_->nextValueId_ = 5200;
    napi_value successResult = PredictInvokeResultObject(true);
    SetInvokeResponse(successResult, true, true);

    EXPECT_TRUE(bridge.NormalizeFunctionCall("surface", "component", call, normalizedArgs, normalizedReturnType));
    EXPECT_EQ(normalizedReturnType, "myReturnType");
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_fail_normalize_when_args_conversion_fails)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(23, "surface", { "allowed" });
    auto call = CreateFunctionCall("allowed", "void");

    JsonValue normalizedArgs;
    std::string normalizedReturnType;

    mockNapiPtr_->nextValueId_ = 5300;
    napi_value successResult = PredictInvokeResultObject(true);
    SetInvokeResponse(successResult, true, true);
    SetObjectProperty(successResult, "normalizedArgs", NewManualTypedValue(napi_function));

    EXPECT_FALSE(bridge.NormalizeFunctionCall("surface", "component", call, normalizedArgs, normalizedReturnType));
}

TEST_F(FunctionBridgeActionCoverageTest, function_bridge_should_fail_normalize_when_clone_args_fails)
{
    auto& bridge = FunctionBridge::GetInstance();
    bridge.RegisterInvokeLocalFunction(env_, CreateCallback());
    CreateSurface(24, "surface", { "allowed" });
    auto call = CreateFunctionCall("allowed", "void");

    JsonValue normalizedArgs;
    std::string normalizedReturnType;

    mockNapiPtr_->nextValueId_ = 5400;
    napi_value successResult = PredictInvokeResultObject(true);
    SetInvokeResponse(successResult, true, true);
    SetObjectProperty(successResult, "normalizedArgs", NewManualNumber(1.0));

    ScopedCjsonAllocFail fail(1);
    EXPECT_FALSE(bridge.NormalizeFunctionCall("surface", "component", call, normalizedArgs, normalizedReturnType));
}
