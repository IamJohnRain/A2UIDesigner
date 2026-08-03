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

#include <array>
#include <gtest/gtest.h>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/A2UI/column/ColumnComponent.h"
#include "components/Component.h"
#include "components/extended/if/IfComponent.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

class DummyChildComponent : public Component {
public:
    explicit DummyChildComponent(ArkUI_NodeHandle handle) : Component(handle, false) {}
    ~DummyChildComponent() override = default;
    std::string GetType() const override
    {
        return "Dummy";
    }
};

std::shared_ptr<DummyChildComponent> CreateChild(uintptr_t handle, const std::string& id)
{
    auto child = std::make_shared<DummyChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(handle));
    child->SetComponentId(id);
    return child;
}

bool FindLastMargin(const MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, std::array<float, 4>& margin)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle != nodeHandle || it->attribute != NODE_MARGIN || it->values.size() != 4) {
            continue;
        }
        margin = { it->values[0].f32, it->values[1].f32, it->values[2].f32, it->values[3].f32 };
        return true;
    }
    return false;
}

std::vector<std::string> CollectChildIds(const std::shared_ptr<Component>& parent)
{
    std::vector<std::string> result;
    if (parent == nullptr) {
        return result;
    }
    for (const auto& child : parent->GetChildren()) {
        if (child != nullptr) {
            result.push_back(child->GetComponentId());
        }
    }
    return result;
}

std::list<std::string> BuildChildIdListFromOrder(const std::string& order)
{
    std::list<std::string> childIds;
    for (char token : order) {
        childIds.emplace_back(1, token);
    }
    return childIds;
}

void ProcessArrivalSequence(const std::string& arrivalOrder, const std::list<std::string>& declaredOrder,
    std::shared_ptr<ColumnComponent>& parent, std::map<std::string, std::shared_ptr<Component>>& allComponents,
    std::map<char, std::shared_ptr<DummyChildComponent>>& children)
{
    std::string parentId = "p";
    for (char token : arrivalOrder) {
        if (token == 'p') {
            if (parent == nullptr) {
                parent = std::make_shared<ColumnComponent>();
                parent->SetComponentId(parentId);
                allComponents[parentId] = parent;
            }
            parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
            continue;
        }

        std::string childId(1, token);
        auto childIt = children.find(token);
        if (childIt == children.end()) {
            auto child = CreateChild(0x500 + static_cast<uintptr_t>(token), childId);
            children[token] = child;
            childIt = children.find(token);
        }
        allComponents[childId] = childIt->second;
        if (parent != nullptr) {
            childIt->second->AttachToParentIfNeeded(allComponents, parentId);
        }
    }
}

void ExpectOrderAndSpacing(const std::shared_ptr<ColumnComponent>& parent,
    const std::map<char, std::shared_ptr<DummyChildComponent>>& children, const std::string& expectedOrder,
    MockArkUINativeProvider* provider)
{
    ASSERT_NE(parent, nullptr);
    std::vector<std::string> actualOrder = CollectChildIds(parent);
    ASSERT_EQ(actualOrder.size(), expectedOrder.size());
    for (size_t i = 0; i < expectedOrder.size(); ++i) {
        EXPECT_EQ(actualOrder[i], std::string(1, expectedOrder[i]));
    }

    size_t totalChildren = expectedOrder.size();
    for (size_t i = 0; i < totalChildren; ++i) {
        char childToken = expectedOrder[i];
        auto childIt = children.find(childToken);
        ASSERT_TRUE(childIt != children.end());
        std::array<float, 4> childMargin = {};
        ASSERT_TRUE(FindLastMargin(provider, childIt->second->GetNativeView(), childMargin));

        float expectedTop = 0.0F;
        float expectedBottom = 0.0F;

        if (totalChildren == 1) {
            expectedTop = 0.0F;
            expectedBottom = 0.0F;
        } else if (totalChildren == 2) {
            if (i == 0) {
                expectedBottom = 4.0F;
            } else if (i == 1) {
                expectedTop = 4.0F;
            }
        } else if (totalChildren >= 3) {
            if (i == 0) {
                expectedTop = 0.0F;
                expectedBottom = 4.0F;
            } else if (i == totalChildren - 1) {
                expectedTop = 4.0F;
                expectedBottom = 0.0F;
            } else {
                expectedTop = 4.0F;
                expectedBottom = 4.0F;
            }
        }

        EXPECT_FLOAT_EQ(childMargin[0], expectedTop);
        EXPECT_FLOAT_EQ(childMargin[2], expectedBottom);
    }
}

void RunArrivalAndRebuildScenario(const std::string& initialChildOrder, const std::string& initialArrivalOrder,
    const std::string& rebuiltChildOrder, const std::string& postRebuildArrivalOrder,
    const std::string& expectedFinalOrder, MockArkUINativeProvider* provider)
{
    std::shared_ptr<ColumnComponent> parent = nullptr;
    std::map<std::string, std::shared_ptr<Component>> allComponents;
    std::map<char, std::shared_ptr<DummyChildComponent>> children;

    std::list<std::string> declaredOrder = BuildChildIdListFromOrder(initialChildOrder);
    ProcessArrivalSequence(initialArrivalOrder, declaredOrder, parent, allComponents, children);

    if (!rebuiltChildOrder.empty()) {
        ASSERT_NE(parent, nullptr);
        declaredOrder = BuildChildIdListFromOrder(rebuiltChildOrder);
        parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    }

    if (!postRebuildArrivalOrder.empty()) {
        ProcessArrivalSequence(postRebuildArrivalOrder, declaredOrder, parent, allComponents, children);
    }

    ExpectOrderAndSpacing(parent, children, expectedFinalOrder, provider);
}

} // namespace

class ComponentAttachOrderTest : public A2UITest {};

/**
 * @tc.name: ComponentAttachOrderTest001
 * @tc.desc: Verify AttachStaticChildrenByIds keeps declared children order and spacing.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest001)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("parent");

    auto child1 = CreateChild(0x101, "c1");
    auto child2 = CreateChild(0x102, "c2");
    auto child3 = CreateChild(0x103, "c3");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "parent", parent }, { "c1", child1 },
        { "c2", child2 }, { "c3", child3 } };
    std::list<std::string> declaredOrder = { "c2", "c1", "c3" };

    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::vector<std::string> actualOrder = CollectChildIds(parent);
    ASSERT_EQ(actualOrder.size(), 3U);
    EXPECT_EQ(actualOrder[0], "c2");
    EXPECT_EQ(actualOrder[1], "c1");
    EXPECT_EQ(actualOrder[2], "c3");
}

/**
 * @tc.name: ComponentAttachOrderTest002
 * @tc.desc: Verify AttachToParentIfNeeded inserts late child by declared order and keeps spacing correct.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest002)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("parent");

    auto child2 = CreateChild(0x202, "c2");
    auto child3 = CreateChild(0x203, "c3");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "parent", parent }, { "c2", child2 },
        { "c3", child3 } };
    std::list<std::string> declaredOrder = { "c1", "c2", "c3" };

    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    std::vector<std::string> initialOrder = CollectChildIds(parent);
    ASSERT_EQ(initialOrder.size(), 2U);
    EXPECT_EQ(initialOrder[0], "c2");
    EXPECT_EQ(initialOrder[1], "c3");

    auto child1 = CreateChild(0x201, "c1");
    allComponents["c1"] = child1;
    std::string parentId = "parent";
    child1->AttachToParentIfNeeded(allComponents, parentId);

    std::vector<std::string> finalOrder = CollectChildIds(parent);
    ASSERT_EQ(finalOrder.size(), 3U);
    EXPECT_EQ(finalOrder[0], "c1");
    EXPECT_EQ(finalOrder[1], "c2");
    EXPECT_EQ(finalOrder[2], "c3");
}

/**
 * @tc.name: ComponentAttachOrderTest003
 * @tc.desc: Verify multiple late children keep declared order when arrival is out-of-order.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest003)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("parent");

    auto child3 = CreateChild(0x303, "c3");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "parent", parent }, { "c3", child3 } };
    std::list<std::string> declaredOrder = { "c1", "c2", "c3" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::vector<std::string> initialOrder = CollectChildIds(parent);
    ASSERT_EQ(initialOrder.size(), 1U);
    EXPECT_EQ(initialOrder[0], "c3");

    auto child2 = CreateChild(0x302, "c2");
    allComponents["c2"] = child2;
    std::string parentId = "parent";
    child2->AttachToParentIfNeeded(allComponents, parentId);

    std::vector<std::string> midOrder = CollectChildIds(parent);
    ASSERT_EQ(midOrder.size(), 2U);
    EXPECT_EQ(midOrder[0], "c2");
    EXPECT_EQ(midOrder[1], "c3");

    auto child1 = CreateChild(0x301, "c1");
    allComponents["c1"] = child1;
    child1->AttachToParentIfNeeded(allComponents, parentId);

    std::vector<std::string> finalOrder = CollectChildIds(parent);
    ASSERT_EQ(finalOrder.size(), 3U);
    EXPECT_EQ(finalOrder[0], "c1");
    EXPECT_EQ(finalOrder[1], "c2");
    EXPECT_EQ(finalOrder[2], "c3");
}

/**
 * @tc.name: ComponentAttachOrderTest004
 * @tc.desc: Verify rebuild can restore declared children order after manual reorder.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest004)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("parent");

    auto child1 = CreateChild(0x401, "c1");
    auto child2 = CreateChild(0x402, "c2");
    auto child3 = CreateChild(0x403, "c3");
    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "parent", parent }, { "c1", child1 },
        { "c2", child2 }, { "c3", child3 } };
    std::list<std::string> declaredOrder = { "c1", "c2", "c3" };

    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::vector<std::string> movedOrder = CollectChildIds(parent);
    ASSERT_EQ(movedOrder.size(), 3U);
    EXPECT_EQ(movedOrder[0], "c1");
    EXPECT_EQ(movedOrder[1], "c2");
    EXPECT_EQ(movedOrder[2], "c3");

    std::list<std::string> declaredOrder2 = { "c3", "c2", "c1" };
    parent->AttachStaticChildrenByIds(declaredOrder2, allComponents);

    std::vector<std::string> rebuiltOrder = CollectChildIds(parent);
    ASSERT_EQ(rebuiltOrder.size(), 3U);
    EXPECT_EQ(rebuiltOrder[0], "c3");
    EXPECT_EQ(rebuiltOrder[1], "c2");
    EXPECT_EQ(rebuiltOrder[2], "c1");
}

/**
 * @tc.name: ComponentAttachOrderTest005
 * @tc.desc: Verify declared order abcde when arrival order is abcdep.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest005)
{
    RunArrivalAndRebuildScenario("abcde", "abcdep", "", "", "abcde", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest006
 * @tc.desc: Verify declared order abcde when arrival order is edcbap.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest006)
{
    RunArrivalAndRebuildScenario("abcde", "edcbap", "", "", "abcde", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest007
 * @tc.desc: Verify declared order abcde when arrival order is pabcde.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest007)
{
    RunArrivalAndRebuildScenario("abcde", "pabcde", "", "", "abcde", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest008
 * @tc.desc: Verify declared order abcde when arrival order is pedcba.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest008)
{
    RunArrivalAndRebuildScenario("abcde", "pedcba", "", "", "abcde", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest009
 * @tc.desc: Verify declared order abcde when arrival order is edcpba.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest009)
{
    RunArrivalAndRebuildScenario("abcde", "edcpba", "", "", "abcde", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest010
 * @tc.desc: Verify rebuild order changes from abcde to edcba after initial edcpba arrival.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest010)
{
    RunArrivalAndRebuildScenario("abcde", "edcpba", "edcba", "", "edcba", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest011
 * @tc.desc: Verify late a/b follow rebuilt order edcba after initial edcp arrival.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest011)
{
    RunArrivalAndRebuildScenario("abcde", "edcp", "edcba", "ab", "edcba", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest012
 * @tc.desc: Verify duplicate child ids in descriptor do not create duplicate mounted children.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest012)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("p");
    auto childA = CreateChild(0x612, "a");
    auto childB = CreateChild(0x613, "b");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p", parent }, { "a", childA },
        { "b", childB } };
    std::list<std::string> declaredOrder = { "a", "a", "b" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::vector<std::string> order = CollectChildIds(parent);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], "a");
    EXPECT_EQ(order[1], "b");
}

/**
 * @tc.name: ComponentAttachOrderTest013
 * @tc.desc: Verify descriptor gaps (missing child ids) still preserve relative order of existing children.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest013)
{
    RunArrivalAndRebuildScenario("axb", "abp", "", "", "ab", mockArkUIPtr_);
}

/**
 * @tc.name: ComponentAttachOrderTest014
 * @tc.desc: Verify child already mounted under another parent is skipped by AttachStaticChildrenByIds.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest014)
{
    auto parent1 = std::make_shared<ColumnComponent>();
    parent1->SetComponentId("p1");
    auto parent2 = std::make_shared<ColumnComponent>();
    parent2->SetComponentId("p2");
    auto childA = CreateChild(0x614, "a");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p1", parent1 }, { "p2", parent2 },
        { "a", childA } };
    std::list<std::string> declaredOrder = { "a" };
    parent1->AttachStaticChildrenByIds(declaredOrder, allComponents);
    parent2->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::vector<std::string> order1 = CollectChildIds(parent1);
    std::vector<std::string> order2 = CollectChildIds(parent2);
    ASSERT_EQ(order1.size(), 1U);
    EXPECT_EQ(order1[0], "a");
    EXPECT_TRUE(order2.empty());
    EXPECT_EQ(childA->GetParentId(), "p1");
}

/**
 * @tc.name: ComponentAttachOrderTest014a
 * @tc.desc: Verify later explicit parent relation does not steal a child
 * already mounted by If.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest014a)
{
    auto container = std::make_shared<ColumnComponent>();
    container->SetComponentId("c");
    auto declaredParent = std::make_shared<ColumnComponent>();
    declaredParent->SetComponentId("a");
    auto ifParent = std::make_shared<IfComponent>();
    ifParent->SetComponentId("if");
    auto childB = CreateChild(0x614A, "b");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "c", container }, { "a", declaredParent },
        { "if", ifParent }, { "b", childB } };

    container->AttachStaticChildrenByIds({ "a" }, allComponents);
    ifParent->AttachStaticChildrenByIds({ "b" }, allComponents);
    ASSERT_TRUE(ifParent->HasChild(childB));
    EXPECT_EQ(childB->GetParentId(), "if");

    declaredParent->AttachStaticChildrenByIds({ "b" }, allComponents);
    EXPECT_TRUE(declaredParent->GetChildren().empty());
    std::string parentId = "a";
    childB->AttachToParentIfNeeded(allComponents, parentId);

    EXPECT_TRUE(ifParent->HasChild(childB));
    EXPECT_FALSE(declaredParent->HasChild(childB));
    EXPECT_EQ(childB->GetParentId(), "if");
    EXPECT_EQ(childB->GetParent().get(), ifParent.get());
    ASSERT_TRUE(container->HasChild(declaredParent));
}

/**
 * @tc.name: ComponentAttachOrderTest015
 * @tc.desc: Verify late child not in parent childIds appends to end.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest015)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("p");
    auto childA = CreateChild(0x615, "a");
    auto childB = CreateChild(0x616, "b");
    auto childZ = CreateChild(0x617, "z");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p", parent }, { "a", childA },
        { "b", childB } };
    std::list<std::string> declaredOrder = { "a", "b" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    allComponents["z"] = childZ;

    std::string parentId = "p";
    childZ->AttachToParentIfNeeded(allComponents, parentId);

    std::vector<std::string> order = CollectChildIds(parent);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], "a");
    EXPECT_EQ(order[1], "b");
}

/**
 * @tc.name: ComponentAttachOrderTest016
 * @tc.desc: Verify unchanged childIds with already matched order short-circuits without extra margin updates.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest016)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("p");
    auto childA = CreateChild(0x618, "a");
    auto childB = CreateChild(0x619, "b");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p", parent }, { "a", childA },
        { "b", childB } };
    std::list<std::string> declaredOrder = { "a", "b" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    size_t recordCountBefore = mockArkUIPtr_->setAttributeRecords_.size();
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    size_t recordCountAfter = mockArkUIPtr_->setAttributeRecords_.size();

    std::vector<std::string> order = CollectChildIds(parent);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], "a");
    EXPECT_EQ(order[1], "b");
    EXPECT_EQ(recordCountAfter, recordCountBefore);
}

/**
 * @tc.name: ComponentAttachOrderTest017
 * @tc.desc: Verify AddChildAt move boundary and unchanged childIds short-circuit behavior.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest017)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("p");
    auto childA = CreateChild(0x620, "a");
    auto childB = CreateChild(0x621, "b");
    auto childC = CreateChild(0x622, "c");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p", parent }, { "a", childA },
        { "b", childB }, { "c", childC } };
    std::list<std::string> declaredOrder = { "a", "b", "c" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    parent->AddChildAt(childC, 0);

    std::vector<std::string> movedOrder = CollectChildIds(parent);
    ASSERT_EQ(movedOrder.size(), 3U);
    EXPECT_EQ(movedOrder[0], "a");
    EXPECT_EQ(movedOrder[1], "c");
    EXPECT_EQ(movedOrder[2], "b");

    size_t recordCountBefore = mockArkUIPtr_->setAttributeRecords_.size();
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);
    size_t recordCountAfter = mockArkUIPtr_->setAttributeRecords_.size();

    std::vector<std::string> rebuiltOrder = CollectChildIds(parent);
    ASSERT_EQ(rebuiltOrder.size(), 3U);
    EXPECT_EQ(rebuiltOrder[0], "a");
    EXPECT_EQ(rebuiltOrder[1], "c");
    EXPECT_EQ(rebuiltOrder[2], "b");
    EXPECT_EQ(recordCountAfter, recordCountBefore);
}

/**
 * @tc.name: ComponentAttachOrderTest018
 * @tc.desc: Verify empty childIds clears mounted children and resets their spacing.
 * @tc.type: FUNC
 */
TEST_F(ComponentAttachOrderTest, ComponentAttachOrderTest018)
{
    auto parent = std::make_shared<ColumnComponent>();
    parent->SetComponentId("p");
    auto childA = CreateChild(0x623, "a");
    auto childB = CreateChild(0x624, "b");
    auto childC = CreateChild(0x625, "c");

    std::map<std::string, std::shared_ptr<Component>> allComponents = { { "p", parent }, { "a", childA },
        { "b", childB }, { "c", childC } };
    std::list<std::string> declaredOrder = { "a", "b", "c" };
    parent->AttachStaticChildrenByIds(declaredOrder, allComponents);

    std::list<std::string> emptyOrder;
    parent->AttachStaticChildrenByIds(emptyOrder, allComponents);

    EXPECT_TRUE(CollectChildIds(parent).empty());
}
