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

#include "ArkUINativeAPI.h"

#include <gtest/gtest.h>

#include "TestFixture.h"

using namespace NativeModule;

using A2UITest_ArkUI = A2UITest;

/**
 * @tc.name: ArkUINativeAPITest001
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return non-null provider after set.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest001)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto* provider = ArkUINativeAPI::GetInstance().GetProvider();
    ASSERT_NE(provider, nullptr);
}

/**
 * @tc.name: ArkUINativeAPITest002
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: delegate node adapter create.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest002)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle1 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    auto handle2 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    EXPECT_NE(handle1, nullptr);
    EXPECT_NE(handle2, nullptr);
    EXPECT_NE(handle1, handle2);
    EXPECT_EQ(mockArkUIPtr_->createdAdapters_.size(), 2u);
}

/**
 * @tc.name: ArkUINativeAPITest003
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: track adapter dispose.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest003)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    ArkUINativeAPI::GetInstance().NodeAdapter_Dispose(handle);
    ASSERT_EQ(mockArkUIPtr_->disposedAdapters_.size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->disposedAdapters_[0], handle);
}

/**
 * @tc.name: ArkUINativeAPITest004
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: set total node count.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest004)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    int32_t result = ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 10);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[handle], 10u);
}

/**
 * @tc.name: ArkUINativeAPITest005
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: register and unregister event receiver.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest005)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    int userData = 42;
    int32_t regResult = ArkUINativeAPI::GetInstance().NodeAdapter_RegisterEventReceiver(handle, &userData, nullptr);
    EXPECT_EQ(regResult, 0);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_[handle], &userData);
    int32_t unregResult = ArkUINativeAPI::GetInstance().NodeAdapter_UnregisterEventReceiver(handle);
    EXPECT_EQ(unregResult, 0);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_.count(handle), 0u);
}

/**
 * @tc.name: ArkUINativeAPITest006
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: track node content add and remove.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest006)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto content = reinterpret_cast<ArkUI_NodeContentHandle>(0x100);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x200);
    int32_t addResult = ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node);
    EXPECT_EQ(addResult, 0);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content][0], node);
    int32_t removeResult = ArkUINativeAPI::GetInstance().NodeContent_RemoveNode(content, node);
    EXPECT_EQ(removeResult, 0);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 0u);
}

/**
 * @tc.name: ArkUINativeAPITest007
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: keep get node from NAPI value overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest007)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    ArkUI_NodeContentHandle contentHandle = reinterpret_cast<ArkUI_NodeContentHandle>(0x1111);
    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x2000));
    mockArkUIPtr_->SetGetNodeContentFromNapiValueResult(-1);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeContentFromNapiValue(nullptr, nullptr, &contentHandle), -1);
    EXPECT_EQ(contentHandle, reinterpret_cast<ArkUI_NodeContentHandle>(0x1111));
    mockArkUIPtr_->ResetGetNodeContentFromNapiValueResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeContentFromNapiValue(nullptr, nullptr, &contentHandle), 0);
    EXPECT_EQ(contentHandle, reinterpret_cast<ArkUI_NodeContentHandle>(0x2000));
    ArkUI_NodeHandle nodeHandle = reinterpret_cast<ArkUI_NodeHandle>(0x2222);
    mockArkUIPtr_->SetNodeHandleResult(reinterpret_cast<ArkUI_NodeHandle>(0x3000));
    mockArkUIPtr_->SetGetNodeHandleFromNapiValueResult(-2);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeHandleFromNapiValue(nullptr, nullptr, &nodeHandle), -2);
    EXPECT_EQ(nodeHandle, reinterpret_cast<ArkUI_NodeHandle>(0x2222));
    mockArkUIPtr_->ResetGetNodeHandleFromNapiValueResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeHandleFromNapiValue(nullptr, nullptr, &nodeHandle), 0);
    EXPECT_EQ(nodeHandle, reinterpret_cast<ArkUI_NodeHandle>(0x3000));
}

/**
 * @tc.name: ArkUINativeAPITest008
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return mock native node api.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest008)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto* api = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(api, nullptr);
}

/**
 * @tc.name: ArkUINativeAPITest009
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: reset all mock state.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest009)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    auto content = reinterpret_cast<ArkUI_NodeContentHandle>(0x100);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x200);
    ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node);
    auto adapterEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x300);
    auto nodeEvent = reinterpret_cast<ArkUI_NodeEvent*>(0x350);
    auto dialogEvent = reinterpret_cast<ArkUI_DialogDismissEvent*>(0x400);
    int userData = 1;
    auto removedNode = reinterpret_cast<ArkUI_NodeHandle>(0x201);
    auto stringEvent = reinterpret_cast<ArkUI_StringAsyncEvent*>(0x360);
    mockArkUIPtr_->SetNodeAdapterEventUserData(adapterEvent, &userData);
    mockArkUIPtr_->SetNodeAdapterEventType(adapterEvent, 7);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(adapterEvent, 3);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(adapterEvent, removedNode);
    ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetNodeId(adapterEvent, 1);
    ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetItem(adapterEvent, node);
    mockArkUIPtr_->SetNodeEventHandle(nodeEvent, node);
    mockArkUIPtr_->SetNodeEventType(nodeEvent, 8);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(nodeEvent, stringEvent);
    mockArkUIPtr_->SetDialogDismissEventUserData(dialogEvent, &userData);
    ArkUINativeAPI::GetInstance().DialogDismissEvent_SetShouldBlockDismiss(dialogEvent, true);
    mockArkUIPtr_->ResetAllMocks();
    EXPECT_TRUE(mockArkUIPtr_->createdAdapters_.empty());
    EXPECT_TRUE(mockArkUIPtr_->disposedAdapters_.empty());
    EXPECT_TRUE(mockArkUIPtr_->nodeContentMapping_.empty());
    EXPECT_TRUE(mockArkUIPtr_->totalNodeCounts_.empty());
    EXPECT_TRUE(mockArkUIPtr_->registeredReceivers_.empty());
    EXPECT_TRUE(mockArkUIPtr_->nodeAdapterEventNodeIds_.empty());
    EXPECT_TRUE(mockArkUIPtr_->nodeAdapterEventItems_.empty());
    EXPECT_TRUE(mockArkUIPtr_->dialogDismissShouldBlockDismiss_.empty());
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetUserData(adapterEvent), nullptr);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetType(adapterEvent), 0u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetItemIndex(adapterEvent), 0u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetRemovedNode(adapterEvent), nullptr);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetNodeHandle(nodeEvent), nullptr);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetEventType(nodeEvent), 0u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetStringAsyncEvent(nodeEvent), nullptr);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().DialogDismissEvent_GetUserData(dialogEvent), nullptr);
}

/**
 * @tc.name: ArkUINativeAPITest010
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: restore default behavior after reset all mocks.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest010)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto content = reinterpret_cast<ArkUI_NodeContentHandle>(0x100);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x200);
    auto adapterEvent = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x300);
    ArkUI_NodeContentHandle contentHandle = nullptr;
    ArkUI_NodeHandle nodeHandle = reinterpret_cast<ArkUI_NodeHandle>(0x1234);
    mockArkUIPtr_->SetGetNodeContentFromNapiValueResult(-1);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeContentFromNapiValue(nullptr, nullptr, &contentHandle), -1);
    mockArkUIPtr_->SetNodeHandleResult(reinterpret_cast<ArkUI_NodeHandle>(0x3456));
    mockArkUIPtr_->SetGetNodeHandleFromNapiValueResult(-2);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeHandleFromNapiValue(nullptr, nullptr, &nodeHandle), -2);
    EXPECT_EQ(nodeHandle, reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    mockArkUIPtr_->SetNodeContentAddResult(-3);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node), -3);
    EXPECT_TRUE(mockArkUIPtr_->nodeContentMapping_.empty());
    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    mockArkUIPtr_->SetNodeAdapterSetTotalNodeCountResult(-4);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 10), -4);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_.count(handle), 0u);
    mockArkUIPtr_->SetNodeAdapterEventSetNodeIdResult(-5);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetNodeId(adapterEvent, 123), -5);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_.count(adapterEvent), 0u);
    mockArkUIPtr_->ResetAllMocks();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeContentFromNapiValue(nullptr, nullptr, &contentHandle), 0);
    EXPECT_EQ(contentHandle, reinterpret_cast<ArkUI_NodeContentHandle>(static_cast<intptr_t>(1)));
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeHandleFromNapiValue(nullptr, nullptr, &nodeHandle), 0);
    EXPECT_EQ(nodeHandle, nullptr);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node), 0);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content][0], node);
    handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 10), 0);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[handle], 10u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetNodeId(adapterEvent, 123), 0);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_[adapterEvent], 123);
}

/**
 * @tc.name: ArkUINativeAPITest011
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: restart adapter handle sequence after reset all
 * mocks.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest011)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle1 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    auto handle2 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    EXPECT_NE(handle1, handle2);
    mockArkUIPtr_->ResetAllMocks();
    auto resetHandle1 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    auto resetHandle2 = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    EXPECT_EQ(resetHandle1, reinterpret_cast<ArkUI_NodeAdapterHandle>(static_cast<intptr_t>(1)));
    EXPECT_EQ(resetHandle2, reinterpret_cast<ArkUI_NodeAdapterHandle>(static_cast<intptr_t>(2)));
    EXPECT_EQ(mockArkUIPtr_->createdAdapters_.size(), 2u);
}

/**
 * @tc.name: ArkUINativeAPITest012
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: keep node content operation overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest012)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto content = reinterpret_cast<ArkUI_NodeContentHandle>(0x100);
    auto insertContent = reinterpret_cast<ArkUI_NodeContentHandle>(0x101);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x200);
    auto insertNode = reinterpret_cast<ArkUI_NodeHandle>(0x201);
    mockArkUIPtr_->SetNodeContentAddResult(-1);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node), -1);
    EXPECT_TRUE(mockArkUIPtr_->nodeContentMapping_.empty());
    mockArkUIPtr_->ResetNodeContentAddResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node), 0);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content][0], node);
    mockArkUIPtr_->SetNodeContentInsertResult(-2);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_InsertNode(insertContent, insertNode, 0), -2);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_.count(insertContent), 0u);
    mockArkUIPtr_->ResetNodeContentInsertResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_InsertNode(insertContent, insertNode, 0), 0);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[insertContent].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[insertContent][0], insertNode);
    mockArkUIPtr_->SetNodeContentRemoveResult(-3);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_RemoveNode(content, node), -3);
    ASSERT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content][0], node);
    mockArkUIPtr_->ResetNodeContentRemoveResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeContent_RemoveNode(content, node), 0);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 0u);
}

/**
 * @tc.name: ArkUINativeAPITest013
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: not duplicate node in content add.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest013)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto content = reinterpret_cast<ArkUI_NodeContentHandle>(0x100);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x200);
    ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node);
    ArkUINativeAPI::GetInstance().NodeContent_AddNode(content, node);
    EXPECT_EQ(mockArkUIPtr_->nodeContentMapping_[content].size(), 1u);
}

/**
 * @tc.name: ArkUINativeAPITest014
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: dispose clears adapter state.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest014)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 5);
    ArkUINativeAPI::GetInstance().NodeAdapter_RegisterEventReceiver(handle, nullptr, nullptr);
    ArkUINativeAPI::GetInstance().NodeAdapter_Dispose(handle);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_.count(handle), 0u);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_.count(handle), 0u);
}

/**
 * @tc.name: ArkUINativeAPITest015
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: keep node adapter mutation overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest015)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto handle = ArkUINativeAPI::GetInstance().NodeAdapter_Create();
    int userData = 42;
    mockArkUIPtr_->SetNodeAdapterSetTotalNodeCountResult(-4);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 10), -4);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_.count(handle), 0u);
    mockArkUIPtr_->ResetNodeAdapterSetTotalNodeCountResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_SetTotalNodeCount(handle, 10), 0);
    EXPECT_EQ(mockArkUIPtr_->totalNodeCounts_[handle], 10u);
    mockArkUIPtr_->SetNodeAdapterRegisterEventReceiverResult(-5);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_RegisterEventReceiver(handle, &userData, nullptr), -5);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_.count(handle), 0u);
    mockArkUIPtr_->ResetNodeAdapterRegisterEventReceiverResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_RegisterEventReceiver(handle, &userData, nullptr), 0);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_[handle], &userData);
    mockArkUIPtr_->SetNodeAdapterUnregisterEventReceiverResult(-6);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_UnregisterEventReceiver(handle), -6);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_[handle], &userData);
    mockArkUIPtr_->ResetNodeAdapterUnregisterEventReceiverResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapter_UnregisterEventReceiver(handle), 0);
    EXPECT_EQ(mockArkUIPtr_->registeredReceivers_.count(handle), 0u);
}

/**
 * @tc.name: ArkUINativeAPITest016
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return configured node adapter event values.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest016)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x500);
    int userData = 99;
    auto removedNode = reinterpret_cast<ArkUI_NodeHandle>(0x501);
    mockArkUIPtr_->SetNodeAdapterEventUserData(event, &userData);
    mockArkUIPtr_->SetNodeAdapterEventType(event, 7);
    mockArkUIPtr_->SetNodeAdapterEventItemIndex(event, 3);
    mockArkUIPtr_->SetNodeAdapterEventRemovedNode(event, removedNode);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetUserData(event), &userData);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetType(event), 7u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetItemIndex(event), 3u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_GetRemovedNode(event), removedNode);
}

/**
 * @tc.name: ArkUINativeAPITest017
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: keep node adapter event setter overrides until reset.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest017)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto event = reinterpret_cast<ArkUI_NodeAdapterEvent*>(0x600);
    auto item = reinterpret_cast<ArkUI_NodeHandle>(0x601);
    mockArkUIPtr_->SetNodeAdapterEventSetNodeIdResult(-7);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetNodeId(event, 123), -7);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_.count(event), 0u);
    mockArkUIPtr_->ResetNodeAdapterEventSetNodeIdResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetNodeId(event, 123), 0);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventNodeIds_[event], 123);
    mockArkUIPtr_->SetNodeAdapterEventSetItemResult(-8);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetItem(event, item), -8);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_.count(event), 0u);
    mockArkUIPtr_->ResetNodeAdapterEventSetItemResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeAdapterEvent_SetItem(event, item), 0);
    EXPECT_EQ(mockArkUIPtr_->nodeAdapterEventItems_[event], item);
}

/**
 * @tc.name: ArkUINativeAPITest018
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return configured node event values.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest018)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto event = reinterpret_cast<ArkUI_NodeEvent*>(0x700);
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x701);
    auto stringEvent = reinterpret_cast<ArkUI_StringAsyncEvent*>(0x702);
    mockArkUIPtr_->SetNodeEventHandle(event, node);
    mockArkUIPtr_->SetNodeEventType(event, 9);
    mockArkUIPtr_->SetNodeEventStringAsyncEvent(event, stringEvent);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetNodeHandle(event), node);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetEventType(event), 9u);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetStringAsyncEvent(event), stringEvent);
}

/**
 * @tc.name: ArkUINativeAPITest019
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: track dialog dismiss event values.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest019)
{
    /**
     * @tc.steps: step1. Configure the mock ArkUI state if needed and invoke the ArkUINativeAPI wrapper interface.
     * @tc.expected: The wrapper return values and recorded mock state match the expectation.
     */

    auto event = reinterpret_cast<ArkUI_DialogDismissEvent*>(0x800);
    int userData = 77;
    mockArkUIPtr_->SetDialogDismissEventUserData(event, &userData);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().DialogDismissEvent_GetUserData(event), &userData);
    ArkUINativeAPI::GetInstance().DialogDismissEvent_SetShouldBlockDismiss(event, true);
    EXPECT_TRUE(mockArkUIPtr_->dialogDismissShouldBlockDismiss_[event]);
    ArkUINativeAPI::GetInstance().DialogDismissEvent_SetShouldBlockDismiss(event, false);
    EXPECT_FALSE(mockArkUIPtr_->dialogDismissShouldBlockDismiss_[event]);
}

/**
 * @tc.name: ArkUINativeAPITest020
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return configured node component event values.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest020)
{
    auto event = reinterpret_cast<ArkUI_NodeEvent*>(0x900);
    ArkUI_NodeComponentEvent componentEvent = {};
    componentEvent.data[0].i32 = 1;
    mockArkUIPtr_->SetNodeEventComponentEvent(event, &componentEvent);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().NodeEvent_GetNodeComponentEvent(event), &componentEvent);
}

/**
 * @tc.name: ArkUINativeAPITest021
 * @tc.desc: Verify the following ArkUINativeAPI wrapper behavior: return native node unique id.
 * @tc.type: FUNC
 */
TEST_F(A2UITest_ArkUI, ArkUINativeAPITest021)
{
    auto node = reinterpret_cast<ArkUI_NodeHandle>(0x901);
    mockArkUIPtr_->SetNodeUniqueId(node, 12345);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeUniqueId(node), 12345);

    mockArkUIPtr_->SetGetNodeUniqueIdResult(-1);
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeUniqueId(node), -1);
    mockArkUIPtr_->ResetGetNodeUniqueIdResult();
    EXPECT_EQ(ArkUINativeAPI::GetInstance().GetNodeUniqueId(node), 12345);
}
