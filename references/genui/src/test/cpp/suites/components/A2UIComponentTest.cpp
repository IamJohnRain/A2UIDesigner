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
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#define private public
#define protected public
#include "components/A2UI/A2UIComponent.h"
#include "components/Component.h"
#undef protected
#undef private
#include "utils/JsonAdapter.h"

#include "ArkUINativeAPI.h"
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
    int32_t setUserDataCount = 0;
    int32_t addNodeEventReceiverCount = 0;
    int32_t removeNodeEventReceiverCount = 0;
    int32_t registerNodeEventCount = 0;
    int32_t unregisterNodeEventCount = 0;
    int32_t insertChildAtCount = 0;
    int32_t setAttributeCount = 0;
    ArkUI_NodeEventCallback nodeEventReceiver = nullptr;
    std::map<ArkUI_NodeHandle, void*> userData;
    std::vector<NativeAttributeCall> attributeCalls;
    std::vector<std::tuple<ArkUI_NodeHandle, ArkUI_NodeHandle, int32_t>> insertCalls;
    std::vector<int32_t> registeredEventTypes;
    std::vector<int32_t> unregisteredEventTypes;
};

NativeCallTracker g_tracker;

void ResetTracker()
{
    g_tracker = NativeCallTracker {};
}

int32_t TrackSetUserData(ArkUI_NodeHandle node, void* userData)
{
    ++g_tracker.setUserDataCount;
    g_tracker.userData[node] = userData;
    return 0;
}

void* TrackGetUserData(ArkUI_NodeHandle node)
{
    auto it = g_tracker.userData.find(node);
    return it == g_tracker.userData.end() ? nullptr : it->second;
}

int32_t TrackAddNodeEventReceiver(ArkUI_NodeHandle, ArkUI_NodeEventCallback callback)
{
    ++g_tracker.addNodeEventReceiverCount;
    g_tracker.nodeEventReceiver = callback;
    return 0;
}

int32_t TrackRemoveNodeEventReceiver(ArkUI_NodeHandle, ArkUI_NodeEventCallback)
{
    ++g_tracker.removeNodeEventReceiverCount;
    return 0;
}

int32_t TrackRegisterNodeEvent(ArkUI_NodeHandle, int32_t eventType, int32_t, void*)
{
    ++g_tracker.registerNodeEventCount;
    g_tracker.registeredEventTypes.push_back(eventType);
    return 0;
}

int32_t TrackUnregisterNodeEvent(ArkUI_NodeHandle, int32_t eventType)
{
    ++g_tracker.unregisterNodeEventCount;
    g_tracker.unregisteredEventTypes.push_back(eventType);
    return 0;
}

int32_t TrackInsertChildAt(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    ++g_tracker.insertChildAtCount;
    g_tracker.insertCalls.emplace_back(parent, child, index);
    return 0;
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

class BasicChildComponent : public Component {
public:
    explicit BasicChildComponent(ArkUI_NodeHandle handle) : Component(handle, false) {}
};

class A2UIComponentProbe : public A2UIComponent {
public:
    explicit A2UIComponentProbe(ArkUI_NodeHandle nativeView, bool ownsNativeView = true, bool isCompositeType = false)
        : A2UIComponent(nativeView, ownsNativeView, isCompositeType)
    {}

    void InvokeOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void InvokeOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void InvokeApplyCommonAttributes(const JsonValue& descriptor)
    {
        ApplyCommonAttributes(descriptor);
    }

    bool InvokeIsKnownAdditionalDescriptorKey(const std::string& propertyName) const
    {
        return IsKnownAdditionalDescriptorKey(propertyName);
    }
};

class A2UIComponentTest : public A2UITest {
protected:
    ArkUI_NativeNodeAPI_1* api_ = nullptr;
    ArkUI_NativeNodeAPI_1 savedApi_ {};

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetTracker();

        api_ = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
        ASSERT_NE(api_, nullptr);
        savedApi_ = *api_;

        api_->setUserData = TrackSetUserData;
        api_->getUserData = TrackGetUserData;
        api_->addNodeEventReceiver = TrackAddNodeEventReceiver;
        api_->removeNodeEventReceiver = TrackRemoveNodeEventReceiver;
        api_->registerNodeEvent = TrackRegisterNodeEvent;
        api_->unregisterNodeEvent = TrackUnregisterNodeEvent;
        api_->insertChildAt = TrackInsertChildAt;
        api_->setAttribute = TrackSetAttribute;
    }

    void TearDown() override
    {
        if (api_ != nullptr) {
            *api_ = savedApi_;
        }
        ResetTracker();
        A2UITest::TearDown();
    }
};

TEST_F(A2UIComponentTest, should_register_and_unregister_node_receiver_in_lifecycle)
{
    ArkUI_NodeHandle handle = reinterpret_cast<ArkUI_NodeHandle>(0x1001);
    {
        auto component = std::make_shared<A2UIComponentProbe>(handle, false);
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(g_tracker.setUserDataCount, 1);
        EXPECT_EQ(g_tracker.addNodeEventReceiverCount, 1);
        auto it = g_tracker.userData.find(handle);
        ASSERT_NE(it, g_tracker.userData.end());
        EXPECT_EQ(it->second, component.get());
    }

    EXPECT_EQ(g_tracker.removeNodeEventReceiverCount, 1);
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 0);
}

TEST_F(A2UIComponentTest, should_register_and_unregister_click_event_as_handlers_change)
{
    ArkUI_NodeHandle handle = reinterpret_cast<ArkUI_NodeHandle>(0x1002);
    auto component = std::make_shared<A2UIComponentProbe>(handle, false);
    ASSERT_NE(component, nullptr);

    component->RegisterOnClick([]() {});
    EXPECT_EQ(g_tracker.registerNodeEventCount, 1);
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 0);

    component->RegisterOnClick([]() {});
    component->SetAuxiliaryOnClick("", []() {});
    component->SetAuxiliaryOnClick("aux", []() {});
    EXPECT_EQ(g_tracker.registerNodeEventCount, 1);

    component->SetAuxiliaryOnClick("aux", nullptr);
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 0);

    component->RegisterOnClick(nullptr);
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 1);

    component->SetAuxiliaryOnClick("aux", []() {});
    EXPECT_EQ(g_tracker.registerNodeEventCount, 2);

    component->RemoveAuxiliaryOnClick("");
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 1);

    component->RemoveAuxiliaryOnClick("aux");
    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 2);
}

TEST_F(A2UIComponentTest, should_dispatch_click_event_to_primary_and_auxiliary_handlers)
{
    ArkUI_NodeHandle handle = reinterpret_cast<ArkUI_NodeHandle>(0x1003);
    auto component = std::make_shared<A2UIComponentProbe>(handle, false);
    ASSERT_NE(component, nullptr);
    ASSERT_NE(g_tracker.nodeEventReceiver, nullptr);

    int32_t primaryCount = 0;
    int32_t auxiliaryCount = 0;
    component->RegisterOnClick([&primaryCount]() { ++primaryCount; });
    component->SetAuxiliaryOnClick("extra", [&auxiliaryCount]() { ++auxiliaryCount; });

    ArkUI_NodeEvent event {};
    mockArkUIPtr_->SetNodeEventHandle(&event, handle);
    mockArkUIPtr_->SetNodeEventType(&event, NODE_ON_CLICK);
    g_tracker.nodeEventReceiver(&event);
    EXPECT_EQ(primaryCount, 1);
    EXPECT_EQ(auxiliaryCount, 1);

    mockArkUIPtr_->SetNodeEventType(&event, NODE_TEXT_INPUT_ON_CHANGE);
    g_tracker.nodeEventReceiver(&event);
    EXPECT_EQ(primaryCount, 1);
    EXPECT_EQ(auxiliaryCount, 1);
}

TEST_F(A2UIComponentTest, should_ignore_event_when_user_data_not_found)
{
    auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1004), false);
    ASSERT_NE(component, nullptr);
    ASSERT_NE(g_tracker.nodeEventReceiver, nullptr);

    int32_t calledCount = 0;
    component->RegisterOnClick([&calledCount]() { ++calledCount; });

    ArkUI_NodeEvent event {};
    mockArkUIPtr_->SetNodeEventHandle(&event, reinterpret_cast<ArkUI_NodeHandle>(0xDEAD));
    mockArkUIPtr_->SetNodeEventType(&event, NODE_ON_CLICK);
    g_tracker.nodeEventReceiver(&event);

    EXPECT_EQ(calledCount, 0);
}

TEST_F(A2UIComponentTest, should_insert_child_only_when_parent_and_child_views_are_valid)
{
    auto parent = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1005), false);
    ASSERT_NE(parent, nullptr);

    auto validChild = std::make_shared<BasicChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x2001));
    auto invalidChild = std::make_shared<BasicChildComponent>(nullptr);

    parent->InvokeOnAddChild(nullptr, 0);
    parent->InvokeOnAddChild(invalidChild, 0);
    EXPECT_EQ(g_tracker.insertChildAtCount, 0);

    parent->InvokeOnAddChild(validChild, 2);
    EXPECT_EQ(g_tracker.insertChildAtCount, 1);
    ASSERT_FALSE(g_tracker.insertCalls.empty());
    EXPECT_EQ(std::get<2>(g_tracker.insertCalls.back()), 2);

    parent->InvokeOnMoveChild(validChild, 0, 4);
    EXPECT_EQ(g_tracker.insertChildAtCount, 2);
    EXPECT_EQ(std::get<2>(g_tracker.insertCalls.back()), 4);

    auto nullParent = std::make_shared<A2UIComponentProbe>(nullptr, false);
    nullParent->InvokeOnAddChild(validChild, 1);
    EXPECT_EQ(g_tracker.insertChildAtCount, 2);
}

TEST_F(A2UIComponentTest, should_apply_a2ui_common_attributes_when_descriptor_contains_supported_fields)
{
    auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1006), false);
    ASSERT_NE(component, nullptr);

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(
        R"({
            "widthPercent": 0.5,
            "heightPercent": 0.25,
            "backgroundColor": 255,
            "paddingTop": 1,
            "paddingLeft": 4,
            "marginRight": 8,
            "borderRadius": 6
        })");
    ASSERT_NE(adapter, nullptr);

    component->InvokeApplyCommonAttributes(adapter->GetRoot());

    EXPECT_EQ(CountAttributeCall(NODE_WIDTH_PERCENT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_HEIGHT_PERCENT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_BACKGROUND_COLOR), 1);
    EXPECT_EQ(CountAttributeCall(NODE_PADDING), 1);
    EXPECT_EQ(CountAttributeCall(NODE_MARGIN), 1);
    EXPECT_EQ(CountAttributeCall(NODE_BORDER_RADIUS), 1);
}

TEST_F(A2UIComponentTest, should_apply_percent_margin_radius_and_border_width_attributes)
{
    auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1010), false);
    ASSERT_NE(component, nullptr);

    component->SetMarginPercent(0.1F, 0.2F, 0.3F, 0.4F);
    component->SetBorderRadiusPercent(10.0F, 20.0F, 30.0F, 40.0F);
    component->SetBorderWidthPercent(0.5F);

    EXPECT_EQ(CountAttributeCall(NODE_MARGIN_PERCENT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_BORDER_RADIUS_PERCENT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_BORDER_WIDTH_PERCENT), 1);
}

TEST_F(A2UIComponentTest, should_skip_background_color_when_value_is_zero)
{
    auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1007), false);
    ASSERT_NE(component, nullptr);

    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(R"({"backgroundColor":0})");
    ASSERT_NE(adapter, nullptr);
    component->InvokeApplyCommonAttributes(adapter->GetRoot());

    EXPECT_EQ(CountAttributeCall(NODE_BACKGROUND_COLOR), 0);
}

TEST_F(A2UIComponentTest, should_not_treat_a2ui_common_attributes_as_additional_descriptor_keys)
{
    auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1008), false);
    ASSERT_NE(component, nullptr);

    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("widthPercent"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("heightPercent"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("backgroundColor"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("paddingTop"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("marginLeft"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("borderRadius"));
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("unsupported"));
}

TEST_F(A2UIComponentTest, should_unregister_click_event_on_destroy_when_registered)
{
    {
        auto component = std::make_shared<A2UIComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x1009), false);
        ASSERT_NE(component, nullptr);
        component->RegisterOnClick([]() {});
        EXPECT_EQ(g_tracker.registerNodeEventCount, 1);
        EXPECT_EQ(g_tracker.unregisterNodeEventCount, 0);
    }

    EXPECT_EQ(g_tracker.unregisterNodeEventCount, 1);
    EXPECT_EQ(g_tracker.removeNodeEventReceiverCount, 1);
}

TEST_F(A2UIComponentTest, should_register_dispatch_and_unregister_appear_event)
{
    ArkUI_NodeHandle handle = reinterpret_cast<ArkUI_NodeHandle>(0x1011);
    auto component = std::make_shared<A2UIComponentProbe>(handle, false);
    ASSERT_NE(component, nullptr);
    ASSERT_NE(g_tracker.nodeEventReceiver, nullptr);

    int32_t appearCount = 0;
    component->RegisterNodeEventHandler(NODE_EVENT_ON_APPEAR, [&appearCount]() { ++appearCount; });

    ASSERT_EQ(g_tracker.registeredEventTypes.size(), 1u);
    EXPECT_EQ(g_tracker.registeredEventTypes.back(), NODE_EVENT_ON_APPEAR);

    ArkUI_NodeEvent event {};
    mockArkUIPtr_->SetNodeEventHandle(&event, handle);
    mockArkUIPtr_->SetNodeEventType(&event, NODE_EVENT_ON_APPEAR);
    g_tracker.nodeEventReceiver(&event);
    EXPECT_EQ(appearCount, 1);

    mockArkUIPtr_->SetNodeEventType(&event, NODE_ON_CLICK_EVENT);
    g_tracker.nodeEventReceiver(&event);
    EXPECT_EQ(appearCount, 1);

    component->RegisterNodeEventHandler(NODE_EVENT_ON_APPEAR, nullptr);
    ASSERT_EQ(g_tracker.unregisteredEventTypes.size(), 1u);
    EXPECT_EQ(g_tracker.unregisteredEventTypes.back(), NODE_EVENT_ON_APPEAR);
}

} // namespace
