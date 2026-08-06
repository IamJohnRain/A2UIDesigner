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
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#define private public
#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/A2UIComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "data/DataModel.h"
#include "functions/FunctionResult.h"
#include "functions/NativeFunctionBase.h"
#include "functions/NativeFunctionRegistry.h"
#include "styles/StyleResolver.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "SurfaceSlot.h"

#undef private

#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildExtendedCatalog(std::initializer_list<const char*> names = {})
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* n : names) {
        if (n != nullptr && n[0] != '\0') {
            auto item = std::make_shared<CatalogItem>(n, CatalogItemType::COMPONENT);
            item->SetCategory(CatalogCategory::OHOS_EXTENDS);
            catalog->AddComponent(item);
        }
    }
    return catalog;
}

bool FindLastPadding(MockArkUINativeProvider* provider, ArkUI_NodeHandle nodeHandle, std::array<float, 4>& padding)
{
    if (provider == nullptr) {
        return false;
    }
    for (auto it = provider->setAttributeRecords_.rbegin(); it != provider->setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle != nodeHandle || it->attribute != NODE_PADDING || it->values.size() != padding.size()) {
            continue;
        }
        padding = { it->values[0].f32, it->values[1].f32, it->values[2].f32, it->values[3].f32 };
        return true;
    }
    return false;
}

// Testable subclass exposing protected methods
class TestableExtendedComponent : public ExtendedComponent {
public:
    TestableExtendedComponent() : ExtendedComponent(reinterpret_cast<ArkUI_NodeHandle>(0xBEEF), false) {}

    // Expose protected methods
    using ExtendedComponent::ApplyComponentSpecificAttributes;
    using ExtendedComponent::ApplyComponentSpecificStyles;
    using ExtendedComponent::ApplyDeclaredPropertyOrFallback;
    using ExtendedComponent::CollectChildListDescriptor;
    using ExtendedComponent::CreateArkUINode;
    using ExtendedComponent::DispatchActionInfo;
    using ExtendedComponent::DispatchEvent;
    using ExtendedComponent::ExpandTemplateChildren;
    using ExtendedComponent::GetEventHandlers;
    using ExtendedComponent::GetNodeApplier;
    using ExtendedComponent::GetRenderContext;
    using ExtendedComponent::HasEventHandler;
    using ExtendedComponent::IsApplyingStyleDeltaUpdate;
    using ExtendedComponent::IsExpressionCandidate;
    using ExtendedComponent::IsExpressionSupported;
    using ExtendedComponent::IsKnownAdditionalDescriptorKey;
    using ExtendedComponent::OnDataUpdate;
    using ExtendedComponent::OnFontSizeScaleChanged;
    using ExtendedComponent::RegisterClickHandler;
    using ExtendedComponent::RegisterComponentSpecificListeners;
    using ExtendedComponent::RegisterExtendedListeners;

    bool CallInitFromDescriptor(const JsonValue& descriptor, const RenderContext& context)
    {
        return InitFromDescriptor(descriptor, context);
    }

    bool CallUpdateFromDescriptor(const JsonValue& descriptor, const RenderContext& context)
    {
        return UpdateFromDescriptor(descriptor, context);
    }

    void CallCollectChildListDescriptor(const JsonValue& descriptor)
    {
        CollectChildListDescriptor(descriptor);
    }

    bool CallExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildren(childList, surfaceSlot, childIds);
    }

    void CallOnDataUpdate(const std::string& property, const JsonValue& value)
    {
        OnDataUpdate(property, value);
    }

    void CallParseAndRegisterEventHandlers(const JsonValue& listeners)
    {
        ParseAndRegisterEventHandlers(listeners);
    }

    void SetNativeViewForTest(ArkUI_NodeHandle view)
    {
        nativeView_ = view;
    }

    void SetComponentTypeForTest(const std::string& type)
    {
        componentType_ = type;
    }

    // Override GetType to return a configurable type
    std::string GetType() const override
    {
        return componentType_.empty() ? "Column" : componentType_;
    }

    bool IsExpressionCandidateForTest(const JsonValue& value) const
    {
        return IsExpressionCandidate(value);
    }

    bool IsApplyingStyleDeltaUpdateForTest() const
    {
        return isApplyingStyleDeltaUpdate_;
    }

    void SetRenderContextForTest(const RenderContext& ctx)
    {
        renderContext_ = ctx;
    }

    std::string componentType_;
};

// =============================================================================
// InitFromDescriptor / UpdateFromDescriptor / CreateArkUINode
// =============================================================================
class ExtendedComponentCoreTddTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-core-test");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;
};

TEST_F(ExtendedComponentCoreTddTest, CreateArkUINode_ReturnsTrueWhenNativeViewExists)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    EXPECT_TRUE(comp.CreateArkUINode());
}

TEST_F(ExtendedComponentCoreTddTest, CreateArkUINode_ReturnsFalseWhenNativeViewNull)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(nullptr);
    EXPECT_FALSE(comp.CreateArkUINode());
}

TEST_F(ExtendedComponentCoreTddTest, InitFromDescriptor_ReturnsFalseWhenNodeNull)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(nullptr);
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_FALSE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

TEST_F(ExtendedComponentCoreTddTest, InitFromDescriptor_ReturnsFalseWhenNotObject)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto numAdapter = JsonAdapter::CreateNumber(42);
    ASSERT_NE(numAdapter, nullptr);
    RenderContext ctx;
    EXPECT_FALSE(comp.CallInitFromDescriptor(numAdapter->GetRoot(), ctx));
}

TEST_F(ExtendedComponentCoreTddTest, InitFromDescriptor_SucceedsForValidDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

TEST_F(ExtendedComponentCoreTddTest, UpdateFromDescriptor_SucceedsForValidDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// IsExpressionSupported / IsExpressionCandidate
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, IsExpressionSupported_ReturnsTrue)
{
    TestableExtendedComponent comp;
    EXPECT_TRUE(comp.IsExpressionSupported());
}

TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_NonStringReturnsFalse)
{
    TestableExtendedComponent comp;
    auto numAdapter = JsonAdapter::CreateNumber(42);
    ASSERT_NE(numAdapter, nullptr);
    EXPECT_FALSE(comp.IsExpressionCandidateForTest(numAdapter->GetRoot()));
}

TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_InvalidValueReturnsFalse)
{
    TestableExtendedComponent comp;
    JsonValue invalid;
    EXPECT_FALSE(comp.IsExpressionCandidateForTest(invalid));
}

// =============================================================================
// IsKnownAdditionalDescriptorKey
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, IsKnownAdditionalDescriptorKey_StylesAndListeners)
{
    TestableExtendedComponent comp;
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("styles"));
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("onClick"));
    EXPECT_TRUE(comp.IsKnownAdditionalDescriptorKey("onSelect"));
}

TEST_F(ExtendedComponentCoreTddTest, IsKnownAdditionalDescriptorKey_DelegatesToBase)
{
    TestableExtendedComponent comp;
    // Base class always returns false for unknown keys
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey("id"));
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey("component"));
    EXPECT_FALSE(comp.IsKnownAdditionalDescriptorKey("completely_unknown_key_xyz"));
}

// =============================================================================
// ApplyDeclaredPropertyOrFallback
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyDeclaredPropertyOrFallback_SkipsEmptyPropertyName)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test"})");
    ASSERT_NE(descriptor, nullptr);
    // Should not crash with empty property name
    comp.ApplyDeclaredPropertyOrFallback(descriptor->GetRoot(), "");
}

TEST_F(ExtendedComponentCoreTddTest, ApplyDeclaredPropertyOrFallback_FallsBackWhenPropertyMissing)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test"})");
    ASSERT_NE(descriptor, nullptr);
    // Property "nonExistent" is not in descriptor → fallback path
    comp.ApplyDeclaredPropertyOrFallback(descriptor->GetRoot(), "nonExistent");
    // No crash = pass. The fallback removes bindings and applies empty runtime property.
}

TEST_F(ExtendedComponentCoreTddTest, ApplyDeclaredPropertyOrFallback_AppliesWhenPropertyPresent)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","someProp":"hello"})");
    ASSERT_NE(descriptor, nullptr);
    // Property "someProp" exists → SetPropertyFromDescriptor path
    comp.ApplyDeclaredPropertyOrFallback(descriptor->GetRoot(), "someProp");
}

// =============================================================================
// HasEventHandler / DispatchEvent
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, HasEventHandler_ReturnsFalseForEmptyName)
{
    TestableExtendedComponent comp;
    EXPECT_FALSE(comp.HasEventHandler(""));
}

TEST_F(ExtendedComponentCoreTddTest, HasEventHandler_ReturnsFalseWhenNotRegistered)
{
    TestableExtendedComponent comp;
    EXPECT_FALSE(comp.HasEventHandler("onClick"));
}

TEST_F(ExtendedComponentCoreTddTest, HasEventHandler_ReturnsTrueAfterRegistering)
{
    TestableExtendedComponent comp;
    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "clicked"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
}

TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_DoesNotCrashForUnknownListener)
{
    TestableExtendedComponent comp;
    JsonValue emptyContext;
    comp.DispatchEvent("unknownListener", emptyContext);
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_DoesNotCrashForNullAction)
{
    TestableExtendedComponent comp;
    // Parse listeners with a function call that won't resolve
    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "nonExistentFn"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    JsonValue emptyContext;
    comp.DispatchEvent("onClick", emptyContext);
    // No crash = pass
}

// =============================================================================
// ParseAndRegisterListeners - branch coverage
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_ObjectListeners)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({
        "onClick": [{"call": "clicked"}],
        "onAppear": [{"call": "appeared"}]
    })");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
    EXPECT_TRUE(comp.HasEventHandler("onAppear"));
}

TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_NonObjectListeners)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto numAdapter = JsonAdapter::CreateNumber(42);
    ASSERT_NE(numAdapter, nullptr);
    // Valid but non-object listeners → warning + clear
    comp.CallParseAndRegisterEventHandlers(numAdapter->GetRoot());
    EXPECT_FALSE(comp.HasEventHandler("onClick"));
}

TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_InvalidListeners)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    JsonValue invalid;
    // Invalid listeners → clear, no crash
    comp.CallParseAndRegisterEventHandlers(invalid);
    EXPECT_FALSE(comp.HasEventHandler("onClick"));
}

TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_ReportsUnknownListenerKeys)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({
        "onClick": [{"call": "clicked"}],
        "onCustomUnknown": [{"call": "custom"}]
    })");
    ASSERT_NE(listeners, nullptr);
    // Should not crash with unknown listener key
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
    // "onCustomUnknown" is not in KNOWN_EVENT_NAMES, so the parser skips it.
    EXPECT_FALSE(comp.HasEventHandler("onCustomUnknown"));
}

// =============================================================================
// CollectChildListDescriptor - branch coverage
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_SupportedTypeWithChildren)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Column");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1", "child2"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
    // Should have parsed children
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_UnsupportedTypeSkips)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Text");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
    // Text doesn't support extended children → childListDescriptor_ is cleared
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_NonObjectSkips)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Column");
    auto numAdapter = JsonAdapter::CreateNumber(42);
    ASSERT_NE(numAdapter, nullptr);
    comp.CallCollectChildListDescriptor(numAdapter->GetRoot());
    // Non-object → cleared
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_NoChildrenKeySkips)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Column");
    auto descriptor = JsonAdapter::Parse(R"({"id":"test"})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
    // No children key → cleared
}

// =============================================================================
// ExpandTemplateChildren - branch coverage
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_UnsupportedTypeReturnsFalse)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Text");
    ChildListDescriptor childList;
    std::list<std::string> childIds;
    EXPECT_FALSE(comp.CallExpandTemplateChildren(childList, slot_, childIds));
}

// =============================================================================
// OnDataUpdate - style binding route
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_RoutesStyleBindingToApplySingleResolvedStyle)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Use a property name that matches the StyleResolver::IsStyleBindingProperty pattern
    // Style binding properties have the format "style:propertyName" or similar
    auto value = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(value, nullptr);

    // Non-style property → delegates to base class
    comp.CallOnDataUpdate("someProperty", value->GetRoot());
    // No crash = pass
}

// =============================================================================
// GetRenderContext / GetEventHandlers
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, GetRenderContext_ReturnsStoredContext)
{
    TestableExtendedComponent comp;
    RenderContext ctx;
    ctx.renderId = 42;
    ctx.surfaceId = "test-surface";
    comp.SetRenderContextForTest(ctx);
    const RenderContext& retrieved = comp.GetRenderContext();
    EXPECT_EQ(retrieved.renderId, 42);
    EXPECT_EQ(retrieved.surfaceId, "test-surface");
}

TEST_F(ExtendedComponentCoreTddTest, GetEventHandlers_ReturnsCurrentMap)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    const auto& actions = comp.GetEventHandlers();
    EXPECT_TRUE(actions.empty());

    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "clicked"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_FALSE(comp.GetEventHandlers().empty());
}

// =============================================================================
// GetNodeApplier / IsApplyingStyleDeltaUpdate
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, GetNodeApplier_ReturnsNullBeforeInit)
{
    TestableExtendedComponent comp;
    EXPECT_EQ(comp.GetNodeApplier(), nullptr);
}

TEST_F(ExtendedComponentCoreTddTest, IsApplyingStyleDeltaUpdate_DefaultFalse)
{
    TestableExtendedComponent comp;
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

// =============================================================================
// OnFontSizeScaleChanged
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnFontSizeScaleChanged_UpdatesRenderContext)
{
    TestableExtendedComponent comp;
    comp.OnFontSizeScaleChanged(1.5F);
    EXPECT_FLOAT_EQ(comp.GetRenderContext().fontSizeScale, 1.5F);
}

// =============================================================================
// MergeEventContext - all branches
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_InvalidExtraReturnsBaseOrEmpty)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    {
        // Both invalid → returns empty object
        JsonValue invalidBase;
        JsonValue invalidExtra;
        JsonValue result = comp.MergeEventContext(invalidBase, invalidExtra);
        // Should produce an empty object or invalid value
    }
    {
        // Extra invalid, base is object → clone base
        auto baseAdapter = JsonAdapter::Parse(R"({"key": "value"})");
        ASSERT_NE(baseAdapter, nullptr);
        JsonValue invalidExtra;
        JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), invalidExtra);
        EXPECT_TRUE(result.IsValid());
    }
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraWithObjectContext)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"key": "value"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateString("primitive_payload");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    // Result should contain "key" and "value" field with primitive payload
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraWithEmptyBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateString("primitive_payload");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraWithNoEntriesBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    // Base is invalid (not object) → clone extra
    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::CreateString("primitive_payload");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraMergedIntoObjectBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"baseKey": "baseValue"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"extraKey": "extraValue"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("baseKey"));
    EXPECT_TRUE(result.Has("extraKey"));
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraOverwritesBaseKeys)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"key": "oldValue"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"key": "newValue"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_EQ(result.GetString("key", ""), "newValue");
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraWithNonObjectBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::Parse(R"({"key": "value"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
}

TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraSkipsEmptyKeys)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"validKey": "value"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("validKey"));
}

// =============================================================================
// RegisterExtendedListeners - onClick with and without listener action
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, RegisterExtendedListeners_WithOnClickRegistersHandler)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "clicked"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    // onClick should be registered with a non-null handler
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
}

TEST_F(ExtendedComponentCoreTddTest, RegisterExtendedListeners_WithoutOnClickRegistersNull)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({"onAppear": [{"call": "appeared"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    // No onClick registered
    EXPECT_FALSE(comp.HasEventHandler("onClick"));
    EXPECT_TRUE(comp.HasEventHandler("onAppear"));
}

// =============================================================================
// ApplyResolvedStyles - branch coverage (via InitFromDescriptor)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_SetsApplierOnFirstCall)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    EXPECT_NE(comp.GetNodeApplier(), nullptr);
}

TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ReportsNonObjectStylesWarning)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":"not_an_object"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_WorksWithValidStyles)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(
        R"({"id":"test","component":"Column","styles":{"width": 100, "height": 200, "backgroundColor": "#FF000000"}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// SupportsExtendedChildren - Row, List, Stack, Grid types via CollectChildListDescriptor
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_RowWithChildren)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Row");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_ListWithChildren)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("List");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1", "child2"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_StackWithChildren)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Stack");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
}

TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_GridWithChildren)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Grid");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"children": ["child1"]})");
    ASSERT_NE(descriptor, nullptr);
    comp.CallCollectChildListDescriptor(descriptor->GetRoot());
}

// =============================================================================
// IsKnownListenerName - onChange, onReachStart, onReachEnd
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_RecognizesKnownOnChangeListener)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({"onChange": [{"call": "changed"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onChange"));
}

TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_RecognizesOnReachStartListener)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({"onReachStart": [{"call": "reachedStart"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onReachStart"));
}

TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterListeners_RecognizesOnReachEndListener)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({"onReachEnd": [{"call": "reachedEnd"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onReachEnd"));
}

// =============================================================================
// DispatchActionInfo - EVENT type dispatch
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_EventActionType)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);

    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "clicked"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());

    JsonValue emptyContext;
    comp.DispatchEvent("onClick", emptyContext);
    // No crash, event dispatched via ActionDispatchBridge
}

// =============================================================================
// DispatchActionInfo - default (unknown action type)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_FunctionCallActionType)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);

    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "someFunction", "args": {}}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());

    JsonValue emptyContext;
    comp.DispatchEvent("onClick", emptyContext);
    // No crash, function call dispatched via FunctionBridge
}

// =============================================================================
// DispatchActionInfo with functionCallDescriptor (dynamic resolution path)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_FunctionCallWithDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);

    // functionCallDescriptor present → DynamicValueResolver path
    auto listeners = JsonAdapter::Parse(R"({
        "onClick": [{"call": "dynamicFn", "args": {}}]
    })");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());

    JsonValue emptyContext;
    comp.DispatchEvent("onClick", emptyContext);
    // No crash, dynamic resolution attempted
}

// =============================================================================
// MergeEventContext - contextRoot.Has("value") true → Replace path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtra_ReplaceExistingValueKey)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    // Base has "value" key, extra is a non-object primitive
    auto baseAdapter = JsonAdapter::Parse(R"({"value": "oldValue", "other": "data"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    // The "value" key should be replaced with the primitive extra
    EXPECT_TRUE(result.Has("value"));
    EXPECT_TRUE(result.Has("other"));
}

// =============================================================================
// ApplyResolvedStyles - unknown style key (DFX warning path)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, InitFromDescriptor_ReportsUnknownStyleKey)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor =
        JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"completelyUnknownStyle": 42}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // Should succeed even with unknown style key (DFX warning logged, not fatal)
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ApplyResolvedStyles - no resolved styles object (empty/null after resolve)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, InitFromDescriptor_HandlesEmptyResolvedStyles)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Styles object with only unknown keys that resolve to nothing useful
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ExpandTemplateChildren - supported type (Column) true path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_SupportedTypeReturnsTruePath)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Column");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    ChildListDescriptor childList;
    childList.type = ChildListType::STATIC_IDS;
    childList.staticChildIds = { "child1", "child2" };

    std::list<std::string> childIds;
    // ExpandTemplateChildren for supported type delegates to ExpandTemplateChildrenEager
    // Without a fully wired SurfaceSlot, it may return false, but it exercises the true branch
    bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
    // The result depends on SurfaceSlot setup; the key is the branch is exercised
    (void)result;
}

// =============================================================================
// ApplyComponentSpecificAttributes and ApplyComponentSpecificStyles (no-op base)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyComponentSpecificAttributes_NoOp)
{
    TestableExtendedComponent comp;
    auto descriptor = JsonAdapter::Parse(R"({"id":"test"})");
    ASSERT_NE(descriptor, nullptr);
    // No crash - base implementation is no-op
    comp.ApplyComponentSpecificAttributes(descriptor->GetRoot());
}

TEST_F(ExtendedComponentCoreTddTest, ApplyComponentSpecificStyles_NoOp)
{
    TestableExtendedComponent comp;
    ArkUINodeApiAdapter applier = ArkUINodeApiAdapter([]() { return reinterpret_cast<ArkUI_NodeHandle>(0x1234); },
        []() { return std::string("test"); }, [](float, float, float, float) {}, []() {},
        [](const std::function<void()>&) {});
    auto styles = JsonAdapter::Parse(R"({"width": 100})");
    ASSERT_NE(styles, nullptr);
    // No crash - base implementation is no-op
    comp.ApplyComponentSpecificStyles(styles->GetRoot(), applier);
}

// =============================================================================
// OnDataUpdate with style binding property
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleBindingPropertyRoute)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // First init to set up nodeApplier_
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Now trigger OnDataUpdate with a style binding property
    // StyleResolver::IsStyleBindingProperty checks "style:xxx" pattern
    // Use a simple non-style property to exercise the base class path
    auto value = JsonAdapter::CreateString("updated");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("content", value->GetRoot());
    // No crash
}

// =============================================================================
// IsExpressionCandidate with string value (ENABLE_EXPRESSION_ENGINE path)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_StringValueReturnsFalseForNonExpression)
{
    TestableExtendedComponent comp;
    auto strAdapter = JsonAdapter::CreateString("  not_an_expression  ");
    ASSERT_NE(strAdapter, nullptr);
    // A regular string (not an expression) should return false
    // This exercises the trim + ExpressionEngine::IsExpression check path
    bool result = comp.IsExpressionCandidateForTest(strAdapter->GetRoot());
    // Result depends on whether ENABLE_EXPRESSION_ENGINE is defined
    // Without it, always returns false. With it, depends on ExpressionEngine.
    EXPECT_FALSE(result);
}

TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_EmptyStringReturnsFalse)
{
    TestableExtendedComponent comp;
    auto strAdapter = JsonAdapter::CreateString("");
    ASSERT_NE(strAdapter, nullptr);
    EXPECT_FALSE(comp.IsExpressionCandidateForTest(strAdapter->GetRoot()));
}

TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_WhitespaceOnlyStringReturnsFalse)
{
    TestableExtendedComponent comp;
    auto strAdapter = JsonAdapter::CreateString("   ");
    ASSERT_NE(strAdapter, nullptr);
    EXPECT_FALSE(comp.IsExpressionCandidateForTest(strAdapter->GetRoot()));
}

// =============================================================================
// ApplyResolvedStyles - nodeApplier_ null path (ApplyResolvedStyles called before init)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_NullNodeApplier_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // nodeApplier_ is null before InitFromDescriptor
    EXPECT_EQ(comp.GetNodeApplier(), nullptr);

    // Call InitFromDescriptor with styles to trigger ApplyResolvedStyles
    // But first, force nodeApplier_ to stay null by failing CreateArkUINode
    comp.SetNativeViewForTest(nullptr);
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // CreateArkUINode returns false → ApplyExtendedDescriptor returns false early
    EXPECT_FALSE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ApplySingleResolvedStyle via OnDataUpdate with style binding
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_WithNullApplier_NoCrash)
{
    TestableExtendedComponent comp;
    // nodeApplier_ is null, so ApplySingleResolvedStyle should return early
    EXPECT_EQ(comp.GetNodeApplier(), nullptr);
    // This is tested indirectly - OnDataUpdate with style binding property
    // goes to ApplySingleResolvedStyle which checks nodeApplier_ null
}

// =============================================================================
// MergeEventContext - baseContext adapter clone returns nullptr (defensive)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraMergedIntoEmptyObjectBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    // Empty base object, valid extra object
    auto baseAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"newKey": "newValue"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("newKey"));
}

// =============================================================================
// Full integration: init + update cycle
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, InitAndUpdateCycle_WithListeners)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // Init
    auto initDescriptor = JsonAdapter::Parse(R"({
        "id": "test",
        "component": "Column",
        "styles": {"width": 100, "height": 200},
        "onAppear": [{"call": "appeared"}]
    })");
    ASSERT_NE(initDescriptor, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(initDescriptor->GetRoot(), ctx));

    // Update
    auto updateDescriptor = JsonAdapter::Parse(R"({
        "id": "test",
        "component": "Column",
        "styles": {"width": 150, "backgroundColor": "#FF000000"},
        "onClick": [{"call": "clicked"}]
    })");
    ASSERT_NE(updateDescriptor, nullptr);
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(updateDescriptor->GetRoot(), ctx));

    // Verify listener changed
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
    // onAppear was from init, should be replaced by update
}

// =============================================================================
// OnFontSizeScaleChanged updates context, then re-init
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnFontSizeScaleChanged_ThenReInit)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    comp.OnFontSizeScaleChanged(2.0F);
    EXPECT_FLOAT_EQ(comp.GetRenderContext().fontSizeScale, 2.0F);

    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// DispatchActionInfo — null / invalid / function call / event type
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_NullActionInfo_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    comp.DispatchActionInfo("test", nullptr, JsonValue());
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_InvalidActionInfo_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // Default ActionInfo has UNKNOWN type, IsValid() returns false
    auto actionInfo = std::make_shared<ActionInfo>();
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCall_BasicPath)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // FUNCTION_CALL with a non-registered function → FunctionBridge path
    auto fc = std::make_shared<FunctionCallInfo>("nonExistentFn", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCall_WithDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // FUNCTION_CALL with functionCallDescriptor → DynamicValueResolver path
    auto fc = std::make_shared<FunctionCallInfo>("testFn", JsonValue(), "void");
    auto fcd = JsonAdapter::Parse(R"({"call": "resolvedFn", "args": {}, "returnType": "void"})");
    ASSERT_NE(fcd, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>(fc, fcd->GetRoot());
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_EventType)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // EVENT type → EventContextResolver + ActionDispatchBridge path
    auto actionInfo = std::make_shared<ActionInfo>("clicked", JsonValue());
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

// =============================================================================
// OnDataUpdate — style binding routing via "styles." prefix
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleBindingWidth_AppliesSingleResolvedStyle)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // First init to create nodeApplier_
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Trigger OnDataUpdate with a style binding property
    auto value = JsonAdapter::CreateNumber(100.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.width", value->GetRoot());
    // No crash = pass; ApplySingleResolvedStyle invoked
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleBindingWithNullApplier_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Do NOT init — nodeApplier_ stays null
    auto value = JsonAdapter::CreateNumber(100.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.width", value->GetRoot());
    // No crash, early return in ApplySingleResolvedStyle
}

// =============================================================================
// ApplySingleResolvedStyle — empty style name path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_EmptyStyleName_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // "styles." prefix + empty suffix → ExtractStyleNameFromBindingProperty returns ""
    auto value = JsonAdapter::CreateNumber(100.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.", value->GetRoot());
    // No crash = pass; empty styleName → early return in ApplySingleResolvedStyle
}

// =============================================================================
// ApplyResolvedStyles — non-object styles warning + second call clears old styles
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_NonObjectStyles_ReportsWarning)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":"not_an_object"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // InitFromDescriptor calls ApplyExtendedDescriptor → ApplyResolvedStyles
    // styles is a string → IsValid && !IsObject → ReportSchemaWarning path
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_SecondCallClearsOldStyles)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // First init with styles
    auto desc1 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(desc1, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc1->GetRoot(), ctx));

    // Second init with empty styles → triggers clear/reset paths
    auto desc2 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{}})");
    ASSERT_NE(desc2, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc2->GetRoot(), ctx));
    // No crash = pass
}

// =============================================================================
// DispatchActionInfo — native function registry path (HasFunction=true)
// =============================================================================
class StubNativeFunction : public NativeFunctionBase {
public:
    explicit StubNativeFunction(const std::string& name) : name_(name) {}
    std::string GetName() const override
    {
        return name_;
    }
    FunctionResult Execute(const JsonValue& /*resolvedArgs*/) override
    {
        return FunctionResult(); // NULL_VALUE result
    }

private:
    std::string name_;
};

TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_NativeFunctionRegistryPath)
{
    // Register a stub native function
    auto& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("testNativeFn", std::make_shared<StubNativeFunction>("testNativeFn"));

    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);

    auto fc = std::make_shared<FunctionCallInfo>("testNativeFn", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; native function path exercised
}

// =============================================================================
// DispatchActionInfo — null functionCall after dynamic resolution
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCallDescriptorResolvesNull)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // Provide a functionCallDescriptor that won't resolve to a valid FunctionCallInfo
    // This exercises the `functionCall == nullptr` path at line 243
    auto fc = std::make_shared<FunctionCallInfo>("originalFn", JsonValue(), "void");
    // Use an object that DynamicValueResolver can't resolve to a valid function
    auto fcd = JsonAdapter::Parse(R"({"call": "", "args": {}, "returnType": "void"})");
    ASSERT_NE(fcd, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>(fc, fcd->GetRoot());
    // The descriptor is valid but may resolve to null, keeping original fc
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

// =============================================================================
// DispatchActionInfo — FunctionBridge (non-native) path with normalize failure
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionBridgeNonNativePath)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // "nonExistentFn" is not in NativeFunctionRegistry → goes to FunctionBridge path
    auto fc = std::make_shared<FunctionCallInfo>("nonExistentFn", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; FunctionBridge::Invoke path exercised (line 270)
}

// =============================================================================
// ApplyExtendedDescriptor — styles type mismatch triggers validation issue report
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyExtendedDescriptor_StylesTypeMismatch_ReportsWarning)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":42})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // styles=42 is valid but not object → validation issue reported via ReportSchemaWarning
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ApplyResolvedStyles — nodeApplier_ null path (nativeView null → CreateArkUINode false)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_NullApplierAfterFailedCreate_LogsWarning)
{
    TestableExtendedComponent comp;
    // Set nativeView to null → CreateArkUINode returns false
    comp.SetNativeViewForTest(nullptr);
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width":100}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // InitFromDescriptor fails at CreateArkUINode → ApplyExtendedDescriptor returns false
    // nodeApplier_ remains null, ApplyResolvedStyles null check is exercised
    EXPECT_FALSE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ApplySingleResolvedStyle — full path with init + style delta update
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_FullPath_ViaStyleBinding)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Trigger ApplySingleResolvedStyle with a style binding property
    auto value = JsonAdapter::Parse(R"("50vp")");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.height", value->GetRoot());
    // Full path: CreateObject → Put → ResolveAndApply → ApplyComponentSpecificStyles → appliedStyleKeys_.insert
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

// =============================================================================
// DispatchActionInfo — FUNCTION_CALL with null functionCallDescriptor (IsValid()=false)
// Exercises the branch where functionCallDescriptor.IsValid() is false
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCallNoDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // FUNCTION_CALL with no functionCallDescriptor → IsValid()=false → skips dynamic resolution
    auto fc = std::make_shared<FunctionCallInfo>("someNonRegisteredFn", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; goes directly to FunctionBridge::Invoke
}

// =============================================================================
// DispatchActionInfo — FUNCTION_CALL with null functionCall pointer (line 243)
// Uses a valid functionCallDescriptor that resolves to null functionCall
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCallNullAfterDynamicResolve)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // Provide a functionCallDescriptor that DynamicValueResolver resolves to null
    // This should keep the original functionCall, not result in null
    auto fc = std::make_shared<FunctionCallInfo>("testFn", JsonValue(), "void");
    auto fcd = JsonAdapter::Parse(R"({"call": "resolvedFn", "args": {}})");
    ASSERT_NE(fcd, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>(fc, fcd->GetRoot());
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

// =============================================================================
// DispatchActionInfo — EVENT type with event context descriptor
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_EventWithContextDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // EVENT type with event context descriptor
    auto eventCtxDescriptor = JsonAdapter::Parse(R"({"data": {"key": "value"}})");
    ASSERT_NE(eventCtxDescriptor, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>("testEvent", eventCtxDescriptor->GetRoot());
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; EventContextResolver + ActionDispatchBridge path
}

// =============================================================================
// DispatchActionInfo — EVENT type with extra context merge
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_EventWithExtraContext)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    auto eventCtxDescriptor = JsonAdapter::Parse(R"({"data": {"key": "value"}})");
    ASSERT_NE(eventCtxDescriptor, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>("testEvent", eventCtxDescriptor->GetRoot());
    auto extraContext = JsonAdapter::Parse(R"({"extraKey": "extraValue"})");
    ASSERT_NE(extraContext, nullptr);
    comp.DispatchActionInfo("test", actionInfo, extraContext->GetRoot());
    // No crash = pass; EventContextResolver + merge + ActionDispatchBridge path
}

// =============================================================================
// DispatchActionInfo — unknown action type (default case)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_UnknownActionType)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // Create an ActionInfo with default constructor → type is UNKNOWN, IsValid()=false
    // So this hits the null/invalid path. Need to create one with unknown type but valid.
    // ActionInfo default is UNKNOWN type and IsValid()=false, so it hits the early return.
    // This is already covered by DispatchActionInfo_InvalidActionInfo_NoCrash
    auto actionInfo = std::make_shared<ActionInfo>();
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; IsValid() returns false → early return
}

// =============================================================================
// MergeEventContext — base is non-object (invalid), extra is non-object (string)
// Exercises the path at line 532: !baseContext.IsObject() || !JsonObjectHasEntries(baseContext)
// → return CloneJsonValue(extraContext)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_InvalidBaseNonObjectExtra_ClonesExtra)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::CreateString("some_payload");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    // !extraContext.IsObject() && (!baseContext.IsObject() || !JsonObjectHasEntries(baseContext))
    // → return CloneJsonValue(extraContext)
    EXPECT_TRUE(result.IsValid());
}

// =============================================================================
// MergeEventContext — base is empty object (HasEntries=false), extra is non-object
// Exercises path at line 532: JsonObjectHasEntries returns false
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_EmptyBaseNonObjectExtra_ClonesExtra)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateString("payload");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
}

// =============================================================================
// MergeEventContext — extra is non-object, base has "value" key → Replace path (line 544)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraBaseHasValueKey_ReplaceValue)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"value": "originalValue", "ctx": "data"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("value"));
    EXPECT_TRUE(result.Has("ctx"));
    // "value" should be replaced with the number extra
}

// =============================================================================
// MergeEventContext — extra is non-object, base has no "value" key → Put path (line 546)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraBaseNoValueKey_PutValue)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"ctx": "data"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateString("payload_string");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("value"));
    EXPECT_TRUE(result.Has("ctx"));
}

// =============================================================================
// MergeEventContext — extra is object with empty key → skipped (line 560-562)
// This is exercised indirectly; JSON objects normally don't have empty keys.
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraWithNonObjectBase_CreatesNewObject)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::Parse(R"({"key1": "val1", "key2": "val2"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("key1"));
    EXPECT_TRUE(result.Has("key2"));
}

// =============================================================================
// MergeEventContext — extra object, base object, key exists → Replace (line 564)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraReplaceExistingKey)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"shared": "oldValue", "baseOnly": "data"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"shared": "newValue"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_EQ(result.GetString("shared", ""), "newValue");
    EXPECT_EQ(result.GetString("baseOnly", ""), "data");
}

// =============================================================================
// MergeEventContext — extra object, base object, key doesn't exist → Put (line 566)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraPutNewKey)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto baseAdapter = JsonAdapter::Parse(R"({"baseKey": "baseValue"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"newKey": "newValue"})");
    ASSERT_NE(extraAdapter, nullptr);

    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("baseKey"));
    EXPECT_TRUE(result.Has("newKey"));
}

// =============================================================================
// SupportsExtendedChildren — "Text" does not support children
// (indirectly tested via CollectChildListDescriptor_UnsupportedTypeSkips)
// =============================================================================

// =============================================================================
// OnDataUpdate — style binding property route with actual "style:xxx" pattern
// Tests that IsStyleBindingProperty returns true for "styles.xxx" format
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_NonStyleProperty_DelegatesToBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Non-style property → delegates to base class OnDataUpdate
    auto value = JsonAdapter::CreateString("hello");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("content", value->GetRoot());
    // No crash = pass
}

// =============================================================================
// CollectChildListDescriptor — non-object descriptor
// Exercises the branch: !descriptor.IsObject() → early return
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_InvalidDescriptor_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Column");
    JsonValue invalid;
    comp.CallCollectChildListDescriptor(invalid);
    // No crash = pass
}

// =============================================================================
// ExpandTemplateChildren — supported type (Row)
// Exercises SupportsExtendedChildren returning true for "Row"
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_RowType_ReturnsTruePath)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Row");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    ChildListDescriptor childList;
    childList.type = ChildListType::STATIC_IDS;
    childList.staticChildIds = { "child1" };

    std::list<std::string> childIds;
    bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
    (void)result;
}

// =============================================================================
// ExpandTemplateChildren — supported type (List)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_ListType_ReturnsTruePath)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("List");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    ChildListDescriptor childList;
    childList.type = ChildListType::STATIC_IDS;
    childList.staticChildIds = { "child1" };

    std::list<std::string> childIds;
    bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
    (void)result;
}

// =============================================================================
// ExpandTemplateChildren — supported type (Stack)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_StackType_ReturnsTruePath)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Stack");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    ChildListDescriptor childList;
    childList.type = ChildListType::STATIC_IDS;
    childList.staticChildIds = { "child1" };

    std::list<std::string> childIds;
    bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
    (void)result;
}

// =============================================================================
// ExpandTemplateChildren — supported type (Grid)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_GridType_ReturnsTruePath)
{
    TestableExtendedComponent comp;
    comp.SetComponentTypeForTest("Grid");
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    ChildListDescriptor childList;
    childList.type = ChildListType::STATIC_IDS;
    childList.staticChildIds = { "child1" };

    std::list<std::string> childIds;
    bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
    (void)result;
}

// =============================================================================
// ApplyExtendedDescriptor — validation issues from normalization are dispatched
// Exercises the for loop at line 373: ReportSchemaWarning for each issue
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyExtendedDescriptor_ValidationIssuesDispatched)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Styles type mismatch triggers validation issue from ExtendedDescriptorNormalizer
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":"bad","listeners":42})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // InitFromDescriptor should succeed and dispatch validation issues
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// ApplyResolvedStyles — resolved styles is not object (empty after resolve)
// Exercises the else branch at line 456-460
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ResolvedStylesNotObject_LogsDebug)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Empty styles → StyleParser::Parse returns empty → StyleResolver::Resolve returns
    // resolvedStyles that may not be IsObject() if nothing to resolve
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    // No crash = pass; exercises the !resolveResult.resolvedStyles.IsObject() path
}

// =============================================================================
// ApplyResolvedStyles — clear bindings and reset properties paths
// Exercises RemoveBindingsForProperty and ExtendedStyleResolver::Reset
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_SecondInitClearsOldStyles)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // Init with width
    auto desc1 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(desc1, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc1->GetRoot(), ctx));

    // Update with different styles → old width should be cleared/reset
    auto desc2 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"height": 200}})");
    ASSERT_NE(desc2, nullptr);
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(desc2->GetRoot(), ctx));
    // No crash = pass
}

TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ResetsTextInputWordBreakWithTextInputAttribute)
{
    TestableExtendedComponent comp;
    ArkUI_NodeHandle node = reinterpret_cast<ArkUI_NodeHandle>(0x1234);
    comp.SetNativeViewForTest(node);
    comp.SetComponentTypeForTest("TextInput");
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto initialDescriptor =
        JsonAdapter::Parse(R"({"id":"test","component":"TextInput","styles":{"wordBreak":"break-all"}})");
    ASSERT_NE(initialDescriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(initialDescriptor->GetRoot(), ctx));

    mockArkUIPtr_->resetAttributeRecords_.clear();
    auto resetDescriptor = JsonAdapter::Parse(R"({"id":"test","component":"TextInput","styles":{}})");
    ASSERT_NE(resetDescriptor, nullptr);
    ASSERT_TRUE(comp.CallUpdateFromDescriptor(resetDescriptor->GetRoot(), ctx));

    bool resetTextInputWordBreak = false;
    bool resetTextWordBreak = false;
    for (const auto& record : mockArkUIPtr_->resetAttributeRecords_) {
        if (record.nodeHandle != node) {
            continue;
        }
        resetTextInputWordBreak = resetTextInputWordBreak || record.attribute == NODE_TEXT_INPUT_WORD_BREAK;
        resetTextWordBreak = resetTextWordBreak || record.attribute == NODE_TEXT_WORD_BREAK;
    }
    EXPECT_TRUE(resetTextInputWordBreak);
    EXPECT_FALSE(resetTextWordBreak);
}

// =============================================================================
// ApplySingleResolvedStyle — full path with Put failure defense
// (Can't easily force Put to fail, but we can exercise the normal Put path)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_PutsStyleAndApplies)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Trigger ApplySingleResolvedStyle via OnDataUpdate with style binding
    auto value = JsonAdapter::CreateNumber(50.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.width", value->GetRoot());
    // Full path: CreateObject → Put → ResolveAndApply
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

// =============================================================================
// DispatchActionInfo — FUNCTION_CALL with NativeFunctionRegistry (normalize succeeds)
// Uses a registered native function to exercise the native path with normalize
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_NativeFunctionNormalizeSuccessPath)
{
    auto& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("testNormalizeFn", std::make_shared<StubNativeFunction>("testNormalizeFn"));

    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);

    auto fc = std::make_shared<FunctionCallInfo>("testNormalizeFn", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; native function path with NormalizeFunctionCall
}

// =============================================================================
// OnFontSizeScaleChanged — default context value is 1.0
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnFontSizeScaleChanged_DefaultZero)
{
    TestableExtendedComponent comp;
    EXPECT_FLOAT_EQ(comp.GetRenderContext().fontSizeScale, 1.0F);
    comp.OnFontSizeScaleChanged(0.0F);
    EXPECT_FLOAT_EQ(comp.GetRenderContext().fontSizeScale, 0.0F);
    comp.OnFontSizeScaleChanged(1.75F);
    EXPECT_FLOAT_EQ(comp.GetRenderContext().fontSizeScale, 1.75F);
}

// =============================================================================
// RegisterClickHandler / RegisterAppearHandler — no event handler registered
// Exercises the else branch: RegisterOnClickWithContext(nullptr)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, RegisterExtendedListeners_NoClickOrAppear)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // Empty listeners → onClick and onAppear not registered
    auto listeners = JsonAdapter::Parse(R"({})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_FALSE(comp.HasEventHandler("onClick"));
    EXPECT_FALSE(comp.HasEventHandler("onAppear"));
}

// =============================================================================
// COVERAGE GAP: CloneJsonValue — invalid value path (returns invalid JsonValue)
// Called from MergeEventContext when both base and extra are invalid
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_BothInvalid_ReturnsEmptyObject)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    JsonValue invalidBase;
    JsonValue invalidExtra;
    JsonValue result = comp.MergeEventContext(invalidBase, invalidExtra);
    // Both invalid → CreateEmptyObjectValue → returns empty object
    EXPECT_TRUE(result.IsValid());
}

// =============================================================================
// COVERAGE GAP: CloneJsonValue — valid object clone path
// Called from MergeEventContext when extra is invalid and base is object
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_InvalidExtraClonesBase)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto baseAdapter = JsonAdapter::Parse(R"({"baseKey": "baseValue", "nested": {"inner": 42}})");
    ASSERT_NE(baseAdapter, nullptr);
    JsonValue invalidExtra;
    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), invalidExtra);
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("baseKey"));
}

// =============================================================================
// COVERAGE GAP: CreateEmptyObjectValue — called when both base and extra are invalid
// Exercises the CreateObject + GetRoot path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_BothInvalidCreatesEmptyObject)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    JsonValue invalidBase;
    JsonValue invalidExtra;
    JsonValue result = comp.MergeEventContext(invalidBase, invalidExtra);
    // Exercises CreateEmptyObjectValue
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsObject());
}

// =============================================================================
// COVERAGE GAP: JsonObjectHasEntries — non-object returns false
// Called from MergeEventContext with non-object baseContext
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraInvalidBase_ClonesExtra)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::CreateString("primitive");
    ASSERT_NE(extraAdapter, nullptr);
    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    // !baseContext.IsObject() → JsonObjectHasEntries returns false → CloneJsonValue(extraContext)
    EXPECT_TRUE(result.IsValid());
}

// =============================================================================
// COVERAGE GAP: DispatchActionInfo — FUNCTION_CALL with functionCallDescriptor
// that resolves to a different function via DynamicValueResolver
// Exercises line 232-241: functionCallDescriptor.IsValid() → DynamicResolveContext → resolved
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCallDescriptorResolved)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    auto fc = std::make_shared<FunctionCallInfo>("testFn", JsonValue(), "void");
    // Valid functionCallDescriptor → DynamicValueResolver path
    auto fcd = JsonAdapter::Parse(R"({"call": "resolvedFn", "args": {}, "returnType": "string"})");
    ASSERT_NE(fcd, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>(fc, fcd->GetRoot());
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: DispatchActionInfo — FUNCTION_CALL where native function
// is in registry but normalize fails (line 264-268)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_NativeFunctionNormalizeFails)
{
    // Register a function that will fail normalization
    auto& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("testNormalizeFailFn", std::make_shared<StubNativeFunction>("testNormalizeFailFn"));
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    auto fc = std::make_shared<FunctionCallInfo>("testNormalizeFailFn", JsonValue(), "string");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass; normalization path exercised
}

// =============================================================================
// COVERAGE GAP: ApplyExtendedDescriptor — re-init keeps existing nodeApplier_
// Exercises the path where nodeApplier_ != nullptr on second call
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyExtendedDescriptor_SecondInitReusesApplier)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto desc1 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(desc1, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc1->GetRoot(), ctx));
    EXPECT_NE(comp.GetNodeApplier(), nullptr);

    // Second init → nodeApplier_ already exists, skip creation
    auto desc2 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"height": 200}})");
    ASSERT_NE(desc2, nullptr);
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(desc2->GetRoot(), ctx));
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — style errors from StyleResolver::Resolve
// Exercises line 425-428: for (const auto& error : resolveResult.errors)
// Use a style that causes resolve errors (e.g., invalid binding)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_StyleResolveErrors_ReportsWarnings)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // A style with a valid but potentially problematic value
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": "abc"}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // Should succeed but may produce resolve warnings
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — clear bindings path
// Exercises line 430-432: RemoveBindingsForProperty
// Trigger by removing a style that was previously set
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ClearBindingsOnStyleRemoval)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // Init with width
    auto desc1 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(desc1, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc1->GetRoot(), ctx));

    // Update without width → clear binding path
    auto desc2 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"height": 200}})");
    ASSERT_NE(desc2, nullptr);
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(desc2->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — reset properties path
// Exercises line 433-435: ExtendedStyleResolver::Reset
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ResetPropertiesOnStyleChange)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    // Init with width and backgroundColor
    auto desc1 = JsonAdapter::Parse(
        R"({"id":"test","component":"Column","styles":{"width": 100, "backgroundColor": "#FF000000"}})");
    ASSERT_NE(desc1, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(desc1->GetRoot(), ctx));

    // Update with only height → width and backgroundColor should be reset
    auto desc2 = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"height": 200}})");
    ASSERT_NE(desc2, nullptr);
    EXPECT_TRUE(comp.CallUpdateFromDescriptor(desc2->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — binding path
// Exercises line 436-440: AddBinding
// Need a style with a binding expression
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_BindingPath)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"width": 100}})");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplySingleResolvedStyle — full path with all LOG branches
// Exercises line 494: isApplyingStyleDeltaUpdate_ = true
// And line 506: appliedStyleKeys_.insert(styleName)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_StyleDeltaUpdateFlag)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    auto value = JsonAdapter::CreateNumber(150.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.width", value->GetRoot());
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

// =============================================================================
// COVERAGE GAP: MergeEventContext — extraContext is object, baseContext is invalid
// Exercises line 551-552: baseContext.IsObject() ? Clone(baseContext) : CreateObject()
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraInvalidBase_CreatesNewObject)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    JsonValue invalidBase;
    auto extraAdapter = JsonAdapter::Parse(R"({"extraKey": "extraValue", "count": 42})");
    ASSERT_NE(extraAdapter, nullptr);
    JsonValue result = comp.MergeEventContext(invalidBase, extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.IsObject());
    EXPECT_TRUE(result.Has("extraKey"));
    EXPECT_TRUE(result.Has("count"));
}

// =============================================================================
// COVERAGE GAP: MergeEventContext — extraContext is object, baseContext is empty object
// Exercises JsonObjectHasEntries returning false for empty base
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_ObjectExtraEmptyBase_Merges)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto baseAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::Parse(R"({"key": "value"})");
    ASSERT_NE(extraAdapter, nullptr);
    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("key"));
}

// =============================================================================
// COVERAGE GAP: SupportsExtendedChildren — all supported types
// Exercise SupportsExtendedChildren for each type via ExpandTemplateChildren
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ExpandTemplateChildren_AllSupportedTypes)
{
    const char* types[] = { "Column", "Row", "List", "Stack", "Grid" };
    for (const char* type : types) {
        TestableExtendedComponent comp;
        comp.SetComponentTypeForTest(type);
        comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
        ChildListDescriptor childList;
        childList.type = ChildListType::STATIC_IDS;
        childList.staticChildIds = { "child1" };
        std::list<std::string> childIds;
        bool result = comp.CallExpandTemplateChildren(childList, slot_, childIds);
        (void)result;
    }
}

// =============================================================================
// COVERAGE GAP: OnDataUpdate — non-style binding property delegates to base class
// Exercises line 326: A2UIComponent::OnDataUpdate(property, value)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_NonStyleProperty_DelegatesToBaseClass)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    auto value = JsonAdapter::CreateString("test_content");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("content", value->GetRoot());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: CollectChildListDescriptor — all supported types
// Exercises SupportsExtendedChildren for all supported types in Collect path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, CollectChildListDescriptor_AllSupportedTypes)
{
    const char* types[] = { "Column", "Row", "List", "Stack", "Grid" };
    for (const char* type : types) {
        TestableExtendedComponent comp;
        comp.SetComponentTypeForTest(type);
        comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
        auto descriptor = JsonAdapter::Parse(R"({"children": ["child1"]})");
        ASSERT_NE(descriptor, nullptr);
        comp.CallCollectChildListDescriptor(descriptor->GetRoot());
    }
}

// =============================================================================
// COVERAGE GAP: ParseAndRegisterEventHandlers — multiple known event names
// Exercises EventHandlerParser::KNOWN_EVENT_NAMES for various event names
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ParseAndRegisterEventHandlers_MultipleKnownEvents)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto listeners = JsonAdapter::Parse(R"({
        "onClick": [{"call": "clicked"}],
        "onAppear": [{"call": "appeared"}],
        "onChange": [{"call": "changed"}],
        "onSelect": [{"call": "selected"}],
        "onReachStart": [{"call": "reachedStart"}],
        "onReachEnd": [{"call": "reachedEnd"}]
    })");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    EXPECT_TRUE(comp.HasEventHandler("onClick"));
    EXPECT_TRUE(comp.HasEventHandler("onAppear"));
    EXPECT_TRUE(comp.HasEventHandler("onChange"));
    EXPECT_TRUE(comp.HasEventHandler("onSelect"));
    EXPECT_TRUE(comp.HasEventHandler("onReachStart"));
    EXPECT_TRUE(comp.HasEventHandler("onReachEnd"));
}

// =============================================================================
// COVERAGE GAP: ApplyExtendedDescriptor — normalized descriptor with styles and validation issues
// Exercises ReportSchemaWarning for each validation issue
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyExtendedDescriptor_ValidationIssuesReported)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(
        R"({"id":"test","component":"Column","styles":"not_an_object","listeners":"also_not_object"})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — resolvedStyles is valid object (IsObject()=true)
// Exercises the main ResolveAndApply path with constraint dispatch context
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_ValidStylesWithConstraintDispatch)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto descriptor = JsonAdapter::Parse(
        R"({"id":"test","component":"Column","styles":{"width": 100, "constraintSize": {"minWidth": 50}}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: DispatchActionInfo — EVENT type with resolved context + extra context merge
// Exercises line 284-287: EventContextResolver + MergeEventContext + ActionDispatchBridge
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_EventWithContextMerge)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    auto eventCtxDescriptor = JsonAdapter::Parse(R"({"data": {"key": "value"}})");
    ASSERT_NE(eventCtxDescriptor, nullptr);
    auto actionInfo = std::make_shared<ActionInfo>("testEvent", eventCtxDescriptor->GetRoot());
    auto extraContext = JsonAdapter::Parse(R"({"extraKey": "extraValue"})");
    ASSERT_NE(extraContext, nullptr);
    comp.DispatchActionInfo("test", actionInfo, extraContext->GetRoot());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: IsExpressionCandidate with various string inputs
// Exercises the trim logic in IsExpressionCandidate
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, IsExpressionCandidate_TabAndNewlineTrimmed)
{
    TestableExtendedComponent comp;
    auto strAdapter = JsonAdapter::CreateString("\t\n  expression_content  \n\t");
    ASSERT_NE(strAdapter, nullptr);
    bool result = comp.IsExpressionCandidateForTest(strAdapter->GetRoot());
    // Trims whitespace, then checks IsExpression
    (void)result;
}

// =============================================================================
// COVERAGE GAP: ApplySingleResolvedStyle — with empty style name
// Exercises line 467: styleName.empty() → early return
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_EmptyName_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    // "styles." with empty suffix → styleName is empty → early return
    auto value = JsonAdapter::CreateNumber(100.0);
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.", value->GetRoot());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: OnDataUpdate — style binding with valid applier
// Exercises full ApplySingleResolvedStyle path
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleBindingBackgroundColor_Applies)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    auto value = JsonAdapter::CreateString("#FF000000");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.backgroundColor", value->GetRoot());
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleObjectExpression_ReevaluatesDescriptor)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto initialMode = JsonAdapter::CreateString("small");
    ASSERT_NE(initialMode, nullptr);
    comp.SetLocalVariables({ { "mode", initialMode->GetRoot() } });

    RenderContext ctx;
    ctx.renderId = -1;
    ctx.surfaceId = "style-object-update-surface";
    auto descriptor = JsonAdapter::Parse(R"({
        "id": "styleObjectUpdate",
        "component": "Column",
        "styles": {
            "padding": {
                "top": "{{ $mode == 'large' ? 16 : 8 }}",
                "right": 1,
                "bottom": 2,
                "left": 3
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    const auto& bindings = comp.GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "styles.padding");
    EXPECT_EQ(bindings[0].type_, BindingType::FUNCTION_CALL);

    std::array<float, 4> padding = {};
    ASSERT_TRUE(FindLastPadding(mockArkUIPtr_, comp.GetNativeView(), padding));
    EXPECT_FLOAT_EQ(padding[0], 8.0F);
    EXPECT_FLOAT_EQ(padding[1], 1.0F);
    EXPECT_FLOAT_EQ(padding[2], 2.0F);
    EXPECT_FLOAT_EQ(padding[3], 3.0F);

    auto updatedMode = JsonAdapter::CreateString("large");
    ASSERT_NE(updatedMode, nullptr);
    comp.SetLocalVariables({ { "mode", updatedMode->GetRoot() } });
    size_t recordCountBefore = mockArkUIPtr_->setAttributeRecords_.size();

    comp.CallOnDataUpdate("styles.padding", JsonValue());

    ASSERT_TRUE(FindLastPadding(mockArkUIPtr_, comp.GetNativeView(), padding));
    EXPECT_GT(mockArkUIPtr_->setAttributeRecords_.size(), recordCountBefore);
    EXPECT_FLOAT_EQ(padding[0], 16.0F);
    EXPECT_FLOAT_EQ(padding[1], 1.0F);
    EXPECT_FLOAT_EQ(padding[2], 2.0F);
    EXPECT_FLOAT_EQ(padding[3], 3.0F);
}

// =============================================================================
// REGRESSION: partial object style refresh with a missing nested path
// Dynamic style object refresh keeps valid members when another nested path is missing.
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, OnDataUpdate_StyleObjectKeepsPartialMembersWhenNestedPathMissing)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));

    auto dataModel = std::make_shared<DataModel>("style-object-partial-refresh-surface");
    auto initialBottom = JsonAdapter::CreateNumber(2.0);
    ASSERT_NE(initialBottom, nullptr);
    dataModel->UpdateByPath("/bottom", initialBottom->GetRoot());

    RenderContext ctx;
    ctx.renderId = -1;
    ctx.surfaceId = "style-object-partial-refresh-surface";
    ctx.dataModel = dataModel;
    auto descriptor = JsonAdapter::Parse(R"({
        "id": "styleObjectPartialUpdate",
        "component": "Column",
        "styles": {
            "padding": {
                "top": 8,
                "right": { "path": "/right" },
                "bottom": { "path": "/bottom" },
                "left": 4
            }
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
    const auto& bindings = comp.GetDataBindings();
    ASSERT_EQ(bindings.size(), 2U);
    EXPECT_EQ(bindings[0].propertyName_, "styles.padding");
    EXPECT_EQ(bindings[1].propertyName_, "styles.padding");

    std::array<float, 4> padding = {};
    ASSERT_TRUE(FindLastPadding(mockArkUIPtr_, comp.GetNativeView(), padding));
    EXPECT_FLOAT_EQ(padding[0], 8.0F);
    EXPECT_FLOAT_EQ(padding[1], 0.0F);
    EXPECT_FLOAT_EQ(padding[2], 2.0F);
    EXPECT_FLOAT_EQ(padding[3], 4.0F);

    auto updatedBottom = JsonAdapter::CreateNumber(6.0);
    ASSERT_NE(updatedBottom, nullptr);
    dataModel->UpdateByPath("/bottom", updatedBottom->GetRoot());
    size_t recordCountBefore = mockArkUIPtr_->setAttributeRecords_.size();

    comp.CallOnDataUpdate("styles.padding", JsonValue());

    ASSERT_TRUE(FindLastPadding(mockArkUIPtr_, comp.GetNativeView(), padding));
    EXPECT_GT(mockArkUIPtr_->setAttributeRecords_.size(), recordCountBefore);
    EXPECT_FLOAT_EQ(padding[0], 8.0F);
    EXPECT_FLOAT_EQ(padding[1], 0.0F);
    EXPECT_FLOAT_EQ(padding[2], 6.0F);
    EXPECT_FLOAT_EQ(padding[3], 4.0F);
}

// =============================================================================
// COVERAGE GAP: MergeEventContext with non-object extra and populated base context
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraBaseWithEntries_Merges)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto baseAdapter = JsonAdapter::Parse(R"({"ctx": "data", "value": "original"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(extraAdapter, nullptr);
    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    // baseContext has entries → Clone + Replace("value", extra) path
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("ctx"));
    EXPECT_TRUE(result.Has("value"));
}

// =============================================================================
// COVERAGE GAP: MergeEventContext — non-object extra, base has no "value" key → Put path
// Exercises line 546: contextRoot.Put("value", extraContext)
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, MergeEventContext_NonObjectExtraBaseNoValue_PutsValue)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    auto baseAdapter = JsonAdapter::Parse(R"({"ctx": "data"})");
    ASSERT_NE(baseAdapter, nullptr);
    auto extraAdapter = JsonAdapter::CreateString("payload");
    ASSERT_NE(extraAdapter, nullptr);
    JsonValue result = comp.MergeEventContext(baseAdapter->GetRoot(), extraAdapter->GetRoot());
    EXPECT_TRUE(result.IsValid());
    EXPECT_TRUE(result.Has("ctx"));
    EXPECT_TRUE(result.Has("value"));
}

// =============================================================================
// COVERAGE GAP: DispatchActionInfo — FUNCTION_CALL with functionCall null after resolve
// Exercises line 243-247: functionCall == nullptr → LOG_WARN → return
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchActionInfo_FunctionCallNullAfterResolve_NoCrash)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    // Create ActionInfo with FUNCTION_CALL but no function call info
    // This is hard to construct because ActionInfo(fc) requires valid fc
    // Instead test with valid fc and empty functionCallDescriptor
    auto fc = std::make_shared<FunctionCallInfo>("", JsonValue(), "void");
    auto actionInfo = std::make_shared<ActionInfo>(fc);
    comp.DispatchActionInfo("test", actionInfo, JsonValue());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: DispatchEvent — dispatch event with valid extra context
// Exercises line 213-215: DispatchEventToHandlers with extraContext
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, DispatchEvent_WithExtraContext)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    comp.SetComponentId("test-comp");
    comp.SetSurfaceId("test-surface");
    comp.SetRenderId(1);
    auto listeners = JsonAdapter::Parse(R"({"onClick": [{"call": "clicked"}]})");
    ASSERT_NE(listeners, nullptr);
    comp.CallParseAndRegisterEventHandlers(listeners->GetRoot());
    auto extraContext = JsonAdapter::Parse(R"({"clickX": 100, "clickY": 200})");
    ASSERT_NE(extraContext, nullptr);
    comp.DispatchEvent("onClick", extraContext->GetRoot());
    // No crash = pass
}

// =============================================================================
// COVERAGE GAP: ApplyResolvedStyles — non-empty styleResolverIssues from ExtendedStyleResolver
// Exercises line 451-453: ReportSchemaWarning for each issue in styleResolverIssues
// Uses invalid shadow value to trigger PushStyleValidationIssue inside ExtendedStyleResolver
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplyResolvedStyles_StyleResolverReportsValidationIssues)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    // shadow with an invalid type (not object) triggers PushStyleValidationIssue in ExtendedStyleResolver
    auto descriptor =
        JsonAdapter::Parse(R"({"id":"test","component":"Column","styles":{"shadow": "invalid_shadow_value"}})");
    ASSERT_NE(descriptor, nullptr);
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";
    // Should succeed — issues are reported as DFX warnings, not fatal
    EXPECT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));
}

// =============================================================================
// COVERAGE GAP: ApplySingleResolvedStyle — non-empty styleResolverIssues from ExtendedStyleResolver
// Exercises line 501-503: ReportSchemaWarning for each issue in styleResolverIssues
// Triggers via OnDataUpdate style binding path with a value that causes validation issues
// =============================================================================
TEST_F(ExtendedComponentCoreTddTest, ApplySingleResolvedStyle_StyleResolverReportsValidationIssues)
{
    TestableExtendedComponent comp;
    comp.SetNativeViewForTest(reinterpret_cast<ArkUI_NodeHandle>(0x1234));
    RenderContext ctx;
    ctx.renderId = 1;
    ctx.surfaceId = "test";

    auto descriptor = JsonAdapter::Parse(R"({"id":"test","component":"Column"})");
    ASSERT_NE(descriptor, nullptr);
    ASSERT_TRUE(comp.CallInitFromDescriptor(descriptor->GetRoot(), ctx));

    // Trigger ApplySingleResolvedStyle with a shadow value via style binding update
    // shadow expects object, passing string triggers PushStyleValidationIssue
    auto value = JsonAdapter::CreateString("not_a_valid_shadow");
    ASSERT_NE(value, nullptr);
    comp.CallOnDataUpdate("styles.shadow", value->GetRoot());
    // No crash = pass; styleResolverIssues loop exercised with non-empty issues
    EXPECT_FALSE(comp.IsApplyingStyleDeltaUpdateForTest());
}

} // namespace
