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
#include "components/extended/ExtendedCheckboxComponent.h"
#include "utils/JsonAdapter.h"

#include "SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCheckboxCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto item = std::make_shared<CatalogItem>("Checkbox", CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(item);
    return catalog;
}

} // namespace

class DispatchActionInfoTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-dispatch-test");
        slot_.SetRenderId(42);
    }

    SurfaceSlot slot_;
};

TEST_F(DispatchActionInfoTest, L0_should_enter_native_short_circuit_when_native_function_exists)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "value": false,
            "listeners": {
                "onChange": {
                    "type": "function",
                    "call": "required",
                    "args": {"value": "test"},
                    "returnType": "boolean"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(cb->GetCheckboxNodeForTest(), nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(DispatchActionInfoTest, L0_should_fallback_to_bridge_when_native_function_not_found)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "value": false,
            "listeners": {
                "onChange": {
                    "type": "function",
                    "call": "unknownFunction",
                    "args": {},
                    "returnType": "void"
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}

TEST_F(DispatchActionInfoTest, L0_should_dispatch_event_type_action)
{
    slot_.SetCatalog(BuildCheckboxCatalog());
    auto message = JsonAdapter::Parse(R"({
        "components": [{
            "id": "root",
            "component": "Checkbox",
            "value": false,
            "listeners": {
                "onChange": {
                    "type": "event",
                    "name": "checkboxChanged",
                    "context": {"key": {"value": "test"}}
                }
            }
        }]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    auto cb = std::dynamic_pointer_cast<ExtendedCheckboxComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(cb, nullptr);

    ArkUI_NodeEvent fakeEvent = {};
    mockArkUIPtr_->SetNodeEventHandle(&fakeEvent, cb->GetCheckboxNodeForTest());
    mockArkUIPtr_->SetNodeEventType(&fakeEvent, NODE_CHECKBOX_EVENT_ON_CHANGE);

    bool dispatched = mockArkUIPtr_->DispatchNodeEvent(cb->GetCheckboxNodeForTest(), &fakeEvent);
    EXPECT_TRUE(dispatched);
}
