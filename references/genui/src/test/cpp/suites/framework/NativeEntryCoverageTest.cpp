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
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/text/TextComponent.h"
#include "components/Component.h"
#include "components/actions/BuiltInActions.h"
#include "components/actions/NativeActionRegistry.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/CrossLanguageAttributeBridge.h"
#include "functions/FunctionBridge.h"
#include "functions/NativeFunctionRegistry.h"
#include "functions/RuntimeErrorDispatchBridge.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/DisplayDensityUtils.h"
#include "utils/JsonAdapter.h"

#include "NativeEntry.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceErrorCodes.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

constexpr int32_t NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID = 830199;

constexpr int32_t NATIVE_ENTRY_TEST_RENDER_ID = 830100;
constexpr char NATIVE_ENTRY_TEST_SURFACE_A[] = "native-entry-surface-a";
constexpr char NATIVE_ENTRY_TEST_SURFACE_B[] = "native-entry-surface-b";

std::shared_ptr<Catalog> BuildTextCatalog()
{
    auto catalog = std::make_shared<Catalog>("catalog-native-entry");
    auto item = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::A2UI_STANDARD);
    item->SetInnerNative(true);
    catalog->AddComponent(item);
    return catalog;
}

std::shared_ptr<Catalog> BuildExtendedCatalog(std::initializer_list<const char*> componentNames)
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* componentName : componentNames) {
        if (componentName == nullptr || componentName[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

std::shared_ptr<TextComponent> CreateTextComponent(const std::string& id)
{
    auto text = std::make_shared<TextComponent>();
    text->SetComponentId(id);
    text->SetRenderId(NATIVE_ENTRY_TEST_RENDER_ID);
    text->SetSurfaceId(NATIVE_ENTRY_TEST_SURFACE_A);
    return text;
}

std::shared_ptr<Component> CreatePlainComponent(const std::string& id)
{
    auto component = std::make_shared<Component>(reinterpret_cast<ArkUI_NodeHandle>(0x7777), false);
    component->SetComponentId(id);
    component->SetRenderId(NATIVE_ENTRY_TEST_RENDER_ID);
    component->SetSurfaceId(NATIVE_ENTRY_TEST_SURFACE_A);
    return component;
}

struct CrossLanguageAttributeBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref callbackRef_ = nullptr;
};

struct FunctionBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref invokeLocalFunctionRef_ = nullptr;
};

struct ActionDispatchBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref dispatchActionRef_ = nullptr;
};

struct RuntimeErrorDispatchBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref dispatchRuntimeErrorRef_ = nullptr;
};

struct WarningDispatchBridgeMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref dispatchWarningRef_ = nullptr;
};

void ResetCrossLanguageAttributeBridge()
{
    auto* bridge = reinterpret_cast<CrossLanguageAttributeBridgeMirror*>(&CrossLanguageAttributeBridge::GetInstance());
    bridge->napiEnv_ = nullptr;
    bridge->callbackRef_ = nullptr;
}

void ResetCallbackBridges()
{
    auto* functionBridge = reinterpret_cast<FunctionBridgeMirror*>(&FunctionBridge::GetInstance());
    functionBridge->napiEnv_ = nullptr;
    functionBridge->invokeLocalFunctionRef_ = nullptr;

    auto* actionBridge = reinterpret_cast<ActionDispatchBridgeMirror*>(&ActionDispatchBridge::GetInstance());
    actionBridge->napiEnv_ = nullptr;
    actionBridge->dispatchActionRef_ = nullptr;

    auto* runtimeBridge =
        reinterpret_cast<RuntimeErrorDispatchBridgeMirror*>(&RuntimeErrorDispatchBridge::GetInstance());
    runtimeBridge->napiEnv_ = nullptr;
    runtimeBridge->dispatchRuntimeErrorRef_ = nullptr;

    auto* warningBridge = reinterpret_cast<WarningDispatchBridgeMirror*>(&WarningDispatchBridge::GetInstance());
    warningBridge->napiEnv_ = nullptr;
    warningBridge->dispatchWarningRef_ = nullptr;
}
} // namespace

class NativeEntryCoverageTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);
    intptr_t manualValueId_ = 0x500000;

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetCallbackBridges();
        ResetCrossLanguageAttributeBridge();
    }

    napi_value CreateInt32Arg(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    napi_value CreateBoolArg(bool value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateBoolean(env_, value, &result);
        return result;
    }

    napi_value CreateDoubleArg(double value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateDouble(env_, value, &result);
        return result;
    }

    napi_value CreateFunctionArg()
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateFunction(env_, "callback", NAPI_AUTO_LENGTH, nullptr, nullptr, &result);
        return result;
    }

    napi_value CreateStringArg(const std::string& value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateStringUtf8(env_, value.c_str(), value.size(), &result);
        return result;
    }

    napi_value CreateCatalogArg(
        const std::string& catalogId, const std::string& componentName, CatalogCategory category, bool isInnerNative)
    {
        napi_value catalog = nullptr;
        mockNapiPtr_->CreateObject(env_, &catalog);
        mockNapiPtr_->SetNamedProperty(env_, catalog, "id", CreateStringArg(catalogId));

        napi_value components = nullptr;
        mockNapiPtr_->CreateArrayWithLength(env_, 1, &components);

        napi_value item = nullptr;
        mockNapiPtr_->CreateObject(env_, &item);
        mockNapiPtr_->SetNamedProperty(env_, item, "name", CreateStringArg(componentName));
        mockNapiPtr_->SetNamedProperty(env_, item, "category", CreateInt32Arg(static_cast<int32_t>(category)));
        mockNapiPtr_->SetNamedProperty(
            env_, item, "type", CreateInt32Arg(static_cast<int32_t>(CatalogItemType::COMPONENT)));
        mockNapiPtr_->SetNamedProperty(env_, item, "isInnerNative", CreateBoolArg(isInnerNative));
        mockNapiPtr_->SetElement(env_, components, 0, item);

        mockNapiPtr_->SetNamedProperty(env_, catalog, "components", components);
        return catalog;
    }

    napi_value CreateProcessOptionsArg(bool isExtend)
    {
        napi_value options = nullptr;
        mockNapiPtr_->CreateObject(env_, &options);
        mockNapiPtr_->SetNamedProperty(env_, options, "supportsMultipleSurfaces", CreateBoolArg(false));
        mockNapiPtr_->SetNamedProperty(env_, options, "maxSurfaceCount", CreateInt32Arg(-1));
        mockNapiPtr_->SetNamedProperty(env_, options, "isExtend", CreateBoolArg(isExtend));
        return options;
    }

    void ExpectProcessSuccess(napi_value result)
    {
        ASSERT_NE(result, nullptr);
        napi_value successValue = nullptr;
        mockNapiPtr_->GetNamedProperty(env_, result, "success", &successValue);
        ASSERT_NE(successValue, nullptr);
        EXPECT_TRUE(mockNapiPtr_->boolValues_[successValue]);
    }

    void ExpectProcessFailure(napi_value result, const std::string& errorCode)
    {
        ASSERT_NE(result, nullptr);
        napi_value successValue = nullptr;
        mockNapiPtr_->GetNamedProperty(env_, result, "success", &successValue);
        ASSERT_NE(successValue, nullptr);
        EXPECT_FALSE(mockNapiPtr_->boolValues_[successValue]);

        napi_value errorCodeValue = nullptr;
        mockNapiPtr_->GetNamedProperty(env_, result, "errorCode", &errorCodeValue);
        ASSERT_NE(errorCodeValue, nullptr);
        EXPECT_EQ(mockNapiPtr_->stringValues_[errorCodeValue], errorCode);
    }

    bool GetBoolProperty(napi_value object, const std::string& key, bool defaultValue)
    {
        napi_value value = nullptr;
        if (object == nullptr || mockNapiPtr_->GetNamedProperty(env_, object, key.c_str(), &value) != napi_ok ||
            value == nullptr) {
            return defaultValue;
        }
        auto valueIt = mockNapiPtr_->boolValues_.find(value);
        return valueIt != mockNapiPtr_->boolValues_.end() ? valueIt->second : defaultValue;
    }

    std::string GetStringProperty(napi_value object, const std::string& key, const std::string& defaultValue)
    {
        napi_value value = nullptr;
        if (object == nullptr || mockNapiPtr_->GetNamedProperty(env_, object, key.c_str(), &value) != napi_ok ||
            value == nullptr) {
            return defaultValue;
        }
        auto valueIt = mockNapiPtr_->stringValues_.find(value);
        return valueIt != mockNapiPtr_->stringValues_.end() ? valueIt->second : defaultValue;
    }

    double GetNumberProperty(napi_value object, const std::string& key, double defaultValue)
    {
        napi_value value = nullptr;
        if (object == nullptr || mockNapiPtr_->GetNamedProperty(env_, object, key.c_str(), &value) != napi_ok ||
            value == nullptr) {
            return defaultValue;
        }
        auto valueIt = mockNapiPtr_->numberValues_.find(value);
        return valueIt != mockNapiPtr_->numberValues_.end() ? valueIt->second : defaultValue;
    }

    napi_value GetProperty(napi_value object, const std::string& key)
    {
        napi_value value = nullptr;
        if (object == nullptr || mockNapiPtr_->GetNamedProperty(env_, object, key.c_str(), &value) != napi_ok) {
            return nullptr;
        }
        return value;
    }

    std::string GetStringValue(napi_value value, const std::string& defaultValue)
    {
        if (value == nullptr) {
            return defaultValue;
        }
        auto valueIt = mockNapiPtr_->stringValues_.find(value);
        return valueIt != mockNapiPtr_->stringValues_.end() ? valueIt->second : defaultValue;
    }

    uint32_t GetArrayLength(napi_value array)
    {
        uint32_t length = 0;
        if (array == nullptr || mockNapiPtr_->GetArrayLength(env_, array, &length) != napi_ok) {
            return 0;
        }
        return length;
    }

    napi_value GetArrayElement(napi_value array, uint32_t index)
    {
        napi_value value = nullptr;
        if (array == nullptr || mockNapiPtr_->GetElement(env_, array, index, &value) != napi_ok) {
            return nullptr;
        }
        return value;
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

    napi_value PredictNormalizeResultObject() const
    {
        return RawValue(static_cast<intptr_t>(mockNapiPtr_->nextValueId_ + 10));
    }

    void TearDown() override
    {
        auto& renderManager = RenderManager::GetInstance();
        if (renderManager.HasRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID)) {
            renderManager.RemoveRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
        }
        ResetCallbackBridges();
        ResetCrossLanguageAttributeBridge();
        A2UITest::TearDown();
    }
};

TEST_F(NativeEntryCoverageTest, should_create_and_destroy_render_slot_via_napi)
{
    auto& renderManager = RenderManager::GetInstance();

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_FALSE(renderManager.HasRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID));

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(-1) });
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_FALSE(renderManager.HasRenderSlot(-1));

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_EQ(InitRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_TRUE(renderManager.HasRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID));

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(DestroyRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_TRUE(renderManager.HasRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID));

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_EQ(DestroyRenderSlot(env_, cbInfo_), nullptr);
    EXPECT_FALSE(renderManager.HasRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID));
}

TEST_F(NativeEntryCoverageTest, should_bind_and_unbind_surface_content_handle_via_napi)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    ArkUI_NodeContentHandle contentHandle = reinterpret_cast<ArkUI_NodeContentHandle>(0x3010);
    mockArkUIPtr_->SetNodeContentHandleResult(contentHandle);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg("node-content") });
    EXPECT_EQ(BindSurfaceToRender(env_, cbInfo_), nullptr);
    EXPECT_EQ(slot.GetContentHandle(), contentHandle);
    EXPECT_EQ(surface.GetContentHandle(), contentHandle);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(999999), CreateStringArg("node-content") });
    EXPECT_EQ(BindSurfaceToRender(env_, cbInfo_), nullptr);
    EXPECT_EQ(slot.GetContentHandle(), contentHandle);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_EQ(UnbindSurfaceFromRender(env_, cbInfo_), nullptr);
    EXPECT_EQ(slot.GetContentHandle(), nullptr);
    EXPECT_EQ(surface.GetContentHandle(), nullptr);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(BindSurfaceToRender(env_, cbInfo_), nullptr);
    EXPECT_EQ(UnbindSurfaceFromRender(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_register_dispatch_and_function_callbacks_via_napi_entry)
{
    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(RegisterInvokeLocalFunction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchAction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchSchemaWarning(env_, cbInfo_), nullptr);
    EXPECT_TRUE(mockNapiPtr_->refToValue_.empty());

    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("not-function") });
    EXPECT_EQ(RegisterInvokeLocalFunction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchAction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchSchemaWarning(env_, cbInfo_), nullptr);
    EXPECT_TRUE(mockNapiPtr_->refToValue_.empty());

    mockNapiPtr_->SetCallbackArgs({ CreateFunctionArg() });
    EXPECT_EQ(RegisterInvokeLocalFunction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchAction(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchRuntimeError(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterDispatchSchemaWarning(env_, cbInfo_), nullptr);
    EXPECT_EQ(mockNapiPtr_->refToValue_.size(), 4U);
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(NativeEntryCoverageTest, should_evaluate_expression_and_reject_invalid_expression_args)
{
    mockNapiPtr_->SetCallbackArgs({});
    napi_value invalidResult = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(invalidResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(invalidResult, "success", true));
    EXPECT_EQ(GetStringProperty(invalidResult, "type", ""), "undefined");

    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ 1 + 2 }}") });
    napi_value result = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));
    EXPECT_EQ(GetStringProperty(result, "type", ""), "number");
    EXPECT_DOUBLE_EQ(GetNumberProperty(result, "value", 0.0), 3.0);
}
#endif

TEST_F(NativeEntryCoverageTest, should_sync_component_bound_data_model_via_napi)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);

    std::shared_ptr<Component> component = CreateTextComponent("text-1");
    component->AddBinding("text", "/title");
    surface.GetAllComponents()["text-1"] = component;

    std::unique_ptr<JsonAdapter> root = JsonAdapter::Parse(R"({"title":"old"})");
    ASSERT_NE(root, nullptr);
    std::shared_ptr<DataModel> dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(root->GetRoot());

    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A),
            CreateStringArg("text-1"), CreateStringArg("text"), CreateStringArg(R"("new")") });
    napi_value result = SyncComponentBoundDataModel(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));

    std::optional<JsonValue> updatedValue = dataModel->GetNode("/title");
    ASSERT_TRUE(updatedValue.has_value());
    EXPECT_EQ(updatedValue->GetStringValue(""), "new");

    mockNapiPtr_->SetCallbackArgs({});
    napi_value invalidResult = SyncComponentBoundDataModel(env_, cbInfo_);
    ASSERT_NE(invalidResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(invalidResult, "success", true));
    EXPECT_EQ(GetStringProperty(invalidResult, "errorCode", ""), "INVALID_ARGUMENT");
}

TEST_F(NativeEntryCoverageTest, should_return_no_surface_matched_when_sync_bound_data_model_surface_is_missing)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg("missing"),
        CreateStringArg("text-1"), CreateStringArg("text"), CreateStringArg(R"("new")") });
    napi_value result = SyncComponentBoundDataModel(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success", true));
    EXPECT_EQ(GetStringProperty(result, "errorCode", ""), "SURFACE_NOT_FOUND");
    EXPECT_EQ(GetStringProperty(result, "errorMessage", ""), "Surface not found: missing");
    EXPECT_EQ(
        static_cast<int32_t>(GetNumberProperty(result, "surfaceResultCode", 0)), SURFACE_ERROR_NO_SURFACE_MATCHED);
}

TEST_F(NativeEntryCoverageTest, should_evaluate_dynamic_value_native_function_call_via_napi)
{
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, CreateFunctionArg());

    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    std::shared_ptr<Catalog> catalog = BuildExtendedCatalog({ "Checkbox" });
    catalog->AddFunction(std::make_shared<CatalogItem>("getCheckboxGroupValues", CatalogItemType::LOCAL_FUNCTION));
    surface.SetCatalog(catalog);

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "meal-1",
            "component": "Checkbox",
            "value": "Meal A",
            "select": true,
            "group": "m_pick"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID),
        CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A), CreateStringArg("caller"),
        CreateStringArg(R"({"call":"getCheckboxGroupValues","args":{"group":"m_pick"},)"
                        R"("returnType":"array"})"),
        CreateBoolArg(true) });

    mockNapiPtr_->nextValueId_ = 6000;
    napi_value normalizeResult = PredictNormalizeResultObject();
    mockNapiPtr_->valueTypes_[normalizeResult] = napi_object;
    mockNapiPtr_->objectProperties_[normalizeResult] = {};
    SetObjectProperty(normalizeResult, "success", NewManualBool(true));
    SetObjectProperty(normalizeResult, "normalizedReturnType", NewManualString("array"));
    napi_value normalizedArgs = NewManualObject();
    SetObjectProperty(normalizedArgs, "group", NewManualString("m_pick"));
    SetObjectProperty(normalizeResult, "normalizedArgs", normalizedArgs);

    napi_value result = EvaluateDynamicValue(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success", false));

    napi_value value = GetProperty(result, "value");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(GetArrayLength(value), 1u);
    EXPECT_EQ(GetStringValue(GetArrayElement(value, 0), ""), "Meal A");
}

TEST_F(NativeEntryCoverageTest, should_use_create_surface_catalog_id_for_extended_protocol)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    napi_value catalog = CreateCatalogArg("catalog-native-entry", "Text", CatalogCategory::OHOS_EXTENDS, false);
    const std::string createDsl = R"({"version":"v0.9","createSurface":{)"
                                  R"("surfaceId":"native-entry-surface-a","catalogId":"ohos.a2ui.extended.catalog"}})";
    const std::string updateDsl = R"({"version":"v0.9","updateComponents":{)"
                                  R"("surfaceId":"native-entry-surface-a","components":[)"
                                  R"({"id":"root","component":"Text","text":"hello"}]}})";

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog,
        CreateProcessOptionsArg(true) });
    ExpectProcessSuccess(ProcessMessage(env_, cbInfo_));

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(updateDsl), catalog });
    ExpectProcessSuccess(ProcessMessage(env_, cbInfo_));

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    ASSERT_NE(surface, nullptr);
    std::shared_ptr<Component> component = surface->FindComponentById("root");
    ASSERT_NE(component, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedTextComponent>(component), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_parse_catalog_descriptor_flags_and_functions)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);

    napi_value catalog = nullptr;
    mockNapiPtr_->CreateObject(env_, &catalog);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "id", CreateStringArg("catalog-preserved-descriptors"));

    napi_value components = nullptr;
    mockNapiPtr_->CreateArrayWithLength(env_, 2, &components);
    napi_value componentItem = nullptr;
    mockNapiPtr_->CreateObject(env_, &componentItem);
    mockNapiPtr_->SetNamedProperty(env_, componentItem, "name", CreateStringArg("DynamicResolveProbe"));
    mockNapiPtr_->SetNamedProperty(
        env_, componentItem, "category", CreateInt32Arg(static_cast<int32_t>(CatalogCategory::A2UI_STANDARD)));
    mockNapiPtr_->SetNamedProperty(
        env_, componentItem, "type", CreateInt32Arg(static_cast<int32_t>(CatalogItemType::COMPONENT)));
    mockNapiPtr_->SetNamedProperty(env_, componentItem, "preserveDynamicDescriptors", CreateBoolArg(true));
    mockNapiPtr_->SetElement(env_, components, 0, componentItem);
    napi_value unnamedComponent = nullptr;
    mockNapiPtr_->CreateObject(env_, &unnamedComponent);
    mockNapiPtr_->SetElement(env_, components, 1, unnamedComponent);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "components", components);

    napi_value functions = nullptr;
    mockNapiPtr_->CreateArrayWithLength(env_, 2, &functions);
    napi_value functionItem = nullptr;
    mockNapiPtr_->CreateObject(env_, &functionItem);
    mockNapiPtr_->SetNamedProperty(env_, functionItem, "name", CreateStringArg("customFunction"));
    mockNapiPtr_->SetNamedProperty(
        env_, functionItem, "type", CreateInt32Arg(static_cast<int32_t>(CatalogItemType::LOCAL_FUNCTION)));
    mockNapiPtr_->SetElement(env_, functions, 0, functionItem);
    napi_value unnamedFunction = nullptr;
    mockNapiPtr_->CreateObject(env_, &unnamedFunction);
    mockNapiPtr_->SetElement(env_, functions, 1, unnamedFunction);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "functions", functions);

    const std::string createDsl = R"({"version":"v0.9","createSurface":{)"
                                  R"("surfaceId":"native-entry-surface-a",)"
                                  R"("catalogId":"catalog-preserved-descriptors"}})";
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog });
    ExpectProcessSuccess(ProcessMessage(env_, cbInfo_));

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    ASSERT_NE(surface, nullptr);
    std::shared_ptr<Catalog> parsedCatalog = surface->GetCatalog();
    ASSERT_NE(parsedCatalog, nullptr);
    std::shared_ptr<CatalogItem> parsedComponent = parsedCatalog->GetCatalogItemByName("DynamicResolveProbe");
    ASSERT_NE(parsedComponent, nullptr);
    EXPECT_TRUE(parsedComponent->ShouldPreserveDynamicDescriptors());
    EXPECT_FALSE(parsedComponent->IsInnerNative());
    EXPECT_EQ(parsedCatalog->GetComponents().size(), 1U);
    EXPECT_TRUE(parsedCatalog->HasFunction("customFunction"));
    EXPECT_EQ(parsedCatalog->GetFunctions().size(), 1U);
}

TEST_F(NativeEntryCoverageTest, should_ignore_non_array_catalog_collections)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);

    napi_value catalog = nullptr;
    mockNapiPtr_->CreateObject(env_, &catalog);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "id", CreateStringArg("catalog-non-array-collections"));
    napi_value components = nullptr;
    mockNapiPtr_->CreateObject(env_, &components);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "components", components);
    mockNapiPtr_->SetNamedProperty(env_, catalog, "functions", CreateStringArg("not-an-array"));

    const std::string createDsl = R"({"version":"v0.9","createSurface":{)"
                                  R"("surfaceId":"native-entry-surface-b",)"
                                  R"("catalogId":"catalog-non-array-collections"}})";
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog });
    ExpectProcessSuccess(ProcessMessage(env_, cbInfo_));

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_B);
    ASSERT_NE(surface, nullptr);
    ASSERT_NE(surface->GetCatalog(), nullptr);
    EXPECT_TRUE(surface->GetCatalog()->GetComponents().empty());
    EXPECT_TRUE(surface->GetCatalog()->GetFunctions().empty());
}

TEST_F(NativeEntryCoverageTest, should_reject_create_surface_when_catalog_id_is_missing_even_with_extended_catalog)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    napi_value catalog = CreateCatalogArg(A2UI_EXTENDED_CATALOG_ID, "Text", CatalogCategory::OHOS_EXTENDS, false);
    const std::string createDsl = R"({"version":"v0.9","createSurface":{"surfaceId":"native-entry-surface-a"}})";

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog });
    napi_value result = ProcessMessage(env_, cbInfo_);
    ExpectProcessFailure(result, "CATALOG_ID_MISSING");

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    EXPECT_EQ(surface, nullptr);

    napi_value surfaceResultCodeValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, result, "surfaceResultCode", &surfaceResultCodeValue);
    ASSERT_NE(surfaceResultCodeValue, nullptr);
    auto surfaceResultCodeIt = mockNapiPtr_->numberValues_.find(surfaceResultCodeValue);
    ASSERT_NE(surfaceResultCodeIt, mockNapiPtr_->numberValues_.end());
    EXPECT_EQ(static_cast<int32_t>(surfaceResultCodeIt->second), SURFACE_RESULT_SCHEMA_CATALOG_ID_MISSING);
}

TEST_F(NativeEntryCoverageTest, should_report_catalog_id_missing_before_extended_protocol_mismatch)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    napi_value catalog = CreateCatalogArg(A2UI_EXTENDED_CATALOG_ID, "Text", CatalogCategory::OHOS_EXTENDS, false);
    const std::string createDsl = R"({"version":"v0.9","createSurface":{"surfaceId":"native-entry-surface-a"}})";

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog,
        CreateProcessOptionsArg(true) });
    napi_value result = ProcessMessage(env_, cbInfo_);
    ExpectProcessFailure(result, "CATALOG_ID_MISSING");

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    EXPECT_EQ(surface, nullptr);

    napi_value surfaceResultCodeValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, result, "surfaceResultCode", &surfaceResultCodeValue);
    ASSERT_NE(surfaceResultCodeValue, nullptr);
    auto surfaceResultCodeIt = mockNapiPtr_->numberValues_.find(surfaceResultCodeValue);
    ASSERT_NE(surfaceResultCodeIt, mockNapiPtr_->numberValues_.end());
    EXPECT_EQ(static_cast<int32_t>(surfaceResultCodeIt->second), SURFACE_RESULT_SCHEMA_CATALOG_ID_MISSING);
}

TEST_F(NativeEntryCoverageTest, should_reject_extended_create_surface_when_controller_expects_standard_protocol)
{
    RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    napi_value catalog = CreateCatalogArg(A2UI_EXTENDED_CATALOG_ID, "Text", CatalogCategory::OHOS_EXTENDS, false);
    const std::string createDsl = R"({"version":"v0.9","createSurface":{)"
                                  R"("surfaceId":"native-entry-surface-a","catalogId":"ohos.a2ui.extended.catalog"}})";

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(createDsl), catalog,
        CreateProcessOptionsArg(false) });
    ExpectProcessFailure(ProcessMessage(env_, cbInfo_), "PROTOCOL_MISMATCH");

    SurfaceSlot* surface = RenderManager::GetInstance().FindSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    EXPECT_EQ(surface, nullptr);
}

TEST_F(NativeEntryCoverageTest, should_return_empty_result_when_set_root_fill_mode_args_are_invalid)
{
    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(SetRootFillMode(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_set_root_fill_mode_for_existing_render_slot)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    slot.GetSurfaceManager()->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateBoolArg(true) });
    EXPECT_EQ(SetRootFillMode(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_EQ(UnbindSurfaceFromRender(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_return_empty_arrays_for_missing_render_slot)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    napi_value ids = GetSurfaceIds(env_, cbInfo_);
    ASSERT_NE(ids, nullptr);
    uint32_t length = 99;
    mockNapiPtr_->GetArrayLength(env_, ids, &length);
    EXPECT_EQ(length, 0u);

    EXPECT_EQ(GetLatestSurfaceId(env_, cbInfo_), nullptr);

    napi_value popResult = PopSurface(env_, cbInfo_);
    ASSERT_NE(popResult, nullptr);
    napi_value successValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, popResult, "success", &successValue);
    ASSERT_NE(successValue, nullptr);
    EXPECT_FALSE(mockNapiPtr_->boolValues_[successValue]);
}

TEST_F(NativeEntryCoverageTest, should_pop_surface_and_return_latest_id_when_surface_exists)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_B);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    napi_value popResult = PopSurface(env_, cbInfo_);
    ASSERT_NE(popResult, nullptr);
    napi_value successValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, popResult, "success", &successValue);
    ASSERT_NE(successValue, nullptr);
    EXPECT_TRUE(mockNapiPtr_->boolValues_[successValue]);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_NE(GetLatestSurfaceId(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_validate_custom_component_checks_for_non_custom_component)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildTextCatalog());
    surface.UpdateComponents(
        JsonAdapter::Parse(R"({"components":[{"id":"text-1","component":"Text","content":"hello"}]})")->GetRoot());

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID),
        CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A), CreateStringArg("text-1"), CreateStringArg(R"({"value":"x"})") });
    napi_value result = ValidateCustomComponentChecks(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    napi_value validValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, result, "valid", &validValue);
    ASSERT_NE(validValue, nullptr);
    EXPECT_TRUE(mockNapiPtr_->boolValues_[validValue]);
}

TEST_F(NativeEntryCoverageTest, should_handle_register_locale_and_invalid_custom_component_registration)
{
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("en-US") });
    EXPECT_EQ(RegisterLocale(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({ CreateFunctionArg() });
    EXPECT_EQ(RegisterLocale(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(RegisterCreateCustomComponent(env_, cbInfo_), nullptr);
    EXPECT_EQ(RegisterUpdateCustomComponent(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_register_cross_language_attribute_callback_and_ignore_invalid_input)
{
    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(RegisterCrossLanguageAttributeCallback(env_, cbInfo_), nullptr);
    EXPECT_TRUE(mockNapiPtr_->refToValue_.empty());

    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("invalid") });
    EXPECT_EQ(RegisterCrossLanguageAttributeCallback(env_, cbInfo_), nullptr);
    EXPECT_TRUE(mockNapiPtr_->refToValue_.empty());

    mockNapiPtr_->SetCallbackArgs({ CreateFunctionArg() });
    EXPECT_EQ(RegisterCrossLanguageAttributeCallback(env_, cbInfo_), nullptr);
    EXPECT_FALSE(mockNapiPtr_->refToValue_.empty());
}

TEST_F(NativeEntryCoverageTest, should_return_latest_surface_id_and_surface_ids_when_render_slot_exists)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_B);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_NE(GetSurfaceIds(env_, cbInfo_), nullptr);
    EXPECT_NE(GetLatestSurfaceId(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_return_default_valid_result_when_custom_component_is_missing)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildTextCatalog());
    surface.SetRootComponent(CreatePlainComponent("plain-1"));

    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A),
            CreateStringArg("plain-1"), CreateStringArg(R"({"value":"x"})") });
    napi_value result = ValidateCustomComponentChecks(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    napi_value validValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, result, "valid", &validValue);
    ASSERT_NE(validValue, nullptr);
    EXPECT_TRUE(mockNapiPtr_->boolValues_[validValue]);
}

TEST_F(NativeEntryCoverageTest, should_clear_custom_component_dynamic_value_subscription_via_napi)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    auto dataModel = surface.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    std::unique_ptr<JsonAdapter> data = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(data, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto component = std::make_shared<CustomComponent>("DynamicResolveProbe", true);
    component->SetRenderId(NATIVE_ENTRY_TEST_RENDER_ID);
    component->SetSurfaceId(NATIVE_ENTRY_TEST_SURFACE_A);
    component->SetComponentId("dynamic-resolve-probe");
    surface.SetRootComponent(component);

    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({"path":"/user/name"})");
    ASSERT_NE(descriptor, nullptr);
    std::string errorMessage;
    ASSERT_TRUE(component->RegisterDynamicValueCallback(
        "value", descriptor->GetRoot(), env_, CreateFunctionArg(), &errorMessage));
    EXPECT_FALSE(mockNapiPtr_->refToValue_.empty());

    mockNapiPtr_->SetCallbackArgs(
        { CreateDoubleArg(static_cast<double>(component->GetCustomComponentHandle())), CreateStringArg("value") });
    napi_value result = ClearCustomComponentDynamicValue(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    napi_value successValue = nullptr;
    mockNapiPtr_->GetNamedProperty(env_, result, "success", &successValue);
    ASSERT_NE(successValue, nullptr);
    EXPECT_TRUE(mockNapiPtr_->boolValues_[successValue]);
    EXPECT_TRUE(mockNapiPtr_->refToValue_.empty());
}

TEST_F(NativeEntryCoverageTest, should_reject_invalid_clear_custom_component_dynamic_value_requests)
{
    mockNapiPtr_->SetCallbackArgs({});
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");

    mockNapiPtr_->SetCallbackArgs({ nullptr, CreateStringArg("value") });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");

    mockNapiPtr_->SetCallbackArgs({ CreateDoubleArg(1.0), nullptr });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");

    mockNapiPtr_->SetCallbackArgs({ CreateDoubleArg(1.0), CreateStringArg("value") });
    mockNapiPtr_->SetGetValueDoubleStatus(napi_number_expected);
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");
    mockNapiPtr_->ResetGetValueDoubleStatus();

    mockNapiPtr_->SetCallbackArgs(
        { CreateDoubleArg(std::numeric_limits<double>::quiet_NaN()), CreateStringArg("value") });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");

    mockNapiPtr_->SetCallbackArgs({ CreateDoubleArg(0.0), CreateStringArg("value") });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");

    mockNapiPtr_->SetCallbackArgs({ CreateDoubleArg(999999.0), CreateStringArg("value") });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "CUSTOM_COMPONENT_NOT_FOUND");

    CustomComponent component("DynamicResolveProbe", true);
    mockNapiPtr_->SetCallbackArgs(
        { CreateDoubleArg(static_cast<double>(component.GetCustomComponentHandle())), CreateStringArg("") });
    ExpectProcessFailure(ClearCustomComponentDynamicValue(env_, cbInfo_), "INVALID_ARGUMENT");
}

TEST_F(NativeEntryCoverageTest, should_execute_get_radio_value_for_extended_radio_component)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildExtendedCatalog({ "Radio" }));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "radio-1",
            "component": "Radio",
            "group": "grp1",
            "value": "opt1",
            "checked": true
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedRadioComponent>(surface.FindComponentById("radio-1")), nullptr);

    std::unique_ptr<JsonAdapter> args = JsonAdapter::Parse(R"({"group":"grp1"})");
    ASSERT_NE(args, nullptr);
    DynamicResolveContext context = {
        .renderId = NATIVE_ENTRY_TEST_RENDER_ID, .surfaceId = NATIVE_ENTRY_TEST_SURFACE_A, .componentId = "caller"
    };
    ResolvedValue functionResult =
        NativeFunctionRegistry::GetInstance().Execute("getRadioValue", args->GetRoot(), context, "string");
    ASSERT_TRUE(functionResult.success);
    EXPECT_EQ(functionResult.value.GetStringValue(""), "opt1");
}

TEST_F(NativeEntryCoverageTest, should_ignore_custom_component_action_for_extended_radio_component)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildExtendedCatalog({ "Radio" }));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "radio-1",
            "component": "Radio",
            "onClick": [{"call": "radioClicked"}]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));
    ASSERT_NE(std::dynamic_pointer_cast<ExtendedRadioComponent>(surface.FindComponentById("radio-1")), nullptr);

    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env_, CreateFunctionArg());
    mockNapiPtr_->callFunctionArgsHistory_.clear();
    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A),
            CreateStringArg("radio-1"), CreateStringArg("click"), CreateStringArg(R"({"x":7,"y":13})") });

    EXPECT_EQ(DispatchCustomComponentAction(env_, cbInfo_), nullptr);
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());
}

TEST_F(NativeEntryCoverageTest, should_ignore_custom_component_action_for_non_custom_component)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildTextCatalog());

    std::unique_ptr<JsonAdapter> message =
        JsonAdapter::Parse(R"({"components":[{"id":"text-1","component":"Text","content":"hello"}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    size_t dispatchCountBefore = mockNapiPtr_->callFunctionCallCount_;
    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A),
            CreateStringArg("text-1"), CreateStringArg("change"), CreateStringArg(R"({"checked":true})") });

    EXPECT_EQ(DispatchCustomComponentAction(env_, cbInfo_), nullptr);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, dispatchCountBefore);
}

TEST_F(NativeEntryCoverageTest, should_update_select_runtime_value_from_on_select_context)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildExtendedCatalog({ "Select" }));

    std::unique_ptr<JsonAdapter> message =
        JsonAdapter::Parse(R"({"components":[{"id":"sel-1","component":"Select"}]})");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    std::shared_ptr<CustomComponent> select =
        std::dynamic_pointer_cast<CustomComponent>(surface.FindComponentById("sel-1"));
    ASSERT_NE(select, nullptr);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID),
        CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A), CreateStringArg("sel-1"), CreateStringArg("onSelect"),
        CreateStringArg(R"({"index":2,"value":"Option C"})") });

    EXPECT_EQ(DispatchCustomComponentAction(env_, cbInfo_), nullptr);

    EXPECT_EQ(select->GetCustomProperty("selected").GetNumberValue(-1), 2);
    EXPECT_EQ(select->GetCustomProperty("value").GetStringValue(""), "Option C");
}

TEST_F(NativeEntryCoverageTest, should_dispatch_select_on_click_event_handler)
{
    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env_, CreateFunctionArg());
    RegisterBuiltInActions(NativeActionRegistry::GetInstance());

    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    SurfaceSlot& surface = manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);
    surface.SetCatalog(BuildExtendedCatalog({ "Select" }));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "sel-click",
            "component": "Select",
            "onClick": [{
                "call": "dispatchEvent",
                "args": {
                    "eventName": "selectClicked",
                    "context": { "scene": "select" }
                }
            }]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surface.UpdateComponents(message->GetRoot()));

    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateStringArg(NATIVE_ENTRY_TEST_SURFACE_A),
            CreateStringArg("sel-click"), CreateStringArg("onClick"), CreateStringArg(R"({"x":12,"y":34})") });

    EXPECT_EQ(DispatchCustomComponentAction(env_, cbInfo_), nullptr);

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_FALSE(mockNapiPtr_->lastCallFunctionArgs_.empty());
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringProperty(request, "name", ""), "selectClicked");
    EXPECT_EQ(GetStringProperty(request, "sourceComponentId", ""), "sel-click");

    napi_value dispatchedContext = nullptr;
    ASSERT_EQ(mockNapiPtr_->GetNamedProperty(env_, request, "context", &dispatchedContext), napi_ok);
    ASSERT_NE(dispatchedContext, nullptr);
    EXPECT_EQ(GetStringProperty(dispatchedContext, "scene", ""), "select");
    EXPECT_DOUBLE_EQ(GetNumberProperty(dispatchedContext, "x", 0.0), 12.0);
    EXPECT_DOUBLE_EQ(GetNumberProperty(dispatchedContext, "y", 0.0), 34.0);
}

TEST_F(NativeEntryCoverageTest, should_dispatch_event_handler_action_with_merged_event_context)
{
    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env_, CreateFunctionArg());
    RegisterBuiltInActions(NativeActionRegistry::GetInstance());

    std::unique_ptr<JsonAdapter> args = JsonAdapter::Parse(R"({
        "eventName": "extendedChanged",
        "context": {
            "scene": "manual",
            "value": "fromContext"
        }
    })");
    ASSERT_NE(args, nullptr);
    std::unique_ptr<JsonAdapter> eventContext = JsonAdapter::Parse(R"({
        "value": true,
        "status": "All"
    })");
    ASSERT_NE(eventContext, nullptr);

    EventHandlerChainExecutor::ExecutionContext context;
    context.renderId = NATIVE_ENTRY_TEST_RENDER_ID;
    context.surfaceId = NATIVE_ENTRY_TEST_SURFACE_A;
    context.componentId = "checkboxGroupA";
    context.eventContext = eventContext->GetRoot();

    NativeActionRegistry::GetInstance().Execute("dispatchEvent", args->GetRoot(), context);

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_FALSE(mockNapiPtr_->lastCallFunctionArgs_.empty());
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_.front();
    EXPECT_EQ(GetStringProperty(request, "name", ""), "extendedChanged");
    EXPECT_EQ(GetStringProperty(request, "sourceComponentId", ""), "checkboxGroupA");

    napi_value dispatchedContext = nullptr;
    ASSERT_EQ(mockNapiPtr_->GetNamedProperty(env_, request, "context", &dispatchedContext), napi_ok);
    ASSERT_NE(dispatchedContext, nullptr);
    EXPECT_EQ(GetStringProperty(dispatchedContext, "scene", ""), "manual");
    EXPECT_TRUE(GetBoolProperty(dispatchedContext, "value", false));
    EXPECT_EQ(GetStringProperty(dispatchedContext, "status", ""), "All");
}

TEST_F(NativeEntryCoverageTest, should_set_display_density_via_napi)
{
    DisplayDensityUtils::GetInstance().ClearDisplayDensity(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID);

    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID);
    mockNapiPtr_->SetCallbackArgs(
        { CreateInt32Arg(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID), CreateDoubleArg(2.5) });
    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    float vp = DisplayDensityUtils::GetInstance().ConvertPxToVp(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID, 100.0F);
    EXPECT_FLOAT_EQ(vp, 40.0F);

    RenderManager::GetInstance().RemoveRenderSlot(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID);
    DisplayDensityUtils::GetInstance().ClearDisplayDensity(NATIVE_ENTRY_DISPLAY_DENSITY_TEST_RENDER_ID);
}

TEST_F(NativeEntryCoverageTest, should_reject_set_display_density_with_insufficient_args)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID) });
    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(SetDisplayDensity(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_update_theme_mode_and_breakpoint_via_napi)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
    std::shared_ptr<SurfaceManager> manager = slot.GetSurfaceManager();
    ASSERT_NE(manager, nullptr);
    manager->CreateSurface(NATIVE_ENTRY_TEST_SURFACE_A);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateInt32Arg(1) });
    EXPECT_EQ(UpdateThemeMode(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateInt32Arg(3) });
    EXPECT_EQ(UpdateBreakpoint(env_, cbInfo_), nullptr);

    RenderManager::GetInstance().RemoveRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
}

TEST_F(NativeEntryCoverageTest, should_reject_update_theme_mode_and_breakpoint_with_invalid_args)
{
    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(UpdateThemeMode(env_, cbInfo_), nullptr);
    EXPECT_EQ(UpdateBreakpoint(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_reject_update_theme_mode_and_breakpoint_for_missing_render_slot)
{
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(999999), CreateInt32Arg(0) });
    EXPECT_EQ(UpdateThemeMode(env_, cbInfo_), nullptr);
    EXPECT_EQ(UpdateBreakpoint(env_, cbInfo_), nullptr);
}

TEST_F(NativeEntryCoverageTest, should_set_font_size_scale_via_napi)
{
    RenderSlot& slot = RenderManager::GetInstance().CreateRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateDoubleArg(1.5) });
    EXPECT_EQ(SetFontSizeScale(env_, cbInfo_), nullptr);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(SetFontSizeScale(env_, cbInfo_), nullptr);

    RenderManager::GetInstance().RemoveRenderSlot(NATIVE_ENTRY_TEST_RENDER_ID);
}

TEST_F(NativeEntryCoverageTest, should_set_api_version_via_napi)
{
    SystemProperties::GetInstance().SetApiVersion(0);
    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(NATIVE_ENTRY_TEST_RENDER_ID), CreateInt32Arg(12) });
    EXPECT_EQ(SetApiVersion(env_, cbInfo_), nullptr);
    EXPECT_EQ(SystemProperties::GetInstance().GetApiVersion(), 12);

    mockNapiPtr_->SetCallbackArgs({});
    EXPECT_EQ(SetApiVersion(env_, cbInfo_), nullptr);
}
