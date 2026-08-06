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

#ifndef A2UI_MOCK_ARKUI_NATIVE_PROVIDER_H
#define A2UI_MOCK_ARKUI_NATIVE_PROVIDER_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "IArkUINativeProvider.h"

namespace NativeModule {

class MockArkUINativeProvider : public IArkUINativeProvider {
public:
    struct SetAttributeRecord {
        ArkUI_NodeHandle nodeHandle = nullptr;
        int32_t attribute = 0;
        std::vector<ArkUI_NumberValue> values;
        std::string stringValue;
    };

    struct ResetAttributeRecord {
        ArkUI_NodeHandle nodeHandle = nullptr;
        int32_t attribute = 0;
    };

    struct CreatedNodeRecord {
        ArkUI_NodeHandle nodeHandle = nullptr;
        ArkUI_NodeType nodeType = 0;
    };

    struct LayoutRecord {
        ArkUI_NodeHandle nodeHandle = nullptr;
        int32_t x = 0;
        int32_t y = 0;
    };

    static MockArkUINativeProvider& GetInstance();
    ~MockArkUINativeProvider() override;

    ArkUI_NativeNodeAPI_1* GetNativeNodeAPI() override;

    int32_t GetNodeContentFromNapiValue(
        napi_env env, napi_value value, ArkUI_NodeContentHandle* contentHandle) override;
    int32_t GetNodeHandleFromNapiValue(napi_env env, napi_value value, ArkUI_NodeHandle* handle) override;

    int32_t NodeContent_AddNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle) override;
    int32_t NodeContent_InsertNode(
        ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, int32_t position) override;
    int32_t NodeContent_RemoveNode(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle) override;

    ArkUI_NodeAdapterHandle NodeAdapter_Create() override;
    void NodeAdapter_Dispose(ArkUI_NodeAdapterHandle handle) override;
    int32_t NodeAdapter_SetTotalNodeCount(ArkUI_NodeAdapterHandle handle, uint32_t count) override;
    int32_t NodeAdapter_RegisterEventReceiver(
        ArkUI_NodeAdapterHandle handle, void* userData, void (*callback)(ArkUI_NodeAdapterEvent*)) override;
    int32_t NodeAdapter_UnregisterEventReceiver(ArkUI_NodeAdapterHandle handle) override;

    void* NodeAdapterEvent_GetUserData(const ArkUI_NodeAdapterEvent* event) override;
    uint32_t NodeAdapterEvent_GetType(const ArkUI_NodeAdapterEvent* event) override;
    uint32_t NodeAdapterEvent_GetItemIndex(const ArkUI_NodeAdapterEvent* event) override;
    ArkUI_NodeHandle NodeAdapterEvent_GetRemovedNode(const ArkUI_NodeAdapterEvent* event) override;
    int32_t NodeAdapterEvent_SetNodeId(ArkUI_NodeAdapterEvent* event, int32_t nodeId) override;
    int32_t NodeAdapterEvent_SetItem(ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle item) override;
    int32_t NodeAdapter_ReloadAllItems(ArkUI_NodeAdapterHandle handle) override;

    ArkUI_NodeHandle NodeEvent_GetNodeHandle(const ArkUI_NodeEvent* event) override;
    ArkUI_NodeEventType NodeEvent_GetEventType(const ArkUI_NodeEvent* event) override;
    ArkUI_NodeComponentEvent* NodeEvent_GetNodeComponentEvent(const ArkUI_NodeEvent* event) override;
    ArkUI_StringAsyncEvent* NodeEvent_GetStringAsyncEvent(const ArkUI_NodeEvent* event) override;

    ArkUI_NativeDialogAPI_1* GetNativeDialogAPI() override;
    void* DialogDismissEvent_GetUserData(ArkUI_DialogDismissEvent* event) override;
    void DialogDismissEvent_SetShouldBlockDismiss(ArkUI_DialogDismissEvent* event, bool shouldBlock) override;

    int32_t SetCrossLanguageOption(ArkUI_NodeHandle node, bool enabled) override;
    int32_t GetNodeUniqueId(ArkUI_NodeHandle node, int32_t* uniqueId) override;

    void ResetAllMocks();

    static std::unique_ptr<MockArkUINativeProvider> Create()
    {
        return std::unique_ptr<MockArkUINativeProvider>(new MockArkUINativeProvider());
    }

    void SetGetNodeContentFromNapiValueResult(int32_t result);
    void ResetGetNodeContentFromNapiValueResult();
    void SetNodeContentHandleResult(ArkUI_NodeContentHandle handle);
    void ResetNodeContentHandleResult();
    void SetGetNodeHandleFromNapiValueResult(int32_t result);
    void ResetGetNodeHandleFromNapiValueResult();
    void SetNodeHandleResult(ArkUI_NodeHandle handle);
    void ResetNodeHandleResult();
    void SetNodeContentAddResult(int32_t result);
    void ResetNodeContentAddResult();
    void SetNodeContentInsertResult(int32_t result);
    void ResetNodeContentInsertResult();
    void SetNodeContentRemoveResult(int32_t result);
    void ResetNodeContentRemoveResult();
    void SetNodeAdapterSetTotalNodeCountResult(int32_t result);
    void ResetNodeAdapterSetTotalNodeCountResult();
    void SetNodeAdapterRegisterEventReceiverResult(int32_t result);
    void ResetNodeAdapterRegisterEventReceiverResult();
    void SetNodeAdapterUnregisterEventReceiverResult(int32_t result);
    void ResetNodeAdapterUnregisterEventReceiverResult();
    void SetNodeAdapterEventUserData(const ArkUI_NodeAdapterEvent* event, void* userData);
    void SetNodeAdapterEventType(const ArkUI_NodeAdapterEvent* event, uint32_t type);
    void SetNodeAdapterEventItemIndex(const ArkUI_NodeAdapterEvent* event, uint32_t index);
    void SetNodeAdapterEventRemovedNode(const ArkUI_NodeAdapterEvent* event, ArkUI_NodeHandle node);
    void SetNodeAdapterEventSetNodeIdResult(int32_t result);
    void ResetNodeAdapterEventSetNodeIdResult();
    void SetNodeAdapterEventSetItemResult(int32_t result);
    void ResetNodeAdapterEventSetItemResult();
    bool DispatchNodeAdapterEvent(ArkUI_NodeAdapterHandle handle, ArkUI_NodeAdapterEvent* event);
    void SetNodeEventHandle(const ArkUI_NodeEvent* event, ArkUI_NodeHandle handle);
    void SetNodeEventType(const ArkUI_NodeEvent* event, uint32_t type);
    void SetNodeEventComponentEvent(const ArkUI_NodeEvent* event, ArkUI_NodeComponentEvent* componentEvent);
    void SetNodeEventStringAsyncEvent(const ArkUI_NodeEvent* event, ArkUI_StringAsyncEvent* stringAsyncEvent);
    bool DispatchNodeEvent(ArkUI_NodeHandle nodeHandle, ArkUI_NodeEvent* event);
    int32_t RegisterCustomEvent(ArkUI_NodeHandle node, ArkUI_NodeCustomEventType eventType, void* userData);
    void UnregisterCustomEvent(ArkUI_NodeHandle node, ArkUI_NodeCustomEventType eventType);
    bool DispatchMeasureEvent(ArkUI_NodeHandle node, int32_t percentReferenceWidth, int32_t percentReferenceHeight);
    bool DispatchLayoutEvent(ArkUI_NodeHandle node, int32_t x, int32_t y);
    void SetDialogDismissEventUserData(ArkUI_DialogDismissEvent* event, void* userData);
    void SetNodeUniqueId(ArkUI_NodeHandle node, int32_t uniqueId);
    void SetGetNodeUniqueIdResult(int32_t result);
    void ResetGetNodeUniqueIdResult();
    void SetDialogCloseResult(int32_t result);
    void ResetDialogCloseResult();
    void SetDialogCloseTriggersDismissCallback(bool enabled);
    void ResetDialogCloseTriggersDismissCallback();
    void TrackSetAttribute(int32_t attribute);

    std::map<ArkUI_NodeContentHandle, std::vector<ArkUI_NodeHandle>> nodeContentMapping_;
    std::map<ArkUI_NodeHandle, void*> nodeUserData_;
    std::map<ArkUI_NodeHandle, std::vector<ArkUI_NodeEventCallback>> nodeEventReceivers_;
    std::map<ArkUI_NodeHandle, std::vector<ArkUI_NodeCustomEventCallback>> nodeCustomEventReceivers_;
    std::map<ArkUI_NodeHandle, std::set<ArkUI_NodeEventType>> registeredNodeEvents_;
    std::vector<ArkUI_NodeAdapterHandle> createdAdapters_;
    std::vector<ArkUI_NodeAdapterHandle> disposedAdapters_;
    std::vector<int32_t> setAttributeTypes_;
    std::vector<int32_t> resetAttributeTypes_;
    std::vector<ResetAttributeRecord> resetAttributeRecords_;
    std::map<ArkUI_NodeAdapterHandle, uint32_t> totalNodeCounts_;
    std::map<ArkUI_NodeAdapterHandle, void*> registeredReceivers_;
    std::map<ArkUI_NodeAdapterHandle, void (*)(ArkUI_NodeAdapterEvent*)> registeredAdapterCallbacks_;
    std::map<ArkUI_NodeAdapterEvent*, int64_t> nodeAdapterEventNodeIds_;
    std::map<ArkUI_NodeAdapterEvent*, ArkUI_NodeHandle> nodeAdapterEventItems_;
    std::map<ArkUI_DialogDismissEvent*, bool> dialogDismissShouldBlockDismiss_;
    std::map<ArkUI_NodeHandle, bool> crossLanguageOptionNodes_;
    std::map<ArkUI_NodeHandle, int32_t> nodeUniqueIds_;
    std::vector<SetAttributeRecord> setAttributeRecords_;
    std::map<std::pair<ArkUI_NodeHandle, int32_t>, SetAttributeRecord> currentAttributes_;
    std::vector<CreatedNodeRecord> createdNodes_;
    std::vector<ArkUI_NodeHandle> disposedNodes_;
    std::map<ArkUI_NodeHandle, std::vector<ArkUI_NodeHandle>> nodeChildren_;
    std::map<ArkUI_NodeHandle, ArkUI_NodeHandle> nodeParents_;
    std::map<ArkUI_NodeHandle, ArkUI_IntSize> measuredSizes_;
    std::vector<ArkUI_NodeHandle> measuredNodes_;
    std::vector<LayoutRecord> layoutRecords_;
    std::vector<std::pair<ArkUI_NodeHandle, ArkUI_NodeDirtyFlag>> dirtyNodes_;
    std::vector<ArkUI_NativeDialogHandle> createdDialogs_;
    std::vector<ArkUI_NativeDialogHandle> disposedDialogs_;
    std::vector<ArkUI_NativeDialogHandle> closedDialogs_;
    std::map<ArkUI_NativeDialogHandle, ArkUI_NodeHandle> dialogContentHandles_;
    int32_t nativeDialogApiCallCount_ = 0;

private:
    struct CustomEventRegistration {
        ArkUI_NodeCustomEventType eventType = 0;
        void* userData = nullptr;
    };

    struct DialogDismissRegistration {
        void* userData = nullptr;
        void (*callback)(ArkUI_DialogDismissEvent*) = nullptr;
    };

    MockArkUINativeProvider();
    MockArkUINativeProvider(const MockArkUINativeProvider&) = delete;
    MockArkUINativeProvider& operator=(const MockArkUINativeProvider&) = delete;

public:
    static MockArkUINativeProvider* GetActiveInstance();
    ArkUI_NativeDialogHandle CreateDialog();
    void DisposeDialog(ArkUI_NativeDialogHandle handle);
    int32_t SetDialogContent(ArkUI_NativeDialogHandle handle, ArkUI_NodeHandle content);
    int32_t RegisterDialogDismissReceiver(
        ArkUI_NativeDialogHandle handle, void* userData, void (*callback)(ArkUI_DialogDismissEvent*));
    int32_t ShowDialog(ArkUI_NativeDialogHandle handle);
    int32_t CloseDialog(ArkUI_NativeDialogHandle handle);

private:
    void TrackAdapterCreation(ArkUI_NodeAdapterHandle handle);
    void TrackAdapterDisposal(ArkUI_NodeAdapterHandle handle);
    void TrackNodeContentOperation(ArkUI_NodeContentHandle contentHandle, ArkUI_NodeHandle nodeHandle, bool isAdd);

    static MockArkUINativeProvider* activeInstance_;
    static ArkUI_NativeNodeAPI_1 mockNodeAPI_;
    static ArkUI_NativeDialogAPI_1 mockDialogAPI_;
    int32_t getNodeContentFromNapiValueResult_ = 0;
    ArkUI_NodeContentHandle nodeContentHandleResult_ =
        reinterpret_cast<ArkUI_NodeContentHandle>(static_cast<intptr_t>(1));
    int32_t getNodeHandleFromNapiValueResult_ = 0;
    ArkUI_NodeHandle nodeHandleResult_ = nullptr;
    int32_t nodeContentAddResult_ = 0;
    int32_t nodeContentInsertResult_ = 0;
    int32_t nodeContentRemoveResult_ = 0;
    int32_t nodeAdapterSetTotalNodeCountResult_ = 0;
    int32_t nodeAdapterRegisterEventReceiverResult_ = 0;
    int32_t nodeAdapterUnregisterEventReceiverResult_ = 0;
    std::map<const ArkUI_NodeAdapterEvent*, void*> nodeAdapterEventUserData_;
    std::map<const ArkUI_NodeAdapterEvent*, uint32_t> nodeAdapterEventTypes_;
    std::map<const ArkUI_NodeAdapterEvent*, uint32_t> nodeAdapterEventItemIndexes_;
    std::map<const ArkUI_NodeAdapterEvent*, ArkUI_NodeHandle> nodeAdapterEventRemovedNodes_;
    int32_t nodeAdapterEventSetNodeIdResult_ = 0;
    int32_t nodeAdapterEventSetItemResult_ = 0;
    std::map<const ArkUI_NodeEvent*, ArkUI_NodeHandle> nodeEventHandles_;
    std::map<const ArkUI_NodeEvent*, ArkUI_NodeEventType> nodeEventTypes_;
    std::map<const ArkUI_NodeEvent*, ArkUI_NodeComponentEvent*> nodeEventComponentEvents_;
    std::map<const ArkUI_NodeEvent*, ArkUI_StringAsyncEvent*> nodeEventStringAsyncEvents_;
    std::map<ArkUI_NodeHandle, std::map<ArkUI_NodeCustomEventType, CustomEventRegistration>> customEventRegistrations_;
    std::map<ArkUI_DialogDismissEvent*, void*> dialogDismissEventUserData_;
    std::map<ArkUI_NativeDialogHandle, DialogDismissRegistration> dialogDismissRegistrations_;
    int32_t getNodeUniqueIdResult_ = 0;
    int32_t dialogCloseResult_ = 0;
    bool dialogCloseTriggersDismissCallback_ = false;
    int32_t nextAdapterHandle_ = 1;
    int32_t nextDialogHandle_ = 1;
    int32_t nextDialogDismissEventId_ = 1;
};

} // namespace NativeModule

#endif // A2UI_MOCK_ARKUI_NATIVE_PROVIDER_H
