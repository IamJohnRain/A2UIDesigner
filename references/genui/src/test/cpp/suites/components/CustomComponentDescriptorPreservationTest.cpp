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
#include "components/custom/CustomComponent.h"
#undef private

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "functions/FunctionBridge.h"
#include "functions/NativeFunctionBase.h"
#include "functions/NativeFunctionRegistry.h"
#include "utils/JsonAdapter.h"

#include "NapiBridge.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

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

class FunctionBridgeResetGuard {
public:
    FunctionBridgeResetGuard(MockNapiProvider* mockNapi, napi_env env) : mockNapi_(mockNapi), env_(env) {}

    ~FunctionBridgeResetGuard()
    {
        if (mockNapi_ == nullptr) {
            return;
        }
        napi_value callback = nullptr;
        mockNapi_->CreateFunction(env_, "resetFunctionBridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        mockNapi_->SetCreateReferenceStatus(napi_invalid_arg);
        FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, callback);
        mockNapi_->ResetCreateReferenceStatus();
    }

private:
    MockNapiProvider* mockNapi_ = nullptr;
    napi_env env_ = nullptr;
};

class CallbackFailingNativeFunction : public NativeFunctionBase {
public:
    std::string GetName() const override
    {
        return "coverageCallbackFailure";
    }

    FunctionResult Execute(const JsonValue&) override
    {
        auto* mockNapi = dynamic_cast<MockNapiProvider*>(&NapiBridge::GetInstance().Provider());
        if (mockNapi != nullptr) {
            mockNapi->SetCallFunctionStatus(napi_generic_failure);
        }
        return FunctionResult(std::string("resolved"));
    }
};

napi_value RawNapiValue(intptr_t id)
{
    return reinterpret_cast<napi_value>(id);
}

bool PreparePassthroughNormalizeResponse(MockNapiProvider* mockNapi)
{
    if (mockNapi == nullptr) {
        return false;
    }

    napi_value success = nullptr;
    if (mockNapi->CreateBoolean(nullptr, true, &success) != napi_ok || success == nullptr) {
        return false;
    }

    intptr_t firstValueId = static_cast<intptr_t>(mockNapi->nextValueId_);
    napi_value normalizedArgs = RawNapiValue(firstValueId + 5);
    napi_value result = RawNapiValue(firstValueId + 10);
    mockNapi->valueTypes_[result] = napi_object;
    mockNapi->objectProperties_[result] = { { "success", success }, { "normalizedArgs", normalizedArgs } };
    return true;
}

size_t CountBindingsForProperty(const Component& component, const std::string& propertyName)
{
    size_t count = 0U;
    for (const auto& binding : component.GetDataBindings()) {
        if (binding.propertyName_ == propertyName) {
            ++count;
        }
    }
    return count;
}

} // namespace

class CustomComponentDescriptorPreservationTest : public A2UITest {};

TEST_F(CustomComponentDescriptorPreservationTest, should_preserve_dynamic_descriptors_in_public_custom_props)
{
    constexpr int32_t renderId = 770;
    const std::string surfaceId = "surface-public-custom-props";
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    auto dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(data, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    CustomComponent component("DynamicResolveProbe", true);
    component.SetRenderId(renderId);
    component.SetSurfaceId(surfaceId);
    component.SetComponentId("public-custom-props");
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({"value":{"path":"/user/name"}})");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());
    JsonValue resolvedValue = component.GetCustomProperty("value");
    ASSERT_TRUE(resolvedValue.IsString());
    EXPECT_EQ(resolvedValue.GetStringValue(""), "Alice");

    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    JsonValue rawValue = customProps.GetItem("value");
    ASSERT_TRUE(rawValue.IsObject());
    EXPECT_EQ(rawValue.GetString("path", ""), "/user/name");
}

TEST_F(CustomComponentDescriptorPreservationTest, should_replace_and_remove_preserved_dynamic_descriptor)
{
    constexpr int32_t renderId = 771;
    const std::string surfaceId = "surface-replace-dynamic-descriptor";
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    auto dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(data, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    CustomComponent component("DynamicResolveProbe", true);
    component.SetRenderId(renderId);
    component.SetSurfaceId(surfaceId);
    component.SetComponentId("replace-dynamic-descriptor");

    std::unique_ptr<JsonAdapter> pathDescriptor = JsonAdapter::Parse(R"({"value":{"path":"/user/name"}})");
    ASSERT_NE(pathDescriptor, nullptr);
    component.ApplyCustomProperties(pathDescriptor->GetRoot());
    ASSERT_EQ(component.rawDynamicProperties_.count("value"), 1U);

    std::unique_ptr<JsonAdapter> literalDescriptor = JsonAdapter::Parse(R"({"value":"literal"})");
    ASSERT_NE(literalDescriptor, nullptr);
    component.ApplyCustomProperties(literalDescriptor->GetRoot());
    EXPECT_TRUE(component.rawDynamicProperties_.empty());
    EXPECT_EQ(component.GetCustomProperty("value").GetStringValue(""), "literal");

    JsonValue literalProps = component.BuildCustomProps();
    ASSERT_TRUE(literalProps.IsObject());
    EXPECT_EQ(literalProps.GetString("value", ""), "literal");

    std::unique_ptr<JsonAdapter> emptyDescriptor = JsonAdapter::Parse(R"({})");
    ASSERT_NE(emptyDescriptor, nullptr);
    component.ApplyCustomProperties(emptyDescriptor->GetRoot());
    EXPECT_FALSE(component.GetCustomProperty("value").IsValid());
    EXPECT_TRUE(component.rawDynamicProperties_.empty());
}

TEST_F(CustomComponentDescriptorPreservationTest, should_preserve_unresolved_function_call_descriptor)
{
    CustomComponent component("DynamicResolveProbe", true);
    component.SetComponentId("preserve-function-call");
    std::unique_ptr<JsonAdapter> descriptor =
        JsonAdapter::Parse(R"({"value":{"call":"missingFunction","returnType":"string"}})");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    ASSERT_EQ(component.rawDynamicProperties_.count("value"), 1U);
    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    JsonValue rawValue = customProps.GetItem("value");
    ASSERT_TRUE(rawValue.IsObject());
    EXPECT_EQ(rawValue.GetString("call", ""), "missingFunction");
    EXPECT_EQ(rawValue.GetString("returnType", ""), "string");
}

TEST_F(CustomComponentDescriptorPreservationTest, should_clear_one_shot_callback_when_dispatch_fails)
{
    napi_env env = reinterpret_cast<napi_env>(0x7710);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    std::unique_ptr<JsonAdapter> literalValue = JsonAdapter::CreateString("resolved-now");
    ASSERT_NE(literalValue, nullptr);

    CustomComponent component("Panel");
    std::string errorMessage;
    mockNapiPtr_->SetCallFunctionStatus(napi_generic_failure);
    bool registered =
        component.RegisterDynamicValueCallback("headline", literalValue->GetRoot(), env, callback, &errorMessage);
    mockNapiPtr_->ResetCallFunctionStatus();

    EXPECT_FALSE(registered);
    EXPECT_EQ(errorMessage, "failed to dispatch resolved value");
    EXPECT_TRUE(component.dynamicValueCallbacks_.empty());

    mockNapiPtr_->SetCallFunctionStatus(napi_generic_failure);
    EXPECT_FALSE(component.RegisterDynamicValueCallback("subtitle", literalValue->GetRoot(), env, callback, nullptr));
    mockNapiPtr_->ResetCallFunctionStatus();
    EXPECT_TRUE(component.dynamicValueCallbacks_.empty());
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(CustomComponentDescriptorPreservationTest, should_treat_literal_object_with_global_dependency_as_one_shot)
{
    napi_env env = reinterpret_cast<napi_env>(0x7711);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({"value":"{{ $__widthBreakpoint }}"})");
    ASSERT_NE(descriptor, nullptr);

    CustomComponent component("Panel");
    std::string errorMessage;
    ASSERT_TRUE(
        component.RegisterDynamicValueCallback("headline", descriptor->GetRoot(), env, callback, &errorMessage));

    EXPECT_TRUE(component.dynamicValueCallbacks_.empty());
    EXPECT_TRUE(component.dynamicResolverBindingKeys_.empty());
    EXPECT_EQ(CountBindingsForProperty(component, "headline"), 0U);
}
#endif

TEST_F(CustomComponentDescriptorPreservationTest, should_clear_persistent_function_callback_when_initial_dispatch_fails)
{
    constexpr int32_t renderId = 773;
    const std::string surfaceId = "surface-persistent-callback-failure";
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    auto catalog = std::make_shared<Catalog>("catalog-persistent-callback-failure");
    catalog->AddFunction(std::make_shared<CatalogItem>("coverageCallbackFailure", CatalogItemType::LOCAL_FUNCTION));
    surface.SetCatalog(catalog);
    std::unique_ptr<JsonAdapter> initialData = JsonAdapter::Parse(R"({"form":{"title":"current-title"}})");
    ASSERT_NE(initialData, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/", initialData->GetRoot());

    napi_env env = reinterpret_cast<napi_env>(0x7730);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    napi_value bridgeCallback = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env, "functionBridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &bridgeCallback),
        napi_ok);
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env, bridgeCallback);
    FunctionBridgeResetGuard bridgeResetGuard(mockNapiPtr_, env);
    NativeFunctionRegistry::GetInstance().Register(
        "coverageCallbackFailure", std::make_shared<CallbackFailingNativeFunction>());

    auto component = std::make_shared<CustomComponent>("Panel");
    component->SetComponentId("persistent-callback-failure");
    component->SetSurfaceId(surfaceId);
    component->SetRenderId(renderId);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(
        R"({"call":"coverageCallbackFailure","returnType":"string","args":{"value":{"path":"/form/title"}}})");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(PreparePassthroughNormalizeResponse(mockNapiPtr_));
    std::string errorMessage;
    bool registered =
        component->RegisterDynamicValueCallback("title", descriptor->GetRoot(), env, callback, &errorMessage);
    mockNapiPtr_->ResetCallFunctionStatus();

    EXPECT_FALSE(registered);
    EXPECT_EQ(errorMessage, "failed to dispatch resolved value");
    EXPECT_TRUE(component->dynamicValueCallbacks_.empty());
    EXPECT_TRUE(component->dynamicResolverBindingKeys_.empty());
    EXPECT_EQ(CountBindingsForProperty(*component, "title"), 0U);

    ASSERT_TRUE(PreparePassthroughNormalizeResponse(mockNapiPtr_));
    EXPECT_FALSE(component->RegisterDynamicValueCallback("subtitle", descriptor->GetRoot(), env, callback, nullptr));
    mockNapiPtr_->ResetCallFunctionStatus();
    EXPECT_TRUE(component->dynamicValueCallbacks_.empty());
    EXPECT_TRUE(component->dynamicResolverBindingKeys_.empty());
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(CustomComponentDescriptorPreservationTest, should_keep_function_callback_for_global_variable_dependency)
{
    constexpr int32_t renderId = 775;
    const std::string surfaceId = "surface-function-global-callback";
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    auto catalog = std::make_shared<Catalog>("catalog-function-global-callback");
    catalog->AddFunction(std::make_shared<CatalogItem>("formatString", CatalogItemType::LOCAL_FUNCTION));
    surface.SetCatalog(catalog);

    napi_env env = reinterpret_cast<napi_env>(0x7750);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    napi_value bridgeCallback = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env, "functionBridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &bridgeCallback),
        napi_ok);
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env, bridgeCallback);
    FunctionBridgeResetGuard bridgeResetGuard(mockNapiPtr_, env);

    auto component = std::make_shared<CustomComponent>("Panel");
    component->SetComponentId("function-global-callback");
    component->SetSurfaceId(surfaceId);
    component->SetRenderId(renderId);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({
        "call": "formatString",
        "returnType": "string",
        "args": { "value": "{{ $__widthBreakpoint }}" }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(PreparePassthroughNormalizeResponse(mockNapiPtr_));
    std::string errorMessage;
    ASSERT_TRUE(component->RegisterDynamicValueCallback("title", descriptor->GetRoot(), env, callback, &errorMessage))
        << errorMessage;
    ASSERT_EQ(CountBindingsForProperty(*component, "title"), 1U);
    ASSERT_EQ(component->GetDataBindings()[0].type_, BindingType::FUNCTION_CALL);
    EXPECT_TRUE(component->GetDataBindings()[0].dataPath_.empty());
    ASSERT_EQ(component->GetDataBindings()[0].globalVarDeps_.size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].globalVarDeps_[0], "__widthBreakpoint");
    EXPECT_TRUE(component->dynamicValueCallbacks_.find("title") != component->dynamicValueCallbacks_.end());
}
#endif

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(CustomComponentDescriptorPreservationTest, should_keep_expression_callback_for_global_variable_dependency)
{
    constexpr int32_t renderId = 774;
    const std::string surfaceId = "surface-expression-global-callback";
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);

    napi_env env = reinterpret_cast<napi_env>(0x7740);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);

    auto component = std::make_shared<CustomComponent>("Panel");
    component->SetComponentId("expression-global-callback");
    component->SetSurfaceId(surfaceId);
    component->SetRenderId(renderId);
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"("{{ $__widthBreakpoint }}")");
    ASSERT_NE(descriptor, nullptr);

    std::string errorMessage;
    ASSERT_TRUE(component->RegisterDynamicValueCallback("title", descriptor->GetRoot(), env, callback, &errorMessage))
        << errorMessage;
    ASSERT_EQ(CountBindingsForProperty(*component, "title"), 1U);
    ASSERT_EQ(component->GetDataBindings()[0].type_, BindingType::EXPRESSION);
    EXPECT_TRUE(component->GetDataBindings()[0].dataPath_.empty());
    ASSERT_EQ(component->GetDataBindings()[0].globalVarDeps_.size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].globalVarDeps_[0], "__widthBreakpoint");
    EXPECT_TRUE(component->dynamicValueCallbacks_.find("title") != component->dynamicValueCallbacks_.end());
}
#endif

TEST_F(CustomComponentDescriptorPreservationTest,
    should_forward_descriptor_preservation_for_standard_and_extended_custom_components)
{
    constexpr int32_t renderId = 772;
    RenderSlotCleanupGuard cleanup(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);

    SurfaceSlot& standardSurface = surfaceManager->CreateSurface("surface-standard-custom-route");
    auto standardCatalog = std::make_shared<Catalog>("catalog-standard-custom-route");
    auto standardItem = std::make_shared<CatalogItem>("Button", CatalogItemType::COMPONENT);
    standardItem->SetCategory(CatalogCategory::A2UI_STANDARD);
    standardItem->SetPreserveDynamicDescriptors(true);
    standardCatalog->AddComponent(standardItem);
    standardSurface.SetCatalog(standardCatalog);
    std::unique_ptr<JsonAdapter> standardMessage =
        JsonAdapter::Parse(R"({"components":[{"id":"standard-button","component":"Button"}]})");
    ASSERT_NE(standardMessage, nullptr);
    ASSERT_TRUE(standardSurface.UpdateComponents(standardMessage->GetRoot()));
    std::shared_ptr<CustomComponent> standardComponent =
        std::dynamic_pointer_cast<CustomComponent>(standardSurface.FindComponentById("standard-button"));
    ASSERT_NE(standardComponent, nullptr);
    EXPECT_TRUE(standardComponent->preserveDynamicDescriptors_);

    SurfaceSlot& extendedSurface = surfaceManager->CreateSurface("surface-extended-custom-route");
    auto extendedCatalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto extendedItem = std::make_shared<CatalogItem>("DynamicResolveProbe", CatalogItemType::COMPONENT);
    extendedItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    extendedItem->SetPreserveDynamicDescriptors(true);
    extendedCatalog->AddComponent(extendedItem);
    auto extendedButtonItem = std::make_shared<CatalogItem>("Extended.Button", CatalogItemType::COMPONENT);
    extendedButtonItem->SetCategory(CatalogCategory::OHOS_EXTENDS);
    extendedButtonItem->SetPreserveDynamicDescriptors(true);
    extendedCatalog->AddComponent(extendedButtonItem);
    extendedSurface.SetCatalog(extendedCatalog);
    std::unique_ptr<JsonAdapter> extendedMessage =
        JsonAdapter::Parse(R"({"components":[{"id":"extended-probe","component":"DynamicResolveProbe"}]})");
    ASSERT_NE(extendedMessage, nullptr);
    ASSERT_TRUE(extendedSurface.UpdateComponents(extendedMessage->GetRoot()));
    std::shared_ptr<CustomComponent> extendedComponent =
        std::dynamic_pointer_cast<CustomComponent>(extendedSurface.FindComponentById("extended-probe"));
    ASSERT_NE(extendedComponent, nullptr);
    EXPECT_TRUE(extendedComponent->preserveDynamicDescriptors_);

    std::unique_ptr<JsonAdapter> extendedButtonMessage =
        JsonAdapter::Parse(R"({"components":[{"id":"extended-button","component":"Extended.Button"}]})");
    ASSERT_NE(extendedButtonMessage, nullptr);
    ASSERT_TRUE(extendedSurface.UpdateComponents(extendedButtonMessage->GetRoot()));
    std::shared_ptr<Component> extendedButton = extendedSurface.FindComponentById("extended-button");
    ASSERT_NE(extendedButton, nullptr);
}
