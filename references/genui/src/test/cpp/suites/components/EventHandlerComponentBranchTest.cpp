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

#define private public
#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/actions/EventHandlerChainExecutor.h"
#include "components/actions/EventHandlerParser.h"
#include "components/actions/NativeActionRegistry.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "components/extended/ExtendedCheckboxGroupComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/ExtendedListComponent.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "components/extended/ExtendedTextInputComponent.h"
#include "components/extended/ExtendedToggleComponent.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"
#undef private

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedCatalog(std::initializer_list<const char*> names)
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* name : names) {
        auto item = std::make_shared<CatalogItem>(name, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

} // namespace

class EventHandlerComponentBranchTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-event-handler-branch");
        slot_.SetRenderId(100);
    }

    void TearDown() override
    {
        NativeActionRegistry::GetInstance().Clear();
        capturedContextAdapter_.reset();
        capturedEventContext_ = JsonValue();
        A2UITest::TearDown();
    }

    void RegisterCaptureEventContextAction()
    {
        NativeActionRegistry::GetInstance().Register(
            "captureEventContext", [this](const JsonValue& args, EventHandlerChainExecutor::ExecutionContext& ctx) {
                static_cast<void>(args);
                capturedContextAdapter_ = JsonAdapter::Clone(ctx.eventContext);
                capturedEventContext_ =
                    capturedContextAdapter_ != nullptr ? capturedContextAdapter_->GetRoot() : JsonValue();
                ++captureCount_;
                return JsonValue();
            });
    }

    SurfaceSlot slot_;
    std::unique_ptr<JsonAdapter> capturedContextAdapter_;
    JsonValue capturedEventContext_;
    int32_t captureCount_ = 0;
};

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_button)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn1",
            "component": "Button",
            "content": "Click",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto btn = slot_.FindComponentById("btn1");
    ASSERT_NE(btn, nullptr);
    auto extBtn = std::dynamic_pointer_cast<ExtendedButtonComponent>(btn);
    ASSERT_NE(extBtn, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_textinput)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "ti1",
            "component": "TextInput",
            "text": "hello",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto ti = slot_.FindComponentById("ti1");
    ASSERT_NE(ti, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onChange_handler_for_textinput)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "ti2",
            "component": "TextInput",
            "text": "hello",
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto ti = slot_.FindComponentById("ti2");
    ASSERT_NE(ti, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_toggle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "tog1",
            "component": "Toggle",
            "isOn": false,
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto tog = slot_.FindComponentById("tog1");
    ASSERT_NE(tog, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onChange_handler_for_toggle)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "tog2",
            "component": "Toggle",
            "isOn": false,
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto tog = slot_.FindComponentById("tog2");
    ASSERT_NE(tog, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_radio)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "rad1",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto rad = slot_.FindComponentById("rad1");
    ASSERT_NE(rad, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onChange_handler_for_radio)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "rad2",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1",
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto rad = slot_.FindComponentById("rad2");
    ASSERT_NE(rad, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_checkbox)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb1",
            "component": "Checkbox",
            "select": false,
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = slot_.FindComponentById("cb1");
    ASSERT_NE(cb, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onChange_handler_for_checkbox)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb2",
            "component": "Checkbox",
            "select": false,
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = slot_.FindComponentById("cb2");
    ASSERT_NE(cb, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onChange_handler_for_checkboxgroup)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "CheckboxGroup", "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cbg1",
            "component": "CheckboxGroup",
            "group": "grp1",
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = slot_.FindComponentById("cbg1");
    ASSERT_NE(cbg, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onReachStart_handler_for_list)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "List" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "list1",
            "component": "List",
            "onReachStart": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto list = slot_.FindComponentById("list1");
    ASSERT_NE(list, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onReachEnd_handler_for_list)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "List" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "list2",
            "component": "List",
            "onReachEnd": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto list = slot_.FindComponentById("list2");
    ASSERT_NE(list, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_text)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "txt1",
            "component": "Text",
            "content": "Hello",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto txt = slot_.FindComponentById("txt1");
    ASSERT_NE(txt, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onAppear_handler_for_text)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Text" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "txt2",
            "component": "Text",
            "content": "Hello",
            "onAppear": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto txt = slot_.FindComponentById("txt2");
    ASSERT_NE(txt, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_textinput_without_event_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "ti3",
            "component": "TextInput",
            "text": "hello"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto ti = slot_.FindComponentById("ti3");
    ASSERT_NE(ti, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_toggle_without_event_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "tog3",
            "component": "Toggle",
            "isOn": false
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto tog = slot_.FindComponentById("tog3");
    ASSERT_NE(tog, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_radio_without_event_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "rad3",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto rad = slot_.FindComponentById("rad3");
    ASSERT_NE(rad, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_list_without_event_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "List" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "list3",
            "component": "List"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto list = slot_.FindComponentById("list3");
    ASSERT_NE(list, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_toggle_change_event)
{
    RegisterCaptureEventContextAction();
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "tog4",
            "component": "Toggle",
            "isOn": false,
            "onChange": [
                { "call": "captureEventContext", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto tog = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("tog4"));
    ASSERT_NE(tog, nullptr);
    ASSERT_NE(tog->GetToggleNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, tog->GetToggleNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TOGGLE_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(tog->GetToggleNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(captureCount_, 1);
    ASSERT_TRUE(capturedEventContext_.IsObject());
    JsonValue eventData = capturedEventContext_.GetItem("eventData");
    ASSERT_TRUE(eventData.IsObject());
    EXPECT_TRUE(eventData.GetBool("isOn", false));
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_toggle_same_value_skip)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Toggle" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "tog5",
            "component": "Toggle",
            "isOn": true,
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto tog = std::dynamic_pointer_cast<ExtendedToggleComponent>(slot_.FindComponentById("tog5"));
    ASSERT_NE(tog, nullptr);
    ASSERT_NE(tog->GetToggleNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, tog->GetToggleNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TOGGLE_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(tog->GetToggleNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_textinput_change_event)
{
    RegisterCaptureEventContextAction();
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "ti4",
            "component": "TextInput",
            "text": "",
            "onChange": [
                { "call": "captureEventContext", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("ti4"));
    ASSERT_NE(ti, nullptr);
    ASSERT_NE(ti->GetNativeView(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, ti->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TEXT_INPUT_ON_CHANGE);

    ArkUI_StringAsyncEvent stringEvent = {};
    std::string newValue = "newText";
    stringEvent.pStr = newValue.data();
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(ti->GetNativeView(), &fakeEvent);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(captureCount_, 1);
    ASSERT_TRUE(capturedEventContext_.IsObject());
    JsonValue eventData = capturedEventContext_.GetItem("eventData");
    ASSERT_TRUE(eventData.IsObject());
    EXPECT_EQ(eventData.GetString("value", ""), "newText");
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_textinput_same_value_skip)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "TextInput" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "ti5",
            "component": "TextInput",
            "text": "hello",
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto ti = std::dynamic_pointer_cast<ExtendedTextInputComponent>(slot_.FindComponentById("ti5"));
    ASSERT_NE(ti, nullptr);
    ASSERT_NE(ti->GetNativeView(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, ti->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_TEXT_INPUT_ON_CHANGE);

    ArkUI_StringAsyncEvent stringEvent = {};
    std::string sameValue = "hello";
    stringEvent.pStr = sameValue.data();
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(ti->GetNativeView(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_radio_change_event)
{
    RegisterCaptureEventContextAction();
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "rad4",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1",
            "onChange": [
                { "call": "captureEventContext", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto rad = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("rad4"));
    ASSERT_NE(rad, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, rad->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_RADIO_EVENT_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(rad->GetNativeView(), &fakeEvent);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(captureCount_, 1);
    ASSERT_TRUE(capturedEventContext_.IsObject());
    JsonValue eventData = capturedEventContext_.GetItem("eventData");
    ASSERT_TRUE(eventData.IsObject());
    EXPECT_TRUE(eventData.GetBool("isChecked", false));
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_radio_same_value_skip)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Radio" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "rad5",
            "component": "Radio",
            "value": "opt1",
            "group": "grp1",
            "checked": true,
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto rad = std::dynamic_pointer_cast<ExtendedRadioComponent>(slot_.FindComponentById("rad5"));
    ASSERT_NE(rad, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, rad->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_RADIO_EVENT_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(rad->GetNativeView(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_button_onClick_event)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn2",
            "component": "Button",
            "content": "Click",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto btn = slot_.FindComponentById("btn2");
    ASSERT_NE(btn, nullptr);
    ASSERT_NE(btn->GetNativeView(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, btn->GetNativeView());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_ON_CLICK);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].f32 = 10.0f;
    componentEvent.data[1].f32 = 20.0f;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(btn->GetNativeView(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_button_with_action_instead_of_event_handler)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn3",
            "component": "Button",
            "content": "Action",
            "action": {
                "type": "function",
                "call": "echo",
                "args": {},
                "returnType": "void"
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto btn = slot_.FindComponentById("btn3");
    ASSERT_NE(btn, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_button_without_click_handler_or_action)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Button" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "btn4",
            "component": "Button",
            "content": "NoAction"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto btn = slot_.FindComponentById("btn4");
    ASSERT_NE(btn, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_onClick_handler_for_checkboxgroup)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cbg2",
            "component": "CheckboxGroup",
            "group": "grp1",
            "onClick": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = slot_.FindComponentById("cbg2");
    ASSERT_NE(cbg, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_render_checkboxgroup_without_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cbg3",
            "component": "CheckboxGroup",
            "group": "grp1"
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = slot_.FindComponentById("cbg3");
    ASSERT_NE(cbg, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_checkbox_change_with_event_handler)
{
    RegisterCaptureEventContextAction();
    slot_.SetCatalog(BuildExtendedCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb3",
            "component": "Checkbox",
            "select": false,
            "onChange": [
                { "call": "captureEventContext", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("cb3"));
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(cb->GetCheckboxNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(captureCount_, 1);
    ASSERT_TRUE(capturedEventContext_.IsObject());
    JsonValue eventData = capturedEventContext_.GetItem("eventData");
    ASSERT_TRUE(eventData.IsObject());
    EXPECT_TRUE(eventData.GetBool("value", false));
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_checkbox_same_value_skip)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "Checkbox" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cb4",
            "component": "Checkbox",
            "select": true,
            "onChange": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("cb4"));
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(cb->GetCheckboxNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(&fakeEvent, &componentEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_parse_list_with_both_reach_handlers)
{
    slot_.SetCatalog(BuildExtendedCatalog({ "List" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "list4",
            "component": "List",
            "onReachStart": [
                { "call": "echo", "args": {} }
            ],
            "onReachEnd": [
                { "call": "echo", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto list = slot_.FindComponentById("list4");
    ASSERT_NE(list, nullptr);
}

TEST_F(EventHandlerComponentBranchTest, L0_should_dispatch_checkboxgroup_change_event)
{
    RegisterCaptureEventContextAction();
    slot_.SetCatalog(BuildExtendedCatalog({ "CheckboxGroup" }));
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "cbg4",
            "component": "CheckboxGroup",
            "group": "grp1",
            "onChange": [
                { "call": "captureEventContext", "args": {} }
            ]
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cbg = std::dynamic_pointer_cast<ExtendedCheckboxGroupComponent>(slot_.FindComponentById("cbg4"));
    ASSERT_NE(cbg, nullptr);
    ASSERT_NE(cbg->GetCheckboxGroupNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cbg->GetCheckboxGroupNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_GROUP_EVENT_ON_CHANGE);

    ArkUI_StringAsyncEvent stringEvent = {};
    std::string eventValue = "Name:first,second;Status:1";
    stringEvent.pStr = eventValue.data();
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(&fakeEvent, &stringEvent);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cbg->GetCheckboxGroupNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(captureCount_, 1);
    ASSERT_TRUE(capturedEventContext_.IsObject());
    JsonValue eventData = capturedEventContext_.GetItem("eventData");
    ASSERT_TRUE(eventData.IsObject());
    JsonValue value = eventData.GetItem("value");
    ASSERT_TRUE(value.IsArray());
    ASSERT_EQ(value.GetArraySize(), 2);
    EXPECT_EQ(value.GetArrayItem(0).GetStringValue(""), "first");
    EXPECT_EQ(value.GetArrayItem(1).GetStringValue(""), "second");
    EXPECT_EQ(eventData.GetString("status", ""), "Part");
}
