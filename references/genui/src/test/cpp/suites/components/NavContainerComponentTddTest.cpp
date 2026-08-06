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

#define private public
#include "components/extended/NavContainerComponent.h"
#undef private

#include "A2UIComponentTddTestHelper.h"
#include "SchemaErrorCodes.h"
#include "SchemaWarningTestHelper.h"

using namespace NativeModule;

namespace {

class NavContainerComponentProbe : public NavContainerComponent {
public:
    PropertyDeclaration InvokeGetPrivatePropertyDeclaration(const std::string& propertyName)
    {
        return GetPrivatePropertyDeclaration(propertyName);
    }

    void InvokeApplyPrivateAttributes(const JsonValue& descriptor)
    {
        ApplyPrivateAttributes(descriptor);
    }

    void InvokeApplyRuntimeCurrentIndex(const JsonValue& value)
    {
        ApplyRuntimeProperty("currentIndex", value, true);
    }

    void InvokeCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }

    void InvokeOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void InvokeOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void InvokeOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    int32_t InvokeResolveVisibleIndex(size_t childCount) const
    {
        return ResolveVisibleIndex(childCount);
    }

    void InjectNullChildForTest()
    {
        children_.push_back(nullptr);
    }
};

class FakeChildComponent : public Component {
public:
    explicit FakeChildComponent(const std::string& id, ArkUI_NodeHandle nativeView)
        : Component(nativeView, false), type_("Text")
    {
        SetComponentId(id);
    }

    std::string GetType() const override
    {
        return type_;
    }

private:
    std::string type_;
};

} // namespace

class NavContainerComponentTddTest : public A2UIComponentTddTest {};

/**
 * @tc.name: NavContainer 构造与类型
 * @tc.desc: 覆盖 NavContainer 的节点创建与类型返回分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_create_column_node_and_report_nav_container_type)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();

    EXPECT_EQ(nav->GetType(), "NavContainer");
    EXPECT_EQ(nav->GetNativeView(), FindCreatedNode(ARKUI_NODE_COLUMN));
}

/**
 * @tc.name: NavContainer currentIndex 可见性
 * @tc.desc: 覆盖 currentIndex 解析、异常值回退与子节点可见性分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_apply_current_index_and_fallback_invalid_visibility)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();
    auto descriptor = ParseJson(R"({"id":"nav","component":"NavContainer","currentIndex":1,"children":["a","b"]})");
    ASSERT_NE(descriptor, nullptr);

    nav->InvokeApplyPrivateAttributes(descriptor->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);

    auto childA = std::make_shared<FakeChildComponent>("a", reinterpret_cast<ArkUI_NodeHandle>(0x101));
    auto childB = std::make_shared<FakeChildComponent>("b", reinterpret_cast<ArkUI_NodeHandle>(0x102));
    nav->AddChild(childA);
    nav->AddChild(childB);
    ExpectI32Attribute(childA->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(childB->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":-1})")->GetRoot());
    ExpectI32Attribute(childA->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
    ExpectI32Attribute(childB->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":99})")->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    ExpectI32Attribute(childA->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(childB->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
}

/**
 * @tc.name: NavContainer currentIndex 异常值校验
 * @tc.desc: 覆盖负数、等于 children.length 和大于 children.length 时的不合法参数告警与回退。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_report_invalid_value_when_current_index_is_out_of_bounds)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto nav = std::make_shared<NavContainerComponentProbe>();
    nav->SetRenderId(COMPONENT_TDD_RENDER_ID);
    nav->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    nav->SetComponentId("nav");

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":-1,"children":["page-a","page-b"]})")->GetRoot());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "nav.currentIndex"), 1U);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 0);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":2,"children":["page-a","page-b"]})")->GetRoot());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "nav.currentIndex"), 2U);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":3,"children":["page-a","page-b"]})")->GetRoot());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "nav.currentIndex"), 3U);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":1,"children":["page-a","page-b"]})")->GetRoot());
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "nav.currentIndex"), 3U);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
}

/**
 * @tc.name: NavContainer currentIndex 正向越界回退
 * @tc.desc: currentIndex 大于最后一个子节点索引时回退到最后一个子节点。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_clamp_overflow_current_index_to_last_child)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();
    auto descriptor = ParseJson(R"({"currentIndex":4,"children":["page-a","page-b","page-c"]})");
    ASSERT_NE(descriptor, nullptr);

    nav->InvokeApplyPrivateAttributes(descriptor->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 2);

    auto childA = std::make_shared<FakeChildComponent>("page-a", reinterpret_cast<ArkUI_NodeHandle>(0x121));
    auto childB = std::make_shared<FakeChildComponent>("page-b", reinterpret_cast<ArkUI_NodeHandle>(0x122));
    auto childC = std::make_shared<FakeChildComponent>("page-c", reinterpret_cast<ArkUI_NodeHandle>(0x123));
    nav->AddChild(childA);
    nav->AddChild(childB);
    nav->AddChild(childC);

    ExpectI32Attribute(childA->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(childB->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(childC->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
}

/**
 * @tc.name: NavContainer 动态 currentIndex 异常值校验
 * @tc.desc: 覆盖数据绑定或表达式更新解析后，按实际子节点数量执行边界校验。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_report_invalid_value_for_dynamic_current_index_update)
{
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);

    auto nav = std::make_shared<NavContainerComponentProbe>();
    nav->SetRenderId(COMPONENT_TDD_RENDER_ID);
    nav->SetSurfaceId(COMPONENT_TDD_SURFACE_ID);
    nav->SetComponentId("navDynamic");

    auto childA = std::make_shared<FakeChildComponent>("page-a", reinterpret_cast<ArkUI_NodeHandle>(0x111));
    auto childB = std::make_shared<FakeChildComponent>("page-b", reinterpret_cast<ArkUI_NodeHandle>(0x112));
    nav->AddChild(childA);
    nav->AddChild(childB);

    nav->InvokeApplyRuntimeCurrentIndex(ParseJson(R"(1)")->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "navDynamic.currentIndex"),
        0U);

    nav->InvokeApplyRuntimeCurrentIndex(ParseJson(R"(2)")->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "navDynamic.currentIndex"),
        1U);

    nav->InvokeApplyRuntimeCurrentIndex(ParseJson(R"(-1)")->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 0);
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "navDynamic.currentIndex"),
        2U);

    nav->InvokeApplyRuntimeCurrentIndex(ParseJson(R"(1.5)")->GetRoot());
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 0);
    EXPECT_EQ(
        TestHelpers::CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "navDynamic.currentIndex"),
        3U);
}

/**
 * @tc.name: NavContainer 导航目标切换
 * @tc.desc: 覆盖 children 解析、目标不存在早退和目标存在时的切换分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_collect_static_children_and_navigate_to_target)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();
    auto descriptor = ParseJson(R"({"children":["page-a","page-b"]})");
    ASSERT_NE(descriptor, nullptr);

    nav->InvokeCollectChildListDescriptor(descriptor->GetRoot());
    ASSERT_EQ(nav->GetChildListDescriptor().staticChildIds.size(), 2U);

    auto childA = std::make_shared<FakeChildComponent>("page-a", reinterpret_cast<ArkUI_NodeHandle>(0x201));
    auto childB = std::make_shared<FakeChildComponent>("page-b", reinterpret_cast<ArkUI_NodeHandle>(0x202));
    nav->AddChild(childA);
    nav->AddChild(childB);

    EXPECT_FALSE(nav->NavigateToTargetComponent(""));
    EXPECT_FALSE(nav->NavigateToTargetComponent("missing"));
    EXPECT_TRUE(nav->NavigateToTargetComponent("page-b"));
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    ExpectI32Attribute(childA->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_NONE);
    ExpectI32Attribute(childB->GetNativeView(), NODE_VISIBILITY, ARKUI_VISIBILITY_VISIBLE);
}

/**
 * @tc.name: NavContainer 私有属性与空子节点分支
 * @tc.desc: 覆盖 GetPrivatePropertyDeclaration 的命中/未命中分支以及 childCount=0 的夹紧分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_cover_private_property_and_empty_child_clamp_branches)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();

    PropertyDeclaration declaration = nav->InvokeGetPrivatePropertyDeclaration("currentIndex");
    EXPECT_EQ(declaration.name, "currentIndex");
    EXPECT_EQ(declaration.type, PropertyValueType::NUMBER);

    PropertyDeclaration unknownDeclaration = nav->InvokeGetPrivatePropertyDeclaration("unknown");
    EXPECT_TRUE(unknownDeclaration.name.empty());

    EXPECT_EQ(nav->InvokeResolveVisibleIndex(0), 0);
}

/**
 * @tc.name: NavContainer 子节点移动与移除
 * @tc.desc: 覆盖 OnMoveChild / OnRemoveChild 的刷新分支以及空指针子节点的跳过分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_cover_move_remove_and_null_child_refresh_branches)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();
    nav->InjectNullChildForTest();

    auto childA = std::make_shared<FakeChildComponent>("page-a", reinterpret_cast<ArkUI_NodeHandle>(0x301));
    auto childB = std::make_shared<FakeChildComponent>("page-b", reinterpret_cast<ArkUI_NodeHandle>(0x302));
    nav->AddChild(childA);
    nav->AddChild(childB);

    nav->InvokeApplyPrivateAttributes(ParseJson(R"({"currentIndex":1})")->GetRoot());
    nav->InvokeOnMoveChild(childB, 1, 0);
    nav->InvokeOnRemoveChild(childA);

    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    EXPECT_GT(CountAttributeCall(childA->GetNativeView(), NODE_VISIBILITY), 0);
    EXPECT_GT(CountAttributeCall(childB->GetNativeView(), NODE_VISIBILITY), 0);
}

/**
 * @tc.name: NavContainer children 描述符早退
 * @tc.desc: 覆盖非对象描述符和缺少 children 字段时的重置与早退分支。
 * @tc.type: FUNC
 */
TEST_F(NavContainerComponentTddTest, L0_should_reset_child_list_descriptor_when_descriptor_is_invalid_or_missing)
{
    auto nav = std::make_shared<NavContainerComponentProbe>();

    auto validDescriptor = ParseJson(R"({"children":["page-a"]})");
    ASSERT_NE(validDescriptor, nullptr);
    nav->InvokeCollectChildListDescriptor(validDescriptor->GetRoot());
    ASSERT_EQ(nav->GetChildListDescriptor().type, ChildListType::STATIC_IDS);
    ASSERT_EQ(nav->GetChildListDescriptor().staticChildIds.size(), 1U);

    nav->InvokeCollectChildListDescriptor(JsonValue());
    EXPECT_EQ(nav->GetChildListDescriptor().type, ChildListType::INVALID);
    EXPECT_TRUE(nav->GetChildListDescriptor().staticChildIds.empty());

    auto missingChildrenDescriptor = ParseJson(R"({"currentIndex":0})");
    ASSERT_NE(missingChildrenDescriptor, nullptr);
    nav->InvokeCollectChildListDescriptor(missingChildrenDescriptor->GetRoot());
    EXPECT_EQ(nav->GetChildListDescriptor().type, ChildListType::INVALID);
    EXPECT_TRUE(nav->GetChildListDescriptor().staticChildIds.empty());
}
