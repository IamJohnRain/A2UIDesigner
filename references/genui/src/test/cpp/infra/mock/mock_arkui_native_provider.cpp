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

#include "mock_arkui_native_provider.h"

#include <algorithm>

namespace NativeModule {

namespace {

MockArkUINativeProvider* g_activeMockArkUIProvider = nullptr;

MockArkUINativeProvider* GetActiveProvider()
{
    return MockArkUINativeProvider::GetActiveInstance();
}

ArkUI_NodeHandle MockCreateNode(ArkUI_NodeType)
{
    return reinterpret_cast<ArkUI_NodeHandle>(0x1);
}
void MockDisposeNode(ArkUI_NodeHandle) {}
int32_t MockAddChild(ArkUI_NodeHandle, ArkUI_NodeHandle)
{
    return 0;
}
int32_t MockRemoveChild(ArkUI_NodeHandle, ArkUI_NodeHandle)
{
    return 0;
}
int32_t MockInsertChildAt(ArkUI_NodeHandle, ArkUI_NodeHandle, int32_t)
{
    return 0;
}
int32_t MockSetAttribute(ArkUI_NodeHandle node, int32_t attribute, ArkUI_AttributeItem* item)
{
    if (g_activeMockArkUIProvider == nullptr) {
        return 0;
    }
    MockArkUINativeProvider::SetAttributeRecord record;
    record.nodeHandle = node;
    record.attribute = attribute;
    if (item != nullptr) {
        if (item->value != nullptr && item->size > 0) {
            record.values.assign(item->value, item->value + item->size);
        }
        if (item->string != nullptr) {
            record.stringValue = item->string;
        }
    }
    g_activeMockArkUIProvider->setAttributeRecords_.push_back(record);
    return 0;
}
int32_t MockResetAttribute(ArkUI_NodeHandle node, int32_t attribute)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider != nullptr) {
        provider->resetAttributeTypes_.push_back(attribute);
        provider->resetAttributeRecords_.push_back({ node, attribute });
    }
    return 0;
}
int32_t MockSetUserData(ArkUI_NodeHandle node, void* userData)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr) {
        return 0;
    }
    provider->nodeUserData_[node] = userData;
    return 0;
}

void* MockGetUserData(ArkUI_NodeHandle node)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr) {
        return nullptr;
    }

    auto it = provider->nodeUserData_.find(node);
    return it != provider->nodeUserData_.end() ? it->second : nullptr;
}

int32_t MockAddNodeEventReceiver(ArkUI_NodeHandle node, ArkUI_NodeEventCallback callback)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr || callback == nullptr) {
        return 0;
    }

    provider->nodeEventReceivers_[node].push_back(callback);
    return 0;
}

int32_t MockRemoveNodeEventReceiver(ArkUI_NodeHandle node, ArkUI_NodeEventCallback callback)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr) {
        return 0;
    }

    auto receiverIt = provider->nodeEventReceivers_.find(node);
    if (receiverIt == provider->nodeEventReceivers_.end()) {
        return 0;
    }

    auto& callbacks = receiverIt->second;
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
    if (callbacks.empty()) {
        provider->nodeEventReceivers_.erase(receiverIt);
    }
    return 0;
}

int32_t MockRegisterNodeEvent(ArkUI_NodeHandle node, int32_t eventType, int32_t, void*)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr) {
        return 0;
    }

    provider->registeredNodeEvents_[node].insert(static_cast<ArkUI_NodeEventType>(eventType));
    return 0;
}

int32_t MockUnregisterNodeEvent(ArkUI_NodeHandle node, int32_t eventType)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider == nullptr) {
        return 0;
    }

    auto registeredIt = provider->registeredNodeEvents_.find(node);
    if (registeredIt == provider->registeredNodeEvents_.end()) {
        return 0;
    }

    registeredIt->second.erase(static_cast<ArkUI_NodeEventType>(eventType));
    if (registeredIt->second.empty()) {
        provider->registeredNodeEvents_.erase(registeredIt);
    }
    return 0;
}

ArkUI_NativeDialogHandle MockCreateDialog()
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    return provider == nullptr ? nullptr : provider->CreateDialog();
}

void MockDisposeDialog(ArkUI_NativeDialogHandle handle)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    if (provider != nullptr) {
        provider->DisposeDialog(handle);
    }
}

int32_t MockSetDialogContent(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    return provider == nullptr ? -1 : provider->SetDialogContent(handle, content);
}
int32_t MockSetDialogAlignment(ArkUI_NativeDialogHandle, ArkUI_DialogAlignment, float, float)
{
    return 0;
}
int32_t MockSetDialogModalMode(ArkUI_NativeDialogHandle, bool)
{
    return 0;
}
int32_t MockSetDialogAutoCancel(ArkUI_NativeDialogHandle, bool)
{
    return 0;
}
int32_t MockRegisterDialogDismissReceiver(
    ArkUI_NativeDialogHandle handle, void* userData, void (*callback)(ArkUI_DialogDismissEvent*))
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    return provider == nullptr ? -1 : provider->RegisterDialogDismissReceiver(handle, userData, callback);
}

int32_t MockShowDialog(ArkUI_NativeDialogHandle handle, bool)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    return provider == nullptr ? -1 : provider->ShowDialog(handle);
}

int32_t MockCloseDialog(ArkUI_NativeDialogHandle handle)
{
    MockArkUINativeProvider* provider = GetActiveProvider();
    return provider == nullptr ? -1 : provider->CloseDialog(handle);
}
int32_t MockEnableDialogCustomStyle(ArkUI_NativeDialogHandle, bool)
{
    return 0;
}

ArkUI_NativeNodeAPI_1 CreateSafeMockNodeAPI()
{
    ArkUI_NativeNodeAPI_1 api = {};
    api.createNode = MockCreateNode;
    api.disposeNode = MockDisposeNode;
    api.addChild = MockAddChild;
    api.removeChild = MockRemoveChild;
    api.insertChildAt = MockInsertChildAt;
    api.setAttribute = MockSetAttribute;
    api.resetAttribute = MockResetAttribute;
    api.setUserData = MockSetUserData;
    api.getUserData = MockGetUserData;
    api.addNodeEventReceiver = MockAddNodeEventReceiver;
    api.removeNodeEventReceiver = MockRemoveNodeEventReceiver;
    api.registerNodeEvent = MockRegisterNodeEvent;
    api.unregisterNodeEvent = MockUnregisterNodeEvent;
    return api;
}

ArkUI_NativeDialogAPI_1 CreateSafeMockDialogAPI()
{
    ArkUI_NativeDialogAPI_1 api = {};
    api.create = MockCreateDialog;
    api.dispose = MockDisposeDialog;
    api.setContent = MockSetDialogContent;
    api.setContentAlignment = MockSetDialogAlignment;
    api.setModalMode = MockSetDialogModalMode;
    api.setAutoCancel = MockSetDialogAutoCancel;
    api.registerOnWillDismissWithUserData = MockRegisterDialogDismissReceiver;
    api.show = MockShowDialog;
    api.close = MockCloseDialog;
    api.enableCustomStyle = MockEnableDialogCustomStyle;
    return api;
}

} // namespace

MockArkUINativeProvider* MockArkUINativeProvider::activeInstance_ = nullptr;
ArkUI_NativeNodeAPI_1 MockArkUINativeProvider::mockNodeAPI_ = CreateSafeMockNodeAPI();
ArkUI_NativeDialogAPI_1 MockArkUINativeProvider::mockDialogAPI_ = CreateSafeMockDialogAPI();

MockArkUINativeProvider::MockArkUINativeProvider() : nextAdapterHandle_(1)
{
    g_activeMockArkUIProvider = this;
    activeInstance_ = this;
}

MockArkUINativeProvider::~MockArkUINativeProvider()
{
    if (g_activeMockArkUIProvider == this) {
        g_activeMockArkUIProvider = nullptr;
    }
    if (activeInstance_ == this) {
        activeInstance_ = nullptr;
    }
}

MockArkUINativeProvider& MockArkUINativeProvider::GetInstance()
{
    static MockArkUINativeProvider instance;
    return instance;
}

ArkUI_NativeNodeAPI_1* MockArkUINativeProvider::GetNativeNodeAPI()
{
    return &mockNodeAPI_;
}

int32_t MockArkUINativeProvider::GetNodeContentFromNapiValue(
    napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle)
{
    static_cast<void>(env);
    static_cast<void>(value);

    if (getNodeContentFromNapiValueResult_ != 0) {
        return getNodeContentFromNapiValueResult_;
    }

    if (contentHandle != nullptr) {
        *contentHandle = nodeContentHandleResult_;
    }
    return 0;
}

int32_t MockArkUINativeProvider::GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle)
{
    static_cast<void>(env);
    static_cast<void>(value);

    if (getNodeHandleFromNapiValueResult_ != 0) {
        return getNodeHandleFromNapiValueResult_;
    }

    if (handle != nullptr) {
        *handle = nodeHandleResult_;
    }
    return 0;
}

int32_t MockArkUINativeProvider::NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    if (nodeContentAddResult_ != 0) {
        return nodeContentAddResult_;
    }

    TrackNodeContentOperation(contentHandle, nodeHandle, true);
    return 0;
}

int32_t MockArkUINativeProvider::NodeContent_InsertNode(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position)
{
    static_cast<void>(position);

    if (nodeContentInsertResult_ != 0) {
        return nodeContentInsertResult_;
    }

    TrackNodeContentOperation(contentHandle, nodeHandle, true);
    return 0;
}

int32_t MockArkUINativeProvider::NodeContent_RemoveNode(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle)
{
    if (nodeContentRemoveResult_ != 0) {
        return nodeContentRemoveResult_;
    }

    TrackNodeContentOperation(contentHandle, nodeHandle, false);
    return 0;
}

ArkUI_NodeAdapterHandle MockArkUINativeProvider::NodeAdapter_Create()
{
    auto handle = reinterpret_cast<ArkUI_NodeAdapterHandle>(static_cast<intptr_t>(nextAdapterHandle_));
    TrackAdapterCreation(handle);
    ++nextAdapterHandle_;
    return handle;
}

void MockArkUINativeProvider::NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle)
{
    TrackAdapterDisposal(handle);
    totalNodeCounts_.erase(handle);
    registeredReceivers_.erase(handle);
}

int32_t MockArkUINativeProvider::NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count)
{
    if (nodeAdapterSetTotalNodeCountResult_ != 0) {
        return nodeAdapterSetTotalNodeCountResult_;
    }

    totalNodeCounts_[handle] = count;
    return 0;
}

int32_t MockArkUINativeProvider::NodeAdapter_RegisterEventReceiver(
    ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*))
{
    if (nodeAdapterRegisterEventReceiverResult_ != 0) {
        return nodeAdapterRegisterEventReceiverResult_;
    }

    registeredReceivers_[handle] = userData;
    registeredAdapterCallbacks_[handle] = callback;
    return 0;
}

int32_t MockArkUINativeProvider::NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle)
{
    if (nodeAdapterUnregisterEventReceiverResult_ != 0) {
        return nodeAdapterUnregisterEventReceiverResult_;
    }

    registeredReceivers_.erase(handle);
    registeredAdapterCallbacks_.erase(handle);
    return 0;
}

void* MockArkUINativeProvider::NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event)
{
    auto it = nodeAdapterEventUserData_.find(event);
    return it != nodeAdapterEventUserData_.end() ? it->second : nullptr;
}

uint32_t MockArkUINativeProvider::NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event)
{
    auto it = nodeAdapterEventTypes_.find(event);
    return it != nodeAdapterEventTypes_.end() ? it->second : 0;
}

uint32_t MockArkUINativeProvider::NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event)
{
    auto it = nodeAdapterEventItemIndexes_.find(event);
    return it != nodeAdapterEventItemIndexes_.end() ? it->second : 0;
}

ArkUI_NodeHandle MockArkUINativeProvider::NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event)
{
    auto it = nodeAdapterEventRemovedNodes_.find(event);
    return it != nodeAdapterEventRemovedNodes_.end() ? it->second : nullptr;
}

int32_t MockArkUINativeProvider::NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle)
{
    return 0;
}

int32_t MockArkUINativeProvider::NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId)
{
    if (nodeAdapterEventSetNodeIdResult_ != 0) {
        return nodeAdapterEventSetNodeIdResult_;
    }

    nodeAdapterEventNodeIds_[event] = nodeId;
    return 0;
}

int32_t MockArkUINativeProvider::NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item)
{
    if (nodeAdapterEventSetItemResult_ != 0) {
        return nodeAdapterEventSetItemResult_;
    }

    nodeAdapterEventItems_[event] = item;
    return 0;
}

ArkUI_NodeHandle MockArkUINativeProvider::NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event)
{
    auto it = nodeEventHandles_.find(event);
    return it != nodeEventHandles_.end() ? it->second : nullptr;
}

ArkUI_NodeEventType MockArkUINativeProvider::NodeEvent_GetEventType(const ArkUI_NodeEvent* event)
{
    auto it = nodeEventTypes_.find(event);
    return it != nodeEventTypes_.end() ? it->second : static_cast<ArkUI_NodeEventType>(0);
}

ArkUI_NodeComponentEvent* MockArkUINativeProvider::NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event)
{
    auto it = nodeEventComponentEvents_.find(event);
    return it != nodeEventComponentEvents_.end() ? it->second : nullptr;
}

ArkUI_StringAsyncEvent* MockArkUINativeProvider::NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event)
{
    auto it = nodeEventStringAsyncEvents_.find(event);
    return it != nodeEventStringAsyncEvents_.end() ? it->second : nullptr;
}

ArkUI_NativeDialogAPI_1* MockArkUINativeProvider::GetNativeDialogAPI()
{
    ++nativeDialogApiCallCount_;
    return &mockDialogAPI_;
}

void* MockArkUINativeProvider::DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event)
{
    auto it = dialogDismissEventUserData_.find(event);
    return it != dialogDismissEventUserData_.end() ? it->second : nullptr;
}

void MockArkUINativeProvider::DialogDismissEvent_SetShouldBlockDismiss(
    ArkUI_DialogDismissEvent* event, bool shouldBlock)
{
    dialogDismissShouldBlockDismiss_[event] = shouldBlock;
}

MockArkUINativeProvider* MockArkUINativeProvider::GetActiveInstance()
{
    return activeInstance_;
}

ArkUI_NativeDialogHandle MockArkUINativeProvider::CreateDialog()
{
    auto handle = reinterpret_cast<ArkUI_NativeDialogHandle>(static_cast<intptr_t>(0x1000 + nextDialogHandle_));
    ++nextDialogHandle_;
    createdDialogs_.push_back(handle);
    return handle;
}

void MockArkUINativeProvider::DisposeDialog(ArkUI_NativeDialogHandle handle)
{
    disposedDialogs_.push_back(handle);
    dialogContentHandles_.erase(handle);
    dialogDismissRegistrations_.erase(handle);
}

int32_t MockArkUINativeProvider::SetDialogContent(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content)
{
    dialogContentHandles_[handle] = content;
    return 0;
}

int32_t MockArkUINativeProvider::RegisterDialogDismissReceiver(
    ArkUI_NativeDialogHandle handle, void* userData, void (*callback)(ArkUI_DialogDismissEvent*))
{
    dialogDismissRegistrations_[handle] = { .userData = userData, .callback = callback };
    return 0;
}

int32_t MockArkUINativeProvider::ShowDialog(ArkUI_NativeDialogHandle handle)
{
    return handle == nullptr ? -1 : 0;
}

int32_t MockArkUINativeProvider::CloseDialog(ArkUI_NativeDialogHandle handle)
{
    closedDialogs_.push_back(handle);
    if (dialogCloseTriggersDismissCallback_) {
        auto registrationIt = dialogDismissRegistrations_.find(handle);
        if (registrationIt != dialogDismissRegistrations_.end() && registrationIt->second.callback != nullptr) {
            ArkUI_DialogDismissEvent* event =
                reinterpret_cast<ArkUI_DialogDismissEvent*>(static_cast<intptr_t>(0x2000 + nextDialogDismissEventId_));
            ++nextDialogDismissEventId_;
            dialogDismissEventUserData_[event] = registrationIt->second.userData;
            registrationIt->second.callback(event);
            dialogDismissEventUserData_.erase(event);
            dialogDismissShouldBlockDismiss_.erase(event);
        }
    }
    return dialogCloseResult_;
}

void MockArkUINativeProvider::ResetAllMocks()
{
    nodeContentMapping_.clear();
    nodeUserData_.clear();
    nodeEventReceivers_.clear();
    registeredNodeEvents_.clear();
    createdAdapters_.clear();
    disposedAdapters_.clear();
    setAttributeTypes_.clear();
    resetAttributeTypes_.clear();
    resetAttributeRecords_.clear();
    totalNodeCounts_.clear();
    registeredReceivers_.clear();
    registeredAdapterCallbacks_.clear();
    nodeAdapterEventNodeIds_.clear();
    nodeAdapterEventItems_.clear();
    dialogDismissShouldBlockDismiss_.clear();
    setAttributeRecords_.clear();
    getNodeContentFromNapiValueResult_ = 0;
    nodeContentHandleResult_ = reinterpret_cast<ArkUI_NodeContentHandle>(static_cast<intptr_t>(1));
    getNodeHandleFromNapiValueResult_ = 0;
    nodeHandleResult_ = nullptr;
    nodeContentAddResult_ = 0;
    nodeContentInsertResult_ = 0;
    nodeContentRemoveResult_ = 0;
    nodeAdapterSetTotalNodeCountResult_ = 0;
    nodeAdapterRegisterEventReceiverResult_ = 0;
    nodeAdapterUnregisterEventReceiverResult_ = 0;
    nodeAdapterEventUserData_.clear();
    nodeAdapterEventTypes_.clear();
    nodeAdapterEventItemIndexes_.clear();
    nodeAdapterEventRemovedNodes_.clear();
    nodeAdapterEventSetNodeIdResult_ = 0;
    nodeAdapterEventSetItemResult_ = 0;
    nodeEventHandles_.clear();
    nodeEventTypes_.clear();
    nodeEventComponentEvents_.clear();
    nodeEventStringAsyncEvents_.clear();
    dialogDismissEventUserData_.clear();
    nodeUniqueIds_.clear();
    getNodeUniqueIdResult_ = 0;
    dialogDismissRegistrations_.clear();
    createdDialogs_.clear();
    disposedDialogs_.clear();
    closedDialogs_.clear();
    dialogContentHandles_.clear();
    nativeDialogApiCallCount_ = 0;
    dialogCloseResult_ = 0;
    dialogCloseTriggersDismissCallback_ = false;
    nextAdapterHandle_ = 1;
    nextDialogHandle_ = 1;
    nextDialogDismissEventId_ = 1;
}

void MockArkUINativeProvider::SetGetNodeContentFromNapiValueResult(int32_t result)
{
    getNodeContentFromNapiValueResult_ = result;
}

void MockArkUINativeProvider::ResetGetNodeContentFromNapiValueResult()
{
    getNodeContentFromNapiValueResult_ = 0;
}

void MockArkUINativeProvider::SetNodeContentHandleResult(ArkUI_NodeContentHandle handle)
{
    nodeContentHandleResult_ = handle;
}

void MockArkUINativeProvider::ResetNodeContentHandleResult()
{
    nodeContentHandleResult_ = reinterpret_cast<ArkUI_NodeContentHandle>(static_cast<intptr_t>(1));
}

void MockArkUINativeProvider::SetGetNodeHandleFromNapiValueResult(int32_t result)
{
    getNodeHandleFromNapiValueResult_ = result;
}

void MockArkUINativeProvider::ResetGetNodeHandleFromNapiValueResult()
{
    getNodeHandleFromNapiValueResult_ = 0;
}

void MockArkUINativeProvider::SetNodeHandleResult(ArkUI_NodeHandle handle)
{
    nodeHandleResult_ = handle;
}

void MockArkUINativeProvider::ResetNodeHandleResult()
{
    nodeHandleResult_ = nullptr;
}

void MockArkUINativeProvider::SetNodeContentAddResult(int32_t result)
{
    nodeContentAddResult_ = result;
}

void MockArkUINativeProvider::ResetNodeContentAddResult()
{
    nodeContentAddResult_ = 0;
}

void MockArkUINativeProvider::SetNodeContentInsertResult(int32_t result)
{
    nodeContentInsertResult_ = result;
}

void MockArkUINativeProvider::ResetNodeContentInsertResult()
{
    nodeContentInsertResult_ = 0;
}

void MockArkUINativeProvider::SetNodeContentRemoveResult(int32_t result)
{
    nodeContentRemoveResult_ = result;
}

void MockArkUINativeProvider::ResetNodeContentRemoveResult()
{
    nodeContentRemoveResult_ = 0;
}

void MockArkUINativeProvider::SetNodeAdapterSetTotalNodeCountResult(int32_t result)
{
    nodeAdapterSetTotalNodeCountResult_ = result;
}

void MockArkUINativeProvider::ResetNodeAdapterSetTotalNodeCountResult()
{
    nodeAdapterSetTotalNodeCountResult_ = 0;
}

void MockArkUINativeProvider::SetNodeAdapterRegisterEventReceiverResult(int32_t result)
{
    nodeAdapterRegisterEventReceiverResult_ = result;
}

void MockArkUINativeProvider::ResetNodeAdapterRegisterEventReceiverResult()
{
    nodeAdapterRegisterEventReceiverResult_ = 0;
}

void MockArkUINativeProvider::SetNodeAdapterUnregisterEventReceiverResult(int32_t result)
{
    nodeAdapterUnregisterEventReceiverResult_ = result;
}

void MockArkUINativeProvider::ResetNodeAdapterUnregisterEventReceiverResult()
{
    nodeAdapterUnregisterEventReceiverResult_ = 0;
}

void MockArkUINativeProvider::SetNodeAdapterEventUserData(const ArkUI_NodeAdapterEvent* event, void* userData)
{
    nodeAdapterEventUserData_[event] = userData;
}

void MockArkUINativeProvider::SetNodeAdapterEventType(const ArkUI_NodeAdapterEvent* event, uint32_t type)
{
    nodeAdapterEventTypes_[event] = type;
}

void MockArkUINativeProvider::SetNodeAdapterEventItemIndex(const ArkUI_NodeAdapterEvent* event, uint32_t index)
{
    nodeAdapterEventItemIndexes_[event] = index;
}

void MockArkUINativeProvider::SetNodeAdapterEventRemovedNode(const ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle node)
{
    nodeAdapterEventRemovedNodes_[event] = node;
}

void MockArkUINativeProvider::SetNodeAdapterEventSetNodeIdResult(int32_t result)
{
    nodeAdapterEventSetNodeIdResult_ = result;
}

void MockArkUINativeProvider::ResetNodeAdapterEventSetNodeIdResult()
{
    nodeAdapterEventSetNodeIdResult_ = 0;
}

void MockArkUINativeProvider::SetNodeAdapterEventSetItemResult(int32_t result)
{
    nodeAdapterEventSetItemResult_ = result;
}

void MockArkUINativeProvider::ResetNodeAdapterEventSetItemResult()
{
    nodeAdapterEventSetItemResult_ = 0;
}

bool MockArkUINativeProvider::DispatchNodeAdapterEvent(ArkUI_NodeAdapterHandle handle, ArkUI_NodeAdapterEvent* event)
{
    if (handle == nullptr || event == nullptr) {
        return false;
    }

    auto receiverIt = registeredReceivers_.find(handle);
    auto callbackIt = registeredAdapterCallbacks_.find(handle);
    if (receiverIt == registeredReceivers_.end() || callbackIt == registeredAdapterCallbacks_.end() ||
        callbackIt->second == nullptr) {
        return false;
    }

    nodeAdapterEventUserData_[event] = receiverIt->second;
    callbackIt->second(event);
    return true;
}

void MockArkUINativeProvider::SetNodeEventHandle(const ArkUI_NodeEvent* event, ArkUI_NodeHandle handle)
{
    nodeEventHandles_[event] = handle;
}

void MockArkUINativeProvider::SetNodeEventType(const ArkUI_NodeEvent* event, uint32_t type)
{
    nodeEventTypes_[event] = static_cast<ArkUI_NodeEventType>(type);
}

void MockArkUINativeProvider::SetNodeEventComponentEvent(
    const ArkUI_NodeEvent* event, ArkUI_NodeComponentEvent* componentEvent)
{
    nodeEventComponentEvents_[event] = componentEvent;
}

void MockArkUINativeProvider::SetNodeEventStringAsyncEvent(
    const ArkUI_NodeEvent* event, ArkUI_StringAsyncEvent* stringAsyncEvent)
{
    nodeEventStringAsyncEvents_[event] = stringAsyncEvent;
}

bool MockArkUINativeProvider::DispatchNodeEvent(ArkUI_NodeHandle nodeHandle, ArkUI_NodeEvent* event)
{
    if (event == nullptr) {
        return false;
    }
    if (nodeHandle == nullptr) {
        auto handleIt = nodeEventHandles_.find(event);
        nodeHandle = handleIt != nodeEventHandles_.end() ? handleIt->second : nullptr;
    }
    if (nodeHandle == nullptr) {
        return false;
    }

    auto receiverIt = nodeEventReceivers_.find(nodeHandle);
    if (receiverIt == nodeEventReceivers_.end() || receiverIt->second.empty()) {
        return false;
    }

    ArkUI_NodeEventType eventType = NodeEvent_GetEventType(event);
    auto registeredIt = registeredNodeEvents_.find(nodeHandle);
    if (registeredIt == registeredNodeEvents_.end() ||
        registeredIt->second.find(eventType) == registeredIt->second.end()) {
        return false;
    }

    const std::vector<ArkUI_NodeEventCallback> callbacks = receiverIt->second;
    for (ArkUI_NodeEventCallback callback : callbacks) {
        if (callback != nullptr) {
            callback(event);
        }
    }
    return true;
}

void MockArkUINativeProvider::SetDialogDismissEventUserData(ArkUI_DialogDismissEvent* event, void* userData)
{
    dialogDismissEventUserData_[event] = userData;
}

void MockArkUINativeProvider::SetDialogCloseResult(int32_t result)
{
    dialogCloseResult_ = result;
}

void MockArkUINativeProvider::ResetDialogCloseResult()
{
    dialogCloseResult_ = 0;
}

void MockArkUINativeProvider::SetDialogCloseTriggersDismissCallback(bool enabled)
{
    dialogCloseTriggersDismissCallback_ = enabled;
}

void MockArkUINativeProvider::ResetDialogCloseTriggersDismissCallback()
{
    dialogCloseTriggersDismissCallback_ = false;
}

void MockArkUINativeProvider::TrackAdapterCreation(ArkUI_NodeAdapterHandle handle)
{
    createdAdapters_.push_back(handle);
}

void MockArkUINativeProvider::TrackAdapterDisposal(ArkUI_NodeAdapterHandle handle)
{
    disposedAdapters_.push_back(handle);
}

void MockArkUINativeProvider::TrackSetAttribute(int32_t attribute)
{
    setAttributeTypes_.push_back(attribute);
}

void MockArkUINativeProvider::TrackNodeContentOperation(
    ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, bool isAdd)
{
    auto& nodes = nodeContentMapping_[contentHandle];
    if (isAdd) {
        if (std::find(nodes.begin(), nodes.end(), nodeHandle) == nodes.end()) {
            nodes.push_back(nodeHandle);
        }
    } else {
        auto it = std::find(nodes.begin(), nodes.end(), nodeHandle);
        if (it != nodes.end()) {
            nodes.erase(it);
        }
    }
}

int32_t MockArkUINativeProvider::SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled)
{
    crossLanguageOptionNodes_[node] = enabled;
    return 0;
}

int32_t MockArkUINativeProvider::GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId)
{
    if (getNodeUniqueIdResult_ != 0) {
        return getNodeUniqueIdResult_;
    }
    if (uniqueId == nullptr) {
        return -1;
    }
    auto it = nodeUniqueIds_.find(node);
    *uniqueId = it != nodeUniqueIds_.end() ? it->second : static_cast<int32_t>(reinterpret_cast<intptr_t>(node));
    return 0;
}

void MockArkUINativeProvider::SetNodeUniqueId(ArkUI_NodeHandle node, int32_t uniqueId)
{
    nodeUniqueIds_[node] = uniqueId;
}

void MockArkUINativeProvider::SetGetNodeUniqueIdResult(int32_t result)
{
    getNodeUniqueIdResult_ = result;
}

void MockArkUINativeProvider::ResetGetNodeUniqueIdResult()
{
    getNodeUniqueIdResult_ = 0;
}

} // namespace NativeModule
