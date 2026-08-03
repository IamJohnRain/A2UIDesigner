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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>
#include <limits>
#include <memory>
#include <vector>

#define private public
#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/A2UIComponent.h"
#include "components/custom/CustomComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedComponent.h"
#include "components/extended/ExtendedComponentFactory.h"
#include "components/extended/ExtendedDividerComponent.h"
#include "components/extended/ExtendedGridComponent.h"
#include "components/extended/ExtendedListComponent.h"
#include "components/extended/ExtendedProgressComponent.h"
#include "components/extended/ExtendedRowComponent.h"
#include "components/extended/ExtendedStackComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "components/extended/NavContainerComponent.h"
#include "theme/ThemeBase.h"
#include "utils/JsonAdapter.h"

#include "ArkUINodeApiAdapter.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"
#undef private

using namespace NativeModule;

namespace {

std::shared_ptr<Catalog> BuildCatalogWithLegacyFlag(const std::string& componentName, bool isInnerNative)
{
    auto catalog = std::make_shared<Catalog>("catalog://legacy");
    auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
    item->SetCategory(CatalogCategory::OHOS_EXTENDS);
    item->SetInnerNative(isInnerNative);
    catalog->AddComponent(item);
    return catalog;
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

std::shared_ptr<Catalog> BuildExtendedProtocolCatalog(std::initializer_list<const char*> componentNames = {})
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    for (const char* componentName : componentNames) {
        if (componentName == nullptr || componentName[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(componentName, CatalogItemType::COMPONENT);
        item->SetCategory(CatalogCategory::OHOS_EXTENDS);
        catalog->AddComponent(item);
    }
    return catalog;
}

struct CapturedAttribute {
    bool captured = false;
    ArkUI_NodeHandle node = nullptr;
    int32_t attribute = 0;
    std::vector<ArkUI_NumberValue> values;
};

using SetAttributeCallback = int32_t (*)(
    ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item);
using CreateNodeCallback = ArkUI_NodeHandle (*)(ArkUI_NodeType type);
using RegisterNodeEventCallback = int32_t (*)(
    ArkUI_NodeHandle node, int32_t eventType, int32_t eventId, void* userData);
using ResetAttributeCallback = int32_t (*)(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute);

struct ColumnJustifyCase {
    const char* value = nullptr;
    ArkUI_FlexAlignment expected = ARKUI_FLEX_ALIGNMENT_START;
};

struct ColumnAlignCase {
    const char* value = nullptr;
    ArkUI_HorizontalAlignment expected = ARKUI_HORIZONTAL_ALIGNMENT_START;
};

class TestableExtendedColumnComponent : public ExtendedColumnComponent {
public:
    using ExtendedColumnComponent::GetPrivatePropertyDeclaration;

    bool InvokeIsKnownAdditionalDescriptorKey(const std::string& propertyName) const
    {
        return IsKnownAdditionalDescriptorKey(propertyName);
    }

    void CallApplyComponentSpecificStyles(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void CallOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void CallOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void CallOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    ArkUI_NativeNodeAPI_1* GetNativeNodeApiForTest() const
    {
        return nativeNodeApi_;
    }

    void SetNativeNodeApiForTest(ArkUI_NativeNodeAPI_1* nativeNodeApi)
    {
        nativeNodeApi_ = nativeNodeApi;
    }

    ArkUI_NodeHandle GetNativeViewForTest() const
    {
        return nativeView_;
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class TestableExtendedRowComponent : public ExtendedRowComponent {
public:
    using ExtendedRowComponent::GetPrivatePropertyDeclaration;

    bool InvokeIsKnownAdditionalDescriptorKey(const std::string& propertyName) const
    {
        return IsKnownAdditionalDescriptorKey(propertyName);
    }

    void CallApplyComponentSpecificStyles(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void CallOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void CallOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void CallOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    ArkUI_NativeNodeAPI_1* GetNativeNodeApiForTest() const
    {
        return nativeNodeApi_;
    }

    void SetNativeNodeApiForTest(ArkUI_NativeNodeAPI_1* nativeNodeApi)
    {
        nativeNodeApi_ = nativeNodeApi;
    }

    ArkUI_NodeHandle GetNativeViewForTest() const
    {
        return nativeView_;
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class TestableExtendedGridComponent : public ExtendedGridComponent {
public:
    using ExtendedGridComponent::GetPrivatePropertyDeclaration;

    void CallApplyPrivateAttributes(const JsonValue& descriptor)
    {
        ApplyPrivateAttributes(descriptor);
    }

    void CallApplyComponentSpecificStyles(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void CallOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void CallOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void CallOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    bool CallExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildren(childList, surfaceSlot, childIds);
    }

    void SetApplyingStyleDeltaUpdateForTest(bool isApplyingStyleDeltaUpdate)
    {
        isApplyingStyleDeltaUpdate_ = isApplyingStyleDeltaUpdate;
    }

    void SetNativeNodeApiForTest(ArkUI_NativeNodeAPI_1* nativeNodeApi)
    {
        nativeNodeApi_ = nativeNodeApi;
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

class TestableExtendedListComponent : public ExtendedListComponent {
public:
    using ExtendedListComponent::GetPrivatePropertyDeclaration;

    void CallApplyPrivateAttributes(const JsonValue& descriptor)
    {
        ApplyPrivateAttributes(descriptor);
    }

    void CallApplyComponentSpecificStyles(const JsonValue& styles)
    {
        ArkUINodeApiAdapter applier = CreateNodeApiAdapter(*this);
        ApplyComponentSpecificStyles(styles, applier);
    }

    void CallOnAddChild(const std::shared_ptr<Component>& child, size_t index)
    {
        OnAddChild(child, index);
    }

    void CallOnMoveChild(const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
    {
        OnMoveChild(child, currentIndex, targetIndex);
    }

    void CallOnRemoveChild(const std::shared_ptr<Component>& child)
    {
        OnRemoveChild(child);
    }

    bool CallExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
    {
        return ExpandTemplateChildren(childList, surfaceSlot, childIds);
    }

    void CallRegisterExtendedListeners()
    {
        RegisterExtendedListeners();
    }

    void SetApplyingStyleDeltaUpdateForTest(bool isApplyingStyleDeltaUpdate)
    {
        isApplyingStyleDeltaUpdate_ = isApplyingStyleDeltaUpdate;
    }

    void SetNativeNodeApiForTest(ArkUI_NativeNodeAPI_1* nativeNodeApi)
    {
        nativeNodeApi_ = nativeNodeApi;
    }

    void SetNativeViewForTest(ArkUI_NodeHandle nativeView)
    {
        nativeView_ = nativeView;
    }
};

CapturedAttribute g_columnJustifyAttribute;
CapturedAttribute g_columnAlignAttribute;
std::vector<CapturedAttribute> g_resetAttributes;
std::vector<int32_t> g_registeredNodeEvents;

ArkUI_NodeHandle ReturnNullNodeHandle(ArkUI_NodeType type)
{
    static_cast<void>(type);
    return nullptr;
}

int32_t ReturnSetAttributeFailure(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item)
{
    static_cast<void>(node);
    static_cast<void>(attribute);
    static_cast<void>(item);
    return -1;
}

void CaptureAttribute(CapturedAttribute& record, ArkUI_NodeHandle node, int32_t attribute, ArkUI_AttributeItem* item)
{
    record.captured = true;
    record.node = node;
    record.attribute = attribute;
    record.values.clear();
    if (item != nullptr && item->value != nullptr && item->size > 0) {
        record.values.assign(item->value, item->value + item->size);
    }
}

void ResetColumnJustifyCapture()
{
    g_columnJustifyAttribute = CapturedAttribute();
}

void ResetColumnAlignCapture()
{
    g_columnAlignAttribute = CapturedAttribute();
}

void ResetAttributeCapture()
{
    g_resetAttributes.clear();
}

std::unique_ptr<JsonAdapter> BuildColumnJustifyMessage(const std::string& justifyContent)
{
    return JsonAdapter::Parse(R"({"components":[{"id":"root","component":"Column","styles":{"justifyContent":")" +
                              justifyContent + R"("}}]})");
}

std::unique_ptr<JsonAdapter> BuildColumnAlignMessage(const char* alignItems)
{
    if (alignItems == nullptr) {
        return JsonAdapter::Parse(R"({"components":[{"id":"root","component":"Column"}]})");
    }
    return JsonAdapter::Parse(R"({"components":[{"id":"root","component":"Column","styles":{"alignItems":")" +
                              std::string(alignItems) + R"("}}]})");
}

ChildListDescriptor BuildTemplateChildListDescriptor(
    const std::string& templateComponentId, const std::string& templatePath)
{
    ChildListDescriptor descriptor;
    descriptor.type = ChildListType::TEMPLATE_PATH;
    descriptor.templateComponentId = templateComponentId;
    descriptor.templatePath = templatePath;
    return descriptor;
}

int32_t CaptureColumnSetAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item)
{
    if (attribute == NODE_COLUMN_JUSTIFY_CONTENT) {
        CaptureAttribute(g_columnJustifyAttribute, node, attribute, item);
    } else if (attribute == NODE_COLUMN_ALIGN_ITEMS) {
        CaptureAttribute(g_columnAlignAttribute, node, attribute, item);
    }
    return 0;
}

int32_t CaptureExtendedResetAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute)
{
    CapturedAttribute record;
    CaptureAttribute(record, node, attribute, nullptr);
    g_resetAttributes.push_back(record);
    return 0;
}

int32_t CaptureRegisterNodeEvent(ArkUI_NodeHandle node, int32_t eventType, int32_t eventId, void* userData)
{
    static_cast<void>(node);
    static_cast<void>(eventId);
    static_cast<void>(userData);
    g_registeredNodeEvents.push_back(eventType);
    return 0;
}

bool HasRegisteredNodeEvent(int32_t eventType)
{
    for (int32_t registeredEvent : g_registeredNodeEvents) {
        if (registeredEvent == eventType) {
            return true;
        }
    }
    return false;
}

bool HasResetAttribute(ArkUI_NodeAttributeType attribute)
{
    return std::any_of(g_resetAttributes.begin(), g_resetAttributes.end(),
        [attribute](const CapturedAttribute& record) { return record.attribute == attribute; });
}

const MockArkUINativeProvider::SetAttributeRecord* FindLastAttributeForNode(
    const MockArkUINativeProvider& provider, ArkUI_NodeHandle node, int32_t attribute)
{
    for (auto it = provider.setAttributeRecords_.rbegin(); it != provider.setAttributeRecords_.rend(); ++it) {
        if (it->nodeHandle == node && it->attribute == attribute) {
            return &(*it);
        }
    }
    return nullptr;
}

void ExpectMarginValues(
    const MockArkUINativeProvider::SetAttributeRecord* record, float top, float right, float bottom, float left)
{
    ASSERT_NE(record, nullptr);
    ASSERT_EQ(record->attribute, NODE_MARGIN);
    ASSERT_EQ(record->values.size(), 4U);
    EXPECT_FLOAT_EQ(record->values[0].f32, top);
    EXPECT_FLOAT_EQ(record->values[1].f32, right);
    EXPECT_FLOAT_EQ(record->values[2].f32, bottom);
    EXPECT_FLOAT_EQ(record->values[3].f32, left);
}

void ExpectCommonMarginValues(const Component& component, float top, float right, float bottom, float left)
{
    const CommonMargin& margin = component.GetCommonMargin();
    EXPECT_FLOAT_EQ(margin.top, top);
    EXPECT_FLOAT_EQ(margin.right, right);
    EXPECT_FLOAT_EQ(margin.bottom, bottom);
    EXPECT_FLOAT_EQ(margin.left, left);
}

std::shared_ptr<A2UIComponent> CreateMarginChild(uintptr_t nativeId, float top, float right, float bottom, float left)
{
    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(nativeId), false);
    child->SetMargin(top, right, bottom, left);
    return child;
}

class ScopedSetAttributeCapture {
public:
    explicit ScopedSetAttributeCapture(ArkUI_NativeNodeAPI_1* nativeNodeApi)
        : nativeNodeApi_(nativeNodeApi),
          originalSetAttribute_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->setAttribute)
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->setAttribute = CaptureColumnSetAttribute;
        }
    }

    ~ScopedSetAttributeCapture()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->setAttribute = originalSetAttribute_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    SetAttributeCallback originalSetAttribute_ = nullptr;
};

class ScopedSetAttributeOverride {
public:
    ScopedSetAttributeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, SetAttributeCallback setAttributeCallback)
        : nativeNodeApi_(nativeNodeApi),
          originalSetAttribute_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->setAttribute)
    {
        if (nativeNodeApi_ != nullptr && setAttributeCallback != nullptr) {
            nativeNodeApi_->setAttribute = setAttributeCallback;
        }
    }

    ~ScopedSetAttributeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->setAttribute = originalSetAttribute_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    SetAttributeCallback originalSetAttribute_ = nullptr;
};

class ScopedCreateNodeOverride {
public:
    ScopedCreateNodeOverride(ArkUI_NativeNodeAPI_1* nativeNodeApi, CreateNodeCallback createNodeCallback)
        : nativeNodeApi_(nativeNodeApi),
          originalCreateNode_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->createNode)
    {
        if (nativeNodeApi_ != nullptr && createNodeCallback != nullptr) {
            nativeNodeApi_->createNode = createNodeCallback;
        }
    }

    ~ScopedCreateNodeOverride()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->createNode = originalCreateNode_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    CreateNodeCallback originalCreateNode_ = nullptr;
};

class ScopedResetAttributeCapture {
public:
    explicit ScopedResetAttributeCapture(ArkUI_NativeNodeAPI_1* nativeNodeApi)
        : nativeNodeApi_(nativeNodeApi),
          originalResetAttribute_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->resetAttribute)
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->resetAttribute = CaptureExtendedResetAttribute;
        }
    }

    ~ScopedResetAttributeCapture()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->resetAttribute = originalResetAttribute_;
        }
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    ResetAttributeCallback originalResetAttribute_ = nullptr;
};

class ScopedNodeEventCapture {
public:
    explicit ScopedNodeEventCapture(ArkUI_NativeNodeAPI_1* nativeNodeApi)
        : nativeNodeApi_(nativeNodeApi),
          originalRegisterNodeEvent_(nativeNodeApi == nullptr ? nullptr : nativeNodeApi->registerNodeEvent)
    {
        g_registeredNodeEvents.clear();
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->registerNodeEvent = CaptureRegisterNodeEvent;
        }
    }

    ~ScopedNodeEventCapture()
    {
        if (nativeNodeApi_ != nullptr) {
            nativeNodeApi_->registerNodeEvent = originalRegisterNodeEvent_;
        }
        g_registeredNodeEvents.clear();
    }

private:
    ArkUI_NativeNodeAPI_1* nativeNodeApi_ = nullptr;
    RegisterNodeEventCallback originalRegisterNodeEvent_ = nullptr;
};

class ExtendedComponentTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        slot_.SetSurfaceId("surface-extended-generic");
        slot_.SetRenderId(1);
    }

    SurfaceSlot slot_;

    const MockArkUINativeProvider::SetAttributeRecord* FindLastSetAttribute(int32_t attribute) const
    {
        for (auto iter = mockArkUIPtr_->setAttributeRecords_.rbegin();
             iter != mockArkUIPtr_->setAttributeRecords_.rend(); ++iter) {
            if (iter->attribute == attribute) {
                return &(*iter);
            }
        }
        return nullptr;
    }
};

TEST_F(ExtendedComponentTest, ExtendedComponentTest001)
{
    slot_.SetCatalog(BuildCatalogWithLegacyFlag("Text", true));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "text": "hello"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Text");
    EXPECT_EQ(std::dynamic_pointer_cast<ExtendedComponent>(root), nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest002)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": []
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Column");
    EXPECT_NE(std::dynamic_pointer_cast<ExtendedComponent>(root), nullptr);
}

/**
 * @tc.name: ExtendedComponentTest003
 * @tc.desc: Verify ExtendedTextComponent parses text line-break and font-size related styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest003)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "fontColor": "#FF112233",
                    "fontSize": 18,
                    "fontWeight": 700,
                    "maxLines": 2,
                    "minFontSize": 12,
                    "maxFontSize": 24,
                    "wordBreak": "breakWord",
                    "textOverflow": "ellipsis",
                    "decoration": {
                        "type": "underline",
                        "color": "#ff007dff",
                        "style": "solid",
                        "thicknessScale": 1.5
                    },
                    "textAlign": "center"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::shared_ptr<ExtendedTextComponent> text = std::dynamic_pointer_cast<ExtendedTextComponent>(root);
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetFontColorForTest(), 0xFF112233u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 18.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W700);
    EXPECT_EQ(text->GetMaxLinesForTest(), 2);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), 12.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), 24.0F);
    EXPECT_EQ(text->GetTextOverflowForTest(), 2);
    EXPECT_EQ(text->GetTextAlignForTest(), 1);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 1);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_TRUE(decoration.hasColor);
    EXPECT_EQ(decoration.color, 0xFF007DFFu);
    EXPECT_TRUE(decoration.hasStyle);
    EXPECT_TRUE(decoration.hasThicknessScale);
    EXPECT_FLOAT_EQ(decoration.thicknessScale, 1.5F);
}

/**
 * @tc.name: ExtendedComponentTest004
 * @tc.desc: Verify ExtendedTextComponent supports left/right aliases for textAlign and textOverflow enums.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest004)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "maxLines": 1,
                    "textOverflow": "marquee",
                    "textAlign": "right"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::shared_ptr<ExtendedTextComponent> text = std::dynamic_pointer_cast<ExtendedTextComponent>(root);
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetMaxLinesForTest(), 1);
    EXPECT_EQ(text->GetTextOverflowForTest(), 3);
    EXPECT_EQ(text->GetTextAlignForTest(), 5);
}

/**
 * @tc.name: ExtendedComponentTest005
 * @tc.desc: Verify ExtendedTextComponent falls back to default Text content and styles when values are not provided.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest005)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "fontColor": "#FF112233",
                "fontSize": 18,
                "fontWeight": "bold",
                "maxLines": 2,
                "minFontSize": 12,
                "maxFontSize": 24,
                "wordBreak": "breakWord",
                "textOverflow": "ellipsis",
                "decoration": {
                    "type": "underline",
                    "color": "#ff007dff",
                    "style": "solid",
                    "thicknessScale": 1.5
                },
                "textAlign": "center"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::shared_ptr<ExtendedTextComponent> text = std::dynamic_pointer_cast<ExtendedTextComponent>(root);
    ASSERT_NE(text, nullptr);

    EXPECT_EQ(text->GetTextValueForTest(), "");
    EXPECT_EQ(text->GetFontColorForTest(), 0xE5000000u);
    EXPECT_FLOAT_EQ(text->GetFontSizeForTest(), 16.0F);
    EXPECT_EQ(text->GetFontWeightForTest(), ARKUI_FONT_WEIGHT_W400);
    EXPECT_EQ(text->GetMaxLinesForTest(), -1);
    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetTextOverflowForTest(), 1);
    EXPECT_EQ(text->GetTextAlignForTest(), 0);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
    TextDecorationState decoration = text->GetDecorationForTest();
    EXPECT_EQ(decoration.type, 0);
    EXPECT_EQ(decoration.style, 0);
    EXPECT_FALSE(decoration.hasColor);
    EXPECT_FALSE(decoration.hasStyle);
    EXPECT_FALSE(decoration.hasThicknessScale);
}

/**
 * @tc.name: ExtendedComponentTest006
 * @tc.desc: Verify ExtendedComponent common properties flexShrink/backgroundImage/clip are accepted in styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest006)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "flexShrink": 0.5,
                    "backgroundImage": "https://example.com/image.png",
                    "clip": true
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::shared_ptr<ExtendedComponent> extended = std::dynamic_pointer_cast<ExtendedComponent>(root);
    ASSERT_NE(extended, nullptr);

    std::unique_ptr<JsonAdapter> update = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello2",
                "styles": {
                    "flexShrink": -0.3,
                    "backgroundImage": "",
                    "clip": false
                }
            }
        ]
    })");
    ASSERT_NE(update, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(update->GetRoot()));

    root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    extended = std::dynamic_pointer_cast<ExtendedComponent>(root);
    ASSERT_NE(extended, nullptr);
}

/**
 * @tc.name: ExtendedComponentTest007
 * @tc.desc: Verify ExtendedDividerComponent uses the default 1px thickness and #33000000 color when styles are absent.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest007)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Divider" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Divider");

    std::shared_ptr<ExtendedDividerComponent> divider = std::dynamic_pointer_cast<ExtendedDividerComponent>(root);
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0x33000000u);
}

/**
 * @tc.name: ExtendedComponentTest008
 * @tc.desc: Verify ExtendedDividerComponent parses strokeWidth/color/vertical from styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest008)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Divider" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "100fp",
                    "vertical": true,
                    "color": "#112233"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 100.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "fp");
    EXPECT_TRUE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0xFF112233u);
}

/**
 * @tc.name: ExtendedComponentTest009
 * @tc.desc: Verify ExtendedDividerComponent supports percent strokeWidth and falls back to the default 1px/#33000000
 * when values are invalid.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest009)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Divider" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "25%",
                    "vertical": false,
                    "color": "#AA112233"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedDividerComponent> divider =
        std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 25.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "%");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0xAA112233u);

    std::unique_ptr<JsonAdapter> invalidUpdate = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Divider",
                "styles": {
                    "strokeWidth": "-2vp",
                    "color": "invalidColor"
                }
            }
        ]
    })");
    ASSERT_NE(invalidUpdate, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(invalidUpdate->GetRoot()));

    divider = std::dynamic_pointer_cast<ExtendedDividerComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(divider, nullptr);
    EXPECT_FLOAT_EQ(divider->GetStrokeWidthValueForTest(), 1.0F);
    EXPECT_EQ(divider->GetStrokeWidthUnitForTest(), "px");
    EXPECT_FALSE(divider->GetVerticalForTest());
    EXPECT_EQ(divider->GetColorForTest(), 0x33000000u);
}

/**
 * @tc.name: ExtendedComponentTest010
 * @tc.desc: Verify ExtendedColumnComponent applies justifyContent to native Column justify attribute.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest010)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedSetAttributeCapture capture(nativeNodeApi);

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    const std::vector<ColumnJustifyCase> cases = { { "start", ARKUI_FLEX_ALIGNMENT_START },
        { "center", ARKUI_FLEX_ALIGNMENT_CENTER }, { "end", ARKUI_FLEX_ALIGNMENT_END },
        { "spaceAround", ARKUI_FLEX_ALIGNMENT_SPACE_AROUND }, { "spaceBetween", ARKUI_FLEX_ALIGNMENT_SPACE_BETWEEN },
        { "spaceEvenly", ARKUI_FLEX_ALIGNMENT_SPACE_EVENLY }, { "invalid", ARKUI_FLEX_ALIGNMENT_START } };

    for (const auto& testCase : cases) {
        ResetColumnJustifyCapture();
        std::unique_ptr<JsonAdapter> message = BuildColumnJustifyMessage(testCase.value);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

        std::shared_ptr<Component> root = slot_.FindComponentById("root");
        ASSERT_NE(root, nullptr);
        ASSERT_TRUE(g_columnJustifyAttribute.captured);
        ASSERT_FALSE(g_columnJustifyAttribute.values.empty());
        EXPECT_EQ(g_columnJustifyAttribute.node, root->GetNativeView());
        EXPECT_EQ(g_columnJustifyAttribute.attribute, NODE_COLUMN_JUSTIFY_CONTENT);
        EXPECT_EQ(g_columnJustifyAttribute.values[0].i32, testCase.expected);
    }
}

/**
 * @tc.name: ExtendedComponentTest011
 * @tc.desc: Verify ExtendedColumnComponent applies alignItems enum values and start fallback.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest011)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedSetAttributeCapture capture(nativeNodeApi);

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    const std::vector<ColumnAlignCase> cases = { { nullptr, ARKUI_HORIZONTAL_ALIGNMENT_START },
        { "start", ARKUI_HORIZONTAL_ALIGNMENT_START }, { "center", ARKUI_HORIZONTAL_ALIGNMENT_CENTER },
        { "end", ARKUI_HORIZONTAL_ALIGNMENT_END }, { "invalid", ARKUI_HORIZONTAL_ALIGNMENT_START } };

    for (const auto& testCase : cases) {
        ResetColumnAlignCapture();
        std::unique_ptr<JsonAdapter> message = BuildColumnAlignMessage(testCase.value);
        ASSERT_NE(message, nullptr);
        ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

        std::shared_ptr<Component> root = slot_.FindComponentById("root");
        ASSERT_NE(root, nullptr);
        ASSERT_TRUE(g_columnAlignAttribute.captured);
        ASSERT_FALSE(g_columnAlignAttribute.values.empty());
        EXPECT_EQ(g_columnAlignAttribute.node, root->GetNativeView());
        EXPECT_EQ(g_columnAlignAttribute.attribute, NODE_COLUMN_ALIGN_ITEMS);
        EXPECT_EQ(g_columnAlignAttribute.values[0].i32, testCase.expected);
    }

    ResetColumnAlignCapture();
    std::unique_ptr<JsonAdapter> rootPropertyMessage = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "alignItems": "center"
            }
        ]
    })");
    ASSERT_NE(rootPropertyMessage, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(rootPropertyMessage->GetRoot()));
    ASSERT_TRUE(g_columnAlignAttribute.captured);
    ASSERT_FALSE(g_columnAlignAttribute.values.empty());
    EXPECT_EQ(g_columnAlignAttribute.attribute, NODE_COLUMN_ALIGN_ITEMS);
    EXPECT_EQ(g_columnAlignAttribute.values[0].i32, ARKUI_HORIZONTAL_ALIGNMENT_START);

    TestableExtendedColumnComponent component;
    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("alignItems");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));
}

TEST_F(ExtendedComponentTest, L0_should_treat_styles_and_events_as_known_extended_descriptor_keys)
{
    TestableExtendedColumnComponent columnComponent;
    TestableExtendedRowComponent rowComponent;

    EXPECT_TRUE(columnComponent.InvokeIsKnownAdditionalDescriptorKey("styles"));
    EXPECT_TRUE(columnComponent.InvokeIsKnownAdditionalDescriptorKey("onClick"));
    EXPECT_TRUE(columnComponent.InvokeIsKnownAdditionalDescriptorKey("onAppear"));
    EXPECT_FALSE(columnComponent.InvokeIsKnownAdditionalDescriptorKey("listeners"));
    EXPECT_FALSE(columnComponent.InvokeIsKnownAdditionalDescriptorKey("unknown"));

    EXPECT_TRUE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("styles"));
    EXPECT_TRUE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("onChange"));
    EXPECT_TRUE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("onSelect"));
    EXPECT_TRUE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("onReachStart"));
    EXPECT_FALSE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("listeners"));
    EXPECT_FALSE(rowComponent.InvokeIsKnownAdditionalDescriptorKey("unknown"));
}

TEST_F(ExtendedComponentTest, L0_should_attach_static_children_for_extended_column)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column", "Divider" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": ["dividerLine"],
                "styles": {
                    "width": 220,
                    "height": 140
                }
            },
            {
                "id": "dividerLine",
                "component": "Divider",
                "styles": {
                    "strokeWidth": 4,
                    "color": "#2563EB"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    std::shared_ptr<Component> divider = slot_.FindComponentById("dividerLine");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 1U);
    EXPECT_TRUE(root->HasChild(divider));
    EXPECT_EQ(divider->GetParent(), root);
}

TEST_F(ExtendedComponentTest, L0_should_attach_static_children_for_extended_row)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Row", "Divider" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Row",
                "children": ["dividerLine"],
                "styles": {
                    "width": 220,
                    "height": 140
                }
            },
            {
                "id": "dividerLine",
                "component": "Divider",
                "styles": {
                    "strokeWidth": 4,
                    "color": "#2563EB"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    std::shared_ptr<Component> divider = slot_.FindComponentById("dividerLine");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(divider, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 1U);
    EXPECT_TRUE(root->HasChild(divider));
    EXPECT_EQ(divider->GetParent(), root);
}

/**
 * @tc.name: ExtendedComponentTest012
 * @tc.desc: Verify ExtendedTextComponent ignores invalid min/max font and wordBreak values in styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest012)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "minFontSize": 0,
                    "maxFontSize": -3,
                    "wordBreak": true
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), -1.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), -1.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_BREAK_WORD);
}

/**
 * @tc.name: ExtendedComponentTest013
 * @tc.desc: Verify extended protocol Progress routes to ExtendedProgressComponent and parses value/total/color/type.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest013)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Progress" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": 50,
                "total": 100,
                "styles": {
                    "color": "#007AFF",
                    "type": "ring"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "Progress");

    std::shared_ptr<ExtendedProgressComponent> progress = std::dynamic_pointer_cast<ExtendedProgressComponent>(root);
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 50.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 100.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF007AFFu);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 1);
}

/**
 * @tc.name: ExtendedComponentTest014
 * @tc.desc: Verify Progress color/type only take effect from styles, not the descriptor root.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest014)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Progress" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": 25,
                "total": 80,
                "color": "#FF0000",
                "type": "capsule"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 25.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 80.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF0A59F7u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 0);
}

/**
 * @tc.name: ExtendedComponentTest015
 * @tc.desc: Verify Progress value/total only take effect at the descriptor root, while styles color/type remain valid.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest015)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Progress" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "styles": {
                    "value": 30,
                    "total": 60,
                    "color": "#00FF00",
                    "type": "capsule"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 0.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 100.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF00FF00u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 4);
}

/**
 * @tc.name: ExtendedComponentTest016
 * @tc.desc: Verify Progress accepts string numbers for value/total and normalized type tokens in styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest016)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Progress" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Progress",
                "value": "60",
                "total": "120",
                "styles": {
                    "color": "#112233",
                    "type": " scale_ring "
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedProgressComponent> progress =
        std::dynamic_pointer_cast<ExtendedProgressComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(progress, nullptr);

    EXPECT_FLOAT_EQ(progress->GetValueForTest(), 60.0F);
    EXPECT_FLOAT_EQ(progress->GetTotalForTest(), 120.0F);
    EXPECT_EQ(progress->GetColorForTest(), 0xFF112233u);
    EXPECT_EQ(progress->GetProgressTypeForTest(), 3);
}

/**
 * @tc.name: ExtendedComponentTest027
 * @tc.desc: Verify extended protocol creates NavContainer through the extended factory.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest027)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "NavContainer" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "NavContainer",
                "children": ["page-a", "page-b"],
                "currentIndex": 1
            },
            {
                "id": "page-a",
                "component": "Text",
                "text": "A"
            },
            {
                "id": "page-b",
                "component": "Text",
                "text": "B"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetType(), "NavContainer");
    EXPECT_EQ(std::dynamic_pointer_cast<CustomComponent>(root), nullptr);
    auto nav = std::dynamic_pointer_cast<NavContainerComponent>(root);
    ASSERT_NE(nav, nullptr);
    EXPECT_EQ(nav->GetCurrentIndexForTest(), 1);
    ASSERT_EQ(root->GetChildren().size(), 2U);
}

/**
 * @tc.name: ExtendedComponentTest028
 * @tc.desc: Verify Extended.Tabs only keeps TabContent/Extended.TabContent
 * children in static children list.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest028)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Extended.Tabs", "Extended.TabContent", "TabContent", "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Tabs",
                "children": ["tabExt", "invalidText", "tabShort"]
            },
            {
                "id": "tabExt",
                "component": "TabContent"
            },
            {
                "id": "invalidText",
                "component": "Text",
                "text": "invalid child for tabs"
            },
            {
                "id": "tabShort",
                "component": "TabContent"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> tabs = slot_.FindComponentById("root");
    ASSERT_NE(tabs, nullptr);
    bool hasTabExt = false;
    bool hasTabShort = false;
    bool hasInvalidText = false;
    size_t validChildrenCount = 0;
    for (const auto& child : tabs->GetChildren()) {
        if (child == nullptr) {
            continue;
        }
        validChildrenCount++;
        if (child->GetComponentId() == "tabExt") {
            hasTabExt = true;
        } else if (child->GetComponentId() == "tabShort") {
            hasTabShort = true;
        } else if (child->GetComponentId() == "invalidText") {
            hasInvalidText = true;
        }
    }
    EXPECT_EQ(validChildrenCount, 2u);
    EXPECT_TRUE(hasTabExt);
    EXPECT_TRUE(hasTabShort);
    EXPECT_FALSE(hasInvalidText);
}

/**
 * @tc.name: ExtendedComponentTest029
 * @tc.desc: Verify generic onAppear/onClick listeners register native common events on extended components.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest029)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedNodeEventCapture capture(nativeNodeApi);

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Column" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "onAppear": [{"call": "dispatchEvent", "args": {"eventName": "appeared"}}],
                "onClick": [{"call": "dispatchEvent", "args": {"eventName": "clicked"}}]
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    EXPECT_TRUE(HasRegisteredNodeEvent(NODE_EVENT_ON_APPEAR));
    EXPECT_TRUE(HasRegisteredNodeEvent(NODE_ON_CLICK));
}

/**
 * @tc.name: ExtendedComponentTest030
 * @tc.desc: Verify Extended.TabContent registers title path bindings with children mode.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest030)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Extended.Tabs", "Extended.TabContent" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Tabs",
                "children": ["tab1"]
            },
            {
                "id": "tab1",
                "component": "TabContent",
                "title": { "path": "/tabTitle" }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> tabContent = slot_.FindComponentById("tab1");
    ASSERT_NE(tabContent, nullptr);
    const std::vector<DataBinding>& bindings = tabContent->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1u);
    EXPECT_EQ(bindings[0].propertyName_, "title");
    EXPECT_EQ(bindings[0].dataPath_, "/tabTitle");
}

/**
 * @tc.name: ExtendedComponentTest031
 * @tc.desc: Verify ExtendedTextComponent supports min/max font aliases and normalized wordBreak values in styles.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest031)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "hello",
                "styles": {
                    "minFontSize": " 10 ",
                    "maxFontSize": "26",
                    "wordBreak": "  HYPHENATION  "
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));
    std::shared_ptr<ExtendedTextComponent> text =
        std::dynamic_pointer_cast<ExtendedTextComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(text, nullptr);

    EXPECT_FLOAT_EQ(text->GetMinFontSizeForTest(), 10.0F);
    EXPECT_FLOAT_EQ(text->GetMaxFontSizeForTest(), 26.0F);
    EXPECT_EQ(text->GetWordBreakForTest(), ARKUI_WORD_BREAK_HYPHENATION);
}

/**
 * @tc.name: ExtendedComponentTest032
 * @tc.desc: Verify Extended.Tabs serializes multiple static children into customProps.children.
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, ExtendedComponentTest032)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Extended.Tabs", "Extended.TabContent", "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Tabs",
                "children": ["tabHome", "tabOrders", "tabProfile"]
            },
            {
                "id": "tabHome",
                "component": "TabContent"
            },
            {
                "id": "tabOrders",
                "component": "TabContent"
            },
            {
                "id": "tabProfile",
                "component": "TabContent"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> tabs = slot_.FindComponentById("root");
    ASSERT_NE(tabs, nullptr);
    auto customTabs = std::dynamic_pointer_cast<CustomComponent>(tabs);
    ASSERT_NE(customTabs, nullptr);

    JsonValue customProps = customTabs->descriptor_.customProps;
    ASSERT_TRUE(customProps.IsObject());
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsArray());
    EXPECT_EQ(children.GetArraySize(), 3);
    EXPECT_EQ(children.GetArrayItem(0).GetStringValue(""), "tabHome");
    EXPECT_EQ(children.GetArrayItem(1).GetStringValue(""), "tabOrders");
    EXPECT_EQ(children.GetArrayItem(2).GetStringValue(""), "tabProfile");
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest033)
{
    ExtendedComponentFactory& factory = ExtendedComponentFactory::GetInstance();
    EXPECT_EQ(factory.GetShortName(""), "");
    EXPECT_EQ(factory.GetShortName("Extended."), "Extended.");
    EXPECT_EQ(factory.GetShortName("catalog.layout.Stack"), "Stack");
    EXPECT_EQ(factory.GetShortName("catalog.layout.Grid"), "Grid");
    EXPECT_EQ(factory.GetShortName("catalog.layout.List"), "List");

    factory.RegisterComponent("", nullptr);
    EXPECT_EQ(factory.CreateComponent(""), nullptr);
    EXPECT_EQ(factory.CreateComponent("NonExistent"), nullptr);

    std::shared_ptr<ExtendedComponent> stack = factory.CreateComponent("ohos.Stack");
    std::shared_ptr<ExtendedComponent> grid = factory.CreateComponent("ohos.Grid");
    std::shared_ptr<ExtendedComponent> list = factory.CreateComponent("ohos.List");
    ASSERT_NE(stack, nullptr);
    ASSERT_NE(grid, nullptr);
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(stack->GetType(), "Stack");
    EXPECT_EQ(grid->GetType(), "Grid");
    EXPECT_EQ(list->GetType(), "List");
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest034)
{
    TestableExtendedRowComponent component;

    struct RowAlignCase {
        const char* value;
        ArkUI_ItemAlignment expected;
    };
    const std::vector<RowAlignCase> cases = { { "top", ARKUI_ITEM_ALIGNMENT_START },
        { "start", ARKUI_ITEM_ALIGNMENT_CENTER }, { "center", ARKUI_ITEM_ALIGNMENT_CENTER },
        { "bottom", ARKUI_ITEM_ALIGNMENT_END }, { "end", ARKUI_ITEM_ALIGNMENT_CENTER },
        { "unsupported", ARKUI_ITEM_ALIGNMENT_CENTER } };

    for (const auto& testCase : cases) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        std::unique_ptr<JsonAdapter> styles =
            JsonAdapter::Parse(R"({"alignItems":")" + std::string(testCase.value) + R"("})");
        ASSERT_NE(styles, nullptr);
        component.CallApplyComponentSpecificStyles(styles->GetRoot());
        const MockArkUINativeProvider::SetAttributeRecord* record = FindLastSetAttribute(NODE_FLEX_OPTION);
        ASSERT_NE(record, nullptr);
        ASSERT_GE(record->values.size(), 4U);
        EXPECT_EQ(record->values[3].i32, testCase.expected);
    }

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Row" }));
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> rootPropertyMessage = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Row",
                "alignItems": "top"
            }
        ]
    })");
    ASSERT_NE(rootPropertyMessage, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(rootPropertyMessage->GetRoot()));
    const MockArkUINativeProvider::SetAttributeRecord* record = FindLastSetAttribute(NODE_FLEX_OPTION);
    ASSERT_NE(record, nullptr);
    ASSERT_GE(record->values.size(), 4U);
    EXPECT_EQ(record->values[3].i32, ARKUI_ITEM_ALIGNMENT_CENTER);

    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("alignItems");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));
}

/**
 * @tc.name: ExtendedRow 私有属性声明命中
 * @tc.desc: 覆盖 Row 新增的 itemMargin/wrap 声明路径，并验证 wrap applyValue 的原生映射。
 * @tc.type: FUNC
 */
TEST_F(ExtendedComponentTest, should_expose_extended_row_item_margin_and_wrap_property_declarations)
{
    TestableExtendedRowComponent component;

    PropertyDeclaration itemMarginDeclaration = component.GetPrivatePropertyDeclaration("itemMargin");
    EXPECT_EQ(itemMarginDeclaration.name, "itemMargin");
    ASSERT_TRUE(static_cast<bool>(itemMarginDeclaration.applyValue));

    std::unique_ptr<JsonAdapter> itemMarginValue = JsonAdapter::CreateNumber(18.0);
    ASSERT_NE(itemMarginValue, nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    const MockArkUINativeProvider::SetAttributeRecord* spaceRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_FLEX_SPACE);
    ASSERT_NE(spaceRecord, nullptr);
    ASSERT_EQ(spaceRecord->values.size(), 2U);
    EXPECT_FLOAT_EQ(spaceRecord->values[0].f32, 18.0F);
    EXPECT_FLOAT_EQ(spaceRecord->values[1].f32, 9.0F);

    PropertyDeclaration wrapDeclaration = component.GetPrivatePropertyDeclaration("wrap");
    EXPECT_EQ(wrapDeclaration.name, "wrap");
    EXPECT_EQ(wrapDeclaration.type, PropertyValueType::ENUM_STRING);
    EXPECT_EQ(wrapDeclaration.fallbackString, "noWrap");
    EXPECT_EQ(wrapDeclaration.enumFallback, "noWrap");
    EXPECT_EQ(wrapDeclaration.enumAllowed, (std::vector<std::string> { "noWrap", "wrap" }));
    ASSERT_TRUE(static_cast<bool>(wrapDeclaration.applyValue));

    std::unique_ptr<JsonAdapter> wrapValue = JsonAdapter::CreateString("wrap");
    ASSERT_NE(wrapValue, nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    wrapDeclaration.applyValue(wrapValue->GetRoot());
    const MockArkUINativeProvider::SetAttributeRecord* optionRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_FLEX_OPTION);
    ASSERT_NE(optionRecord, nullptr);
    ASSERT_GE(optionRecord->values.size(), 2U);
    EXPECT_EQ(optionRecord->values[1].i32, ARKUI_FLEX_WRAP_WRAP);
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest035)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List", "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List",
                "children": ["first", "second"],
                "space": 6,
                "styles": {
                    "listDirection": "horizontal",
                    "scrollBar": "on",
                    "nestedScroll": {
                        "scrollForward": "parentFirst",
                        "scrollBackward": "selfFirst"
                    }
                }
            },
            { "id": "first", "component": "Text", "content": "first" },
            { "id": "second", "component": "Text", "content": "second" }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedListComponent> root =
        std::dynamic_pointer_cast<ExtendedListComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 2U);

    const MockArkUINativeProvider::SetAttributeRecord* space = FindLastSetAttribute(NODE_LIST_SPACE);
    const MockArkUINativeProvider::SetAttributeRecord* lanes = FindLastSetAttribute(NODE_LIST_LANES);
    const MockArkUINativeProvider::SetAttributeRecord* direction = FindLastSetAttribute(NODE_LIST_DIRECTION);
    const MockArkUINativeProvider::SetAttributeRecord* scrollBar = FindLastSetAttribute(NODE_SCROLL_BAR_DISPLAY_MODE);
    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll = FindLastSetAttribute(NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(space, nullptr);
    ASSERT_NE(lanes, nullptr);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(scrollBar, nullptr);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_FALSE(space->values.empty());
    ASSERT_FALSE(lanes->values.empty());
    ASSERT_FALSE(direction->values.empty());
    ASSERT_FALSE(scrollBar->values.empty());
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_FLOAT_EQ(space->values[0].f32, 6.0F);
    EXPECT_EQ(lanes->values[0].i32, 1);
    EXPECT_EQ(direction->values[0].i32, ARKUI_AXIS_HORIZONTAL);
    EXPECT_EQ(scrollBar->values[0].i32, ARKUI_SCROLL_BAR_DISPLAY_MODE_ON);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListUpdatesLanesByBreakpointOnConfigChange)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);

    ThemeContext mdContext;
    mdContext.breakpoint = Breakpoint::MD;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(mdContext);
    const MockArkUINativeProvider::SetAttributeRecord* mdLanes = FindLastSetAttribute(NODE_LIST_LANES);
    ASSERT_NE(mdLanes, nullptr);
    ASSERT_FALSE(mdLanes->values.empty());
    EXPECT_EQ(mdLanes->values[0].i32, 2);

    ThemeContext lgContext;
    lgContext.breakpoint = Breakpoint::LG;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(lgContext);
    const MockArkUINativeProvider::SetAttributeRecord* lgLanes = FindLastSetAttribute(NODE_LIST_LANES);
    ASSERT_NE(lgLanes, nullptr);
    ASSERT_FALSE(lgLanes->values.empty());
    EXPECT_EQ(lgLanes->values[0].i32, 3);
}

TEST_F(ExtendedComponentTest, ExtendedListUsesThemeManagerBreakpointWhenSurfaceManagerUpdatesTheme)
{
    constexpr int32_t renderId = 240611;
    const std::string surfaceId = "extended-list-theme-surface";

    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot.GetSurfaceManager();
    ASSERT_NE(surfaceManager, nullptr);

    SurfaceSlot& themedSlot = surfaceManager->CreateSurface(surfaceId);
    themedSlot.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    ASSERT_TRUE(themedSlot.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedListComponent> root =
        std::dynamic_pointer_cast<ExtendedListComponent>(themedSlot.FindComponentById("root"));
    ASSERT_NE(root, nullptr);

    surfaceManager->UpdateBreakpoint(Breakpoint::LG);

    const MockArkUINativeProvider::SetAttributeRecord* lanes = FindLastSetAttribute(NODE_LIST_LANES);
    ASSERT_NE(lanes, nullptr);
    ASSERT_FALSE(lanes->values.empty());
    EXPECT_EQ(lanes->values[0].i32, 3);

    RenderManager::GetInstance().RemoveRenderSlot(renderId);
}

TEST_F(ExtendedComponentTest, ExtendedListAppliesSelfFirstNestedScrollByDefault)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll = FindLastSetAttribute(NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListResolvesOffScrollBarAndParallelNestedScroll)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List",
                "styles": {
                    "listDirection": "diagonal",
                    "scrollBar": "off",
                    "nestedScroll": "paraller"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    const MockArkUINativeProvider::SetAttributeRecord* direction = FindLastSetAttribute(NODE_LIST_DIRECTION);
    const MockArkUINativeProvider::SetAttributeRecord* scrollBar = FindLastSetAttribute(NODE_SCROLL_BAR_DISPLAY_MODE);
    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll = FindLastSetAttribute(NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(scrollBar, nullptr);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_FALSE(direction->values.empty());
    ASSERT_FALSE(scrollBar->values.empty());
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(direction->values[0].i32, ARKUI_AXIS_VERTICAL);
    EXPECT_EQ(scrollBar->values[0].i32, ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_PARALLEL);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_PARALLEL);
}

TEST_F(ExtendedComponentTest, ExtendedListResolvesSelfOnlyNestedScrollWhenStringValid)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "nestedScroll": "selfOnly"
    })");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
}

TEST_F(ExtendedComponentTest, ExtendedListDefaultsNestedScrollObjectFieldsToSelfFirst)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "nestedScroll": {}
    })");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListFallsBackToSelfFirstNestedScrollWhenObjectValuesInvalid)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "nestedScroll": {
            "scrollForward": "invalid",
            "scrollBackward": "unexpected"
        }
    })");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListFallsBackToAutoScrollBarAndSelfFirstNestedScrollForInvalidType)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List",
                "styles": {
                    "scrollBar": "unexpected",
                    "nestedScroll": 12345
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    const MockArkUINativeProvider::SetAttributeRecord* direction = FindLastSetAttribute(NODE_LIST_DIRECTION);
    const MockArkUINativeProvider::SetAttributeRecord* scrollBar = FindLastSetAttribute(NODE_SCROLL_BAR_DISPLAY_MODE);
    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll = FindLastSetAttribute(NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(direction, nullptr);
    ASSERT_NE(scrollBar, nullptr);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_FALSE(direction->values.empty());
    ASSERT_FALSE(scrollBar->values.empty());
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(direction->values[0].i32, ARKUI_AXIS_VERTICAL);
    EXPECT_EQ(scrollBar->values[0].i32, ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListSkipsAddWhenListItemNodeCreationFails)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    TestableExtendedListComponent component;
    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9101), false);

    {
        ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, ReturnNullNodeHandle);
        component.CallOnAddChild(child, 0);
    }

    EXPECT_TRUE(component.listItems_.empty());
    component.CallOnAddChild(nullptr, 0);
    component.CallOnMoveChild(nullptr, 0, 0);
    component.CallOnMoveChild(child, 0, 0);
    component.CallOnRemoveChild(nullptr);
    component.CallOnRemoveChild(child);
    EXPECT_TRUE(component.listItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest036)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Stack", "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Stack",
                "children": ["first", "second"],
                "styles": { "alignContent": "bottomEnd" }
            },
            { "id": "first", "component": "Text", "content": "first" },
            { "id": "second", "component": "Text", "content": "second" }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedStackComponent> root =
        std::dynamic_pointer_cast<ExtendedStackComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 2U);
    const MockArkUINativeProvider::SetAttributeRecord* align = FindLastSetAttribute(NODE_STACK_ALIGN_CONTENT);
    ASSERT_NE(align, nullptr);
    ASSERT_FALSE(align->values.empty());
    EXPECT_EQ(align->values[0].i32, ARKUI_ALIGNMENT_BOTTOM_END);
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest037)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid", "Text" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid",
                "children": ["first", "second"],
                "styles": {
                    "columnsTemplate": "1fr 2fr",
                    "rowsTemplate": "1fr 2fr",
                    "columnsGap": 8,
                    "rowsGap": 12
                }
            },
            { "id": "first", "component": "Text", "content": "first" },
            { "id": "second", "component": "Text", "content": "second" }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedGridComponent> root =
        std::dynamic_pointer_cast<ExtendedGridComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->GetChildren().size(), 2U);
    const MockArkUINativeProvider::SetAttributeRecord* columns = FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* rows = FindLastSetAttribute(NODE_GRID_ROW_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* columnsGap = FindLastSetAttribute(NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap = FindLastSetAttribute(NODE_GRID_ROW_GAP);
    ASSERT_NE(columns, nullptr);
    ASSERT_NE(rows, nullptr);
    ASSERT_NE(columnsGap, nullptr);
    ASSERT_NE(rowsGap, nullptr);
    EXPECT_EQ(columns->stringValue, "1fr 2fr");
    EXPECT_EQ(rows->stringValue, "1fr 2fr");
    ASSERT_FALSE(columnsGap->values.empty());
    ASSERT_FALSE(rowsGap->values.empty());
    EXPECT_FLOAT_EQ(columnsGap->values[0].f32, 8.0F);
    EXPECT_FLOAT_EQ(rowsGap->values[0].f32, 12.0F);
}

TEST_F(ExtendedComponentTest, ExtendedGridKeepsRowsTemplateAutoWhenRowsTemplateIsMissing)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedResetAttributeCapture resetCapture(nativeNodeApi);
    ResetAttributeCapture();

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid",
                "styles": {
                    "columnsTemplate": "1fr 1fr 1fr 1fr",
                    "columnsGap": 8,
                    "rowsGap": 8
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(FindLastSetAttribute(NODE_GRID_ROW_TEMPLATE), nullptr);
    EXPECT_FALSE(HasResetAttribute(NODE_GRID_ROW_TEMPLATE));
}

TEST_F(ExtendedComponentTest, ExtendedGridKeepsColumnsTemplateAutoWhenColumnsTemplateIsMissing)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedResetAttributeCapture resetCapture(nativeNodeApi);
    ResetAttributeCapture();

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid",
                "styles": {
                    "columnsGap": 8,
                    "rowsGap": 8
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_EQ(FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE), nullptr);
    EXPECT_FALSE(HasResetAttribute(NODE_GRID_COLUMN_TEMPLATE));
}

TEST_F(ExtendedComponentTest, ExtendedGridDoesNotUpdateColumnsTemplateWhenBreakpointChangesWithoutExplicitTemplate)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);

    ThemeContext context;
    context.breakpoint = Breakpoint::MD;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(context);
    EXPECT_EQ(FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE), nullptr);

    context.breakpoint = Breakpoint::LG;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(context);
    EXPECT_EQ(FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE), nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedGridKeepsExplicitColumnsTemplateWhenBreakpointChanges)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid",
                "styles": {
                    "columnsTemplate": "2fr 1fr 1fr 1fr"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<Component> root = slot_.FindComponentById("root");
    ASSERT_NE(root, nullptr);

    ThemeContext context;
    context.breakpoint = Breakpoint::XL;
    mockArkUIPtr_->setAttributeRecords_.clear();
    root->OnConfigChange(context);

    EXPECT_EQ(FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE), nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedGridFallsBackToSingleFrWhenTemplateValueIsInvalid)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "Grid" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "Grid",
                "styles": {
                    "columnsTemplate": "",
                    "rowsTemplate": 12345
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    const MockArkUINativeProvider::SetAttributeRecord* columns = FindLastSetAttribute(NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* rows = FindLastSetAttribute(NODE_GRID_ROW_TEMPLATE);
    ASSERT_NE(columns, nullptr);
    ASSERT_NE(rows, nullptr);
    EXPECT_EQ(columns->stringValue, "1fr");
    EXPECT_EQ(rows->stringValue, "1fr");
}

TEST_F(ExtendedComponentTest, ExtendedGridDefaultsGapsToZeroWhenMissingInNonDeltaStyleUpdate)
{
    TestableExtendedGridComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* columnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(columnsGap, nullptr);
    ASSERT_NE(rowsGap, nullptr);
    ASSERT_FALSE(columnsGap->values.empty());
    ASSERT_FALSE(rowsGap->values.empty());
    EXPECT_FLOAT_EQ(columnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(rowsGap->values[0].f32, 0.0F);
}

TEST_F(ExtendedComponentTest, ExtendedGridSkipsGapDefaultsWhenApplyingStyleDeltaUpdate)
{
    TestableExtendedGridComponent component;
    component.SetApplyingStyleDeltaUpdateForTest(true);
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* columnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_GAP);
    EXPECT_EQ(columnsGap, nullptr);
    EXPECT_EQ(rowsGap, nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedGridNormalizesNegativeGapValuesToZero)
{
    TestableExtendedGridComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "columnsGap": -12,
        "rowsGap": -6
    })");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* columnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(columnsGap, nullptr);
    ASSERT_NE(rowsGap, nullptr);
    ASSERT_FALSE(columnsGap->values.empty());
    ASSERT_FALSE(rowsGap->values.empty());
    EXPECT_FLOAT_EQ(columnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(rowsGap->values[0].f32, 0.0F);
}

TEST_F(ExtendedComponentTest, ExtendedGridSkipsAddWhenGridItemNodeCreationFails)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    TestableExtendedGridComponent component;
    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9201), false);

    {
        ScopedCreateNodeOverride createNodeOverride(nativeNodeApi, ReturnNullNodeHandle);
        component.CallOnAddChild(child, 0);
    }

    EXPECT_TRUE(component.gridItems_.empty());
    component.CallOnAddChild(nullptr, 0);
    component.CallOnMoveChild(nullptr, 0, 0);
    component.CallOnMoveChild(child, 0, 0);
    component.CallOnRemoveChild(nullptr);
    component.CallOnRemoveChild(child);
    EXPECT_TRUE(component.gridItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedListRegistersReachBoundaryListenersWhenActionsExist)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);
    ScopedNodeEventCapture nodeEventCapture(nativeNodeApi);

    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List",
                "onReachStart": [{"call": "onReachStartHandler", "args": {}}],
                "onReachEnd": [{"call": "onReachEndHandler", "args": {}}]
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    EXPECT_TRUE(HasRegisteredNodeEvent(NODE_SCROLL_EVENT_ON_REACH_START));
    EXPECT_TRUE(HasRegisteredNodeEvent(NODE_SCROLL_EVENT_ON_REACH_END));
}

TEST_F(ExtendedComponentTest, ExtendedListSkipsStyleDefaultsWhenApplyingStyleDeltaUpdate)
{
    TestableExtendedListComponent component;
    component.SetApplyingStyleDeltaUpdateForTest(true);
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* direction =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_DIRECTION);
    const MockArkUINativeProvider::SetAttributeRecord* scrollBar =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_BAR_DISPLAY_MODE);
    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    EXPECT_EQ(direction, nullptr);
    EXPECT_EQ(scrollBar, nullptr);
    EXPECT_EQ(nestedScroll, nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedListDefaultsMissingNestedScrollStyleToSelfFirstInNonDeltaUpdate)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({})");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListIgnoresNonObjectStylePayload)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    component.CallApplyComponentSpecificStyles(nonObjectStyles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* direction =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_DIRECTION);
    const MockArkUINativeProvider::SetAttributeRecord* scrollBar =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_BAR_DISPLAY_MODE);
    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    EXPECT_EQ(direction, nullptr);
    EXPECT_EQ(scrollBar, nullptr);
    EXPECT_EQ(nestedScroll, nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedListMovesAndRemovesExistingChildSlots)
{
    TestableExtendedListComponent component;
    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9301), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9302), false);

    component.CallOnAddChild(childA, 0);
    component.CallOnAddChild(childB, 1);
    ASSERT_EQ(component.listItems_.size(), 2U);
    ASSERT_EQ(component.listItems_[0].child.lock(), childA);
    ASSERT_EQ(component.listItems_[1].child.lock(), childB);

    component.CallOnMoveChild(childA, 0, 99);
    ASSERT_EQ(component.listItems_.size(), 2U);
    EXPECT_EQ(component.listItems_[0].child.lock(), childB);
    EXPECT_EQ(component.listItems_[1].child.lock(), childA);

    component.CallOnRemoveChild(childA);
    ASSERT_EQ(component.listItems_.size(), 1U);
    EXPECT_EQ(component.listItems_[0].child.lock(), childB);
}

TEST_F(ExtendedComponentTest, ExtendedListHandlesRemoveWhenNativeViewOrApiUnavailable)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9303), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9304), false);

    TestableExtendedListComponent noViewComponent;
    noViewComponent.CallOnAddChild(childA, 0);
    ASSERT_EQ(noViewComponent.listItems_.size(), 1U);
    ArkUI_NodeHandle originalView = noViewComponent.GetNativeView();
    noViewComponent.SetNativeViewForTest(nullptr);
    noViewComponent.CallOnRemoveChild(childA);
    noViewComponent.SetNativeViewForTest(originalView);
    EXPECT_TRUE(noViewComponent.listItems_.empty());

    TestableExtendedListComponent noApiComponent;
    noApiComponent.CallOnAddChild(childB, 0);
    ASSERT_EQ(noApiComponent.listItems_.size(), 1U);
    noApiComponent.SetNativeNodeApiForTest(nullptr);
    noApiComponent.CallOnRemoveChild(childB);
    noApiComponent.SetNativeNodeApiForTest(nativeNodeApi);
    EXPECT_TRUE(noApiComponent.listItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedListFallsBackToSelfFirstNestedScrollWhenStringInvalid)
{
    TestableExtendedListComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "nestedScroll": "invalid"
    })");
    ASSERT_NE(styles, nullptr);
    component.CallApplyComponentSpecificStyles(styles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* nestedScroll =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_SCROLL_NESTED_SCROLL);
    ASSERT_NE(nestedScroll, nullptr);
    ASSERT_EQ(nestedScroll->values.size(), 2U);
    EXPECT_EQ(nestedScroll->values[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    EXPECT_EQ(nestedScroll->values[1].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

TEST_F(ExtendedComponentTest, ExtendedListAppliesNodeAdapterInLazyModeAndSkipsAddChild)
{
    TestableExtendedListComponent component;
    auto adapterNode = std::make_shared<ListAdapterNode>();
    component.SetAdapterNode(adapterNode);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetLazyMode(true);
    const MockArkUINativeProvider::SetAttributeRecord* adapterRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER);
    ASSERT_NE(adapterRecord, nullptr);

    component.mode_ = ExtendedListComponent::Mode::LAZY;
    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9501), false);
    component.CallOnAddChild(child, 0);
    EXPECT_TRUE(component.listItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedListSetLazyModeAndSetAdapterNodeCoverBranchPaths)
{
    TestableExtendedListComponent component;
    auto adapterNode = std::make_shared<ListAdapterNode>();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetLazyMode(false);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER), nullptr);

    component.SetLazyMode(true);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER), nullptr);

    component.mode_ = ExtendedListComponent::Mode::EAGER;
    component.SetAdapterNode(std::shared_ptr<ListAdapterNode>());
    component.SetAdapterNode(adapterNode);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER), nullptr);

    component.SetLazyMode(true);
    component.SetAdapterNode(adapterNode);
    const MockArkUINativeProvider::SetAttributeRecord* adapterRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER);
    ASSERT_NE(adapterRecord, nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedListNormalizesNegativeSpaceAndSkipsNativeMoveWhenViewIsMissing)
{
    TestableExtendedListComponent component;
    PropertyDeclaration spaceDeclaration = component.GetPrivatePropertyDeclaration("space");
    ASSERT_EQ(spaceDeclaration.name, "space");
    ASSERT_TRUE(static_cast<bool>(spaceDeclaration.applyValue));

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> negativeSpace = JsonAdapter::CreateNumber(-8.0);
    ASSERT_NE(negativeSpace, nullptr);
    spaceDeclaration.applyValue(negativeSpace->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* spaceRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_SPACE);
    ASSERT_NE(spaceRecord, nullptr);
    ASSERT_FALSE(spaceRecord->values.empty());
    EXPECT_FLOAT_EQ(spaceRecord->values[0].f32, 0.0F);

    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9502), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9503), false);
    component.CallOnAddChild(childA, 0);
    component.CallOnAddChild(childB, 1);
    ASSERT_EQ(component.listItems_.size(), 2U);

    ArkUI_NodeHandle originalView = component.GetNativeView();
    component.SetNativeViewForTest(nullptr);
    component.CallOnMoveChild(childA, 0, 1);
    component.SetNativeViewForTest(originalView);

    ASSERT_EQ(component.listItems_.size(), 2U);
    EXPECT_EQ(component.listItems_[0].child.lock(), childB);
    EXPECT_EQ(component.listItems_[1].child.lock(), childA);
}

TEST_F(ExtendedComponentTest, ExtendedListSetupLazyAdapterCoversDataFailuresAndRelativePath)
{
    std::unique_ptr<JsonAdapter> templateDescriptor =
        JsonAdapter::Parse(R"({"id":"rowTemplate","component":"Text","content":"row"})");
    ASSERT_NE(templateDescriptor, nullptr);

    ChildListDescriptor absolutePathList = BuildTemplateChildListDescriptor("rowTemplate", "/items");
    ChildListDescriptor relativePathList = BuildTemplateChildListDescriptor("rowTemplate", "items");

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["rowTemplate"] = templateDescriptor->GetRoot();
        localSlot.bindingEngine_ = nullptr;
        component.SetupLazyAdapter(absolutePathList, localSlot);
        EXPECT_EQ(component.GetAdapterNode(), nullptr);
        EXPECT_FALSE(component.IsLazyMode());
    }

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["rowTemplate"] = templateDescriptor->GetRoot();
        component.SetupLazyAdapter(absolutePathList, localSlot);
        EXPECT_EQ(component.GetAdapterNode(), nullptr);
        EXPECT_FALSE(component.IsLazyMode());
    }

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        localSlot.allComponentDescriptorStore_["rowTemplate"] = templateDescriptor->GetRoot();
        component.SetupLazyAdapter(relativePathList, localSlot);
        ASSERT_NE(component.GetAdapterNode(), nullptr);
        EXPECT_TRUE(component.IsLazyMode());
        EXPECT_EQ(component.GetAdapterNode()->GetDataPath(), "items");
        EXPECT_NE(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_LIST_NODE_ADAPTER), nullptr);
    }
}

TEST_F(ExtendedComponentTest, ExtendedListSetupLazyAdapterHandlesTemplateAndDataFailures)
{
    std::unique_ptr<JsonAdapter> templateDescriptor =
        JsonAdapter::Parse(R"({"id":"rowTemplate","component":"Text","content":"row"})");
    ASSERT_NE(templateDescriptor, nullptr);
    ChildListDescriptor childList = BuildTemplateChildListDescriptor("rowTemplate", "/items");

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        component.SetupLazyAdapter(childList, localSlot);
        EXPECT_EQ(component.GetAdapterNode(), nullptr);
    }

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        localSlot.GetDescriptorsById()["rowTemplate"] = templateDescriptor->GetRoot();
        localSlot.bindingEngine_ = nullptr;
        component.SetupLazyAdapter(childList, localSlot);
        EXPECT_EQ(component.GetAdapterNode(), nullptr);
    }

    {
        TestableExtendedListComponent component;
        SurfaceSlot localSlot;
        localSlot.GetDescriptorsById()["rowTemplate"] = templateDescriptor->GetRoot();
        component.SetupLazyAdapter(childList, localSlot);
        EXPECT_EQ(component.GetAdapterNode(), nullptr);
    }
}

TEST_F(ExtendedComponentTest, ExtendedListSetupLazyAdapterReloadsExistingAdapterForAbsolutePath)
{
    std::unique_ptr<JsonAdapter> templateDescriptor =
        JsonAdapter::Parse(R"({"id":"rowTemplate","component":"Text","content":"row"})");
    std::unique_ptr<JsonAdapter> items = JsonAdapter::Parse(R"([
        {"label":"row-0"},
        {"label":"row-1"}
    ])");
    ASSERT_NE(templateDescriptor, nullptr);
    ASSERT_NE(items, nullptr);

    ChildListDescriptor childList = BuildTemplateChildListDescriptor("rowTemplate", "/items");

    TestableExtendedListComponent component;
    SurfaceSlot localSlot;
    localSlot.allComponentDescriptorStore_["rowTemplate"] = templateDescriptor->GetRoot();
    std::shared_ptr<DataModel> dataModel = localSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->UpdateByPath("/items", items->GetRoot());

    component.SetupLazyAdapter(childList, localSlot);
    std::shared_ptr<ListAdapterNode> firstAdapter = component.GetAdapterNode();
    ASSERT_NE(firstAdapter, nullptr);
    EXPECT_TRUE(component.IsLazyMode());
    EXPECT_EQ(firstAdapter->GetDataPath(), "/items");
    EXPECT_EQ(firstAdapter->GetDataModel(), dataModel);

    component.SetupLazyAdapter(childList, localSlot);
    EXPECT_EQ(component.GetAdapterNode(), firstAdapter);
}

TEST_F(ExtendedComponentTest, ExtendedListExpandTemplateChildrenInitializesLazyAdapterAndClearsChildIds)
{
    std::unique_ptr<JsonAdapter> templateDescriptor =
        JsonAdapter::Parse(R"({"id":"rowTemplate","component":"Text","content":"row"})");
    ASSERT_NE(templateDescriptor, nullptr);

    ChildListDescriptor childList = BuildTemplateChildListDescriptor("rowTemplate", "items");

    TestableExtendedListComponent component;
    SurfaceSlot localSlot;
    localSlot.allComponentDescriptorStore_["rowTemplate"] = templateDescriptor->GetRoot();
    std::list<std::string> childIds = { "stale-child" };

    EXPECT_FALSE(component.CallExpandTemplateChildren(childList, localSlot, childIds));
    EXPECT_TRUE(childIds.empty());
    ASSERT_NE(component.GetAdapterNode(), nullptr);
    EXPECT_TRUE(component.IsLazyMode());
}

TEST_F(ExtendedComponentTest, ExtendedListReachBoundaryHandlersCanBeDispatched)
{
    slot_.SetCatalog(BuildExtendedProtocolCatalog({ "List" }));
    std::unique_ptr<JsonAdapter> message = JsonAdapter::Parse(R"({
        "components": [
            {
                "id": "root",
                "component": "List",
                "onReachStart": [{"call": "onReachStartHandler", "args": {}}],
                "onReachEnd": [{"call": "onReachEndHandler", "args": {}}]
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(slot_.UpdateComponents(message->GetRoot()));

    std::shared_ptr<ExtendedListComponent> root =
        std::dynamic_pointer_cast<ExtendedListComponent>(slot_.FindComponentById("root"));
    ASSERT_NE(root, nullptr);

    ArkUI_NodeEvent event {};
    mockArkUIPtr_->SetNodeEventHandle(&event, root->GetNativeView());

    mockArkUIPtr_->SetNodeEventType(&event, NODE_SCROLL_EVENT_ON_REACH_START);
    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(root->GetNativeView(), &event));

    mockArkUIPtr_->SetNodeEventType(&event, NODE_SCROLL_EVENT_ON_REACH_END);
    EXPECT_TRUE(mockArkUIPtr_->DispatchNodeEvent(root->GetNativeView(), &event));
}

TEST_F(ExtendedComponentTest, ExtendedGridAppliesPrivateDefaultsAndFallsBackUnknownPropertyDeclaration)
{
    TestableExtendedGridComponent component;
    PropertyDeclaration declaration = component.GetPrivatePropertyDeclaration("unknownProperty");
    EXPECT_TRUE(declaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(declaration.applyValue));

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> descriptor = JsonAdapter::Parse(R"({})");
    ASSERT_NE(descriptor, nullptr);
    component.CallApplyPrivateAttributes(descriptor->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* columnsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_GAP);
    const MockArkUINativeProvider::SetAttributeRecord* rowsGap =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_GAP);
    ASSERT_NE(columnsGap, nullptr);
    ASSERT_NE(rowsGap, nullptr);
    ASSERT_FALSE(columnsGap->values.empty());
    ASSERT_FALSE(rowsGap->values.empty());
    EXPECT_FLOAT_EQ(columnsGap->values[0].f32, 0.0F);
    EXPECT_FLOAT_EQ(rowsGap->values[0].f32, 0.0F);
}

TEST_F(ExtendedComponentTest, ExtendedGridIgnoresNonObjectStylePayload)
{
    TestableExtendedGridComponent component;
    mockArkUIPtr_->setAttributeRecords_.clear();

    std::unique_ptr<JsonAdapter> nonObjectStyles = JsonAdapter::CreateNumber(2.0);
    ASSERT_NE(nonObjectStyles, nullptr);
    component.CallApplyComponentSpecificStyles(nonObjectStyles->GetRoot());

    const MockArkUINativeProvider::SetAttributeRecord* columnsTemplate =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_COLUMN_TEMPLATE);
    const MockArkUINativeProvider::SetAttributeRecord* rowsTemplate =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_ROW_TEMPLATE);
    EXPECT_EQ(columnsTemplate, nullptr);
    EXPECT_EQ(rowsTemplate, nullptr);
}

TEST_F(ExtendedComponentTest, ExtendedGridMovesAndRemovesExistingChildSlots)
{
    TestableExtendedGridComponent component;
    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9401), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9402), false);

    component.CallOnAddChild(childA, 0);
    component.CallOnAddChild(childB, 1);
    ASSERT_EQ(component.gridItems_.size(), 2U);
    ASSERT_EQ(component.gridItems_[0].child.lock(), childA);
    ASSERT_EQ(component.gridItems_[1].child.lock(), childB);

    component.CallOnMoveChild(childA, 0, 99);
    ASSERT_EQ(component.gridItems_.size(), 2U);
    EXPECT_EQ(component.gridItems_[0].child.lock(), childB);
    EXPECT_EQ(component.gridItems_[1].child.lock(), childA);

    component.CallOnRemoveChild(childA);
    ASSERT_EQ(component.gridItems_.size(), 1U);
    EXPECT_EQ(component.gridItems_[0].child.lock(), childB);
}

TEST_F(ExtendedComponentTest, ExtendedGridSkipsNativeOperationsWhenNativeHandlesAreUnavailable)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    auto childA = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9403), false);
    auto childB = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9404), false);

    TestableExtendedGridComponent noViewComponent;
    ArkUI_NodeHandle originalView = noViewComponent.GetNativeView();
    noViewComponent.SetNativeViewForTest(nullptr);
    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({
        "columnsTemplate": "1fr 1fr",
        "rowsTemplate": "1fr 1fr",
        "columnsGap": 4,
        "rowsGap": 6
    })");
    ASSERT_NE(styles, nullptr);
    noViewComponent.CallApplyComponentSpecificStyles(styles->GetRoot());
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    noViewComponent.CallOnAddChild(childA, 0);
    noViewComponent.SetNativeViewForTest(originalView);
    EXPECT_TRUE(noViewComponent.gridItems_.empty());

    TestableExtendedGridComponent noApiComponent;
    noApiComponent.CallOnAddChild(childB, 0);
    ASSERT_EQ(noApiComponent.gridItems_.size(), 1U);
    noApiComponent.SetNativeNodeApiForTest(nullptr);
    noApiComponent.CallOnRemoveChild(childB);
    noApiComponent.SetNativeNodeApiForTest(nativeNodeApi);
    EXPECT_TRUE(noApiComponent.gridItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedGridSetLazyModeAndSetAdapterNodeCoverBranchPaths)
{
    TestableExtendedGridComponent component;
    auto adapterNode = std::make_shared<GridAdapterNode>();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetLazyMode(false);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    component.SetLazyMode(true);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    component.mode_ = ExtendedGridComponent::Mode::EAGER;
    component.SetAdapterNode(adapterNode);
    EXPECT_EQ(FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER), nullptr);

    component.SetLazyMode(true);
    component.SetAdapterNode(adapterNode);
    const MockArkUINativeProvider::SetAttributeRecord* adapterRecord =
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_GRID_NODE_ADAPTER);
    ASSERT_NE(adapterRecord, nullptr);

    component.mode_ = ExtendedGridComponent::Mode::LAZY;
    auto child = std::make_shared<A2UIComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x9601), false);
    component.CallOnAddChild(child, 0);
    EXPECT_TRUE(component.gridItems_.empty());
}

TEST_F(ExtendedComponentTest, ExtendedGridSetupLazyAdapterHandlesAllFailureBranches)
{
    ArkUI_NativeNodeAPI_1* nativeNodeApi = ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
    ASSERT_NE(nativeNodeApi, nullptr);

    std::unique_ptr<JsonAdapter> templateDescriptor =
        JsonAdapter::Parse(R"({"id":"gridTemplate","component":"Text","content":"row"})");
    ASSERT_NE(templateDescriptor, nullptr);

    ChildListDescriptor absolutePathList = BuildTemplateChildListDescriptor("gridTemplate", "/items");
    ChildListDescriptor relativePathList = BuildTemplateChildListDescriptor("gridTemplate", "items");

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        component.SetNativeNodeApiForTest(nullptr);
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        ArkUI_NodeHandle originalView = component.GetNativeView();
        component.SetNativeViewForTest(nullptr);
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
        component.SetNativeViewForTest(originalView);
    }

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.GetDescriptorsById()["gridTemplate"] = templateDescriptor->GetRoot();
        localSlot.bindingEngine_ = nullptr;
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.GetDescriptorsById()["gridTemplate"] = templateDescriptor->GetRoot();
        EXPECT_FALSE(component.SetupLazyAdapter(absolutePathList, localSlot));
    }

    {
        TestableExtendedGridComponent component;
        SurfaceSlot localSlot;
        localSlot.GetDescriptorsById()["gridTemplate"] = templateDescriptor->GetRoot();
        ScopedSetAttributeOverride setAttributeOverride(nativeNodeApi, ReturnSetAttributeFailure);
        EXPECT_FALSE(component.SetupLazyAdapter(relativePathList, localSlot));
        EXPECT_FALSE(component.IsLazyMode());
    }
}

TEST_F(ExtendedComponentTest, ExtendedGridExpandTemplateChildrenFallsBackToEagerWhenLazySetupFails)
{
    TestableExtendedGridComponent component;
    ChildListDescriptor childList = BuildTemplateChildListDescriptor("missingTemplate", "/items");
    std::list<std::string> childIds;

    EXPECT_FALSE(component.CallExpandTemplateChildren(childList, slot_, childIds));
    EXPECT_TRUE(childIds.empty());
    EXPECT_FALSE(component.IsLazyMode());
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest018)
{
    auto component = std::make_shared<TestableExtendedRowComponent>();
    PropertyDeclaration oldSpaceDeclaration = component->GetPrivatePropertyDeclaration("space");
    EXPECT_TRUE(oldSpaceDeclaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(oldSpaceDeclaration.applyValue));

    PropertyDeclaration itemMarginDeclaration = component->GetPrivatePropertyDeclaration("itemMargin");
    EXPECT_EQ(itemMarginDeclaration.name, "itemMargin");
    EXPECT_EQ(itemMarginDeclaration.type, PropertyValueType::NUMBER);
    EXPECT_FALSE(itemMarginDeclaration.allowDynamic);
    EXPECT_DOUBLE_EQ(itemMarginDeclaration.fallbackNumber, 16.0);
    ASSERT_TRUE(static_cast<bool>(itemMarginDeclaration.applyValue));
    auto applyJustifyContent = [&component](const std::string& justifyValue) {
        std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({"justifyContent":")" + justifyValue + R"("})");
        ASSERT_NE(styles, nullptr);
        component->CallApplyComponentSpecificStyles(styles->GetRoot());
    };
    auto expectSpace = [&component, this](float space, float halfSpace) {
        const MockArkUINativeProvider::SetAttributeRecord* record =
            FindLastAttributeForNode(*mockArkUIPtr_, component->GetNativeView(), NODE_FLEX_SPACE);
        ASSERT_NE(record, nullptr);
        ASSERT_EQ(record->attribute, NODE_FLEX_SPACE);
        ASSERT_EQ(record->values.size(), 2U);
        EXPECT_FLOAT_EQ(record->values[0].f32, space);
        EXPECT_FLOAT_EQ(record->values[1].f32, halfSpace);
    };

    std::unique_ptr<JsonAdapter> itemMarginValue = JsonAdapter::CreateNumber(20.0);
    ASSERT_NE(itemMarginValue, nullptr);

    auto childA = CreateMarginChild(0x6101, 1.0F, 2.0F, 3.0F, 4.0F);
    auto childB = CreateMarginChild(0x6102, 5.0F, 6.0F, 7.0F, 8.0F);
    auto childC = CreateMarginChild(0x6103, 9.0F, 10.0F, 11.0F, 12.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    expectSpace(20.0F, 10.0F);

    const std::vector<std::string> disabledJustifyValues = { "spaceAround", "spaceBetween", "spaceEvenly" };
    for (const auto& justifyValue : disabledJustifyValues) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        applyJustifyContent(justifyValue);
        expectSpace(0.0F, 0.0F);
    }

    mockArkUIPtr_->setAttributeRecords_.clear();
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    expectSpace(0.0F, 0.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    applyJustifyContent("start");
    expectSpace(20.0F, 10.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> negativeItemMarginValue = JsonAdapter::CreateNumber(-4.0);
    ASSERT_NE(negativeItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(negativeItemMarginValue->GetRoot());
    expectSpace(16.0F, 8.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> infiniteItemMarginValue =
        JsonAdapter::CreateNumber(std::numeric_limits<double>::infinity());
    ASSERT_NE(infiniteItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(infiniteItemMarginValue->GetRoot());
    expectSpace(16.0F, 8.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> invalidItemMarginValue = JsonAdapter::CreateString("bad-margin");
    ASSERT_NE(invalidItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(invalidItemMarginValue->GetRoot());
    expectSpace(16.0F, 8.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->AddChild(childA);
    component->AddChild(childB);
    component->AddChild(childC);
    EXPECT_EQ(component->GetChildren().size(), 3U);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->CallOnRemoveChild(childB);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());

    component->RemoveAllChildren();
    EXPECT_TRUE(component->GetChildren().empty());
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest019)
{
    auto component = std::make_shared<TestableExtendedColumnComponent>();
    PropertyDeclaration oldSpaceDeclaration = component->GetPrivatePropertyDeclaration("space");
    EXPECT_TRUE(oldSpaceDeclaration.name.empty());
    EXPECT_FALSE(static_cast<bool>(oldSpaceDeclaration.applyValue));

    PropertyDeclaration itemMarginDeclaration = component->GetPrivatePropertyDeclaration("itemMargin");
    EXPECT_EQ(itemMarginDeclaration.name, "itemMargin");
    EXPECT_EQ(itemMarginDeclaration.type, PropertyValueType::NUMBER);
    EXPECT_FALSE(itemMarginDeclaration.allowDynamic);
    EXPECT_DOUBLE_EQ(itemMarginDeclaration.fallbackNumber, 8.0);
    ASSERT_TRUE(static_cast<bool>(itemMarginDeclaration.applyValue));
    auto applyJustifyContent = [&component](const std::string& justifyValue) {
        std::unique_ptr<JsonAdapter> styles = JsonAdapter::Parse(R"({"justifyContent":")" + justifyValue + R"("})");
        ASSERT_NE(styles, nullptr);
        component->CallApplyComponentSpecificStyles(styles->GetRoot());
    };

    std::unique_ptr<JsonAdapter> itemMarginValue = JsonAdapter::CreateNumber(16.0);
    ASSERT_NE(itemMarginValue, nullptr);

    auto childA = CreateMarginChild(0x7101, 1.0F, 2.0F, 3.0F, 4.0F);
    auto childB = CreateMarginChild(0x7102, 5.0F, 6.0F, 7.0F, 8.0F);
    auto childC = CreateMarginChild(0x7103, 9.0F, 10.0F, 11.0F, 12.0F);

    auto expectDefaultMargins = [&]() {
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childA->GetNativeView(), NODE_MARGIN), 1.0F, 2.0F, 7.0F, 4.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childB->GetNativeView(), NODE_MARGIN), 9.0F, 6.0F, 11.0F, 8.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childC->GetNativeView(), NODE_MARGIN), 13.0F, 10.0F, 11.0F, 12.0F);
    };
    auto expectItemMargin16 = [&]() {
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childA->GetNativeView(), NODE_MARGIN), 1.0F, 2.0F, 11.0F, 4.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childB->GetNativeView(), NODE_MARGIN), 13.0F, 6.0F, 15.0F, 8.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childC->GetNativeView(), NODE_MARGIN), 17.0F, 10.0F, 11.0F, 12.0F);
    };
    auto expectCommonMargins = [&]() {
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childA->GetNativeView(), NODE_MARGIN), 1.0F, 2.0F, 3.0F, 4.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childB->GetNativeView(), NODE_MARGIN), 5.0F, 6.0F, 7.0F, 8.0F);
        ExpectMarginValues(
            FindLastAttributeForNode(*mockArkUIPtr_, childC->GetNativeView(), NODE_MARGIN), 9.0F, 10.0F, 11.0F, 12.0F);
    };

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->AddChild(childA);
    component->AddChild(childB);
    component->AddChild(childC);
    expectDefaultMargins();

    mockArkUIPtr_->setAttributeRecords_.clear();
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    expectItemMargin16();

    const std::vector<std::string> disabledJustifyValues = { "spaceAround", "spaceBetween", "spaceEvenly" };
    for (const auto& justifyValue : disabledJustifyValues) {
        mockArkUIPtr_->setAttributeRecords_.clear();
        applyJustifyContent(justifyValue);
        expectCommonMargins();
    }

    mockArkUIPtr_->setAttributeRecords_.clear();
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    expectCommonMargins();

    mockArkUIPtr_->setAttributeRecords_.clear();
    applyJustifyContent("start");
    expectItemMargin16();

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> negativeItemMarginValue = JsonAdapter::CreateNumber(-2.0);
    ASSERT_NE(negativeItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(negativeItemMarginValue->GetRoot());
    expectDefaultMargins();

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> infiniteItemMarginValue =
        JsonAdapter::CreateNumber(std::numeric_limits<double>::infinity());
    ASSERT_NE(infiniteItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(infiniteItemMarginValue->GetRoot());
    expectDefaultMargins();

    mockArkUIPtr_->setAttributeRecords_.clear();
    std::unique_ptr<JsonAdapter> invalidItemMarginValue = JsonAdapter::CreateString("bad-margin");
    ASSERT_NE(invalidItemMarginValue, nullptr);
    itemMarginDeclaration.applyValue(invalidItemMarginValue->GetRoot());
    expectDefaultMargins();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->CallOnRemoveChild(childB);
    ExpectMarginValues(
        FindLastAttributeForNode(*mockArkUIPtr_, childB->GetNativeView(), NODE_MARGIN), 5.0F, 6.0F, 7.0F, 8.0F);
    ExpectMarginValues(
        FindLastAttributeForNode(*mockArkUIPtr_, childA->GetNativeView(), NODE_MARGIN), 1.0F, 2.0F, 7.0F, 4.0F);
    ExpectMarginValues(
        FindLastAttributeForNode(*mockArkUIPtr_, childC->GetNativeView(), NODE_MARGIN), 13.0F, 10.0F, 11.0F, 12.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->RemoveAllChildren();
    expectCommonMargins();
    EXPECT_TRUE(component->GetChildren().empty());
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest020)
{
    A2UIComponent component(reinterpret_cast<ArkUI_NodeHandle>(0x8101), false);
    mockArkUIPtr_->setAttributeRecords_.clear();
    component.SetMargin(2.0F, 4.0F, 6.0F, 8.0F);
    ExpectCommonMarginValues(component, 2.0F, 4.0F, 6.0F, 8.0F);
    ExpectMarginValues(
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_MARGIN), 2.0F, 4.0F, 6.0F, 8.0F);

    ArkUINodeApiAdapter applier = CreateNodeApiAdapter(component);
    applier.ResetNodeMargin(nullptr);
    ExpectCommonMarginValues(component, 2.0F, 4.0F, 6.0F, 8.0F);

    applier.ResetNodeMargin(reinterpret_cast<ArkUI_NodeHandle>(0x8102));
    ExpectCommonMarginValues(component, 2.0F, 4.0F, 6.0F, 8.0F);

    applier.ResetNodeFontSize(component.GetNativeView());
    ExpectCommonMarginValues(component, 2.0F, 4.0F, 6.0F, 8.0F);

    applier.ResetNodeMargin(component.GetNativeView());
    ExpectCommonMarginValues(component, 0.0F, 0.0F, 0.0F, 0.0F);

    mockArkUIPtr_->setAttributeRecords_.clear();
    applier.SetMargin(1.0F, 3.0F, 5.0F, 7.0F);
    ExpectCommonMarginValues(component, 1.0F, 3.0F, 5.0F, 7.0F);
    ExpectMarginValues(
        FindLastAttributeForNode(*mockArkUIPtr_, component.GetNativeView(), NODE_MARGIN), 1.0F, 3.0F, 5.0F, 7.0F);
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest021)
{
    auto component = std::make_shared<TestableExtendedRowComponent>();
    PropertyDeclaration itemMarginDeclaration = component->GetPrivatePropertyDeclaration("itemMargin");
    ASSERT_TRUE(static_cast<bool>(itemMarginDeclaration.applyValue));
    std::unique_ptr<JsonAdapter> itemMarginValue = JsonAdapter::CreateNumber(12.0);
    ASSERT_NE(itemMarginValue, nullptr);

    auto child = CreateMarginChild(0x6201, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUI_NativeNodeAPI_1* savedNativeNodeApi = component->GetNativeNodeApiForTest();
    ArkUI_NodeHandle savedNativeView = component->GetNativeViewForTest();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->SetNativeNodeApiForTest(nullptr);
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    component->CallOnRemoveChild(child);
    component->CallOnAddChild(nullptr, 0);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    component->SetNativeNodeApiForTest(savedNativeNodeApi);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->SetNativeViewForTest(nullptr);
    component->CallOnAddChild(child, 0);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    component->SetNativeViewForTest(savedNativeView);
}

TEST_F(ExtendedComponentTest, ExtendedComponentTest022)
{
    auto component = std::make_shared<TestableExtendedColumnComponent>();
    PropertyDeclaration itemMarginDeclaration = component->GetPrivatePropertyDeclaration("itemMargin");
    ASSERT_TRUE(static_cast<bool>(itemMarginDeclaration.applyValue));
    std::unique_ptr<JsonAdapter> itemMarginValue = JsonAdapter::CreateNumber(10.0);
    ASSERT_NE(itemMarginValue, nullptr);

    auto child = CreateMarginChild(0x7201, 1.0F, 2.0F, 3.0F, 4.0F);
    ArkUI_NativeNodeAPI_1* savedNativeNodeApi = component->GetNativeNodeApiForTest();
    ArkUI_NodeHandle savedNativeView = component->GetNativeViewForTest();

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->SetNativeNodeApiForTest(nullptr);
    itemMarginDeclaration.applyValue(itemMarginValue->GetRoot());
    component->CallOnRemoveChild(child);
    component->CallOnAddChild(nullptr, 0);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    component->SetNativeNodeApiForTest(savedNativeNodeApi);

    mockArkUIPtr_->setAttributeRecords_.clear();
    component->SetNativeViewForTest(nullptr);
    component->CallOnAddChild(child, 0);
    EXPECT_TRUE(mockArkUIPtr_->setAttributeRecords_.empty());
    component->SetNativeViewForTest(savedNativeView);
}

} // namespace
