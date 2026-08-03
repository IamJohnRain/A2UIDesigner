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

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/CustomComponentFactory.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "functions/ActionDispatchBridge.h"
#include "functions/FunctionBridge.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog(std::initializer_list<const char*> componentNames = {})
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* name : componentNames) {
        if (name == nullptr || name[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(name, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

class TestableCustomComponent : public CustomComponent {
public:
    explicit TestableCustomComponent(const std::string& componentType) : CustomComponent(componentType) {}

    using CustomComponent::ApplyPrivateAttributes;
};

} // namespace

class RadioCustomRoutingTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-radio-routing");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

TEST_F(RadioCustomRoutingTest, L0_should_route_radio_to_custom_component_factory)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Radio"));

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "radio-root",
                "component": "Radio",
                "value": "opt1",
                "checked": false,
                "group": "grp1"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> component = slot_.FindComponentById("radio-root");
    ASSERT_NE(component, nullptr);

    std::shared_ptr<ExtendedRadioComponent> radioComp = std::dynamic_pointer_cast<ExtendedRadioComponent>(component);
    ASSERT_NE(radioComp, nullptr);
    EXPECT_EQ(radioComp->GetType(), "Radio");
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(component), nullptr);
}

TEST_F(RadioCustomRoutingTest, L0_should_keep_button_in_extended_factory)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Button"));

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "btn-root",
                "component": "Button",
                "label": "Click"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto btn = slot_.FindComponentById("btn-root");
    ASSERT_NE(btn, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedButtonComponent>(btn), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(btn), nullptr);
}

TEST_F(RadioCustomRoutingTest, L0_should_keep_text_in_extended_factory)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Text"));

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "txt-root",
                "component": "Text",
                "text": "Hello"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto txt = slot_.FindComponentById("txt-root");
    ASSERT_NE(txt, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedTextComponent>(txt), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(txt), nullptr);
}

TEST_F(RadioCustomRoutingTest, L0_should_keep_column_in_extended_factory)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_TRUE(factory.IsExtendedComponent("Column"));

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "col-root",
                "component": "Column"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto col = slot_.FindComponentById("col-root");
    ASSERT_NE(col, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedColumnComponent>(col), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(col), nullptr);
}

TEST_F(RadioCustomRoutingTest, L0_should_route_mixed_radio_and_button_correctly)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio", "Button" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "radio-1",
                "component": "Radio",
                "value": "v1",
                "group": "g1"
            },
            {
                "id": "btn-1",
                "component": "Button",
                "label": "OK"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto radio = slot_.FindComponentById("radio-1");
    ASSERT_NE(radio, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedRadioComponent>(radio), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(radio), nullptr);

    auto btn = slot_.FindComponentById("btn-1");
    ASSERT_NE(btn, nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedButtonComponent>(btn), nullptr);
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(btn), nullptr);
}

class CustomComponentListenerTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
    }
};

TEST_F(CustomComponentListenerTest, L0_should_parse_listeners_from_descriptor)
{
    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-1");
    component->SetSurfaceId("surface-ls-1");
    component->SetRenderId(1);

    auto descriptor = JsonAdapter::Parse(R"({
        "id": "radio-ls-1",
        "listeners": {
            "onChange": {
                "type": "event",
                "name": "radioChanged",
                "context": {}
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());
}

TEST_F(CustomComponentListenerTest, L0_should_handle_descriptor_without_listeners)
{
    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-2");
    component->SetSurfaceId("surface-ls-2");
    component->SetRenderId(2);

    auto descriptor = JsonAdapter::Parse(R"({ "id": "radio-ls-2" })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());
}

TEST_F(CustomComponentListenerTest, L0_should_dispatch_action_with_no_listeners_without_crash)
{
    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-3");
    component->SetSurfaceId("surface-ls-3");
    component->SetRenderId(3);

    auto descriptor = JsonAdapter::Parse(R"({ "id": "radio-ls-3" })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());

    auto context = JsonAdapter::Parse(R"({"checked": true})");
    ASSERT_NE(context, nullptr);
    component->DispatchEvent("onChange", context->GetRoot());
}

TEST_F(CustomComponentListenerTest, L0_should_dispatch_action_for_unknown_listener_without_crash)
{
    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-4");
    component->SetSurfaceId("surface-ls-4");
    component->SetRenderId(4);

    auto descriptor = JsonAdapter::Parse(R"({
        "id": "radio-ls-4",
        "listeners": {
            "onChange": {
                "type": "event",
                "name": "radioChanged",
                "context": {}
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());

    auto context = JsonAdapter::Parse(R"({"checked": true})");
    ASSERT_NE(context, nullptr);
    component->DispatchEvent("unknownEvent", context->GetRoot());
}

TEST_F(CustomComponentListenerTest, L0_should_dispatch_event_type_listener_via_action_dispatch_bridge)
{
    napi_env env = reinterpret_cast<napi_env>(0x100);
    napi_value callback = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env, "dispatch", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env, callback);

    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-5");
    component->SetSurfaceId("surface-ls-5");
    component->SetRenderId(5);

    auto descriptor = JsonAdapter::Parse(R"({
        "id": "radio-ls-5",
        "listeners": {
            "onChange": {
                "type": "event",
                "name": "radioChanged",
                "context": {}
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());

    auto context = JsonAdapter::Parse(R"({"checked": true})");
    ASSERT_NE(context, nullptr);
    component->DispatchEvent("onChange", context->GetRoot());

    ASSERT_GE(mockNapiPtr_->callFunctionCallCount_, 1u);
}

TEST_F(CustomComponentListenerTest, L0_should_dispatch_primitive_change_context_via_action_dispatch_bridge)
{
    napi_env env = reinterpret_cast<napi_env>(0x100);
    napi_value callback = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env, "dispatch", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    ActionDispatchBridge::GetInstance().RegisterDispatchAction(env, callback);

    auto component = std::make_shared<TestableCustomComponent>("Radio");
    component->SetComponentId("radio-ls-6");
    component->SetSurfaceId("surface-ls-6");
    component->SetRenderId(6);

    auto descriptor = JsonAdapter::Parse(R"({
        "id": "radio-ls-6",
        "listeners": {
            "onChange": {
                "type": "event",
                "name": "radioChanged",
                "context": {
                    "source": "dsl"
                }
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    component->ApplyPrivateAttributes(descriptor->GetRoot());

    auto context = JsonAdapter::CreateBool(true);
    ASSERT_NE(context, nullptr);
    component->DispatchEvent("onChange", context->GetRoot());

    ASSERT_GE(mockNapiPtr_->callFunctionCallCount_, 1u);
    ASSERT_FALSE(mockNapiPtr_->lastCallFunctionArgs_.empty());
    napi_value request = mockNapiPtr_->lastCallFunctionArgs_[0];
    ASSERT_NE(request, nullptr);
    napi_value contextValue = mockNapiPtr_->objectProperties_[request]["context"];
    EXPECT_TRUE(mockNapiPtr_->boolValues_[contextValue]);
}

class CustomComponentDataModelSyncTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-dm-sync");
        slot_.SetRenderId(10);
    }

    SurfaceSlot slot_;
};

TEST_F(CustomComponentDataModelSyncTest, L0_should_not_crash_with_empty_binding_path)
{
    auto component = std::make_shared<CustomComponent>("Radio");
    component->SetComponentId("radio-dm-1");
    component->SetSurfaceId("surface-dm-sync");
    component->SetRenderId(10);
    component->SyncCheckedToBoundDataModel("", true);
}

TEST_F(CustomComponentDataModelSyncTest, L0_should_not_crash_when_surface_not_found)
{
    auto component = std::make_shared<CustomComponent>("Radio");
    component->SetComponentId("radio-dm-2");
    component->SetSurfaceId("nonexistent-surface");
    component->SetRenderId(11);
    component->SyncCheckedToBoundDataModel("/radio/checked", true);
}

TEST_F(CustomComponentDataModelSyncTest, L0_should_write_back_checked_via_slot_update)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Radio" }));

    std::unique_ptr<JsonAdapter> dataModel = JsonAdapter::Parse(R"({
        "value": {
            "radio": {
                "checked": false
            }
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(slot_.UpdateDataModel(dataModel->GetRoot()));

    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "radio-dm-sync",
                "component": "Radio",
                "checked": {
                    "path": "/radio/checked"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto component = slot_.FindComponentById("radio-dm-sync");
    ASSERT_NE(component, nullptr);
    auto radioComp = std::dynamic_pointer_cast<ExtendedRadioComponent>(component);
    ASSERT_NE(radioComp, nullptr);

    std::shared_ptr<BindingEngine> bindingEngine = slot_.GetBindingEngine();
    ASSERT_NE(bindingEngine, nullptr);
    bindingEngine->UpdateDataModelByPath(slot_.GetSurfaceId(), "/radio/checked", JsonAdapter::Parse("true")->GetRoot());

    std::shared_ptr<DataModel> model = bindingEngine->GetOrCreateDataModel(slot_.GetSurfaceId());
    ASSERT_NE(model, nullptr);
    std::optional<JsonValue> checkedNode = model->GetNode("/radio/checked");
    ASSERT_TRUE(checkedNode.has_value());
    EXPECT_TRUE(checkedNode->GetBoolValue(false));
}
