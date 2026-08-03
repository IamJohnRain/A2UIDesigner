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

#ifndef A2UI_COMPONENT_TDD_TEST_HELPER_H
#define A2UI_COMPONENT_TDD_TEST_HELPER_H

#include <arkui/native_dialog.h>
#include <arkui/native_node.h>

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "components/Component.h"
#include "utils/JsonAdapter.h"

#include "ArkUINativeAPI.h"
#include "ArkUINodeApiAdapter.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "TestFixture.h"

namespace NativeModule {
namespace {

constexpr int32_t COMPONENT_TDD_RENDER_ID = 812101;
constexpr char COMPONENT_TDD_SURFACE_ID[] = "a2ui-component-tdd-test";
constexpr uintptr_t GENERATED_NODE_HANDLE_BASE = 0x8100U;
constexpr uintptr_t GENERATED_DIALOG_HANDLE_BASE = 0x9100U;

struct NativeAttributeCall {
    ArkUI_NodeHandle node = nullptr;
    int32_t attribute = -1;
    int32_t size = 0;
    std::vector<ArkUI_NumberValue> values;
    std::string stringValue;
    ArkUI_NodeAdapterHandle adapterHandle = nullptr;
};

struct NativeCreateNodeCall {
    ArkUI_NodeType type = 0;
    ArkUI_NodeHandle node = nullptr;
};

struct NativeDialogDismissRegistration {
    ArkUI_NativeDialogHandle handle = nullptr;
    void* userData = nullptr;
    void (*callback)(ArkUI_DialogDismissEvent*) = nullptr;
};

struct NativeCallTracker {
    std::vector<NativeCreateNodeCall> createNodeCalls;
    std::vector<NativeAttributeCall> attributeCalls;
    std::vector<std::pair<ArkUI_NodeHandle, ArkUI_NodeHandle>> addChildCalls;
    std::vector<std::pair<ArkUI_NodeHandle, ArkUI_NodeHandle>> removeChildCalls;
    std::vector<std::tuple<ArkUI_NodeHandle, ArkUI_NodeHandle, int32_t>> insertChildAtCalls;
    std::vector<std::pair<ArkUI_NodeHandle, int32_t>> resetAttributeCalls;
    std::vector<std::pair<ArkUI_NodeHandle, int32_t>> registerNodeEventCalls;
    std::vector<std::pair<ArkUI_NodeHandle, int32_t>> unregisterNodeEventCalls;
    std::vector<ArkUI_NodeHandle> addNodeEventReceiverCalls;
    std::vector<ArkUI_NodeHandle> removeNodeEventReceiverCalls;
    std::vector<ArkUI_NodeHandle> disposeNodeCalls;
    std::map<ArkUI_NodeHandle, void*> userData;
    int32_t setUserDataCount = 0;
    int32_t addNodeEventReceiverCount = 0;
    int32_t removeNodeEventReceiverCount = 0;
    int32_t unregisterNodeEventCount = 0;
    int32_t disposeNodeCount = 0;
};

struct NativeDialogTracker {
    std::vector<ArkUI_NativeDialogHandle> createCalls;
    std::vector<ArkUI_NativeDialogHandle> disposeCalls;
    std::vector<std::pair<ArkUI_NativeDialogHandle, ArkUI_NodeHandle>> setContentCalls;
    std::vector<std::tuple<ArkUI_NativeDialogHandle, ArkUI_DialogAlignment, float, float>> alignmentCalls;
    std::vector<std::pair<ArkUI_NativeDialogHandle, bool>> modalModeCalls;
    std::vector<std::pair<ArkUI_NativeDialogHandle, bool>> autoCancelCalls;
    std::vector<std::pair<ArkUI_NativeDialogHandle, bool>> showCalls;
    std::vector<ArkUI_NativeDialogHandle> closeCalls;
    std::vector<std::pair<ArkUI_NativeDialogHandle, bool>> customStyleCalls;
    std::vector<NativeDialogDismissRegistration> dismissRegistrations;
};

NativeCallTracker g_tracker;
NativeDialogTracker g_dialogTracker;

void ResetTracker()
{
    g_tracker = NativeCallTracker {};
    g_dialogTracker = NativeDialogTracker {};
}

template<typename TComponent>
ArkUINodeApiAdapter CreateNodeApiAdapter(TComponent& component)
{
    return ArkUINodeApiAdapter([&component]() { return component.GetNativeView(); },
        [&component]() { return component.GetComponentId(); },
        [&component](
            float top, float right, float bottom, float left) { component.SetMargin(top, right, bottom, left); },
        [&component]() { component.ResetCommonMargin(); },
        [&component](const std::function<void()>& onClick) { component.RegisterOnClick(onClick); });
}

template<typename TComponent>
std::shared_ptr<ArkUINodeApiAdapter> CreateSharedNodeApiAdapter(TComponent& component)
{
    return std::make_shared<ArkUINodeApiAdapter>([&component]() { return component.GetNativeView(); },
        [&component]() { return component.GetComponentId(); },
        [&component](
            float top, float right, float bottom, float left) { component.SetMargin(top, right, bottom, left); },
        [&component]() { component.ResetCommonMargin(); },
        [&component](const std::function<void()>& onClick) { component.RegisterOnClick(onClick); });
}

ArkUI_NodeHandle TrackCreateNode(ArkUI_NodeType type)
{
    auto handle = reinterpret_cast<ArkUI_NodeHandle>(
        static_cast<uintptr_t>(GENERATED_NODE_HANDLE_BASE + g_tracker.createNodeCalls.size()));
    g_tracker.createNodeCalls.push_back({ type, handle });
    return handle;
}

void TrackDisposeNode(ArkUI_NodeHandle node)
{
    ++g_tracker.disposeNodeCount;
    g_tracker.disposeNodeCalls.push_back(node);
}

int32_t TrackAddChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    g_tracker.addChildCalls.emplace_back(parent, child);
    return 0;
}

int32_t TrackInsertChildAt(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    g_tracker.insertChildAtCalls.emplace_back(parent, child, index);
    return 0;
}

int32_t TrackRemoveChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    g_tracker.removeChildCalls.emplace_back(parent, child);
    return 0;
}

int32_t TrackSetAttribute(ArkUI_NodeHandle node, int32_t attribute, ArkUI_AttributeItem* item)
{
    NativeAttributeCall call;
    call.node = node;
    call.attribute = attribute;
    if (item != nullptr) {
        call.size = item->size;
        call.stringValue = item->string == nullptr ? "" : item->string;
        call.adapterHandle = reinterpret_cast<ArkUI_NodeAdapterHandle>(item->object);
        if (item->value != nullptr) {
            for (int32_t index = 0; index < item->size; ++index) {
                call.values.push_back(item->value[index]);
            }
        }
    }
    g_tracker.attributeCalls.push_back(call);
    return 0;
}

int32_t TrackResetAttribute(ArkUI_NodeHandle node, int32_t attribute)
{
    g_tracker.resetAttributeCalls.emplace_back(node, attribute);
    return 0;
}

int32_t TrackSetUserData(ArkUI_NodeHandle node, void* userData)
{
    ++g_tracker.setUserDataCount;
    g_tracker.userData[node] = userData;
    return 0;
}

void* TrackGetUserData(ArkUI_NodeHandle node)
{
    auto iter = g_tracker.userData.find(node);
    return iter == g_tracker.userData.end() ? nullptr : iter->second;
}

int32_t TrackAddNodeEventReceiver(ArkUI_NodeHandle node, ArkUI_NodeEventCallback)
{
    ++g_tracker.addNodeEventReceiverCount;
    g_tracker.addNodeEventReceiverCalls.push_back(node);
    return 0;
}

int32_t TrackRemoveNodeEventReceiver(ArkUI_NodeHandle node, ArkUI_NodeEventCallback)
{
    ++g_tracker.removeNodeEventReceiverCount;
    g_tracker.removeNodeEventReceiverCalls.push_back(node);
    return 0;
}

int32_t TrackRegisterNodeEvent(ArkUI_NodeHandle node, int32_t eventType, int32_t, void*)
{
    g_tracker.registerNodeEventCalls.emplace_back(node, eventType);
    return 0;
}

int32_t TrackUnregisterNodeEvent(ArkUI_NodeHandle node, int32_t eventType)
{
    ++g_tracker.unregisterNodeEventCount;
    g_tracker.unregisterNodeEventCalls.emplace_back(node, eventType);
    return 0;
}

ArkUI_NativeDialogHandle TrackCreateDialog()
{
    auto handle = reinterpret_cast<ArkUI_NativeDialogHandle>(
        static_cast<uintptr_t>(GENERATED_DIALOG_HANDLE_BASE + g_dialogTracker.createCalls.size()));
    g_dialogTracker.createCalls.push_back(handle);
    return handle;
}

void TrackDisposeDialog(ArkUI_NativeDialogHandle handle)
{
    g_dialogTracker.disposeCalls.push_back(handle);
}

int32_t TrackSetDialogContent(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content)
{
    g_dialogTracker.setContentCalls.emplace_back(handle, content);
    return 0;
}

int32_t TrackSetDialogAlignment(
    ArkUI_NativeDialogHandle handle, ArkUI_DialogAlignment alignment, float offsetX, float offsetY)
{
    g_dialogTracker.alignmentCalls.emplace_back(handle, alignment, offsetX, offsetY);
    return 0;
}

int32_t TrackSetDialogModalMode(ArkUI_NativeDialogHandle handle, bool modal)
{
    g_dialogTracker.modalModeCalls.emplace_back(handle, modal);
    return 0;
}

int32_t TrackSetDialogAutoCancel(ArkUI_NativeDialogHandle handle, bool autoCancel)
{
    g_dialogTracker.autoCancelCalls.emplace_back(handle, autoCancel);
    return 0;
}

int32_t TrackRegisterDialogDismissReceiver(
    ArkUI_NativeDialogHandle handle, void* userData, void (*callback)(ArkUI_DialogDismissEvent*))
{
    g_dialogTracker.dismissRegistrations.push_back({ handle, userData, callback });
    return 0;
}

int32_t TrackShowDialog(ArkUI_NativeDialogHandle handle, bool showInSubWindow)
{
    g_dialogTracker.showCalls.emplace_back(handle, showInSubWindow);
    return 0;
}

int32_t TrackCloseDialog(ArkUI_NativeDialogHandle handle)
{
    g_dialogTracker.closeCalls.push_back(handle);
    return 0;
}

int32_t TrackEnableDialogCustomStyle(ArkUI_NativeDialogHandle handle, bool enable)
{
    g_dialogTracker.customStyleCalls.emplace_back(handle, enable);
    return 0;
}

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

ArkUI_NodeHandle FindCreatedNode(ArkUI_NodeType type, size_t occurrence = 0)
{
    size_t currentOccurrence = 0;
    for (const auto& call : g_tracker.createNodeCalls) {
        if (call.type != type) {
            continue;
        }
        if (currentOccurrence == occurrence) {
            return call.node;
        }
        ++currentOccurrence;
    }
    return nullptr;
}

const NativeAttributeCall* FindLastAttributeCall(ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto iter = g_tracker.attributeCalls.rbegin(); iter != g_tracker.attributeCalls.rend(); ++iter) {
        if (iter->node == node && iter->attribute == attribute) {
            return &(*iter);
        }
    }
    return nullptr;
}

bool HasResetAttributeCall(ArkUI_NodeHandle node, int32_t attribute)
{
    for (const auto& call : g_tracker.resetAttributeCalls) {
        if (call.first == node && call.second == attribute) {
            return true;
        }
    }
    return false;
}

int32_t CountAttributeCall(ArkUI_NodeHandle node, int32_t attribute)
{
    int32_t count = 0;
    for (const auto& call : g_tracker.attributeCalls) {
        if (call.node == node && call.attribute == attribute) {
            ++count;
        }
    }
    return count;
}

bool HasAddChildCall(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    for (const auto& call : g_tracker.addChildCalls) {
        if (call.first == parent && call.second == child) {
            return true;
        }
    }
    return false;
}

bool HasInsertChildAtCall(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    for (const auto& call : g_tracker.insertChildAtCalls) {
        if (std::get<0>(call) == parent && std::get<1>(call) == child && std::get<2>(call) == index) {
            return true;
        }
    }
    return false;
}

bool HasRegisterNodeEventCall(ArkUI_NodeHandle node, int32_t eventType)
{
    for (const auto& call : g_tracker.registerNodeEventCalls) {
        if (call.first == node && call.second == eventType) {
            return true;
        }
    }
    return false;
}

bool HasUnregisterNodeEventCall(ArkUI_NodeHandle node, int32_t eventType)
{
    for (const auto& call : g_tracker.unregisterNodeEventCalls) {
        if (call.first == node && call.second == eventType) {
            return true;
        }
    }
    return false;
}

bool HasAddNodeEventReceiverCall(ArkUI_NodeHandle node)
{
    for (const auto& call : g_tracker.addNodeEventReceiverCalls) {
        if (call == node) {
            return true;
        }
    }
    return false;
}

bool HasRemoveNodeEventReceiverCall(ArkUI_NodeHandle node)
{
    for (const auto& call : g_tracker.removeNodeEventReceiverCalls) {
        if (call == node) {
            return true;
        }
    }
    return false;
}

bool HasDisposedNode(ArkUI_NodeHandle node)
{
    for (const auto& call : g_tracker.disposeNodeCalls) {
        if (call == node) {
            return true;
        }
    }
    return false;
}

void ExpectI32Attribute(ArkUI_NodeHandle node, int32_t attribute, int32_t expected)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, attribute);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].i32, expected);
}

void ExpectU32Attribute(ArkUI_NodeHandle node, int32_t attribute, uint32_t expected)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, attribute);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_EQ(call->values[0].u32, expected);
}

void ExpectF32Attribute(ArkUI_NodeHandle node, int32_t attribute, float expected)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, attribute);
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->values.empty());
    EXPECT_FLOAT_EQ(call->values[0].f32, expected);
}

void ExpectStringAttribute(ArkUI_NodeHandle node, int32_t attribute, const std::string& expected)
{
    const NativeAttributeCall* call = FindLastAttributeCall(node, attribute);
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->stringValue, expected);
}

class BasicLeafComponent : public Component {
public:
    explicit BasicLeafComponent(ArkUI_NodeHandle nativeView) : Component(nativeView, false) {}

    std::string GetType() const override
    {
        return "Leaf";
    }
};

class A2UIComponentTddTest : public A2UITest {
protected:
    ArkUI_NativeNodeAPI_1* api_ = nullptr;
    ArkUI_NativeNodeAPI_1 savedApi_ {};
    ArkUI_NativeDialogAPI_1* dialogApi_ = nullptr;
    ArkUI_NativeDialogAPI_1 savedDialogApi_ {};

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetTracker();
        api_ = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
        ASSERT_NE(api_, nullptr);
        savedApi_ = *api_;

        api_->createNode = TrackCreateNode;
        api_->disposeNode = TrackDisposeNode;
        api_->addChild = TrackAddChild;
        api_->insertChildAt = TrackInsertChildAt;
        api_->removeChild = TrackRemoveChild;
        api_->setAttribute = TrackSetAttribute;
        api_->resetAttribute = TrackResetAttribute;
        api_->setUserData = TrackSetUserData;
        api_->getUserData = TrackGetUserData;
        api_->addNodeEventReceiver = TrackAddNodeEventReceiver;
        api_->removeNodeEventReceiver = TrackRemoveNodeEventReceiver;
        api_->registerNodeEvent = TrackRegisterNodeEvent;
        api_->unregisterNodeEvent = TrackUnregisterNodeEvent;

        dialogApi_ = ArkUINativeAPI::GetInstance().GetNativeDialogAPI();
        ASSERT_NE(dialogApi_, nullptr);
        savedDialogApi_ = *dialogApi_;
        dialogApi_->create = TrackCreateDialog;
        dialogApi_->dispose = TrackDisposeDialog;
        dialogApi_->setContent = TrackSetDialogContent;
        dialogApi_->setContentAlignment = TrackSetDialogAlignment;
        dialogApi_->setModalMode = TrackSetDialogModalMode;
        dialogApi_->setAutoCancel = TrackSetDialogAutoCancel;
        dialogApi_->registerOnWillDismissWithUserData = TrackRegisterDialogDismissReceiver;
        dialogApi_->show = TrackShowDialog;
        dialogApi_->close = TrackCloseDialog;
        dialogApi_->enableCustomStyle = TrackEnableDialogCustomStyle;
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(COMPONENT_TDD_RENDER_ID);
        if (api_ != nullptr) {
            *api_ = savedApi_;
        }
        if (dialogApi_ != nullptr) {
            *dialogApi_ = savedDialogApi_;
        }
        ResetTracker();
        A2UITest::TearDown();
    }

    void PrepareThemeContext(Component& component)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(COMPONENT_TDD_RENDER_ID);
        std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
        ASSERT_NE(surfaceManager, nullptr);
        surfaceManager->CreateSurface(COMPONENT_TDD_SURFACE_ID, nullptr);
        component.SetRenderId(COMPONENT_TDD_RENDER_ID);
        component.SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    }
};

} // namespace
} // namespace NativeModule

#endif // A2UI_COMPONENT_TDD_TEST_HELPER_H
