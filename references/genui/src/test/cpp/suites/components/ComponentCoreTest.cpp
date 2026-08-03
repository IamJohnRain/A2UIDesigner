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

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "components/Component.h"
#include "utils/JsonAdapter.h"

#include "ArkUINativeAPI.h"
#include "SurfaceSlot.h"
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
    int32_t setAttributeCount = 0;
    int32_t removeChildCount = 0;
    int32_t disposeNodeCount = 0;
    std::vector<NativeAttributeCall> attributeCalls;
    std::vector<std::pair<ArkUI_NodeHandle, ArkUI_NodeHandle>> removeChildCalls;
};

NativeCallTracker g_tracker;

void ResetTracker()
{
    g_tracker = NativeCallTracker {};
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

int32_t TrackRemoveChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    ++g_tracker.removeChildCount;
    g_tracker.removeChildCalls.emplace_back(parent, child);
    return 0;
}

void TrackDisposeNode(ArkUI_NodeHandle)
{
    ++g_tracker.disposeNodeCount;
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

const NativeAttributeCall* FindLastAttributeCall(int32_t attribute)
{
    for (auto it = g_tracker.attributeCalls.rbegin(); it != g_tracker.attributeCalls.rend(); ++it) {
        if (it->attribute == attribute) {
            return &(*it);
        }
    }
    return nullptr;
}

class ComponentProbe : public Component {
public:
    explicit ComponentProbe(ArkUI_NodeHandle nativeView, bool ownsNativeView = false, bool isCompositeType = false)
        : Component(nativeView, ownsNativeView, isCompositeType)
    {}

    std::string GetType() const override
    {
        return typeName_;
    }

    void SetTypeName(const std::string& typeName)
    {
        typeName_ = typeName;
    }

    void InvokeApplyCommonAttributes(const JsonValue& descriptor)
    {
        ApplyCommonAttributes(descriptor);
    }

    void InvokeSetLayoutWeight(float weight)
    {
        SetLayoutWeight(weight);
    }

    void InvokeSetAccessibilityLabel(const std::string& label)
    {
        SetAccessibilityLabel(label);
    }

    void InvokeSetAccessibilityDescription(const std::string& description)
    {
        SetAccessibilityDescription(description);
    }

    void InvokeSetPropertyFromDescriptor(
        const std::string& propertyKey, const JsonValue& descriptor, const std::string& bindingKey = "")
    {
        SetPropertyFromDescriptor(propertyKey, descriptor, bindingKey);
    }

    void InvokeApplySchemaProperty(const std::string& propertyKey, const JsonValue& descriptor)
    {
        ApplySchemaProperty(propertyKey, descriptor);
    }

    void InvokeSetVisibility(A2UIVisibility visibility)
    {
        SetVisibility(visibility);
    }

    void InvokeOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        Component::OnRemoveChild(child);
    }

    void InvokeValidateChecksSpecialProperty(const JsonValue& value)
    {
        ValidateChecksSpecialProperty(value);
    }

    void InvokeValidateActionSpecialProperty(const JsonValue& value)
    {
        ValidateActionSpecialProperty(value);
    }

    bool InvokeShouldValidateUnknownDescriptorFields() const
    {
        return ShouldValidateUnknownDescriptorFields();
    }

    bool InvokeIsKnownAdditionalDescriptorKey(const std::string& propertyName) const
    {
        return IsKnownAdditionalDescriptorKey(propertyName);
    }

    bool InvokeHandleSpecialProperty(const std::string& propertyName, const JsonValue& value)
    {
        return HandleSpecialProperty(propertyName, value);
    }

    bool InvokeIsKnownNestedDescriptorKey(const std::string& objectName, const std::string& propertyName) const
    {
        return IsKnownNestedDescriptorKey(objectName, propertyName);
    }

    void DeclarePrivateProperty(const PropertyDeclaration& declaration)
    {
        privateDeclarations_[declaration.name] = declaration;
    }

    int32_t onAddCount = 0;
    int32_t onMoveCount = 0;
    int32_t onRemoveCount = 0;
    std::vector<std::string> appliedPropertyNames;
    std::vector<std::string> removedPropertyNames;
    std::map<std::string, JsonValue> storedProperties;

protected:
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override
    {
        ++onAddCount;
        lastAddChild = child;
        lastAddIndex = index;
    }

    void OnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex) override
    {
        ++onMoveCount;
        lastMoveChild = child;
        lastMoveFrom = currentIndex;
        lastMoveTo = targetIndex;
    }

    void OnRemoveChild(const std::shared_ptr<Component>& child) override
    {
        ++onRemoveCount;
        Component::OnRemoveChild(child);
    }

    void OnPropertyApplied(const std::string& propertyName, const JsonValue& value) override
    {
        appliedPropertyNames.push_back(propertyName);
        storedProperties[propertyName] = value;
    }

    void OnPropertyRemoved(const std::string& propertyName) override
    {
        removedPropertyNames.push_back(propertyName);
        storedProperties.erase(propertyName);
        Component::OnPropertyRemoved(propertyName);
    }

    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override
    {
        auto it = privateDeclarations_.find(propertyName);
        if (it != privateDeclarations_.end()) {
            return it->second;
        }
        return Component::GetPrivatePropertyDeclaration(propertyName);
    }

private:
    std::string typeName_;
    std::map<std::string, PropertyDeclaration> privateDeclarations_;

public:
    std::shared_ptr<Component> lastAddChild = nullptr;
    std::shared_ptr<Component> lastMoveChild = nullptr;
    size_t lastAddIndex = 0;
    size_t lastMoveFrom = 0;
    size_t lastMoveTo = 0;
};

class BaseMoveComponentProbe : public Component {
public:
    explicit BaseMoveComponentProbe(ArkUI_NodeHandle nativeView) : Component(nativeView, false) {}

    void InvokeBaseMove(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        Component::OnMoveChild(child, currentIndex, targetIndex);
    }

    int32_t addCount = 0;
    size_t lastIndex = 0;

protected:
    void OnAddChild(const std::shared_ptr<Component>&, size_t index) override
    {
        ++addCount;
        lastIndex = index;
    }
};

class ComponentCoreTest : public A2UITest {
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
        api_->setAttribute = TrackSetAttribute;
        api_->removeChild = TrackRemoveChild;
        api_->disposeNode = TrackDisposeNode;
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

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

TEST_F(ComponentCoreTest, should_set_accessibility_group_for_composite_component)
{
    ComponentProbe component(reinterpret_cast<ArkUI_NodeHandle>(0x3001), false, true);
    const NativeAttributeCall* accessibilityGroupCall = FindLastAttributeCall(NODE_ACCESSIBILITY_GROUP);
    ASSERT_NE(accessibilityGroupCall, nullptr);
    ASSERT_FALSE(accessibilityGroupCall->values.empty());
    EXPECT_EQ(accessibilityGroupCall->values[0].i32, 1);
}

TEST_F(ComponentCoreTest, should_dispose_native_view_only_when_component_owns_it)
{
    {
        ComponentProbe ownsNative(reinterpret_cast<ArkUI_NodeHandle>(0x3002), true);
    }
    EXPECT_EQ(g_tracker.disposeNodeCount, 1);

    {
        ComponentProbe nonOwning(reinterpret_cast<ArkUI_NodeHandle>(0x3003), false);
    }
    EXPECT_EQ(g_tracker.disposeNodeCount, 1);
}

TEST_F(ComponentCoreTest, is_root_node_reflects_component_id)
{
    auto root = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4001), false);
    root->SetComponentId("root");
    EXPECT_TRUE(root->IsRootNode());

    auto child = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x4002), false);
    child->SetComponentId("child");
    EXPECT_FALSE(child->IsRootNode());
}

TEST_F(ComponentCoreTest, should_add_and_reorder_children_with_double_mount_protection)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3004), false);
    parent->SetComponentId("parent");
    auto childA = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3005), false);
    auto childB = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3006), false);

    parent->AddChildAt(nullptr, 0);
    EXPECT_EQ(parent->onAddCount, 0);

    parent->AddChildAt(childA, 0);
    parent->AddChildAt(childB, 1);
    EXPECT_EQ(parent->onAddCount, 2);
    EXPECT_TRUE(parent->HasChild(childA));
    EXPECT_TRUE(parent->HasChild(childB));
    EXPECT_EQ(childA->GetParentId(), "parent");

    parent->AddChildAt(childA, 0);
    EXPECT_EQ(parent->onMoveCount, 0);

    parent->AddChildAt(childA, 3);
    EXPECT_EQ(parent->onMoveCount, 1);
    EXPECT_EQ(parent->lastMoveChild, childA);
    EXPECT_EQ(parent->lastMoveFrom, 0U);
    EXPECT_EQ(parent->lastMoveTo, 1U);

    auto otherParent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3007), false);
    otherParent->SetComponentId("other");
    auto childC = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3008), false);
    childC->SetParentId("other");
    childC->SetParent(otherParent);

    parent->AddChildAt(childC, 0);
    EXPECT_FALSE(parent->HasChild(childC));
    EXPECT_EQ(childC->GetParent(), otherParent);
}

TEST_F(ComponentCoreTest, should_clear_children_and_remove_only_valid_native_nodes)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3009), false);
    auto validChild = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3010), false);
    auto invalidChild = std::make_shared<ComponentProbe>(nullptr, false);

    parent->AddChildAt(validChild, 0);
    parent->AddChildAt(invalidChild, 1);
    EXPECT_EQ(parent->GetChildren().size(), 2U);

    parent->ClearChildren();
    EXPECT_EQ(parent->GetChildren().size(), 0U);
    EXPECT_EQ(parent->onRemoveCount, 1);
    EXPECT_EQ(g_tracker.removeChildCount, 1);
}

TEST_F(ComponentCoreTest, should_set_component_id_attribute_only_when_value_changes_and_view_exists)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3011), false);
    ResetTracker();

    component->SetComponentId("first-id");
    EXPECT_EQ(CountAttributeCall(NODE_ID), 1);
    const NativeAttributeCall* idCall = FindLastAttributeCall(NODE_ID);
    ASSERT_NE(idCall, nullptr);
    EXPECT_EQ(idCall->stringValue, "first-id");

    component->SetComponentId("first-id");
    EXPECT_EQ(CountAttributeCall(NODE_ID), 1);

    ComponentProbe noViewComponent(nullptr, false);
    noViewComponent.SetComponentId("second-id");
    EXPECT_EQ(CountAttributeCall(NODE_ID), 1);
    EXPECT_EQ(noViewComponent.GetComponentId(), "second-id");
}

TEST_F(ComponentCoreTest, should_add_and_remove_bindings_without_duplicates)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3012), false);

    component->AddBinding("", "/a");
    component->AddBinding("prop", "");
    EXPECT_TRUE(component->GetDataBindings().empty());

    component->AddBinding("prop", "/a");
    component->AddBinding("prop", "/a");
    component->AddBinding("prop", "/b");
    component->AddBinding("other", "/c");
    EXPECT_EQ(component->GetDataBindings().size(), 3U);

    component->RemoveBindingsForProperty("prop");
    ASSERT_EQ(component->GetDataBindings().size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].propertyName_, "other");
}

TEST_F(ComponentCoreTest, should_apply_common_attributes_for_accessibility_and_weight)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3013), false);
    std::unique_ptr<JsonAdapter> adapter = ParseJson(
        R"({
            "weight": 9.2,
            "accessibility": {
                "label": "label-text",
                "description": "desc-text"
            }
        })");
    ASSERT_NE(adapter, nullptr);

    component->InvokeApplyCommonAttributes(adapter->GetRoot());

    EXPECT_EQ(CountAttributeCall(NODE_LAYOUT_WEIGHT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_TEXT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_DESCRIPTION), 1);
    EXPECT_TRUE(component->storedProperties.find("weight") != component->storedProperties.end());
    EXPECT_TRUE(component->storedProperties.find("accessibility.label") != component->storedProperties.end());
    EXPECT_TRUE(component->storedProperties.find("accessibility.description") != component->storedProperties.end());
}

TEST_F(ComponentCoreTest, should_remove_accessibility_properties_when_accessibility_is_not_object)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3014), false);
    std::unique_ptr<JsonAdapter> preset = ParseJson(R"({"accessibility":{"label":"x","description":"y"}})");
    ASSERT_NE(preset, nullptr);
    component->InvokeApplyCommonAttributes(preset->GetRoot());
    ResetTracker();

    std::unique_ptr<JsonAdapter> invalid = ParseJson(R"({"accessibility":"invalid"})");
    ASSERT_NE(invalid, nullptr);
    component->InvokeApplyCommonAttributes(invalid->GetRoot());

    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_TEXT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_DESCRIPTION), 1);
    const NativeAttributeCall* labelCall = FindLastAttributeCall(NODE_ACCESSIBILITY_TEXT);
    const NativeAttributeCall* descriptionCall = FindLastAttributeCall(NODE_ACCESSIBILITY_DESCRIPTION);
    ASSERT_NE(labelCall, nullptr);
    ASSERT_NE(descriptionCall, nullptr);
    EXPECT_EQ(labelCall->stringValue, "");
    EXPECT_EQ(descriptionCall->stringValue, "");
}

TEST_F(ComponentCoreTest, should_normalize_layout_weight_and_apply_visibility)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3015), false);

    component->InvokeSetLayoutWeight(5.8f);
    const NativeAttributeCall* positiveWeightCall = FindLastAttributeCall(NODE_LAYOUT_WEIGHT);
    ASSERT_NE(positiveWeightCall, nullptr);
    ASSERT_FALSE(positiveWeightCall->values.empty());
    EXPECT_EQ(positiveWeightCall->values[0].u32, 5U);

    component->InvokeSetLayoutWeight(-1.0f);
    const NativeAttributeCall* negativeWeightCall = FindLastAttributeCall(NODE_LAYOUT_WEIGHT);
    ASSERT_NE(negativeWeightCall, nullptr);
    ASSERT_FALSE(negativeWeightCall->values.empty());
    EXPECT_EQ(negativeWeightCall->values[0].u32, 0U);

    component->InvokeSetLayoutWeight(std::numeric_limits<float>::infinity());
    const NativeAttributeCall* infWeightCall = FindLastAttributeCall(NODE_LAYOUT_WEIGHT);
    ASSERT_NE(infWeightCall, nullptr);
    ASSERT_FALSE(infWeightCall->values.empty());
    EXPECT_EQ(infWeightCall->values[0].u32, 0U);

    component->InvokeSetLayoutWeight(std::nanf(""));
    const NativeAttributeCall* nanWeightCall = FindLastAttributeCall(NODE_LAYOUT_WEIGHT);
    ASSERT_NE(nanWeightCall, nullptr);
    ASSERT_FALSE(nanWeightCall->values.empty());
    EXPECT_EQ(nanWeightCall->values[0].u32, 0U);

    component->InvokeSetVisibility(A2UIVisibility::NONE);
    const NativeAttributeCall* visibilityCall = FindLastAttributeCall(NODE_VISIBILITY);
    ASSERT_NE(visibilityCall, nullptr);
    ASSERT_FALSE(visibilityCall->values.empty());
    EXPECT_EQ(visibilityCall->values[0].i32, ARKUI_VISIBILITY_NONE);
}

TEST_F(ComponentCoreTest, should_process_schema_properties_and_dynamic_descriptor_paths)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3016), false);
    ResetTracker();

    std::unique_ptr<JsonAdapter> emptyDescriptor = ParseJson(R"({})");
    ASSERT_NE(emptyDescriptor, nullptr);
    component->InvokeApplySchemaProperty("weight", emptyDescriptor->GetRoot());
    EXPECT_EQ(CountAttributeCall(NODE_LAYOUT_WEIGHT), 1);

    std::unique_ptr<JsonAdapter> weightPathDescriptor = ParseJson(R"({"weight":{"path":"/value"}})");
    ASSERT_NE(weightPathDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("weight", weightPathDescriptor->GetRoot());
    EXPECT_TRUE(component->GetDataBindings().empty());

    std::unique_ptr<JsonAdapter> unknownPathDescriptor = ParseJson(R"({"custom":{"path":"/customPath"}})");
    ASSERT_NE(unknownPathDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("custom", unknownPathDescriptor->GetRoot());
    ASSERT_EQ(component->GetDataBindings().size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].propertyName_, "custom");
    EXPECT_EQ(component->GetDataBindings()[0].dataPath_, "/customPath");

    component->InvokeSetPropertyFromDescriptor("", unknownPathDescriptor->GetRoot());
    EXPECT_EQ(component->GetDataBindings().size(), 1U);
}

TEST_F(ComponentCoreTest, should_skip_dynamic_updates_for_non_dynamic_declared_property)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3017), false);
    ResetTracker();

    std::unique_ptr<JsonAdapter> pathValue = ParseJson(R"({"path":"/weight"})");
    ASSERT_NE(pathValue, nullptr);
    component->OnDataUpdate("weight", pathValue->GetRoot());
    EXPECT_EQ(CountAttributeCall(NODE_LAYOUT_WEIGHT), 0);

    std::unique_ptr<JsonAdapter> literalValue = JsonAdapter::CreateString("hello");
    ASSERT_NE(literalValue, nullptr);
    component->OnDataUpdate("customLiteral", literalValue->GetRoot());
    EXPECT_TRUE(component->storedProperties.find("customLiteral") != component->storedProperties.end());
}

TEST_F(ComponentCoreTest, should_route_remove_property_to_accessibility_reset)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3018), false);
    ResetTracker();

    component->RemoveProperty("accessibility.label");
    component->RemoveProperty("accessibility.description");

    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_TEXT), 1);
    EXPECT_EQ(CountAttributeCall(NODE_ACCESSIBILITY_DESCRIPTION), 1);
    ASSERT_EQ(component->removedPropertyNames.size(), 2U);
    EXPECT_EQ(component->removedPropertyNames[0], "accessibility.label");
    EXPECT_EQ(component->removedPropertyNames[1], "accessibility.description");
}

TEST_F(ComponentCoreTest, should_attach_static_children_by_ids_and_respect_existing_parent)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3019), false);
    parent->SetComponentId("parent");
    auto childA = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3020), false);
    auto childB = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3021), false);
    childA->SetComponentId("A");
    childB->SetComponentId("B");
    childB->SetParentId("other-parent");

    std::map<std::string, std::shared_ptr<Component>> allComponents { { "A", childA }, { "B", childB } };

    parent->AttachStaticChildrenByIds({ "A", "B", "missing" }, allComponents);
    EXPECT_EQ(parent->GetChildren().size(), 1U);
    EXPECT_TRUE(parent->HasChild(childA));
    EXPECT_FALSE(parent->HasChild(childB));
    EXPECT_EQ(childA->GetParentId(), "parent");

    int32_t previousAddCount = parent->onAddCount;
    parent->AttachStaticChildrenByIds({ "A", "B", "missing" }, allComponents);
    EXPECT_EQ(parent->onAddCount, previousAddCount);
}

TEST_F(ComponentCoreTest, should_report_declared_child_id_membership)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3031), false);
    parent->SetComponentId("parent");
    auto childA = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3032), false);
    auto childB = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3033), false);
    childA->SetComponentId("A");
    childB->SetComponentId("B");

    std::map<std::string, std::shared_ptr<Component>> allComponents { { "A", childA }, { "B", childB } };

    parent->AttachStaticChildrenByIds({ "A", "B" }, allComponents);
    EXPECT_TRUE(parent->HasChildId("A"));
    EXPECT_TRUE(parent->HasChildId("B"));
    EXPECT_FALSE(parent->HasChildId("C"));
    EXPECT_FALSE(parent->HasChildId(""));

    parent->AttachStaticChildrenByIds({}, allComponents);
    EXPECT_FALSE(parent->HasChildId("A"));
}

TEST_F(ComponentCoreTest, should_order_build_nodes_by_depth_id_and_pointer)
{
    SurfaceSlot::BuildNodeDepthComparator comparator;
    std::shared_ptr<ComponentProbe> root =
        std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3034), false);
    std::shared_ptr<ComponentProbe> child =
        std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3035), false);
    std::shared_ptr<ComponentProbe> sameIdA =
        std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3036), false);
    std::shared_ptr<ComponentProbe> sameIdB =
        std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3037), false);
    root->SetComponentId("root");
    child->SetComponentId("child");
    sameIdA->SetComponentId("same");
    sameIdB->SetComponentId("same");
    root->SetBuildDepth(0);
    child->SetBuildDepth(1);
    sameIdA->SetBuildDepth(1);
    sameIdB->SetBuildDepth(1);

    EXPECT_FALSE(comparator(root, root));
    EXPECT_TRUE(comparator(nullptr, root));
    EXPECT_FALSE(comparator(root, nullptr));
    EXPECT_TRUE(comparator(root, child));
    EXPECT_TRUE(comparator(child, sameIdA));
    EXPECT_NE(comparator(sameIdA, sameIdB), comparator(sameIdB, sameIdA));
}

TEST_F(ComponentCoreTest, should_not_attach_to_parent_when_parent_id_is_empty_or_missing)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3022), false);
    parent->SetComponentId("parent");
    auto child = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3023), false);
    child->SetComponentId("child");

    std::map<std::string, std::shared_ptr<Component>> allComponents { { "parent", parent }, { "child", child } };

    std::string emptyParentId;
    child->AttachToParentIfNeeded(allComponents, emptyParentId);
    EXPECT_EQ(child->GetParent(), nullptr);

    std::string missingParentId = "missing";
    child->AttachToParentIfNeeded(allComponents, missingParentId);
    EXPECT_EQ(child->GetParent(), nullptr);
}

TEST_F(ComponentCoreTest, should_keep_existing_parent_when_already_mounted_to_same_parent)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3024), false);
    parent->SetComponentId("parent");
    auto child = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3025), false);
    child->SetComponentId("child");

    parent->AddChildAt(child, 0);
    ASSERT_TRUE(parent->HasChild(child));
    ASSERT_EQ(child->GetParent(), parent);

    std::map<std::string, std::shared_ptr<Component>> allComponents { { "parent", parent }, { "child", child } };
    std::string parentId = "parent";
    child->AttachToParentIfNeeded(allComponents, parentId);
    EXPECT_EQ(child->GetParent(), parent);
    EXPECT_EQ(parent->GetChildren().size(), 1U);
}

TEST_F(ComponentCoreTest, should_call_remove_child_only_when_parent_and_child_views_are_valid)
{
    auto parent = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3026), false);
    auto validChild = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3027), false);
    auto invalidChild = std::make_shared<ComponentProbe>(nullptr, false);
    ResetTracker();

    parent->InvokeOnRemoveChild(nullptr);
    parent->InvokeOnRemoveChild(invalidChild);
    EXPECT_EQ(g_tracker.removeChildCount, 0);

    parent->InvokeOnRemoveChild(validChild);
    EXPECT_EQ(g_tracker.removeChildCount, 1);

    ComponentProbe nullParent(nullptr, false);
    nullParent.InvokeOnRemoveChild(validChild);
    EXPECT_EQ(g_tracker.removeChildCount, 1);
}

TEST_F(ComponentCoreTest, should_follow_base_on_move_child_default_behavior)
{
    auto component = std::make_shared<BaseMoveComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3028));
    auto child = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3029), false);
    component->InvokeBaseMove(child, 1, 4);

    EXPECT_EQ(component->addCount, 1);
    EXPECT_EQ(component->lastIndex, 4U);
}

TEST_F(ComponentCoreTest, should_accept_various_shapes_for_checks_and_action_validation_paths)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3030), false);

    std::unique_ptr<JsonAdapter> nonArrayChecks = ParseJson(R"({"value":1})");
    ASSERT_NE(nonArrayChecks, nullptr);
    component->InvokeValidateChecksSpecialProperty(nonArrayChecks->GetRoot());

    std::unique_ptr<JsonAdapter> mixedChecks = ParseJson(
        R"([
            1,
            {"message": 2},
            {"condition": 1, "message": "ok"},
            {"condition": {}, "message": "ok"}
        ])");
    ASSERT_NE(mixedChecks, nullptr);
    component->InvokeValidateChecksSpecialProperty(mixedChecks->GetRoot());

    std::unique_ptr<JsonAdapter> nonObjectAction = ParseJson(R"("bad")");
    ASSERT_NE(nonObjectAction, nullptr);
    component->InvokeValidateActionSpecialProperty(nonObjectAction->GetRoot());

    std::unique_ptr<JsonAdapter> bothEventAndFunction = ParseJson(R"({"event":{},"functionCall":{}})");
    ASSERT_NE(bothEventAndFunction, nullptr);
    component->InvokeValidateActionSpecialProperty(bothEventAndFunction->GetRoot());

    std::unique_ptr<JsonAdapter> missingActionContent = ParseJson(R"({})");
    ASSERT_NE(missingActionContent, nullptr);
    component->InvokeValidateActionSpecialProperty(missingActionContent->GetRoot());

    std::unique_ptr<JsonAdapter> invalidEvent = ParseJson(R"({"event":1})");
    ASSERT_NE(invalidEvent, nullptr);
    component->InvokeValidateActionSpecialProperty(invalidEvent->GetRoot());

    std::unique_ptr<JsonAdapter> missingEventName = ParseJson(R"({"event":{}})");
    ASSERT_NE(missingEventName, nullptr);
    component->InvokeValidateActionSpecialProperty(missingEventName->GetRoot());

    std::unique_ptr<JsonAdapter> invalidEventNameType = ParseJson(R"({"event":{"name":1}})");
    ASSERT_NE(invalidEventNameType, nullptr);
    component->InvokeValidateActionSpecialProperty(invalidEventNameType->GetRoot());

    std::unique_ptr<JsonAdapter> invalidEventContextType = ParseJson(R"({"event":{"name":"clicked","context":1}})");
    ASSERT_NE(invalidEventContextType, nullptr);
    component->InvokeValidateActionSpecialProperty(invalidEventContextType->GetRoot());

    std::unique_ptr<JsonAdapter> invalidFunctionCall = ParseJson(R"({"functionCall":1})");
    ASSERT_NE(invalidFunctionCall, nullptr);
    component->InvokeValidateActionSpecialProperty(invalidFunctionCall->GetRoot());

    std::unique_ptr<JsonAdapter> validAction = ParseJson(R"({"event":{"name":"clicked","context":{}}})");
    ASSERT_NE(validAction, nullptr);
    component->InvokeValidateActionSpecialProperty(validAction->GetRoot());
}

TEST_F(ComponentCoreTest, should_expose_default_virtual_behavior_for_schema_key_helpers)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3031), false);

    EXPECT_TRUE(component->InvokeShouldValidateUnknownDescriptorFields());
    EXPECT_FALSE(component->InvokeIsKnownAdditionalDescriptorKey("anything"));
    EXPECT_FALSE(component->InvokeHandleSpecialProperty("special", JsonValue()));
    EXPECT_TRUE(component->InvokeIsKnownNestedDescriptorKey("accessibility", "label"));
    EXPECT_TRUE(component->InvokeIsKnownNestedDescriptorKey("accessibility", "description"));
    EXPECT_FALSE(component->InvokeIsKnownNestedDescriptorKey("accessibility", "unknown"));
    EXPECT_FALSE(component->InvokeIsKnownNestedDescriptorKey("other", "label"));
}

TEST_F(ComponentCoreTest, should_normalize_private_boolean_and_object_properties)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3032), false);
    component->DeclarePrivateProperty(PropertyDeclaration { .name = "enabled",
        .type = PropertyValueType::BOOLEAN,
        .allowDynamic = true,
        .fallbackBool = false,
        .applyValue = [](const JsonValue&) {} });
    component->DeclarePrivateProperty(PropertyDeclaration {
        .name = "payload", .type = PropertyValueType::OBJECT, .allowDynamic = true, .applyValue = [](const JsonValue&) {
        } });

    std::unique_ptr<JsonAdapter> enabledDescriptor = ParseJson(R"({"enabled":"true"})");
    std::unique_ptr<JsonAdapter> invalidObjectDescriptor = ParseJson(R"({"payload":1})");
    std::unique_ptr<JsonAdapter> validObjectDescriptor = ParseJson(R"({"payload":{"name":"alice"}})");
    ASSERT_NE(enabledDescriptor, nullptr);
    ASSERT_NE(invalidObjectDescriptor, nullptr);
    ASSERT_NE(validObjectDescriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("enabled", enabledDescriptor->GetRoot());
    ASSERT_TRUE(component->storedProperties["enabled"].IsBool());
    EXPECT_TRUE(component->storedProperties["enabled"].GetBoolValue(false));

    component->OnDataUpdate("payload", invalidObjectDescriptor->GetRoot().GetItem("payload"));
    EXPECT_TRUE(component->storedProperties.find("payload") == component->storedProperties.end());

    component->OnDataUpdate("payload", validObjectDescriptor->GetRoot().GetItem("payload"));
    auto payloadIt = component->storedProperties.find("payload");
    ASSERT_TRUE(payloadIt != component->storedProperties.end());
    ASSERT_TRUE(payloadIt->second.IsObject());
    EXPECT_EQ(payloadIt->second.GetString("name", ""), "alice");
}

TEST_F(ComponentCoreTest, should_normalize_private_enum_property_from_invalid_literal_and_type)
{
    auto component = std::make_shared<ComponentProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x3033), false);
    component->DeclarePrivateProperty(PropertyDeclaration { .name = "variant",
        .type = PropertyValueType::ENUM_STRING,
        .allowDynamic = true,
        .fallbackString = "primary",
        .enumAllowed = { "primary", "secondary" },
        .enumFallback = "primary",
        .applyValue = [](const JsonValue&) {} });

    std::unique_ptr<JsonAdapter> invalidLiteralDescriptor = ParseJson(R"({"variant":"danger"})");
    std::unique_ptr<JsonAdapter> invalidTypeDescriptor = ParseJson(R"({"variant":123})");
    ASSERT_NE(invalidLiteralDescriptor, nullptr);
    ASSERT_NE(invalidTypeDescriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("variant", invalidLiteralDescriptor->GetRoot());
    ASSERT_TRUE(component->storedProperties["variant"].IsString());
    EXPECT_EQ(component->storedProperties["variant"].GetStringValue(""), "primary");

    component->InvokeSetPropertyFromDescriptor("variant", invalidTypeDescriptor->GetRoot());
    ASSERT_TRUE(component->storedProperties["variant"].IsString());
    EXPECT_EQ(component->storedProperties["variant"].GetStringValue(""), "primary");
}

} // namespace
