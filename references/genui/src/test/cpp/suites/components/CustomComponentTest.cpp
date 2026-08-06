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

#include <gtest/gtest.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "functions/WarningDispatchBridge.h"

#include "TestFixture.h"

#define private public
#define protected public
#include "components/custom/CustomComponent.h"
#undef protected
#undef private

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/Component.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "functions/FunctionBridge.h"
#include "utils/JsonAdapter.h"

#include "NapiResourceManager.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

using namespace NativeModule;

namespace {

class SimpleChildComponent : public Component {
public:
    explicit SimpleChildComponent(ArkUI_NodeHandle nativeView) : Component(nativeView, false) {}
};

class TypedChildComponent : public Component {
public:
    TypedChildComponent(const std::string& type, ArkUI_NodeHandle nativeView = nullptr)
        : Component(nativeView, false), type_(type)
    {}

    std::string GetType() const override
    {
        return type_;
    }

private:
    std::string type_;
};

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

struct DispatchCallbacks {
    napi_env env = nullptr;
    napi_value warningCallback = nullptr;
};

DispatchCallbacks RegisterDispatchCallbacks(MockNapiProvider* mockNapi)
{
    DispatchCallbacks callbacks;
    if (mockNapi == nullptr) {
        return callbacks;
    }

    callbacks.env = reinterpret_cast<napi_env>(0x1401);
    mockNapi->CreateFunction(
        callbacks.env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &callbacks.warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(callbacks.env, callbacks.warningCallback);

    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionRecv_ = nullptr;
    mockNapi->lastCallFunctionFunc_ = nullptr;
    mockNapi->lastCallFunctionArgs_.clear();
    mockNapi->callFunctionArgsHistory_.clear();
    return callbacks;
}

napi_value GetRequestProperty(const MockNapiProvider* mockNapi, napi_value request, const std::string& key)
{
    if (mockNapi == nullptr || request == nullptr) {
        return nullptr;
    }
    auto objectIt = mockNapi->objectProperties_.find(request);
    if (objectIt == mockNapi->objectProperties_.end()) {
        return nullptr;
    }
    auto propIt = objectIt->second.find(key);
    if (propIt == objectIt->second.end()) {
        return nullptr;
    }
    return propIt->second;
}

std::string GetStringValue(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto it = mockNapi->stringValues_.find(value);
    return it == mockNapi->stringValues_.end() ? "" : it->second;
}

size_t CountWarningRequests(const MockNapiProvider* mockNapi, const std::string& code, const std::string& pathFragment)
{
    if (mockNapi == nullptr) {
        return 0U;
    }

    size_t count = 0U;
    for (const auto& args : mockNapi->callFunctionArgsHistory_) {
        if (args.empty()) {
            continue;
        }
        napi_value request = args.front();
        std::string actualCode = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "code"));
        std::string actualPath = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "path"));
        if (actualCode == code && actualPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

JsonValue CreateStringValue(const std::string& value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

JsonValue CreateNumberValue(double value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateNumber(value);
    return adapter != nullptr ? adapter->GetRoot() : JsonValue();
}

size_t CountBindingsForProperty(const Component& component, const std::string& propertyName)
{
    size_t count = 0U;
    for (const auto& binding : component.GetDataBindings()) {
        if (binding.propertyName_ == propertyName) {
            ++count;
        }
    }
    return count;
}

napi_ref CreateMockFunctionRef(MockNapiProvider* mockNapi, napi_env env)
{
    if (mockNapi == nullptr) {
        return nullptr;
    }

    napi_value callback = nullptr;
    if (mockNapi->CreateFunction(env, "testCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback) != napi_ok ||
        callback == nullptr) {
        return nullptr;
    }

    napi_ref callbackRef = nullptr;
    if (mockNapi->CreateReference(env, callback, 1, &callbackRef) != napi_ok) {
        return nullptr;
    }
    return callbackRef;
}

napi_value RawNapiValue(intptr_t id)
{
    return reinterpret_cast<napi_value>(id);
}

bool PreparePassthroughNormalizeResponse(MockNapiProvider* mockNapi)
{
    if (mockNapi == nullptr) {
        return false;
    }

    napi_value success = nullptr;
    if (mockNapi->CreateBoolean(nullptr, true, &success) != napi_ok || success == nullptr) {
        return false;
    }

    intptr_t firstValueId = static_cast<intptr_t>(mockNapi->nextValueId_);
    napi_value normalizedArgs = RawNapiValue(firstValueId + 5);
    napi_value result = RawNapiValue(firstValueId + 10);
    mockNapi->valueTypes_[result] = napi_object;
    mockNapi->objectProperties_[result] = { { "success", success }, { "normalizedArgs", normalizedArgs } };
    return true;
}

class FunctionBridgeResetGuard {
public:
    FunctionBridgeResetGuard(MockNapiProvider* mockNapi, napi_env env) : mockNapi_(mockNapi), env_(env) {}

    ~FunctionBridgeResetGuard()
    {
        if (mockNapi_ == nullptr) {
            return;
        }
        napi_value callback = nullptr;
        mockNapi_->CreateFunction(env_, "resetFunctionBridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        mockNapi_->SetCreateReferenceStatus(napi_invalid_arg);
        FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env_, callback);
        mockNapi_->ResetCreateReferenceStatus();
    }

private:
    MockNapiProvider* mockNapi_ = nullptr;
    napi_env env_ = nullptr;
};

struct NapiResourceManagerMirror {
    napi_env napiEnv_ = nullptr;
    napi_ref createCustomComponentRef_ = nullptr;
    napi_ref updateCustomComponentRef_ = nullptr;
};

class NapiResourceManagerRefGuard {
public:
    NapiResourceManagerRefGuard(MockNapiProvider* mockNapi, napi_env env, napi_value createFunc, napi_value updateFunc)
    {
        NapiResourceManager* resourceManager = RenderManager::GetInstance().GetNapiResourceManager();
        if (resourceManager == nullptr || mockNapi == nullptr) {
            return;
        }
        mirror_ = reinterpret_cast<NapiResourceManagerMirror*>(resourceManager);
        oldEnv_ = mirror_->napiEnv_;
        oldCreateRef_ = mirror_->createCustomComponentRef_;
        oldUpdateRef_ = mirror_->updateCustomComponentRef_;
        mirror_->napiEnv_ = env;
        if (createFunc != nullptr) {
            mockNapi->CreateReference(env, createFunc, 1, &mirror_->createCustomComponentRef_);
        } else {
            mirror_->createCustomComponentRef_ = nullptr;
        }
        if (updateFunc != nullptr) {
            mockNapi->CreateReference(env, updateFunc, 1, &mirror_->updateCustomComponentRef_);
        } else {
            mirror_->updateCustomComponentRef_ = nullptr;
        }
    }

    ~NapiResourceManagerRefGuard()
    {
        if (mirror_ == nullptr) {
            return;
        }
        mirror_->napiEnv_ = oldEnv_;
        mirror_->createCustomComponentRef_ = oldCreateRef_;
        mirror_->updateCustomComponentRef_ = oldUpdateRef_;
    }

private:
    NapiResourceManagerMirror* mirror_ = nullptr;
    napi_env oldEnv_ = nullptr;
    napi_ref oldCreateRef_ = nullptr;
    napi_ref oldUpdateRef_ = nullptr;
};

class RenderSlotCleanupGuard {
public:
    explicit RenderSlotCleanupGuard(int32_t renderId) : renderId_(renderId) {}
    ~RenderSlotCleanupGuard()
    {
        RenderManager::GetInstance().RemoveRenderSlot(renderId_);
    }

private:
    int32_t renderId_ = -1;
};

napi_value CreateManualNapiObject(MockNapiProvider* mockNapi, intptr_t id)
{
    napi_value value = RawNapiValue(id);
    if (mockNapi != nullptr) {
        mockNapi->valueTypes_[value] = napi_object;
        mockNapi->objectProperties_[value] = {};
    }
    return value;
}

napi_value CreateManualNapiFunction(MockNapiProvider* mockNapi, intptr_t id)
{
    napi_value value = RawNapiValue(id);
    if (mockNapi != nullptr) {
        mockNapi->valueTypes_[value] = napi_function;
    }
    return value;
}

void PreloadCustomComponentCallResults(MockNapiProvider* mockNapi, napi_value contentValue, napi_value childSlotValue,
    napi_value childSlotsValue, int32_t begin, int32_t end)
{
    if (mockNapi == nullptr) {
        return;
    }
    for (int32_t id = begin; id <= end; ++id) {
        napi_value resultObject = RawNapiValue(id);
        mockNapi->valueTypes_[resultObject] = napi_object;
        mockNapi->objectProperties_[resultObject]["content"] = contentValue;
        if (childSlotValue != nullptr) {
            mockNapi->objectProperties_[resultObject]["childSlot"] = childSlotValue;
        }
        if (childSlotsValue != nullptr) {
            mockNapi->objectProperties_[resultObject]["childSlotsObject"] = childSlotsValue;
        }
    }
}

class CustomComponentTest : public A2UITest {};

class CustomComponentProbe : public CustomComponent {
public:
    explicit CustomComponentProbe(const std::string& type) : CustomComponent(type) {}

    using CustomComponent::AcceptsChild;
    using CustomComponent::ApplyCustomProperties;
    using CustomComponent::BuildCustomProps;
    using CustomComponent::CollectChildListDescriptor;
    using CustomComponent::CreateAttributeValue;
    using CustomComponent::GetShortType;
    using CustomComponent::NormalizeCustomProperty;
    using CustomComponent::ParseTabsMapping;
    using CustomComponent::ResolveTabsChildIds;

    void CallOnAttachToParent()
    {
        OnAttachToParent();
    }
};

TEST_F(CustomComponentTest, should_prefer_component_id_for_custom_property_warning_path)
{
    EXPECT_EQ(ResolveCustomPropertyWarningPath("target", "url"), "target.url");
    EXPECT_EQ(ResolveCustomPropertyWarningPath("", "url"), "customProps.url");
}

/**
 * @tc.name: should_recognize_only_exact_tabs_type_in_extended_catalog
 * @tc.desc: 验证扩展协议只识别短名 Tabs，不再把带命名空间的 *.Tabs 当作扩展 Tabs。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_recognize_only_exact_tabs_type_in_extended_catalog)
{
    SurfaceContext extendedContext;
    extendedContext.catalogId = A2UI_EXTENDED_CATALOG_ID;

    CustomComponent shortTabs("Tabs");
    EXPECT_TRUE(shortTabs.IsTabsType());
    EXPECT_FALSE(shortTabs.IsExtendedTabsType());
    shortTabs.SetSurfaceContext(extendedContext);
    EXPECT_TRUE(shortTabs.IsExtendedTabsType());

    CustomComponent formerExtendedTabs("Extended.Tabs");
    formerExtendedTabs.SetSurfaceContext(extendedContext);
    EXPECT_FALSE(formerExtendedTabs.IsTabsType());
    EXPECT_FALSE(formerExtendedTabs.IsExtendedTabsType());

    CustomComponent otherNamespacedTabs("Legacy.Tabs");
    otherNamespacedTabs.SetSurfaceContext(extendedContext);
    EXPECT_FALSE(otherNamespacedTabs.IsTabsType());
    EXPECT_FALSE(otherNamespacedTabs.IsExtendedTabsType());
}

/**
 * @tc.name: 自定义组件Row children集合解析与row槽映射
 * @tc.desc: 覆盖Row的children解析、row-槽位前缀和children序列化分支。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_parse_row_children_into_row_slots_and_custom_props)
{
    CustomComponentProbe rowComponent("Extended.Row");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "children": ["item-a", "item-b", ""]
        })");
    ASSERT_NE(descriptor, nullptr);

    rowComponent.ParseTabsMapping(descriptor->GetRoot());
    ASSERT_EQ(rowComponent.childToSlotMapping_.size(), 2U);
    EXPECT_EQ(rowComponent.childToSlotMapping_["item-a"], "row-0");
    EXPECT_EQ(rowComponent.childToSlotMapping_["item-b"], "row-1");
    ASSERT_EQ(rowComponent.rowChildIds_.size(), 2U);
    EXPECT_EQ(rowComponent.rowChildIds_[0], "item-a");
    EXPECT_EQ(rowComponent.rowChildIds_[1], "item-b");

    JsonValue customProps = rowComponent.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsArray());
    ASSERT_EQ(children.GetArraySize(), 2);
    EXPECT_EQ(children.GetArrayItem(0).GetStringValue(""), "item-a");
    EXPECT_EQ(children.GetArrayItem(1).GetStringValue(""), "item-b");
}

/**
 * @tc.name: 自定义组件children默认解析回退
 * @tc.desc: 覆盖未知类型的CollectChildListDescriptor回退以及Tabs空children回退分支。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_fall_back_to_base_child_list_collection_for_unknown_and_empty_tabs)
{
    CustomComponentProbe unknownComponent("Card");
    std::unique_ptr<JsonAdapter> unknownDescriptor = ParseJson(
        R"({
            "children": ["child-a"]
        })");
    ASSERT_NE(unknownDescriptor, nullptr);
    unknownComponent.CollectChildListDescriptor(unknownDescriptor->GetRoot());
    EXPECT_EQ(unknownComponent.childListDescriptor_.type, ChildListType::INVALID);
    EXPECT_TRUE(unknownComponent.childListDescriptor_.staticChildIds.empty());

    CustomComponentProbe tabsComponent("Tabs");
    std::unique_ptr<JsonAdapter> tabsDescriptor = ParseJson(
        R"({
            "tabs": [
                {"child": ""},
                1
            ]
        })");
    ASSERT_NE(tabsDescriptor, nullptr);
    tabsComponent.CollectChildListDescriptor(tabsDescriptor->GetRoot());
    EXPECT_EQ(tabsComponent.childListDescriptor_.type, ChildListType::STATIC_IDS);
    EXPECT_TRUE(tabsComponent.childListDescriptor_.staticChildIds.empty());
}

/**
 * @tc.name: 自定义组件短类型解析边界
 * @tc.desc: 覆盖GetShortType在无分隔符、正常分隔符和尾部分隔符场景下的分支。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_resolve_short_type_for_plain_and_dotted_names)
{
    CustomComponentProbe component("Card");
    EXPECT_EQ(component.GetShortType("PlainType"), "PlainType");
    EXPECT_EQ(component.GetShortType("Outer.Inner"), "Inner");
    EXPECT_EQ(component.GetShortType("Outer."), "Outer.");
}

/**
 * @tc.name: 自定义动态描述符兼容告警
 * @tc.desc: 覆盖path/call动态描述符、兼容告警、非兼容属性和无renderId早退分支。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_dispatch_dynamic_descriptor_warnings_for_supported_and_unsupported_properties)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent iconComponent("Icon");
    iconComponent.SetComponentId("icon-id");
    iconComponent.SetSurfaceId("surface-id");
    iconComponent.SetRenderId(101);

    std::unique_ptr<JsonAdapter> iconDescriptor = ParseJson(
        R"({
            "name": {"path": "/title"}
        })");
    ASSERT_NE(iconDescriptor, nullptr);
    iconComponent.ApplyCustomProperties(iconDescriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_.back().front();
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")), "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "componentId")), "icon-id");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "icon-id.name");
    EXPECT_NE(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message")).find("local path binding"),
        std::string::npos);
    ASSERT_EQ(iconComponent.GetDataBindings().size(), 1U);
    EXPECT_EQ(iconComponent.GetDataBindings()[0].propertyName_, "name");
    EXPECT_EQ(iconComponent.GetDataBindings()[0].dataPath_, "/title");

    CustomComponent dividerComponent("Divider");
    dividerComponent.SetComponentId("divider-id");
    dividerComponent.SetSurfaceId("surface-id");
    dividerComponent.SetRenderId(102);
    std::unique_ptr<JsonAdapter> dividerDescriptor = ParseJson(
        R"({
            "axis": {"call": "resolveAxis"}
        })");
    ASSERT_NE(dividerDescriptor, nullptr);
    dividerComponent.ApplyCustomProperties(dividerDescriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 2U);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    request = mockNapiPtr_->callFunctionArgsHistory_.back().front();
    EXPECT_NE(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message"))
                  .find("function-call descriptor"),
        std::string::npos);
    EXPECT_TRUE(dividerComponent.GetDataBindings().empty());

    CustomComponent colorComponent("Icon");
    colorComponent.SetComponentId("icon-color");
    colorComponent.SetSurfaceId("surface-id");
    colorComponent.SetRenderId(103);
    std::unique_ptr<JsonAdapter> colorDescriptor = ParseJson(
        R"({
            "color": {"path": "/color"}
        })");
    ASSERT_NE(colorDescriptor, nullptr);
    colorComponent.ApplyCustomProperties(colorDescriptor->GetRoot());
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 2U);
    EXPECT_EQ(colorComponent.GetDataBindings().size(), 1U);
    EXPECT_EQ(colorComponent.GetDataBindings()[0].propertyName_, "color");
    EXPECT_EQ(colorComponent.GetDataBindings()[0].dataPath_, "/color");

    CustomComponent noRenderIcon("Icon");
    noRenderIcon.SetComponentId("icon-no-render");
    noRenderIcon.SetSurfaceId("surface-id");
    std::unique_ptr<JsonAdapter> noRenderDescriptor = ParseJson(
        R"({
            "name": {"path": "/title"}
        })");
    ASSERT_NE(noRenderDescriptor, nullptr);
    noRenderIcon.ApplyCustomProperties(noRenderDescriptor->GetRoot());
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 2U);
}

TEST_F(CustomComponentTest, should_keep_supported_extended_tab_content_style_values_without_schema_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabHome");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(201);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "fontSize": 16,
                "fontWeight": "bold",
                "iconSize": 18,
                "space": 6
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("fontSize"));
    EXPECT_TRUE(styles.Has("fontWeight"));
    EXPECT_TRUE(styles.Has("iconSize"));
    EXPECT_TRUE(styles.Has("space"));
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_invalid_extended_tab_content_style_tokens)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabHome");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(202);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "fontSize": "big",
                "fontWeight": "superBold",
                "iconSize": "big",
                "space": "wide"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 4U);
    ASSERT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 4U);
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[0].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[0].front(), "path")),
        "tabHome.styles.fontSize");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[1].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[1].front(), "path")),
        "tabHome.styles.iconSize");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[2].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[2].front(), "path")),
        "tabHome.styles.space");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[3].front(), "code")),
        "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[3].front(), "path")),
        "tabHome.styles.fontWeight");

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_FALSE(styles.Has("fontSize"));
    EXPECT_FALSE(styles.Has("fontWeight"));
    EXPECT_FALSE(styles.Has("iconSize"));
    EXPECT_FALSE(styles.Has("space"));
}

TEST_F(CustomComponentTest, should_drop_negative_extended_tab_content_metric_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabMetricRange");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(208);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(R"({"styles":{"fontSize":-1,"iconSize":-2,"space":-3}})");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabMetricRange.styles.fontSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabMetricRange.styles.iconSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabMetricRange.styles.space"), 1U);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    EXPECT_FALSE(styles.Has("fontSize"));
    EXPECT_FALSE(styles.Has("iconSize"));
    EXPECT_FALSE(styles.Has("space"));
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_empty_extended_tab_content_style_tokens)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabHome");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(203);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "fontSize": "",
                "fontWeight": "",
                "iconSize": "",
                "space": ""
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 4U);
    ASSERT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 4U);
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[0].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[1].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[2].front(), "code")),
        "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[3].front(), "code")),
        "ERROR_CODE_INVALID_VALUE");
    EXPECT_EQ(GetStringValue(mockNapiPtr_,
                  GetRequestProperty(mockNapiPtr_, mockNapiPtr_->callFunctionArgsHistory_[3].front(), "path")),
        "tabHome.styles.fontWeight");
}

TEST_F(CustomComponentTest, should_dispatch_schema_warning_and_drop_invalid_extended_web_url_value)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(204);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "url": 123
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_.back().front();
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")), "ERROR_CODE_TYPE_MISMATCH");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "webMain.url");
    EXPECT_FALSE(component.GetProperty("url").has_value());
}

TEST_F(CustomComponentTest, should_dispatch_required_warning_when_extended_web_url_is_missing)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(204);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(R"({})");
    ASSERT_NE(descriptor, nullptr);

    component.ValidateComponentDescriptorSchema(descriptor->GetRoot());

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_.back().front();
    EXPECT_EQ(
        GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code")), "ERROR_CODE_REQUIRED_MISS");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path")), "webMain.url");
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_invalid_extended_web_common_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(205);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "width": {},
                "height": "abc",
                "backgroundImage": 123,
                "clip": "maybe",
                "backgroundColor": "red",
                "borderWidth": {
                    "top": "abc"
                },
                "borderColor": {
                    "left": "red"
                },
                "padding": {
                    "bottom": "1pp"
                },
                "layoutWeight": {
                    "value": 1
                },
                "shadow": {
                    "fill": "yes"
                },
                "linearGradient": {
                    "colors": "#FF0000",
                    "direction": "diagonal",
                    "repeating": "maybe"
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMain.styles.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.height"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMain.styles.backgroundImage"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.clip"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.backgroundColor"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMain.styles.borderWidth.top"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.borderColor.left"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMain.styles.padding.bottom"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMain.styles.layoutWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.shadow.fill"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.linearGradient.colors"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.linearGradient.direction"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMain.styles.linearGradient.repeating"), 1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_FALSE(styles.Has("width"));
    EXPECT_FALSE(styles.Has("height"));
    EXPECT_FALSE(styles.Has("backgroundImage"));
    EXPECT_FALSE(styles.Has("clip"));
    EXPECT_FALSE(styles.Has("backgroundColor"));
    EXPECT_FALSE(styles.Has("borderWidth"));
    EXPECT_FALSE(styles.Has("borderColor"));
    ASSERT_TRUE(styles.Has("padding"));
    EXPECT_FALSE(styles.GetItem("padding").Has("bottom"));
    EXPECT_FALSE(styles.Has("layoutWeight"));
    EXPECT_FALSE(styles.Has("shadow"));
    EXPECT_FALSE(styles.Has("linearGradient"));
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_invalid_extended_tabs_common_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(205);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "constraintSize": {
                    "minWidth": -1,
                    "maxWidth": "bad"
                },
                "margin": {
                    "top": 10,
                    "right": ""
                },
                "borderRadius": {
                    "topLeft": "",
                    "topRight": 8
                },
                "borderWidth": "",
                "backgroundImageSizeWithStyle": {
                    "width": "",
                    "height": "60vp"
                },
                "linearGradient": {
                    "direction": "",
                    "colors": [
                        ["#667EEA", 0],
                        ["#764BA2", 1]
                    ]
                },
                "flexShrink": -0.5,
                "shadow": {
                    "radius": -1,
                    "type": "bad",
                    "offsetX": ""
                },
                "visibility": ""
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.constraintSize.minWidth"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsMain.styles.constraintSize.maxWidth"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.margin.right"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.borderRadius.topLeft"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.borderWidth"), 1U);
    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.backgroundImageSizeWithStyle.width"),
        1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.linearGradient.direction"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.flexShrink"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.shadow.radius"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.shadow.type"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsMain.styles.shadow.offsetX"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMain.styles.visibility"), 1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_FALSE(styles.Has("constraintSize"));
    ASSERT_TRUE(styles.Has("margin"));
    EXPECT_FALSE(styles.GetItem("margin").Has("right"));
    ASSERT_TRUE(styles.Has("borderRadius"));
    EXPECT_FALSE(styles.GetItem("borderRadius").Has("topLeft"));
    EXPECT_FALSE(styles.Has("borderWidth"));
    ASSERT_TRUE(styles.Has("backgroundImageSizeWithStyle"));
    EXPECT_FALSE(styles.GetItem("backgroundImageSizeWithStyle").Has("width"));
    ASSERT_TRUE(styles.Has("linearGradient"));
    EXPECT_FALSE(styles.GetItem("linearGradient").Has("direction"));
    EXPECT_FALSE(styles.Has("flexShrink"));
    EXPECT_FALSE(styles.Has("shadow"));
    EXPECT_FALSE(styles.Has("visibility"));
}

TEST_F(CustomComponentTest, should_report_null_extended_common_styles_as_invalid_value)
{
    RegisterDispatchCallbacks(mockNapiPtr_);
    auto countExactWarningRequests = [this](const std::string& code, const std::string& path) {
        size_t count = 0U;
        for (const auto& args : mockNapiPtr_->callFunctionArgsHistory_) {
            if (args.empty()) {
                continue;
            }
            napi_value request = args.front();
            std::string actualCode = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "code"));
            std::string actualPath = GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "path"));
            if (actualCode == code && actualPath == path) {
                ++count;
            }
        }
        return count;
    };

    CustomComponentProbe stylesComponent("Tabs");
    CustomComponentProbe topLevelComponent("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;

    stylesComponent.SetSurfaceContext(surfaceContext);
    stylesComponent.SetComponentId("tabsStylesNull");
    stylesComponent.SetSurfaceId("surface-id");
    stylesComponent.SetRenderId(205);

    auto stylesDescriptor = ParseJson(R"({"styles": null})");
    ASSERT_NE(stylesDescriptor, nullptr);
    stylesComponent.ApplyCustomProperties(stylesDescriptor->GetRoot());

    EXPECT_EQ(countExactWarningRequests("ERROR_CODE_INVALID_VALUE", "tabsStylesNull.styles"), 1U);
    EXPECT_EQ(countExactWarningRequests("ERROR_CODE_TYPE_MISMATCH", "tabsStylesNull.styles"), 0U);

    topLevelComponent.SetSurfaceContext(surfaceContext);
    topLevelComponent.SetComponentId("tabsTopLevelNull");
    topLevelComponent.SetSurfaceId("surface-id");
    topLevelComponent.SetRenderId(206);

    auto topLevelDescriptor = ParseJson(R"({
        "styles": {
            "width": null,
            "height": null,
            "constraintSize": null,
            "backgroundImage": null,
            "backgroundImageSizeWithStyle": null,
            "margin": null,
            "padding": null,
            "borderRadius": null,
            "borderWidth": null,
            "clip": null,
            "backgroundColor": null,
            "borderColor": null,
            "linearGradient": null,
            "layoutWeight": null,
            "flexShrink": null,
            "shadow": null,
            "visibility": null
        }
    })");
    ASSERT_NE(topLevelDescriptor, nullptr);
    topLevelComponent.ApplyCustomProperties(topLevelDescriptor->GetRoot());

    const std::set<std::string> topLevelPaths = { "width", "height", "constraintSize", "backgroundImage",
        "backgroundImageSizeWithStyle", "margin", "padding", "borderRadius", "borderWidth", "clip", "backgroundColor",
        "borderColor", "linearGradient", "layoutWeight", "flexShrink", "shadow", "visibility" };
    for (const auto& path : topLevelPaths) {
        const std::string fullPath = "tabsTopLevelNull.styles." + path;
        EXPECT_EQ(countExactWarningRequests("ERROR_CODE_INVALID_VALUE", fullPath), 1U) << fullPath;
        EXPECT_EQ(countExactWarningRequests("ERROR_CODE_TYPE_MISMATCH", fullPath), 0U) << fullPath;
    }

    CustomComponentProbe nestedComponent("Tabs");
    nestedComponent.SetSurfaceContext(surfaceContext);
    nestedComponent.SetComponentId("tabsNestedNull");
    nestedComponent.SetSurfaceId("surface-id");
    nestedComponent.SetRenderId(207);

    auto nestedDescriptor = ParseJson(R"({
        "styles": {
            "constraintSize": { "minWidth": null, "maxWidth": 100 },
            "backgroundImageSizeWithStyle": { "width": null, "height": "60vp" },
            "margin": { "top": null, "right": 8 },
            "padding": { "bottom": null, "left": "4vp" },
            "borderRadius": { "topLeft": null, "topRight": 8 },
            "borderWidth": { "top": null, "right": 1 },
            "borderColor": { "left": null, "right": "#FF0000" },
            "linearGradient": {
                "colors": null,
                "angle": null,
                "direction": null,
                "repeating": null,
                "stops": null
            },
            "shadow": {
                "style": null,
                "radius": null,
                "offsetX": null,
                "offsetY": null,
                "color": null,
                "type": null,
                "fill": null
            }
        }
    })");
    ASSERT_NE(nestedDescriptor, nullptr);
    nestedComponent.ApplyCustomProperties(nestedDescriptor->GetRoot());

    const std::set<std::string> nestedPaths = { "constraintSize.minWidth", "backgroundImageSizeWithStyle.width",
        "margin.top", "padding.bottom", "borderRadius.topLeft", "borderWidth.top", "borderColor.left",
        "linearGradient.colors", "linearGradient.angle", "linearGradient.direction", "linearGradient.repeating",
        "linearGradient.stops", "shadow.style", "shadow.radius", "shadow.offsetX", "shadow.offsetY", "shadow.color",
        "shadow.type", "shadow.fill" };
    for (const auto& path : nestedPaths) {
        const std::string fullPath = "tabsNestedNull.styles." + path;
        EXPECT_EQ(countExactWarningRequests("ERROR_CODE_INVALID_VALUE", fullPath), 1U) << fullPath;
        EXPECT_EQ(countExactWarningRequests("ERROR_CODE_TYPE_MISMATCH", fullPath), 0U) << fullPath;
    }
}

TEST_F(CustomComponentTest, should_expose_parent_flex_shrink_default_for_extended_tabs)
{
    struct ParentDefaultCase {
        std::string parentType;
        double expectedDefault = 0.0;
    };
    const std::vector<ParentDefaultCase> cases = { { "Column", 0.0 }, { "Row", 0.0 }, { "Flex", 1.0 } };
    for (const auto& testCase : cases) {
        SCOPED_TRACE(testCase.parentType);
        CustomComponentProbe component("Tabs");
        SurfaceContext surfaceContext;
        surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
        component.SetSurfaceContext(surfaceContext);
        auto descriptor = ParseJson(R"({"styles":{"flexShrink":-1}})");
        ASSERT_NE(descriptor, nullptr);
        component.ApplyCustomProperties(descriptor->GetRoot());

        auto parent = std::make_shared<TypedChildComponent>(testCase.parentType);
        component.SetParent(parent);
        component.CallOnAttachToParent();

        EXPECT_TRUE(component.descriptor_.properties.resetFlexShrinkToParentDefault);
        EXPECT_TRUE(component.descriptor_.properties.hasFlexShrinkParentDefault);
        EXPECT_DOUBLE_EQ(component.descriptor_.properties.flexShrinkParentDefault, testCase.expectedDefault);
        auto styles = component.GetProperty("styles");
        ASSERT_TRUE(styles.has_value());
        EXPECT_FALSE(styles->Has("flexShrink"));
    }
}

TEST_F(CustomComponentTest, should_only_expose_parent_default_for_dynamic_or_invalid_flex_shrink)
{
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    auto parent = std::make_shared<TypedChildComponent>("Column");

    CustomComponentProbe dynamicComponent("Tabs");
    dynamicComponent.SetSurfaceContext(surfaceContext);
    auto dynamicDescriptor = ParseJson(R"({"styles":{"flexShrink":"{{ $__dataModel.value }}"}})");
    ASSERT_NE(dynamicDescriptor, nullptr);
    dynamicComponent.ApplyCustomProperties(dynamicDescriptor->GetRoot());
    dynamicComponent.SetParent(parent);
    dynamicComponent.CallOnAttachToParent();
    EXPECT_FALSE(dynamicComponent.descriptor_.properties.resetFlexShrinkToParentDefault);
    EXPECT_TRUE(dynamicComponent.descriptor_.properties.hasFlexShrinkParentDefault);
    EXPECT_DOUBLE_EQ(dynamicComponent.descriptor_.properties.flexShrinkParentDefault, 0.0);

    CustomComponentProbe validComponent("Tabs");
    validComponent.SetSurfaceContext(surfaceContext);
    auto validDescriptor = ParseJson(R"({"styles":{"flexShrink":0.5}})");
    ASSERT_NE(validDescriptor, nullptr);
    validComponent.ApplyCustomProperties(validDescriptor->GetRoot());
    validComponent.SetParent(parent);
    validComponent.CallOnAttachToParent();
    EXPECT_FALSE(validComponent.descriptor_.properties.resetFlexShrinkToParentDefault);
    EXPECT_FALSE(validComponent.descriptor_.properties.hasFlexShrinkParentDefault);
}

TEST_F(CustomComponentTest, should_preserve_expression_strings_in_extended_common_styles)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    const std::vector<std::string> componentTypes = { "Row", "Tabs", "TabContent", "Web" };
    for (const auto& componentType : componentTypes) {
        SCOPED_TRACE(componentType);
        CustomComponentProbe component(componentType);
        SurfaceContext surfaceContext;
        surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
        component.SetSurfaceContext(surfaceContext);
        component.SetComponentId(componentType + "Expr");
        component.SetSurfaceId("surface-id");
        component.SetRenderId(206);

        std::unique_ptr<JsonAdapter> descriptor = ParseJson(
            R"({
                "styles": {
                    "width": "{{ $__dataModel.width }}",
                    "height": "{{ $__dataModel.height }}",
                    "constraintSize": "{{ $__dataModel.constraintSize }}",
                    "backgroundImage": "{{ $__dataModel.backgroundImage }}",
                    "backgroundImageSizeWithStyle": {
                        "width": "{{ $__dataModel.imageWidth }}",
                        "height": "60vp"
                    },
                    "margin": "{{ $__dataModel.margin }}",
                    "borderRadius": {
                        "topLeft": "{{ $__dataModel.radius }}",
                        "topRight": 8
                    },
                    "visibility": "{{ $__dataModel.visibility }}",
                    "clip": "{{ $__dataModel.clip }}",
                    "backgroundColor": "{{ $__dataModel.backgroundColor }}",
                    "borderWidth": {
                        "top": "{{ $__dataModel.borderWidth }}"
                    },
                    "borderColor": {
                        "left": "{{ $__dataModel.borderColor }}"
                    },
                    "padding": {
                        "bottom": "{{ $__dataModel.padding }}"
                    },
                    "layoutWeight": "{{ $__dataModel.layoutWeight }}",
                    "flexShrink": "{{ $__dataModel.flexShrink }}",
                    "shadow": {
                        "style": "{{ $__dataModel.shadowStyle }}",
                        "radius": "{{ $__dataModel.shadowRadius }}",
                        "type": "{{ $__dataModel.shadowType }}"
                    },
                    "linearGradient": {
                        "colors": "{{ $__dataModel.gradientColors }}",
                        "angle": "{{ $__dataModel.gradientAngle }}",
                        "direction": "{{ $__dataModel.gradientDirection }}",
                        "stops": "{{ $__dataModel.gradientStops }}",
                        "repeating": "{{ $__dataModel.gradientRepeating }}"
                    }
                }
            })");
        ASSERT_NE(descriptor, nullptr);

        component.ApplyCustomProperties(descriptor->GetRoot());

        auto stylesOpt = component.GetProperty("styles");
        ASSERT_TRUE(stylesOpt.has_value());
        JsonValue styles = stylesOpt.value();
        ASSERT_TRUE(styles.IsObject());
        EXPECT_EQ(styles.GetItem("width").GetStringValue(""), "{{ $__dataModel.width }}");
        EXPECT_EQ(styles.GetItem("constraintSize").GetStringValue(""), "{{ $__dataModel.constraintSize }}");
        EXPECT_EQ(styles.GetItem("margin").GetStringValue(""), "{{ $__dataModel.margin }}");
        EXPECT_EQ(styles.GetItem("layoutWeight").GetStringValue(""), "{{ $__dataModel.layoutWeight }}");
        EXPECT_EQ(styles.GetItem("flexShrink").GetStringValue(""), "{{ $__dataModel.flexShrink }}");
        EXPECT_EQ(styles.GetItem("backgroundImageSizeWithStyle").GetItem("width").GetStringValue(""),
            "{{ $__dataModel.imageWidth }}");
        EXPECT_EQ(styles.GetItem("borderRadius").GetItem("topLeft").GetStringValue(""), "{{ $__dataModel.radius }}");
        EXPECT_EQ(styles.GetItem("shadow").GetItem("style").GetStringValue(""), "{{ $__dataModel.shadowStyle }}");
        EXPECT_EQ(
            styles.GetItem("linearGradient").GetItem("colors").GetStringValue(""), "{{ $__dataModel.gradientColors }}");
    }

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(CustomComponentTest, should_restore_invalid_background_image_size_to_auto)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsBackgroundSize");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(206);

    std::unique_ptr<JsonAdapter> descriptor =
        ParseJson(R"({"styles":{"backgroundImageSizeWithStyle":"invalid-size"}})");
    ASSERT_NE(descriptor, nullptr);
    component.ApplyCustomProperties(descriptor->GetRoot());

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.Has("backgroundImageSizeWithStyle"));
    EXPECT_EQ(styles.GetItem("backgroundImageSizeWithStyle").GetStringValue(""), "auto");
}

TEST_F(CustomComponentTest, should_keep_valid_extended_tabs_private_properties_without_schema_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsProps");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(206);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "barPosition": "start",
            "vertical": true,
            "scrollable": false,
            "tabIndex": 2
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    ASSERT_TRUE(component.GetProperty("barPosition").has_value());
    ASSERT_TRUE(component.GetProperty("vertical").has_value());
    ASSERT_TRUE(component.GetProperty("scrollable").has_value());
    ASSERT_TRUE(component.GetProperty("tabIndex").has_value());
    EXPECT_EQ(component.GetProperty("barPosition").value().GetStringValue(""), "start");
    EXPECT_TRUE(component.GetProperty("vertical").value().GetBoolValue(false));
    EXPECT_FALSE(component.GetProperty("scrollable").value().GetBoolValue(true));
    EXPECT_DOUBLE_EQ(component.GetProperty("tabIndex").value().GetNumberValue(0.0), 2.0);

    std::unique_ptr<JsonAdapter> dynamicVerticalAdapter = ParseJson(R"({"path":"/tabs/vertical"})");
    std::unique_ptr<JsonAdapter> dynamicTabIndexAdapter = ParseJson(R"({"path":"/tabs/index"})");
    std::unique_ptr<JsonAdapter> functionCallVerticalAdapter = ParseJson(R"({"call":"myFunction"})");
    ASSERT_NE(dynamicVerticalAdapter, nullptr);
    ASSERT_NE(dynamicTabIndexAdapter, nullptr);
    ASSERT_NE(functionCallVerticalAdapter, nullptr);

    JsonValue expressionBarPosition = CreateStringValue("{{ $__dataModel.barPosition }}");
    JsonValue dynamicVertical = dynamicVerticalAdapter->GetRoot();
    JsonValue expressionScrollable = CreateStringValue("{{ $__dataModel.scrollable }}");
    JsonValue dynamicTabIndex = dynamicTabIndexAdapter->GetRoot();
    JsonValue functionCallVertical = functionCallVerticalAdapter->GetRoot();

    component.NormalizeCustomProperty("barPosition", expressionBarPosition);
    component.NormalizeCustomProperty("vertical", dynamicVertical);
    component.NormalizeCustomProperty("scrollable", expressionScrollable);
    component.NormalizeCustomProperty("tabIndex", dynamicTabIndex);
    component.NormalizeCustomProperty("vertical", functionCallVertical);

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    EXPECT_TRUE(expressionBarPosition.IsString());
    EXPECT_TRUE(dynamicVertical.IsObject());
    EXPECT_TRUE(expressionScrollable.IsString());
    EXPECT_TRUE(dynamicTabIndex.IsObject());
    ASSERT_TRUE(functionCallVertical.IsObject());
    EXPECT_EQ(functionCallVertical.GetString("call", ""), "myFunction");
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_invalid_extended_tabs_private_properties)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsPropsInvalid");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(207);

    JsonValue invalidBarPosition = CreateStringValue("left");
    JsonValue barPositionTypeMismatch = CreateNumberValue(1.0);
    JsonValue invalidVertical = CreateStringValue("maybe");
    JsonValue verticalStringTrue = CreateStringValue("true");
    JsonValue verticalStringFalse = CreateStringValue("false");
    JsonValue scrollableStringTrue = CreateStringValue("true");
    JsonValue scrollableStringFalse = CreateStringValue("false");
    std::unique_ptr<JsonAdapter> scrollableTypeMismatchAdapter = ParseJson(R"({"value":true})");
    ASSERT_NE(scrollableTypeMismatchAdapter, nullptr);
    JsonValue scrollableTypeMismatch = scrollableTypeMismatchAdapter->GetRoot();
    JsonValue invalidTabIndex = CreateStringValue("abc");

    component.NormalizeCustomProperty("barPosition", invalidBarPosition);
    component.NormalizeCustomProperty("barPosition", barPositionTypeMismatch);
    component.NormalizeCustomProperty("vertical", invalidVertical);
    component.NormalizeCustomProperty("vertical", verticalStringTrue);
    component.NormalizeCustomProperty("vertical", verticalStringFalse);
    component.NormalizeCustomProperty("scrollable", scrollableStringTrue);
    component.NormalizeCustomProperty("scrollable", scrollableStringFalse);
    component.NormalizeCustomProperty("scrollable", scrollableTypeMismatch);
    component.NormalizeCustomProperty("tabIndex", invalidTabIndex);

    EXPECT_FALSE(invalidBarPosition.IsValid());
    EXPECT_FALSE(barPositionTypeMismatch.IsValid());
    EXPECT_FALSE(invalidVertical.IsValid());
    EXPECT_FALSE(verticalStringTrue.IsValid());
    EXPECT_FALSE(verticalStringFalse.IsValid());
    EXPECT_FALSE(scrollableStringTrue.IsValid());
    EXPECT_FALSE(scrollableStringFalse.IsValid());
    EXPECT_FALSE(scrollableTypeMismatch.IsValid());
    EXPECT_FALSE(invalidTabIndex.IsValid());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsPropsInvalid.barPosition"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsPropsInvalid.barPosition"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsPropsInvalid.vertical"), 3U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsPropsInvalid.scrollable"), 3U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsPropsInvalid.tabIndex"), 1U);
}

TEST_F(CustomComponentTest, should_reject_negative_or_fractional_extended_tabs_index_literals)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsIndexRange");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(209);

    JsonValue negativeTabIndex = CreateNumberValue(-1.0);
    JsonValue fractionalTabIndex = CreateNumberValue(1.5);
    component.NormalizeCustomProperty("tabIndex", negativeTabIndex);
    component.NormalizeCustomProperty("tabIndex", fractionalTabIndex);

    EXPECT_FALSE(negativeTabIndex.IsValid());
    EXPECT_FALSE(fractionalTabIndex.IsValid());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsIndexRange.tabIndex"), 2U);
}

TEST_F(CustomComponentTest, should_reject_extended_tabs_linear_gradient_without_required_colors)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabsMissingGradientColors");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(206);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "backgroundColor": "#DBEAFE",
                "linearGradient": {}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabsMissingGradientColors.styles.linearGradient"),
        1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("backgroundColor"));
    EXPECT_FALSE(styles.Has("linearGradient"));
}

TEST_F(CustomComponentTest, should_dispatch_schema_warnings_for_invalid_extended_tab_content_custom_properties)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabHome");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(207);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "title": 123,
            "icon": 456,
            "selectedSrc": true,
            "tabType": "card",
            "styles": {
                "selectedColor": ""
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabHome.title"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabHome.icon"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabHome.selectedSrc"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "tabHome.tabType"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabHome.styles.selectedColor"), 1U);

    EXPECT_FALSE(component.GetProperty("title").has_value());
    EXPECT_FALSE(component.GetProperty("icon").has_value());
    EXPECT_FALSE(component.GetProperty("selectedSrc").has_value());
    EXPECT_FALSE(component.GetProperty("tabType").has_value());
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    EXPECT_FALSE(stylesOpt.value().Has("selectedColor"));
}

TEST_F(CustomComponentTest, should_dispatch_schema_warning_for_invalid_custom_component_accessibility_value)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Tabs");
    component.SetComponentId("tabsMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(207);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "accessibility": 123
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCommonAttributes(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabsMain.accessibility"), 1U);
    EXPECT_FALSE(component.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_FALSE(component.descriptor_.properties.hasAccessibilityDescription);
}

TEST_F(CustomComponentTest, should_dispatch_schema_warning_for_unknown_custom_component_accessibility_field)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Web");
    component.SetComponentId("webMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(208);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "accessibility": {
                "role": "button"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ValidateComponentDescriptorSchema(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "webMain.accessibility.role"), 1U);
}

TEST_F(CustomComponentTest, should_cover_custom_schema_warning_guards_and_accessibility_known_fields)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Tabs");
    component.SetComponentId("tabsGuard");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(0);

    component.ReportCustomSchemaWarning("ERROR_CODE_TYPE_MISMATCH", "ignored because path empty", "");
    component.ReportCustomSchemaWarning("", "ignored because code empty", "styles.width");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0);

    JsonValue invalidDescriptor;
    component.ValidateComponentDescriptorSchema(invalidDescriptor);
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "accessibility": {
                "label": "ok",
                "description": "ok",
                "role": "button"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ValidateComponentDescriptorSchema(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "tabsGuard.accessibility.role"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "tabsGuard.accessibility.label"), 0U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_UNDEFINED_FIELD", "tabsGuard.accessibility.description"), 0U);
}

TEST_F(CustomComponentTest, should_keep_valid_extended_row_common_styles_without_schema_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Extended.Row");
    component.SetComponentId("rowMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(301);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "width": {"path": "/styles/width"},
                "height": "24vp",
                "constraintSize": {
                    "minWidth": "12vp",
                    "maxWidth": "80vp",
                    "minHeight": "20vp",
                    "maxHeight": "120vp"
                },
                "backgroundImageSize": {"path": "/styles/backgroundImageSize"},
                "margin": {"top": "8vp", "bottom": "8vp"},
                "padding": {"left": "4vp", "right": "4vp"},
                "borderRadius": "8vp",
                "borderWidth": "1vp",
                "clip": true,
                "backgroundColor": "transparent",
                "borderColor": {
                    "top": "transparent",
                    "left": {"path": "/styles/borderLeft"}
                },
                "linearGradient": {
                    "colors": ["#FF0000", "#00FF00"],
                    "angle": 90,
                    "direction": "topRight",
                    "repeating": false,
                    "stops": [0, 1]
                },
                "layoutWeight": "2",
                "flexShrink": 2.6,
                "shadow": {
                    "style": 2,
                    "radius": 8,
                    "offsetX": {"path": "/styles/shadowOffsetX"},
                    "offsetY": 3,
                    "color": "transparent",
                    "type": 1,
                    "fill": "false"
                },
                "visibility": "visible"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("width"));
    EXPECT_TRUE(styles.Has("height"));
    EXPECT_TRUE(styles.Has("constraintSize"));
    EXPECT_TRUE(styles.Has("backgroundImageSize"));
    EXPECT_TRUE(styles.Has("linearGradient"));
    ASSERT_TRUE(styles.Has("flexShrink"));
    EXPECT_DOUBLE_EQ(styles.GetItem("flexShrink").GetNumberValue(0.0), 2.6);
    EXPECT_TRUE(styles.Has("shadow"));
    EXPECT_TRUE(styles.Has("visibility"));
}

TEST_F(CustomComponentTest, should_keep_empty_edge_and_radius_objects_while_rejecting_negative_layout_weight)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Extended.Row");
    component.SetComponentId("rowReset");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(306);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "margin": {},
                "padding": {},
                "borderRadius": {},
                "layoutWeight": -1
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "rowReset.styles.layoutWeight"), 1U);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("margin"));
    EXPECT_TRUE(styles.Has("padding"));
    EXPECT_TRUE(styles.Has("borderRadius"));
    EXPECT_FALSE(styles.Has("layoutWeight"));
}

TEST_F(CustomComponentTest, should_keep_zero_layout_weight_in_extended_common_styles_without_schema_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Extended.Row");
    component.SetComponentId("rowZeroWeight");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(307);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "layoutWeight": 0
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    ASSERT_TRUE(styles.Has("layoutWeight"));
    EXPECT_DOUBLE_EQ(styles.GetItem("layoutWeight").GetNumberValue(-1.0), 0.0);
}

TEST_F(CustomComponentTest, should_dispatch_remaining_invalid_extended_common_style_warnings)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webEdge");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(302);

    std::unique_ptr<JsonAdapter> firstDescriptor = ParseJson(
        R"({
            "styles": {
                "constraintSize": 1,
                "backgroundImage": "bad-image",
                "backgroundImageSizeWithStyle": {"width": ""},
                "backgroundImageSize": "bad-size",
                "margin": true,
                "padding": false,
                "borderRadius": {"all": ""},
                "borderWidth": false,
                "clip": {},
                "borderColor": false,
                "linearGradient": {
                    "angle": {},
                    "stops": true
                },
                "layoutWeight": "",
                "flexShrink": {},
                "shadow": true
            }
        })");
    ASSERT_NE(firstDescriptor, nullptr);
    component.ApplyCustomProperties(firstDescriptor->GetRoot());

    std::unique_ptr<JsonAdapter> secondDescriptor = ParseJson(
        R"({
            "styles": {
                "borderRadius": false,
                "borderColor": "bad-color",
                "linearGradient": false,
                "shadow": {
                    "style": {}
                }
            }
        })");
    ASSERT_NE(secondDescriptor, nullptr);
    component.ApplyCustomProperties(secondDescriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.constraintSize"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.backgroundImage"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.backgroundImageSizeWithStyle"),
        1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.backgroundImageSize"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.margin"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.padding"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.borderRadius.all"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.borderRadius"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webEdge.styles.borderWidth"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webEdge.styles.clip"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webEdge.styles.borderColor"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.borderColor"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webEdge.styles.linearGradient.angle"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.linearGradient.stops"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.linearGradient"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.layoutWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webEdge.styles.flexShrink"), 1U);
    EXPECT_GE(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.shadow"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webEdge.styles.shadow.style"), 1U);
}

TEST_F(CustomComponentTest, should_keep_valid_shadow_and_gradient_fields_while_pruning_invalid_nested_style_entries)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webNested");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(305);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "shadow": {
                    "style": "outerDefaultXs",
                    "offsetY": "bad",
                    "color": [],
                    "type": {},
                    "fill": []
                },
                "margin": {
                    "all": ""
                },
                "linearGradient": {
                    "colors": ["#FF0000", "#00FF00"],
                    "stops": [0, 1],
                    "direction": 3
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webNested.styles.shadow.offsetY"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webNested.styles.shadow.color"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webNested.styles.shadow.type"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webNested.styles.shadow.fill"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webNested.styles.margin.all"), 1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());

    ASSERT_TRUE(styles.Has("shadow"));
    JsonValue shadow = styles.GetItem("shadow");
    EXPECT_EQ(shadow.GetString("style", ""), "outerDefaultXs");
    EXPECT_FALSE(shadow.Has("offsetY"));
    EXPECT_FALSE(shadow.Has("color"));
    EXPECT_FALSE(shadow.Has("type"));
    EXPECT_FALSE(shadow.Has("fill"));

    ASSERT_TRUE(styles.Has("linearGradient"));
    EXPECT_DOUBLE_EQ(styles.GetItem("linearGradient").GetNumber("direction", -1.0), 3.0);
    ASSERT_TRUE(styles.Has("margin"));
    EXPECT_FALSE(styles.GetItem("margin").Has("all"));
}

TEST_F(CustomComponentTest, should_skip_dynamic_nested_style_validation_and_drop_object_gradient_direction)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webDynamicNested");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(306);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "margin": {
                    "all": { "path": "/styles/marginAll" }
                },
                "shadow": {
                    "fill": { "path": "/styles/shadowFill" }
                },
                "linearGradient": {
                    "colors": ["#FF0000", "#00FF00"],
                    "direction": {}
                }
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webDynamicNested.styles.margin"), 0U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webDynamicNested.styles.shadow.fill"), 0U);
    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webDynamicNested.styles.linearGradient.direction"),
        1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    ASSERT_TRUE(styles.Has("margin"));
    ASSERT_TRUE(styles.GetItem("margin").Has("all"));
    ASSERT_TRUE(styles.Has("shadow"));
    ASSERT_TRUE(styles.GetItem("shadow").Has("fill"));
    ASSERT_TRUE(styles.Has("linearGradient"));
    EXPECT_FALSE(styles.GetItem("linearGradient").Has("direction"));
}

TEST_F(CustomComponentTest, should_remove_non_object_tab_content_styles_and_reject_non_string_tab_type)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabEdge");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(303);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": 1,
            "tabType": {}
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabEdge.styles"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabEdge.tabType"), 1U);
    EXPECT_FALSE(component.GetProperty("styles").has_value());
    EXPECT_FALSE(component.GetProperty("tabType").has_value());

    JsonValue invalidValue;
    component.NormalizeCustomProperty("styles", invalidValue);
    EXPECT_FALSE(invalidValue.IsValid());
}

TEST_F(CustomComponentTest, should_keep_valid_tab_content_styles_and_include_catalog_id_in_attribute_value)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.SetComponentId("tabCatalog");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(304);
    component.env_ = reinterpret_cast<napi_env>(0x3040);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "fontSize": 14,
                "iconSize": 16,
                "space": 8,
                "selectedColor": "blue",
                "unSelectedColor": {"path": "/styles/unSelectedColor"},
                "defaultBackgroundColor": "white",
                "selectedBackgroundColor": "black",
                "defaultBorderColor": "gray",
                "selectedBorderColor": "green",
                "fontWeight": 700
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    EXPECT_TRUE(stylesOpt.value().Has("selectedColor"));
    EXPECT_TRUE(stylesOpt.value().Has("unSelectedColor"));
    EXPECT_TRUE(stylesOpt.value().Has("selectedBackgroundColor"));
    EXPECT_TRUE(stylesOpt.value().Has("selectedBorderColor"));
    EXPECT_TRUE(stylesOpt.value().Has("fontWeight"));

    napi_value attributeValue = component.CreateAttributeValue();
    ASSERT_NE(attributeValue, nullptr);
    auto objectIt = mockNapiPtr_->objectProperties_.find(attributeValue);
    ASSERT_NE(objectIt, mockNapiPtr_->objectProperties_.end());
    auto catalogIdIt = objectIt->second.find("catalogId");
    ASSERT_NE(catalogIdIt, objectIt->second.end());
    EXPECT_EQ(GetStringValue(mockNapiPtr_, catalogIdIt->second), A2UI_EXTENDED_CATALOG_ID);
}

TEST_F(CustomComponentTest, should_not_create_protocol_tab_content_styles_from_non_protocol_aliases)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabLegacyAliases");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(308);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "selectColor": "#FF0000",
                "unselectedColor": "#00FF00",
                "selectBackgroundColor": "#0000FF",
                "selectBorderColor": "#333333"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("selectColor"));
    EXPECT_TRUE(styles.Has("unselectedColor"));
    EXPECT_TRUE(styles.Has("selectBackgroundColor"));
    EXPECT_TRUE(styles.Has("selectBorderColor"));
    EXPECT_FALSE(styles.Has("selectedColor"));
    EXPECT_FALSE(styles.Has("unSelectedColor"));
    EXPECT_FALSE(styles.Has("selectedBackgroundColor"));
    EXPECT_FALSE(styles.Has("selectedBorderColor"));
}

TEST_F(CustomComponentTest, should_keep_expression_and_dynamic_extended_tab_content_values_without_warnings)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabExpr");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(307);

    std::unique_ptr<JsonAdapter> titleValue = ParseJson(R"({"text":"plain-object-title"})");
    std::unique_ptr<JsonAdapter> selectedSrcValue = ParseJson(R"({"resource":"selected.png"})");
    std::unique_ptr<JsonAdapter> dynamicTabTypeValue = ParseJson(R"({"path":"/tab/type"})");
    std::unique_ptr<JsonAdapter> styleValue = ParseJson(
        R"({
            "fontSize": "{{ $__dataModel.fontSize }}",
            "iconSize": { "path": "/styles/iconSize" },
            "space": "{{ size($__dataModel.items) }}",
            "fontWeight": { "path": "/styles/fontWeight" }
        })");
    ASSERT_NE(titleValue, nullptr);
    ASSERT_NE(selectedSrcValue, nullptr);
    ASSERT_NE(dynamicTabTypeValue, nullptr);
    ASSERT_NE(styleValue, nullptr);

    JsonValue titleRoot = titleValue->GetRoot();
    JsonValue selectedSrcRoot = selectedSrcValue->GetRoot();
    JsonValue tabTypeExpr = CreateStringValue("{{ $__dataModel.tabType }}");
    JsonValue dynamicTabTypeRoot = dynamicTabTypeValue->GetRoot();
    JsonValue styleRoot = styleValue->GetRoot();

    component.NormalizeCustomProperty("title", titleRoot);
    component.NormalizeCustomProperty("selectedSrc", selectedSrcRoot);
    component.NormalizeCustomProperty("tabType", tabTypeExpr);
    component.NormalizeCustomProperty("tabType", dynamicTabTypeRoot);
    component.NormalizeCustomProperty("styles", styleRoot);

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    EXPECT_TRUE(titleRoot.IsObject());
    EXPECT_TRUE(selectedSrcRoot.IsObject());
    EXPECT_TRUE(tabTypeExpr.IsString());
    EXPECT_TRUE(dynamicTabTypeRoot.IsObject());
    EXPECT_TRUE(styleRoot.IsObject());
    EXPECT_TRUE(styleRoot.Has("fontSize"));
    EXPECT_TRUE(styleRoot.Has("iconSize"));
    EXPECT_TRUE(styleRoot.Has("space"));
    EXPECT_TRUE(styleRoot.Has("fontWeight"));
}

TEST_F(CustomComponentTest, should_dispatch_font_weight_type_mismatch_for_object_value)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabWeight");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(305);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "styles": {
                "fontWeight": {}
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "tabWeight.styles.fontWeight"), 1U);
    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    EXPECT_FALSE(stylesOpt.value().Has("fontWeight"));
}

TEST_F(CustomComponentTest, should_drop_empty_extended_tab_content_child_without_storing_custom_prop)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabHome");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(207);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "child": ""
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    EXPECT_FALSE(component.GetProperty("child").has_value());
}

TEST_F(CustomComponentTest, should_drop_invalid_extended_tab_content_child_values_without_warning)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("TabContent");
    component.SetComponentId("tabChild");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(308);

    JsonValue numericChild = CreateNumberValue(7.0);
    JsonValue blankChild = CreateStringValue("   ");

    component.NormalizeCustomProperty("child", numericChild);
    component.NormalizeCustomProperty("child", blankChild);

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    EXPECT_FALSE(numericChild.IsValid());
    EXPECT_FALSE(blankChild.IsValid());
}

/**
 * @tc.name: 自定义组件子节点接收规则
 * @tc.desc: 覆盖Tabs/TabContent的accept规则、普通组件回退规则和空child分支。
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_accept_tabs_and_base_children_by_component_type)
{
    CustomComponent tabsComponent("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    tabsComponent.SetSurfaceContext(surfaceContext);
    auto exactTabContent =
        std::make_shared<TypedChildComponent>("TabContent", reinterpret_cast<ArkUI_NodeHandle>(0x111));
    auto shortTabContent =
        std::make_shared<TypedChildComponent>("TabContent", reinterpret_cast<ArkUI_NodeHandle>(0x112));
    auto otherChild = std::make_shared<TypedChildComponent>("Text", reinterpret_cast<ArkUI_NodeHandle>(0x113));

    EXPECT_FALSE(tabsComponent.AcceptsChild(nullptr));
    EXPECT_TRUE(tabsComponent.AcceptsChild(exactTabContent));
    EXPECT_TRUE(tabsComponent.AcceptsChild(shortTabContent));
    EXPECT_FALSE(tabsComponent.AcceptsChild(otherChild));

    CustomComponent plainComponent("Card");
    EXPECT_FALSE(plainComponent.AcceptsChild(nullptr));
    EXPECT_TRUE(plainComponent.AcceptsChild(otherChild));
}

TEST_F(CustomComponentTest, should_store_component_type_and_disable_unknown_descriptor_validation)
{
    CustomComponent component("CustomCard");
    EXPECT_EQ(component.GetType(), "CustomCard");
    EXPECT_FALSE(component.ShouldValidateUnknownDescriptorFields());
}

TEST_F(CustomComponentTest, should_collect_tabs_child_ids_from_tabs_descriptor)
{
    CustomComponent tabsComponent("Tabs");
    std::unique_ptr<JsonAdapter> tabsDescriptor = ParseJson(
        R"({
            "tabs": [
                {"child": "tab-a"},
                {"child": ""},
                {"child": "tab-b"},
                1
            ]
        })");
    ASSERT_NE(tabsDescriptor, nullptr);

    tabsComponent.CollectChildListDescriptor(tabsDescriptor->GetRoot());
    EXPECT_EQ(tabsComponent.childListDescriptor_.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(tabsComponent.childListDescriptor_.staticChildIds.size(), 2U);
    auto first = tabsComponent.childListDescriptor_.staticChildIds.begin();
    EXPECT_EQ(*first, "tab-a");
    ++first;
    EXPECT_EQ(*first, "tab-b");
}

TEST_F(CustomComponentTest, should_collect_extended_tabs_child_ids_from_children_descriptor)
{
    CustomComponent tabsComponent("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    tabsComponent.SetSurfaceContext(surfaceContext);
    std::unique_ptr<JsonAdapter> tabsDescriptor = ParseJson(
        R"({
            "children": ["tab-a", "tab-b"]
        })");
    ASSERT_NE(tabsDescriptor, nullptr);

    tabsComponent.CollectChildListDescriptor(tabsDescriptor->GetRoot());
    EXPECT_EQ(tabsComponent.childListDescriptor_.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(tabsComponent.childListDescriptor_.staticChildIds.size(), 2U);
    auto first = tabsComponent.childListDescriptor_.staticChildIds.begin();
    EXPECT_EQ(*first, "tab-a");
    ++first;
    EXPECT_EQ(*first, "tab-b");
}

TEST_F(CustomComponentTest, should_ignore_tabs_field_when_extended_tabs_children_is_missing)
{
    CustomComponent tabsComponent("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    tabsComponent.SetSurfaceContext(surfaceContext);
    std::unique_ptr<JsonAdapter> tabsDescriptor = ParseJson(
        R"({
            "tabs": [{"child": "tab-a"}]
        })");
    ASSERT_NE(tabsDescriptor, nullptr);

    tabsComponent.CollectChildListDescriptor(tabsDescriptor->GetRoot());
    EXPECT_EQ(tabsComponent.childListDescriptor_.type, ChildListType::INVALID);
    EXPECT_TRUE(tabsComponent.childListDescriptor_.staticChildIds.empty());
}

TEST_F(CustomComponentTest, should_collect_row_child_ids_from_children_descriptor)
{
    CustomComponent rowComponent("Extended.Row");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "children": ["item-a", "item-b"]
        })");
    ASSERT_NE(descriptor, nullptr);

    rowComponent.CollectChildListDescriptor(descriptor->GetRoot());
    EXPECT_EQ(rowComponent.childListDescriptor_.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(rowComponent.childListDescriptor_.staticChildIds.size(), 2U);
    auto first = rowComponent.childListDescriptor_.staticChildIds.begin();
    EXPECT_EQ(*first, "item-a");
    ++first;
    EXPECT_EQ(*first, "item-b");
}

TEST_F(CustomComponentTest, should_ignore_child_field_for_tab_content_child_list_descriptor)
{
    CustomComponent tabContentComponent("TabContent");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "child": "legacy-child",
            "children": ["tab-child"]
        })");
    ASSERT_NE(descriptor, nullptr);

    tabContentComponent.CollectChildListDescriptor(descriptor->GetRoot());
    EXPECT_EQ(tabContentComponent.childListDescriptor_.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(tabContentComponent.childListDescriptor_.staticChildIds.size(), 1U);
    EXPECT_EQ(tabContentComponent.childListDescriptor_.staticChildIds.front(), "tab-child");
}

TEST_F(CustomComponentTest, should_return_invalid_for_tab_content_when_only_child_field_exists)
{
    CustomComponent tabContentComponent("TabContent");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "child": "legacy-child"
        })");
    ASSERT_NE(descriptor, nullptr);

    tabContentComponent.CollectChildListDescriptor(descriptor->GetRoot());
    EXPECT_EQ(tabContentComponent.childListDescriptor_.type, ChildListType::INVALID);
    EXPECT_TRUE(tabContentComponent.childListDescriptor_.staticChildIds.empty());
}

TEST_F(CustomComponentTest, should_fallback_when_tabs_field_is_not_array)
{
    CustomComponent tabsComponent("Tabs");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "tabs": {
                "child": "tab-a"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    tabsComponent.CollectChildListDescriptor(descriptor->GetRoot());
    EXPECT_EQ(tabsComponent.childListDescriptor_.type, ChildListType::INVALID);
    EXPECT_TRUE(tabsComponent.childListDescriptor_.staticChildIds.empty());
}

TEST_F(CustomComponentTest, should_apply_common_attributes_and_accessibility_properties)
{
    CustomComponent component("Panel");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "width": 100,
            "height": 50,
            "paddingTop": 1,
            "paddingRight": 2,
            "paddingBottom": 3,
            "paddingLeft": 4,
            "marginTop": 5,
            "marginRight": 6,
            "marginBottom": 7,
            "marginLeft": 8,
            "weight": 9,
            "accessibility": {
                "label": "hello",
                "description": "world",
                "role": "button"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCommonAttributes(descriptor->GetRoot());
    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_TRUE(component.descriptor_.properties.hasHeight);
    EXPECT_TRUE(component.descriptor_.properties.hasWeight);
    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityDescription);
    EXPECT_EQ(component.descriptor_.properties.accessibilityLabel, "hello");
    EXPECT_EQ(component.descriptor_.properties.accessibilityDescription, "world");
    EXPECT_FALSE(component.descriptor_.properties.size.empty());
    EXPECT_FALSE(component.descriptor_.properties.padding.empty());
    EXPECT_FALSE(component.descriptor_.properties.margin.empty());

    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    EXPECT_DOUBLE_EQ(customProps.GetNumber("weight", 0.0), 9.0);
    JsonValue accessibility = customProps.GetItem("accessibility");
    ASSERT_TRUE(accessibility.IsObject());
    EXPECT_EQ(accessibility.GetString("label", ""), "hello");
    EXPECT_EQ(accessibility.GetString("description", ""), "world");
    EXPECT_EQ(accessibility.GetString("role", ""), "button");
}

TEST_F(CustomComponentTest, should_warn_and_reset_accessibility_description_when_value_is_not_string)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("panelMain");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(205);

    std::unique_ptr<JsonAdapter> preset = ParseJson(R"({"accessibility":{"description":"preset"}})");
    ASSERT_NE(preset, nullptr);
    component.ApplyCommonAttributes(preset->GetRoot());
    ASSERT_TRUE(component.descriptor_.properties.hasAccessibilityDescription);
    ASSERT_EQ(component.descriptor_.properties.accessibilityDescription, "preset");

    std::unique_ptr<JsonAdapter> invalid = ParseJson(R"({"accessibility":{"description":123}})");
    ASSERT_NE(invalid, nullptr);
    component.ApplyCommonAttributes(invalid->GetRoot());

    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "panelMain.accessibility.description"), 1U);
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.empty());
    ASSERT_FALSE(mockNapiPtr_->callFunctionArgsHistory_.back().empty());
    napi_value request = mockNapiPtr_->callFunctionArgsHistory_.back().front();
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, request, "message")),
        "Property accessibility.description expects string value, got type 'number', value has been coerced to "
        "string");
    EXPECT_FALSE(component.descriptor_.properties.hasAccessibilityDescription);
    EXPECT_TRUE(component.descriptor_.properties.accessibilityDescription.empty());
}

TEST_F(CustomComponentTest, should_clear_custom_properties_when_descriptor_is_not_object)
{
    CustomComponent component("Panel");
    component.customPropertyNames_.insert("foo");
    component.properties_["foo"] = CreateStringValue("bar");
    ASSERT_FALSE(component.customPropertyNames_.empty());
    ASSERT_FALSE(component.properties_.empty());

    component.ApplyCustomProperties(JsonValue());

    EXPECT_TRUE(component.customPropertyNames_.empty());
    EXPECT_TRUE(component.properties_.empty());
}

TEST_F(CustomComponentTest, should_store_and_read_runtime_custom_property)
{
    CustomComponent component("Panel");
    JsonValue value = CreateStringValue("runtime-value");

    EXPECT_FALSE(component.SetRuntimeCustomProperty("", value));
    EXPECT_FALSE(component.SetRuntimeCustomProperty("runtime", JsonValue()));
    ASSERT_TRUE(component.SetRuntimeCustomProperty("runtime", value));

    JsonValue storedValue = component.GetCustomProperty("runtime");
    ASSERT_TRUE(storedValue.IsValid());
    EXPECT_EQ(storedValue.GetStringValue(""), "runtime-value");
    EXPECT_TRUE(component.customPropertyNames_.find("runtime") != component.customPropertyNames_.end());
}

TEST_F(CustomComponentTest, should_apply_custom_properties_and_dynamic_path_binding)
{
    CustomComponent component("Divider");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "id": "x",
            "weight": 1,
            "foo": 2,
            "axis": {"path": "/layout/axis"},
            "children": ["ignore"]
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_TRUE(component.customPropertyNames_.find("foo") != component.customPropertyNames_.end());
    EXPECT_TRUE(component.customPropertyNames_.find("axis") != component.customPropertyNames_.end());
    EXPECT_TRUE(component.properties_.find("foo") != component.properties_.end());
    ASSERT_EQ(component.GetDataBindings().size(), 1U);
    EXPECT_EQ(component.GetDataBindings()[0].propertyName_, "axis");
    EXPECT_EQ(component.GetDataBindings()[0].dataPath_, "/layout/axis");
}

TEST_F(CustomComponentTest, should_register_choice_picker_value_binding_from_object_or_single_item_array)
{
    CustomComponent component("ChoicePicker");

    std::unique_ptr<JsonAdapter> objectPath = ParseJson(R"({"value":{"path":"/form/value1"}})");
    ASSERT_NE(objectPath, nullptr);
    component.RegisterDataBindings(objectPath->GetRoot());
    ASSERT_EQ(component.GetDataBindings().size(), 1U);
    EXPECT_EQ(component.GetDataBindings()[0].propertyName_, "value");
    EXPECT_EQ(component.GetDataBindings()[0].dataPath_, "/form/value1");

    std::unique_ptr<JsonAdapter> arrayPath = ParseJson(R"({"value":[{"path":"/form/value2"}]})");
    ASSERT_NE(arrayPath, nullptr);
    component.RegisterDataBindings(arrayPath->GetRoot());
    ASSERT_EQ(component.GetDataBindings().size(), 1U);
    EXPECT_EQ(component.GetDataBindings()[0].dataPath_, "/form/value2");

    std::unique_ptr<JsonAdapter> invalidValue = ParseJson(R"({"value":[{"path":""}]})");
    ASSERT_NE(invalidValue, nullptr);
    component.RegisterDataBindings(invalidValue->GetRoot());
    EXPECT_TRUE(component.GetDataBindings().empty());
}

TEST_F(CustomComponentTest, should_refresh_tabs_bindings_and_remove_legacy_tabs_bindings)
{
    CustomComponent component("Tabs");
    component.AddBinding("tabs[0].title", "/old/title");
    component.AddBinding("other", "/keep");

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "tabs": [
                {"title": {"path": "/new/title"}},
                {"title": "static"}
            ]
        })");
    ASSERT_NE(descriptor, nullptr);

    component.RegisterDataBindings(descriptor->GetRoot());

    ASSERT_EQ(component.GetDataBindings().size(), 2U);
    EXPECT_EQ(component.GetDataBindings()[0].propertyName_, "other");
    EXPECT_EQ(component.GetDataBindings()[0].dataPath_, "/keep");
    EXPECT_EQ(component.GetDataBindings()[1].propertyName_, "tabs[0].title");
    EXPECT_EQ(component.GetDataBindings()[1].dataPath_, "/new/title");
}

TEST_F(CustomComponentTest, should_parse_tabs_mapping_only_for_tabs_component)
{
    CustomComponent tabsComponent("Tabs");
    std::unique_ptr<JsonAdapter> tabsDescriptor = ParseJson(
        R"({
            "tabs": [
                {"child": "first"},
                {"child": "second"},
                {"child": ""}
            ]
        })");
    ASSERT_NE(tabsDescriptor, nullptr);
    tabsComponent.ParseTabsMapping(tabsDescriptor->GetRoot());

    ASSERT_EQ(tabsComponent.childToSlotMapping_.size(), 2U);
    EXPECT_EQ(tabsComponent.childToSlotMapping_["first"], "tab-0");
    EXPECT_EQ(tabsComponent.childToSlotMapping_["second"], "tab-1");

    CustomComponent normalComponent("Card");
    normalComponent.ParseTabsMapping(tabsDescriptor->GetRoot());
    EXPECT_TRUE(normalComponent.childToSlotMapping_.empty());
}

TEST_F(CustomComponentTest, should_ignore_tabs_field_for_extended_tabs_mapping)
{
    CustomComponent tabsComponent("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    tabsComponent.SetSurfaceContext(surfaceContext);
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "tabs": [
                {"child": "tab-a"}
            ]
        })");
    ASSERT_NE(descriptor, nullptr);

    tabsComponent.ParseTabsMapping(descriptor->GetRoot());
    EXPECT_TRUE(tabsComponent.childToSlotMapping_.empty());
    EXPECT_TRUE(tabsComponent.tabChildIds_.empty());
}

TEST_F(CustomComponentTest, should_build_custom_props_with_tabs_overrides_other_props_and_bindings)
{
    CustomComponent component("Tabs");
    component.customPropertyNames_ = { "tabs", "tabs[1].title", "other" };

    std::unique_ptr<JsonAdapter> rawTabs = ParseJson(R"([{"title":"raw-title"}])");
    ASSERT_NE(rawTabs, nullptr);
    component.properties_["tabs"] = rawTabs->GetRoot();
    component.properties_["tabs[1].title"] = CreateStringValue("patched-title");
    component.properties_["other"] = CreateNumberValue(3);
    component.AddBinding("other", "/props/other");

    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());

    JsonValue tabs = customProps.GetItem("tabs");
    ASSERT_TRUE(tabs.IsArray());
    EXPECT_GE(tabs.GetArraySize(), 2);
    EXPECT_EQ(tabs.GetArrayItem(1).GetString("title", ""), "patched-title");

    EXPECT_EQ(customProps.GetNumber("other", 0.0), 3.0);
    JsonValue bindings = customProps.GetItem("__a2uiBindings");
    ASSERT_TRUE(bindings.IsObject());
    EXPECT_EQ(bindings.GetString("other", ""), "/props/other");
}

TEST_F(CustomComponentTest, should_skip_children_custom_prop_for_extended_tabs_and_resolve_tabs_children_from_children)
{
    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    component.customPropertyNames_ = { "children", "headline" };
    component.tabChildIds_ = { "tab-a", "tab-b" };
    component.properties_["children"] = CreateStringValue("legacy-child");
    component.properties_["headline"] = CreateStringValue("keep-me");

    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    ASSERT_TRUE(customProps.Has("children"));
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsArray());
    ASSERT_EQ(children.GetArraySize(), 2);
    EXPECT_EQ(children.GetArrayItem(0).GetStringValue(""), "tab-a");
    EXPECT_EQ(children.GetArrayItem(1).GetStringValue(""), "tab-b");
    EXPECT_EQ(customProps.GetString("headline", ""), "keep-me");

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "children": ["tab-a", "tab-b"]
        })");
    ASSERT_NE(descriptor, nullptr);

    std::list<std::string> childIds = component.ResolveTabsChildIds(descriptor->GetRoot());
    ASSERT_EQ(childIds.size(), 2U);
    auto iter = childIds.begin();
    EXPECT_EQ(*iter, "tab-a");
    ++iter;
    EXPECT_EQ(*iter, "tab-b");
}

TEST_F(CustomComponentTest, should_preserve_explicit_empty_children_for_extended_tabs_custom_props)
{
    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(R"({"children":[]})");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyPrivateAttributes(descriptor->GetRoot());
    JsonValue customProps = component.descriptor_.customProps;

    ASSERT_TRUE(customProps.IsObject());
    ASSERT_TRUE(customProps.Has("children"));
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsArray());
    EXPECT_EQ(children.GetArraySize(), 0);
    EXPECT_TRUE(component.tabChildIds_.empty());
}

TEST_F(CustomComponentTest, should_preserve_template_children_for_extended_tabs_when_no_instances_are_resolved)
{
    CustomComponentProbe component("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    component.SetSurfaceContext(surfaceContext);
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "children": {
                "componentId": "tabTemplate",
                "path": "/emptyTabs",
                "indexVar": "tabIndex",
                "itemVar": "tabItem"
            }
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyPrivateAttributes(descriptor->GetRoot());
    JsonValue customProps = component.descriptor_.customProps;

    ASSERT_TRUE(customProps.IsObject());
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsObject());
    EXPECT_EQ(children.GetString("componentId", ""), "tabTemplate");
    EXPECT_EQ(children.GetString("path", ""), "/emptyTabs");
    EXPECT_EQ(children.GetString("indexVar", ""), "tabIndex");
    EXPECT_EQ(children.GetString("itemVar", ""), "tabItem");
    EXPECT_TRUE(component.tabChildIds_.empty());
}

TEST_F(CustomComponentTest, should_not_copy_event_handler_arrays_into_custom_props)
{
    CustomComponent component("Select");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "options": [{"value": "Alpha"}, {"value": "Beta"}],
            "value": "请选择",
            "onSelect": [{
                "call": "setDataModel",
                "args": {
                    "path": "/result",
                    "value": "{{ $context.componentId + ':' + $context.eventData.value }}"
                }
            }]
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyPrivateAttributes(descriptor->GetRoot());

    ASSERT_TRUE(component.descriptor_.customProps.IsObject());
    EXPECT_FALSE(component.descriptor_.customProps.Has("onSelect"));
    EXPECT_TRUE(component.eventHandlers_.find("onSelect") != component.eventHandlers_.end());
    EXPECT_TRUE(component.descriptor_.customProps.Has("options"));
    EXPECT_TRUE(component.descriptor_.customProps.Has("value"));
}

TEST_F(CustomComponentTest, should_update_accessibility_state_on_data_update_and_property_removed)
{
    CustomComponent component("Panel");

    component.OnDataUpdate("accessibility.label", CreateStringValue("new-label"));
    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_EQ(component.descriptor_.properties.accessibilityLabel, "new-label");

    component.OnDataUpdate("accessibility.description", CreateStringValue("new-desc"));
    EXPECT_TRUE(component.descriptor_.properties.hasAccessibilityDescription);
    EXPECT_EQ(component.descriptor_.properties.accessibilityDescription, "new-desc");

    component.OnPropertyRemoved("accessibility.label");
    EXPECT_FALSE(component.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_TRUE(component.descriptor_.properties.accessibilityLabel.empty());

    component.OnPropertyRemoved("accessibility.description");
    EXPECT_FALSE(component.descriptor_.properties.hasAccessibilityDescription);
    EXPECT_TRUE(component.descriptor_.properties.accessibilityDescription.empty());
}

TEST_F(CustomComponentTest, should_include_icon_runtime_size_and_color_in_custom_props)
{
    CustomComponent component("Icon");

    component.OnDataUpdate("size", CreateNumberValue(24.0));
    component.OnDataUpdate("color", CreateNumberValue(static_cast<double>(0xE5000000U)));

    EXPECT_TRUE(component.customPropertyNames_.find("size") != component.customPropertyNames_.end());
    EXPECT_TRUE(component.customPropertyNames_.find("color") != component.customPropertyNames_.end());

    JsonValue customProps = component.BuildCustomProps();
    ASSERT_TRUE(customProps.IsObject());
    EXPECT_DOUBLE_EQ(customProps.GetNumber("size", 0.0), 24.0);
    EXPECT_EQ(customProps.GetUint32("color", 0), 0xE5000000U);
}

TEST_F(CustomComponentTest, should_skip_callback_and_synthetic_property_storage_for_expression_binding_updates)
{
    CustomComponent component("Panel");
    napi_env env = reinterpret_cast<napi_env>(0x2201);
    napi_ref callbackRef = CreateMockFunctionRef(mockNapiPtr_, env);
    ASSERT_NE(callbackRef, nullptr);

    component.dynamicValueCallbacks_["headline"] = { env, callbackRef, "" };
    mockNapiPtr_->callFunctionCallCount_ = 0;

    component.OnDataUpdate("__a2uiExpr__:headline", CreateStringValue("expr-value"));

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
    EXPECT_TRUE(component.properties_.find("__a2uiExpr__:headline") == component.properties_.end());
}

TEST_F(CustomComponentTest, should_dispatch_dynamic_callback_and_remove_expression_binding_for_source_property)
{
    CustomComponent component("Panel");
    napi_env env = reinterpret_cast<napi_env>(0x2202);
    napi_ref callbackRef = CreateMockFunctionRef(mockNapiPtr_, env);
    ASSERT_NE(callbackRef, nullptr);

    component.dynamicValueCallbacks_["headline"] = { env, callbackRef, "" };
    component.dynamicResolverBindingKeys_.insert("headline");
    component.AddBinding("__a2uiExpr__:headline", "/stale/path");
    mockNapiPtr_->callFunctionCallCount_ = 0;

    component.OnDataUpdate("headline", CreateStringValue("live-value"));

    ASSERT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 1U);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, mockNapiPtr_->lastCallFunctionArgs_[0]), "live-value");
    ASSERT_TRUE(component.GetProperty("headline").has_value());
    EXPECT_EQ(component.GetProperty("headline").value().GetStringValue(""), "live-value");
    EXPECT_EQ(CountBindingsForProperty(component, "__a2uiExpr__:headline"), 1U);

    component.OnPropertyRemoved("headline");

    EXPECT_FALSE(component.GetProperty("headline").has_value());
    EXPECT_EQ(CountBindingsForProperty(component, "__a2uiExpr__:headline"), 0U);
}

TEST_F(CustomComponentTest, should_route_expression_binding_updates_to_accessibility_and_icon_source_properties)
{
    CustomComponent tabs("Tabs");
    tabs.OnDataUpdate("__a2uiExpr__:accessibility.label", CreateStringValue("expr-label"));
    tabs.OnDataUpdate("__a2uiExpr__:accessibility.description", CreateStringValue("expr-description"));
    tabs.dynamicResolverBindingKeys_.insert("tabs[0].title");
    tabs.OnDataUpdate("tabs[0].title", CreateStringValue("首页"));

    EXPECT_TRUE(tabs.descriptor_.properties.hasAccessibilityLabel);
    EXPECT_EQ(tabs.descriptor_.properties.accessibilityLabel, "expr-label");
    EXPECT_TRUE(tabs.descriptor_.properties.hasAccessibilityDescription);
    EXPECT_EQ(tabs.descriptor_.properties.accessibilityDescription, "expr-description");
    ASSERT_TRUE(tabs.GetProperty("tabs[0].title").has_value());
    EXPECT_EQ(tabs.GetProperty("tabs[0].title").value().GetStringValue(""), "首页");

    CustomComponent icon("Icon");
    icon.OnDataUpdate("__a2uiExpr__:size", CreateNumberValue(24.0));
    icon.OnDataUpdate("__a2uiExpr__:color", CreateNumberValue(static_cast<double>(0xFF112233U)));
    EXPECT_TRUE(icon.customPropertyNames_.find("size") != icon.customPropertyNames_.end());
    EXPECT_TRUE(icon.customPropertyNames_.find("color") != icon.customPropertyNames_.end());
}

TEST_F(CustomComponentTest, should_not_double_prefix_binding_cleanup_when_expression_binding_property_is_removed)
{
    CustomComponent component("Panel");
    component.AddBinding("__a2uiExpr__:headline", "/headline");

    component.OnPropertyRemoved("__a2uiExpr__:headline");

    EXPECT_EQ(CountBindingsForProperty(component, "__a2uiExpr__:headline"), 1U);
}

TEST_F(CustomComponentTest, should_apply_private_attributes_and_keep_not_created_when_napi_callbacks_missing)
{
    CustomComponent component("Tabs");
    component.SetComponentId("cmp-id");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(123);

    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "tabs": [
                {"child": "c1"},
                {"child": "c2"}
            ],
            "customA": "valueA"
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyPrivateAttributes(descriptor->GetRoot());

    EXPECT_EQ(component.descriptor_.id, "cmp-id");
    EXPECT_EQ(component.descriptor_.surfaceId, "surface-id");
    EXPECT_FALSE(component.hasCreatedCustomComponent_);
    EXPECT_TRUE(component.customPropertyNames_.find("customA") != component.customPropertyNames_.end());
    EXPECT_TRUE(component.descriptor_.customProps.IsValid());
    EXPECT_EQ(component.childToSlotMapping_["c1"], "tab-0");
    EXPECT_EQ(component.childToSlotMapping_["c2"], "tab-1");
}

TEST_F(CustomComponentTest, should_create_attribute_value_with_empty_theme_context)
{
    CustomComponent component("Tabs");
    component.SetComponentId("test-empty-theme");
    component.SetSurfaceId("test-surface");
    component.SetRenderId(125);

    // 没有设置任何主题管理器或空的主题上下文
    napi_value attrValue = component.CreateAttributeValue();
    EXPECT_NE(attrValue, nullptr);
}

TEST_F(CustomComponentTest, should_clear_reference_state_on_reset_references)
{
    CustomComponent component("Panel");
    component.env_ = reinterpret_cast<napi_env>(0x1);
    component.componentContentRef_ = reinterpret_cast<napi_ref>(0x2);
    component.childSlotRef_ = reinterpret_cast<napi_ref>(0x3);
    component.childSlotRefs_["slotA"] = reinterpret_cast<napi_ref>(0x4);
    component.childSlotRefs_["slotB"] = reinterpret_cast<napi_ref>(0x5);
    component.childSlotHandles_["slotA"] = reinterpret_cast<ArkUI_NodeContentHandle>(0x6);
    component.childToSlotMapping_["childA"] = "slotA";

    component.ResetReferences();

    EXPECT_EQ(component.componentContentRef_, nullptr);
    EXPECT_EQ(component.childSlotRef_, nullptr);
    EXPECT_TRUE(component.childSlotRefs_.empty());
    EXPECT_TRUE(component.childSlotHandles_.empty());
    EXPECT_TRUE(component.childToSlotMapping_.empty());
}

TEST_F(CustomComponentTest, should_null_native_view_in_dispose_component_content_when_reference_exists)
{
    CustomComponent component("Panel");
    component.env_ = reinterpret_cast<napi_env>(0x10);
    component.componentContentRef_ = reinterpret_cast<napi_ref>(0x20);
    component.nativeView_ = reinterpret_cast<ArkUI_NodeHandle>(0x30);

    component.DisposeComponentContent();

    EXPECT_EQ(component.nativeView_, nullptr);
}

TEST_F(CustomComponentTest, should_return_early_for_invalid_sync_child_slots_inputs)
{
    CustomComponent component("Tabs");
    component.SyncChildSlots(nullptr);
    EXPECT_TRUE(component.childSlotHandles_.empty());

    component.env_ = reinterpret_cast<napi_env>(0x7);
    component.SyncChildSlots(reinterpret_cast<napi_value>(0x8));
    EXPECT_TRUE(component.childSlotHandles_.empty());
    EXPECT_TRUE(component.childSlotRefs_.empty());
}

TEST_F(CustomComponentTest, should_clear_children_in_remove_all_children_for_single_and_multi_slot_modes)
{
    auto component = std::make_shared<CustomComponent>("Tabs");
    auto child1 = std::make_shared<SimpleChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x901));
    auto child2 = std::make_shared<SimpleChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x902));

    component->childSlotHandle_ = reinterpret_cast<ArkUI_NodeContentHandle>(0xA01);
    component->AddChildAt(child1, 0);
    component->AddChildAt(child2, 1);
    ASSERT_EQ(component->GetChildren().size(), 2U);
    component->RemoveAllChildren();
    EXPECT_TRUE(component->GetChildren().empty());

    component->childSlotHandles_["tab-0"] = reinterpret_cast<ArkUI_NodeContentHandle>(0xA02);
    component->AddChildAt(child1, 0);
    component->RemoveAllChildren();
    EXPECT_TRUE(component->GetChildren().empty());
}

TEST_F(CustomComponentTest, should_find_active_custom_component_by_handle_and_cover_property_path_edges)
{
    EXPECT_EQ(ResolveCustomPropertyWarningPath("  componentA  ", ""), "componentA");
    EXPECT_EQ(ResolveCustomPropertyWarningPath(" \t\r\n", ""), "customProps");
    EXPECT_EQ(CustomComponent::FindByHandle(0U), nullptr);

    CustomComponent component("Panel");
    EXPECT_EQ(CustomComponent::FindByHandle(component.GetCustomComponentHandle()), &component);

    int unrelated = 0;
    EXPECT_EQ(CustomComponent::FindByHandle(reinterpret_cast<uintptr_t>(&unrelated)), nullptr);
}

TEST_F(CustomComponentTest, should_cover_checks_null_runtime_property_and_dynamic_callback_failure_edges)
{
    CustomComponent component("Panel");

    std::string failedMessage;
    component.checksEngine_.reset();
    component.ParseChecks(JsonValue());
    EXPECT_TRUE(component.ValidateChecks(R"(["ignored"])", &failedMessage));

    component.properties_["runtime"] = CreateStringValue("old");
    component.OnPropertyApplied("runtime", JsonValue());
    EXPECT_FALSE(component.GetProperty("runtime").has_value());
    EXPECT_FALSE(component.SetRuntimeCustomProperty("", CreateStringValue("value")));
    EXPECT_FALSE(component.SetRuntimeCustomProperty("runtime", JsonValue()));
    EXPECT_TRUE(component.SetRuntimeCustomProperty("runtime", CreateStringValue("value")));
    ASSERT_TRUE(component.GetCustomProperty("runtime").IsString());
    EXPECT_FALSE(component.GetCustomProperty("missing").IsValid());

    EXPECT_FALSE(component.DispatchDynamicValueCallback("missing", CreateStringValue("value")));
    component.dynamicValueCallbacks_["broken"] = { nullptr, nullptr, "" };
    EXPECT_FALSE(component.DispatchDynamicValueCallback("broken", CreateStringValue("value")));

    napi_env env = reinterpret_cast<napi_env>(0x7100);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    napi_ref callbackRef = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, callback, 1, &callbackRef), napi_ok);
    component.dynamicValueCallbacks_["openFail"] = { env, callbackRef, "" };
    mockNapiPtr_->SetOpenHandleScopeStatus(napi_generic_failure);
    EXPECT_FALSE(component.DispatchDynamicValueCallback("openFail", CreateStringValue("value")));
    mockNapiPtr_->ResetOpenHandleScopeStatus();

    component.dynamicValueCallbacks_["callFail"] = { env, callbackRef, "" };
    mockNapiPtr_->SetCallFunctionStatus(napi_generic_failure);
    EXPECT_FALSE(component.DispatchDynamicValueCallback("callFail", CreateStringValue("value")));
    mockNapiPtr_->ResetCallFunctionStatus();
}

TEST_F(CustomComponentTest, should_register_dynamic_value_callbacks_for_path_and_resolved_values)
{
    napi_env env = reinterpret_cast<napi_env>(0x7200);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);

    CustomComponent component("Panel");
    std::string errorMessage;
    auto validPath = ParseJson(R"({"path":"/form/title"})");
    auto invalidPath = ParseJson(R"({"path":"form/title"})");
    ASSERT_NE(validPath, nullptr);
    ASSERT_NE(invalidPath, nullptr);

    EXPECT_FALSE(component.RegisterDynamicValueCallback("", validPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(errorMessage, "propertyName is empty");
    EXPECT_FALSE(
        component.RegisterDynamicValueCallback("title", validPath->GetRoot(), nullptr, callback, &errorMessage));
    EXPECT_EQ(errorMessage, "callback is not a function");

    mockNapiPtr_->SetCreateReferenceStatus(napi_generic_failure);
    EXPECT_FALSE(component.RegisterDynamicValueCallback("title", validPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(errorMessage, "failed to create callback reference");
    mockNapiPtr_->ResetCreateReferenceStatus();

    EXPECT_FALSE(component.RegisterDynamicValueCallback("title", invalidPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(errorMessage, "path is invalid");

    EXPECT_TRUE(component.RegisterDynamicValueCallback("title", validPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(CountBindingsForProperty(component, "title"), 1U);
    EXPECT_TRUE(component.dynamicResolverBindingKeys_.find("title") != component.dynamicResolverBindingKeys_.end());

    auto literalValue = JsonAdapter::CreateString("resolved-now");
    ASSERT_NE(literalValue, nullptr);
    EXPECT_TRUE(
        component.RegisterDynamicValueCallback("headline", literalValue->GetRoot(), env, callback, &errorMessage));
    EXPECT_TRUE(component.dynamicValueCallbacks_.find("headline") == component.dynamicValueCallbacks_.end());
}

TEST_F(CustomComponentTest, should_keep_function_call_callback_for_data_path_dependencies)
{
    constexpr int32_t renderId = 826;
    const std::string surfaceId = "surface-function-call-callback";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto catalog = std::make_shared<Catalog>("catalog-function-call-callback");
    catalog->AddFunction(std::make_shared<CatalogItem>("formatString", CatalogItemType::LOCAL_FUNCTION));
    surface.SetCatalog(catalog);
    auto initialData = ParseJson(R"({"form":{"title":"current-title"}})");
    ASSERT_NE(initialData, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/", initialData->GetRoot());

    napi_env env = reinterpret_cast<napi_env>(0x8260);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);
    napi_value bridgeCallback = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateFunction(env, "functionBridge", NAPI_AUTO_LENGTH, nullptr, nullptr, &bridgeCallback),
        napi_ok);
    FunctionBridge::GetInstance().RegisterInvokeLocalFunction(env, bridgeCallback);
    FunctionBridgeResetGuard bridgeResetGuard(mockNapiPtr_, env);

    auto component = std::make_shared<CustomComponent>("Panel");
    component->SetComponentId("function-call-callback");
    component->SetSurfaceId(surfaceId);
    component->SetRenderId(renderId);
    auto descriptor =
        ParseJson(R"({"call":"formatString","returnType":"string","args":{"value":{"path":"/form/title"}}})");
    ASSERT_NE(descriptor, nullptr);

    mockNapiPtr_->callFunctionCallCount_ = 0;
    std::string errorMessage;
    ASSERT_TRUE(PreparePassthroughNormalizeResponse(mockNapiPtr_));
    ASSERT_TRUE(component->RegisterDynamicValueCallback("title", descriptor->GetRoot(), env, callback, &errorMessage))
        << errorMessage;
    EXPECT_EQ(CountBindingsForProperty(*component, "title"), 1U);
    ASSERT_EQ(component->GetDataBindings()[0].type_, BindingType::FUNCTION_CALL);
    EXPECT_EQ(component->GetDataBindings()[0].dataPath_, "/form/title");
    size_t initialCallCount = mockNapiPtr_->callFunctionCallCount_;
    ASSERT_EQ(initialCallCount, 2U);

    auto updatedTitle = CreateStringValue("updated-title");
    ASSERT_TRUE(PreparePassthroughNormalizeResponse(mockNapiPtr_));
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/form/title", updatedTitle);

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, initialCallCount + 2U);
    ASSERT_FALSE(mockNapiPtr_->lastCallFunctionArgs_.empty());
    EXPECT_EQ(GetStringValue(mockNapiPtr_, mockNapiPtr_->lastCallFunctionArgs_[0]), "updated-title");
    EXPECT_TRUE(component->dynamicValueCallbacks_.find("title") != component->dynamicValueCallbacks_.end());
}

#ifdef ENABLE_EXPRESSION_ENGINE
TEST_F(CustomComponentTest, should_keep_expression_callback_for_data_model_dependencies)
{
    constexpr int32_t renderId = 827;
    const std::string surfaceId = "surface-expression-callback";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto initialData = ParseJson(R"({"form":{"title":"current-title"}})");
    ASSERT_NE(initialData, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/", initialData->GetRoot());

    napi_env env = reinterpret_cast<napi_env>(0x8270);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);

    auto component = std::make_shared<CustomComponent>("Panel");
    component->SetComponentId("expression-callback");
    component->SetSurfaceId(surfaceId);
    component->SetRenderId(renderId);
    auto descriptor = ParseJson(R"("{{ $__dataModel.form.title }}")");
    ASSERT_NE(descriptor, nullptr);

    mockNapiPtr_->callFunctionCallCount_ = 0;
    std::string errorMessage;
    EXPECT_TRUE(component->RegisterDynamicValueCallback("title", descriptor->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(CountBindingsForProperty(*component, "title"), 1U);
    ASSERT_EQ(component->GetDataBindings()[0].type_, BindingType::EXPRESSION);
    EXPECT_EQ(component->GetDataBindings()[0].dataPath_, "/form/title");
    ASSERT_EQ(component->GetDataBindings()[0].globalVarDeps_.size(), 1U);
    EXPECT_EQ(component->GetDataBindings()[0].globalVarDeps_[0], "__dataModel");
    size_t initialCallbackCount = mockNapiPtr_->callFunctionCallCount_;
    ASSERT_EQ(initialCallbackCount, 1U);

    auto updatedTitle = CreateStringValue("updated-title");
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/form/title", updatedTitle);

    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, initialCallbackCount + 1U);
    ASSERT_FALSE(mockNapiPtr_->lastCallFunctionArgs_.empty());
    EXPECT_EQ(GetStringValue(mockNapiPtr_, mockNapiPtr_->lastCallFunctionArgs_[0]), "updated-title");
    EXPECT_TRUE(component->dynamicValueCallbacks_.find("title") != component->dynamicValueCallbacks_.end());
}
#endif

TEST_F(CustomComponentTest, should_create_custom_component_and_sync_multi_child_slots)
{
    napi_env env = reinterpret_cast<napi_env>(0x7300);
    napi_value createFunction = CreateManualNapiFunction(mockNapiPtr_, 7301);
    napi_value updateFunction = CreateManualNapiFunction(mockNapiPtr_, 7302);
    NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);

    napi_value content = CreateManualNapiObject(mockNapiPtr_, 7310);
    napi_value childSlot = CreateManualNapiObject(mockNapiPtr_, 7311);
    napi_value childSlots = CreateManualNapiObject(mockNapiPtr_, 7312);
    napi_value tab0 = CreateManualNapiObject(mockNapiPtr_, 7313);
    napi_value tab1 = CreateManualNapiObject(mockNapiPtr_, 7314);
    mockNapiPtr_->objectProperties_[childSlots]["tab-0"] = tab0;
    mockNapiPtr_->objectProperties_[childSlots]["tab-1"] = tab1;
    PreloadCustomComponentCallResults(mockNapiPtr_, content, childSlot, childSlots, 1000, 2000);
    mockNapiPtr_->nextValueId_ = 1000;
    mockArkUIPtr_->SetNodeHandleResult(reinterpret_cast<ArkUI_NodeHandle>(0x7303));
    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x7304));

    CustomComponent component("Tabs");
    component.SetComponentId("tabsMain");
    component.SetSurfaceId("surface-create");
    component.SetRenderId(730);

    EXPECT_TRUE(component.CreateCustomComponent());
    EXPECT_TRUE(component.hasCreatedCustomComponent_);
    EXPECT_EQ(component.GetNativeView(), reinterpret_cast<ArkUI_NodeHandle>(0x7303));
    EXPECT_EQ(component.childSlotHandle_, reinterpret_cast<ArkUI_NodeContentHandle>(0x7304));
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());
    EXPECT_TRUE(component.childSlotHandles_.find("tab-1") != component.childSlotHandles_.end());
    EXPECT_NE(component.componentContentRef_, nullptr);
    EXPECT_NE(component.childSlotRef_, nullptr);
}

TEST_F(CustomComponentTest, should_cover_create_custom_component_failure_branches)
{
    napi_env env = reinterpret_cast<napi_env>(0x7400);
    napi_value createFunction = CreateManualNapiFunction(mockNapiPtr_, 7401);
    napi_value updateFunction = CreateManualNapiFunction(mockNapiPtr_, 7402);

    {
        NapiResourceManagerRefGuard missingCallbackGuard(mockNapiPtr_, nullptr, nullptr, nullptr);
        CustomComponent component("Panel");
        EXPECT_FALSE(component.CreateCustomComponent());
    }

    {
        NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);
        CustomComponent component("Panel");
        mockNapiPtr_->SetGetReferenceValueStatus(napi_generic_failure);
        EXPECT_FALSE(component.CreateCustomComponent());
        mockNapiPtr_->ResetGetReferenceValueStatus();
    }

    {
        NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);
        CustomComponent component("Panel");
        mockNapiPtr_->SetCallFunctionStatus(napi_generic_failure);
        EXPECT_FALSE(component.CreateCustomComponent());
        mockNapiPtr_->ResetCallFunctionStatus();
    }

    {
        NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);
        CustomComponent component("Panel");
        mockNapiPtr_->nextValueId_ = 2100;
        for (int32_t id = 2100; id <= 2200; ++id) {
            mockNapiPtr_->valueTypes_[RawNapiValue(id)] = napi_object;
            mockNapiPtr_->objectProperties_[RawNapiValue(id)] = {};
        }
        EXPECT_FALSE(component.CreateCustomComponent());
    }

    {
        NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);
        CustomComponent component("Panel");
        napi_value content = CreateManualNapiObject(mockNapiPtr_, 7410);
        PreloadCustomComponentCallResults(mockNapiPtr_, content, nullptr, nullptr, 2300, 2400);
        mockNapiPtr_->nextValueId_ = 2300;
        mockArkUIPtr_->SetGetNodeHandleFromNapiValueResult(-1);
        EXPECT_FALSE(component.CreateCustomComponent());
        mockArkUIPtr_->ResetGetNodeHandleFromNapiValueResult();
    }
}

TEST_F(CustomComponentTest, should_update_created_custom_component_from_private_attributes_margin_and_config)
{
    napi_env env = reinterpret_cast<napi_env>(0x7500);
    napi_value createFunction = CreateManualNapiFunction(mockNapiPtr_, 7501);
    napi_value updateFunction = CreateManualNapiFunction(mockNapiPtr_, 7502);
    NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);

    napi_value content = CreateManualNapiObject(mockNapiPtr_, 7510);
    napi_value childSlot = CreateManualNapiObject(mockNapiPtr_, 7511);
    napi_value childSlots = CreateManualNapiObject(mockNapiPtr_, 7512);
    napi_value tab0 = CreateManualNapiObject(mockNapiPtr_, 7513);
    mockNapiPtr_->objectProperties_[childSlots]["tab-0"] = tab0;
    PreloadCustomComponentCallResults(mockNapiPtr_, content, childSlot, childSlots, 3000, 4200);
    mockNapiPtr_->nextValueId_ = 3000;
    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x7504));

    CustomComponent component("Tabs");
    component.SetComponentId("tabsUpdate");
    component.SetSurfaceId("surface-update");
    component.SetRenderId(750);
    component.hasCreatedCustomComponent_ = true;
    component.env_ = env;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, content, 1, &component.componentContentRef_), napi_ok);
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, childSlot, 1, &component.childSlotRef_), napi_ok);
    napi_ref slotRef = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, tab0, 1, &slotRef), napi_ok);
    component.childSlotRefs_["tab-0"] = slotRef;

    auto descriptor = ParseJson(
        R"({
            "tabs": [{"child":"tabA","title":{"path":"/tabs/title"}}],
            "customA": "valueA"
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyPrivateAttributes(descriptor->GetRoot());
    component.SetMargin(1.0F, 2.0F, 3.0F, 4.0F);
    ThemeContext themeContext;
    component.OnConfigChange(themeContext);

    EXPECT_GE(mockNapiPtr_->callFunctionCallCount_, 3U);
    ASSERT_EQ(mockNapiPtr_->lastCallFunctionArgs_.size(), 4U);
    EXPECT_TRUE(component.descriptor_.customProps.Has("customA"));
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());
    EXPECT_TRUE(component.descriptor_.properties.hasMargin);
}

TEST_F(CustomComponentTest, should_create_attribute_value_with_common_props_and_data_model_json)
{
    constexpr int32_t renderId = 760;
    const std::string surfaceId = "surface-attr";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto data = ParseJson(R"([{"name":"one"},{"name":"two"}])");
    ASSERT_NE(data, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/items", data->GetRoot());

    CustomComponent component("Tabs");
    component.SetComponentId("tabsAttr");
    component.SetSurfaceId(surfaceId);
    component.SetRenderId(renderId);
    component.env_ = reinterpret_cast<napi_env>(0x7600);
    component.descriptor_.customProps = ParseJson(R"({"tabs":[{"title":"Home"}]})")->GetRoot();
    component.descriptor_.properties.size = R"({"width":320,"height":120})";
    component.descriptor_.properties.padding = R"({"top":1,"right":2,"bottom":3,"left":4})";
    component.descriptor_.properties.margin = R"({"top":5,"right":6,"bottom":7,"left":8})";
    component.descriptor_.properties.hasMargin = true;
    component.descriptor_.properties.hasWidth = true;
    component.descriptor_.properties.width = 320.0;
    component.descriptor_.properties.hasHeight = true;
    component.descriptor_.properties.height = 120.0;
    component.descriptor_.properties.hasWeight = true;
    component.descriptor_.properties.weight = 2.0;
    component.descriptor_.properties.hasAccessibilityLabel = true;
    component.descriptor_.properties.accessibilityLabel = "label";
    component.descriptor_.properties.hasAccessibilityDescription = true;
    component.descriptor_.properties.accessibilityDescription = "description";

    napi_value attributeValue = component.CreateAttributeValue();
    ASSERT_NE(attributeValue, nullptr);
    auto attrIt = mockNapiPtr_->objectProperties_.find(attributeValue);
    ASSERT_NE(attrIt, mockNapiPtr_->objectProperties_.end());
    EXPECT_TRUE(attrIt->second.find("customProps") != attrIt->second.end());
    EXPECT_TRUE(attrIt->second.find("properties") != attrIt->second.end());
    EXPECT_TRUE(attrIt->second.find("dataModelJson") != attrIt->second.end());
    napi_value propertiesValue = attrIt->second["properties"];
    auto propsIt = mockNapiPtr_->objectProperties_.find(propertiesValue);
    ASSERT_NE(propsIt, mockNapiPtr_->objectProperties_.end());
    EXPECT_TRUE(propsIt->second.find("width") != propsIt->second.end());
    EXPECT_TRUE(propsIt->second.find("height") != propsIt->second.end());
    EXPECT_TRUE(propsIt->second.find("weight") != propsIt->second.end());
    EXPECT_TRUE(propsIt->second.find("accessibilityLabel") != propsIt->second.end());
    EXPECT_TRUE(propsIt->second.find("accessibilityDescription") != propsIt->second.end());
}

TEST_F(CustomComponentTest, should_resolve_template_children_and_merge_generated_tabs_and_row_children)
{
    constexpr int32_t renderId = 770;
    const std::string surfaceId = "surface-template";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto data = ParseJson(R"([{"name":"one"},{"name":"two"},{"name":"three"}])");
    ASSERT_NE(data, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/items", data->GetRoot());

    CustomComponentProbe tabs("Tabs");
    tabs.SetSurfaceId(surfaceId);
    auto tabsDescriptor = ParseJson(R"({"children":{"componentId":"tabTpl","path":"/items"}})");
    ASSERT_NE(tabsDescriptor, nullptr);
    std::list<std::string> tabIds = tabs.ResolveTabsChildIds(tabsDescriptor->GetRoot());
    ASSERT_EQ(tabIds.size(), 3U);
    EXPECT_EQ(tabIds.front(), "/itemstabTpl:0:tabTpl");
    tabs.ParseTabsMapping(tabsDescriptor->GetRoot());
    JsonValue tabsProps = tabs.BuildCustomProps();
    ASSERT_TRUE(tabsProps.IsObject());
    ASSERT_TRUE(tabsProps.Has("tabs"));
    EXPECT_EQ(tabsProps.GetItem("tabs").GetArraySize(), 3);

    CustomComponentProbe row("Extended.Row");
    row.SetSurfaceId(surfaceId);
    auto rowDescriptor = ParseJson(R"({"children":{"componentId":"rowTpl","path":"/items"}})");
    ASSERT_NE(rowDescriptor, nullptr);
    std::list<std::string> rowIds = row.ResolveRowChildIds(rowDescriptor->GetRoot());
    ASSERT_EQ(rowIds.size(), 3U);
    EXPECT_EQ(rowIds.front(), "/itemsrowTpl:0:rowTpl");
    row.ParseTabsMapping(rowDescriptor->GetRoot());
    JsonValue rowProps = row.BuildCustomProps();
    ASSERT_TRUE(rowProps.IsObject());
    ASSERT_TRUE(rowProps.Has("children"));
    EXPECT_EQ(rowProps.GetItem("children").GetArraySize(), 3);
}

/**
 * @tc.name: should_preserve_template_children_descriptor_when_extended_tabs_instances_are_generated
 * @tc.desc: 验证扩展 Tabs 已生成模板实例 ID 时仍向 ArkTS 传递原始模板 children 描述符
 * @tc.type: FUNC
 */
TEST_F(CustomComponentTest, should_preserve_template_children_descriptor_when_extended_tabs_instances_are_generated)
{
    CustomComponentProbe tabs("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    tabs.SetSurfaceContext(surfaceContext);
    auto descriptor = ParseJson(R"({"children":{"componentId":"categoryTab","path":"/categories"}})");
    ASSERT_NE(descriptor, nullptr);
    tabs.CollectChildListDescriptor(descriptor->GetRoot());
    tabs.tabChildIds_ = { "/categoriescategoryTab:0:categoryTab", "/categoriescategoryTab:1:categoryTab" };

    JsonValue customProps = tabs.BuildCustomProps();

    ASSERT_TRUE(customProps.IsObject());
    JsonValue children = customProps.GetItem("children");
    ASSERT_TRUE(children.IsObject());
    EXPECT_EQ(children.GetString("componentId", ""), "categoryTab");
    EXPECT_EQ(children.GetString("path", ""), "/categories");
}

TEST_F(CustomComponentTest, should_build_custom_props_for_raw_tabs_non_array_empty_tabs_and_row_children)
{
    CustomComponent tabs("Tabs");
    JsonValue emptyTabsProps = tabs.BuildCustomProps();
    ASSERT_TRUE(emptyTabsProps.IsObject());
    EXPECT_FALSE(emptyTabsProps.GetChild().IsValid());

    tabs.customPropertyNames_ = { "tabs" };
    tabs.properties_["tabs"] = CreateStringValue("legacy-tabs");
    JsonValue rawTabsProps = tabs.BuildCustomProps();
    ASSERT_TRUE(rawTabsProps.IsObject());
    ASSERT_TRUE(rawTabsProps.GetItem("tabs").IsString());
    EXPECT_EQ(rawTabsProps.GetString("tabs", ""), "legacy-tabs");

    CustomComponent generatedTabs("Tabs");
    generatedTabs.tabChildIds_ = { "tab-a", "", "tab-b" };
    JsonValue generatedProps = generatedTabs.BuildCustomProps();
    ASSERT_TRUE(generatedProps.IsObject());
    ASSERT_TRUE(generatedProps.Has("tabs"));
    EXPECT_EQ(generatedProps.GetItem("tabs").GetArraySize(), 2);

    CustomComponent row("Row");
    row.rowChildIds_ = { "row-a", "", "row-b" };
    JsonValue rowProps = row.BuildCustomProps();
    ASSERT_TRUE(rowProps.IsObject());
    ASSERT_TRUE(rowProps.Has("children"));
    EXPECT_EQ(rowProps.GetItem("children").GetArraySize(), 2);
}

TEST_F(CustomComponentTest, should_sync_child_slots_success_null_keys_and_failed_extraction)
{
    CustomComponent component("Tabs");
    component.env_ = reinterpret_cast<napi_env>(0x7800);
    napi_value childSlots = CreateManualNapiObject(mockNapiPtr_, 7801);
    napi_value slotValue = CreateManualNapiObject(mockNapiPtr_, 7802);
    mockNapiPtr_->objectProperties_[childSlots]["tab-0"] = slotValue;

    mockNapiPtr_->SetGetPropertyNamesReturnNullOnce();
    component.SyncChildSlots(childSlots);
    EXPECT_TRUE(component.childSlotHandles_.empty());

    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x7803));
    component.SyncChildSlots(childSlots);
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());
    EXPECT_TRUE(component.childSlotRefs_.find("tab-0") != component.childSlotRefs_.end());

    CustomComponent emptyKeyComponent("Tabs");
    emptyKeyComponent.env_ = reinterpret_cast<napi_env>(0x7804);
    napi_value emptyKeySlots = CreateManualNapiObject(mockNapiPtr_, 7805);
    mockNapiPtr_->objectProperties_[emptyKeySlots][""] = slotValue;
    emptyKeyComponent.SyncChildSlots(emptyKeySlots);
    EXPECT_TRUE(emptyKeyComponent.childSlotHandles_.empty());

    CustomComponent badStringComponent("Tabs");
    badStringComponent.env_ = reinterpret_cast<napi_env>(0x7806);
    mockNapiPtr_->SetGetValueStringUtf8Status(napi_generic_failure);
    badStringComponent.SyncChildSlots(childSlots);
    EXPECT_TRUE(badStringComponent.childSlotHandles_.empty());
    mockNapiPtr_->ResetGetValueStringUtf8Status();

    CustomComponent nullHandleComponent("Tabs");
    nullHandleComponent.env_ = reinterpret_cast<napi_env>(0x7807);
    mockArkUIPtr_->SetNodeContentHandleResult(nullptr);
    nullHandleComponent.SyncChildSlots(childSlots);
    EXPECT_TRUE(nullHandleComponent.childSlotHandles_.empty());
    mockArkUIPtr_->ResetNodeContentHandleResult();
}

TEST_F(CustomComponentTest, should_call_dispose_function_and_cover_child_move_remove_branches)
{
    napi_env env = reinterpret_cast<napi_env>(0x7900);
    napi_value content = CreateManualNapiObject(mockNapiPtr_, 7901);
    napi_value dispose = CreateManualNapiFunction(mockNapiPtr_, 7902);
    mockNapiPtr_->objectProperties_[content]["dispose"] = dispose;

    CustomComponent component("Panel");
    component.env_ = env;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, content, 1, &component.componentContentRef_), napi_ok);
    component.nativeView_ = reinterpret_cast<ArkUI_NodeHandle>(0x7903);
    component.DisposeComponentContent();
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1U);
    EXPECT_EQ(component.nativeView_, nullptr);

    auto child = std::make_shared<SimpleChildComponent>(reinterpret_cast<ArkUI_NodeHandle>(0x7904));
    child->SetComponentId("childA");

    CustomComponent noSlot("Tabs");
    noSlot.OnMoveChild(child, 0, 1);
    noSlot.OnRemoveChild(child);

    CustomComponent multiSlot("Tabs");
    multiSlot.childToSlotMapping_["childA"] = "tab-0";
    multiSlot.childSlotHandles_["tab-0"] = reinterpret_cast<ArkUI_NodeContentHandle>(0x7905);
    multiSlot.OnMoveChild(child, 0, 2);
    ASSERT_TRUE(mockArkUIPtr_->nodeContentMapping_.find(reinterpret_cast<ArkUI_NodeContentHandle>(0x7905)) !=
                mockArkUIPtr_->nodeContentMapping_.end());

    CustomComponent singleSlot("Tabs");
    singleSlot.childSlotHandle_ = reinterpret_cast<ArkUI_NodeContentHandle>(0x7906);
    singleSlot.OnMoveChild(child, 0, 1);
    singleSlot.OnRemoveChild(child);
    EXPECT_TRUE(mockArkUIPtr_->nodeContentMapping_.find(reinterpret_cast<ArkUI_NodeContentHandle>(0x7906)) !=
                mockArkUIPtr_->nodeContentMapping_.end());
}

TEST_F(CustomComponentTest, should_build_json_with_color_mode_when_theme_context_has_no_surface_theme_values)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = false;
    context.iconUrl = "";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);
    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_DOUBLE_EQ(root.GetNumber("colorMode", -1.0), static_cast<double>(static_cast<int32_t>(ThemeMode::LIGHT)));
    EXPECT_FALSE(root.Has("primaryColor"));
    EXPECT_FALSE(root.Has("darkPrimaryColor"));
    EXPECT_FALSE(root.Has("brandColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
    EXPECT_FALSE(root.Has("iconUrl"));
    EXPECT_FALSE(root.Has("agentDisplayName"));
}

TEST_F(CustomComponentTest, should_build_json_with_primary_color_when_has_primary_color)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0xFF123456;
    context.hasDarkPrimaryColor = false;
    context.iconUrl = "";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("primaryColor", ""), "#FF123456");
    EXPECT_DOUBLE_EQ(root.GetNumber("colorMode", -1.0), static_cast<double>(static_cast<int32_t>(ThemeMode::LIGHT)));
    EXPECT_FALSE(root.Has("darkPrimaryColor"));
    EXPECT_FALSE(root.Has("brandColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
    EXPECT_FALSE(root.Has("iconUrl"));
    EXPECT_FALSE(root.Has("agentDisplayName"));
}

TEST_F(CustomComponentTest, should_build_json_with_color_strings_preserving_zero_alpha)
{
    ThemeContext context;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0x00123456;
    context.hasDarkPrimaryColor = true;
    context.darkPrimaryColorArgb = 0x000A0B0C;
    context.hasBrandColor = true;
    context.brandColor = 0x00ABCDEF;

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("primaryColor", ""), "#00123456");
    EXPECT_EQ(root.GetString("darkPrimaryColor", ""), "#000A0B0C");
    EXPECT_EQ(root.GetString("brandColor", ""), "#00ABCDEF");
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
}

TEST_F(CustomComponentTest, should_build_json_with_dark_primary_color_when_has_dark_primary_color)
{
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = true;
    context.darkPrimaryColorArgb = 0xFF654321;
    context.iconUrl = "";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("darkPrimaryColor", ""), "#FF654321");
    EXPECT_DOUBLE_EQ(root.GetNumber("colorMode", -1.0), static_cast<double>(static_cast<int32_t>(ThemeMode::DARK)));
    EXPECT_FALSE(root.Has("primaryColor"));
    EXPECT_FALSE(root.Has("brandColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
}

TEST_F(CustomComponentTest, should_build_json_with_brand_color_when_has_brand_color_and_other_theme_value)
{
    ThemeContext context;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0xFF000000;
    context.hasDarkPrimaryColor = false;
    context.hasBrandColor = true;
    context.brandColor = 0xFFABCDEF;
    context.iconUrl = "";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("brandColor", ""), "#FFABCDEF");
    EXPECT_EQ(root.GetString("primaryColor", ""), "#FF000000");
    EXPECT_FALSE(root.Has("darkPrimaryColor"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
}

TEST_F(CustomComponentTest, should_build_json_with_brand_color_and_color_mode_when_only_brand_color_is_available)
{
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = false;
    context.hasBrandColor = true;
    context.brandColor = 0xFFABCDEF;
    context.iconUrl = "";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("brandColor", ""), "#FFABCDEF");
    EXPECT_DOUBLE_EQ(root.GetNumber("colorMode", -1.0), static_cast<double>(static_cast<int32_t>(ThemeMode::DARK)));
    EXPECT_FALSE(root.Has("primaryColor"));
    EXPECT_FALSE(root.Has("darkPrimaryColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
    EXPECT_FALSE(root.Has("iconUrl"));
    EXPECT_FALSE(root.Has("agentDisplayName"));
}

TEST_F(CustomComponentTest, should_build_json_with_icon_url_when_icon_url_not_empty)
{
    ThemeContext context;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = false;
    context.iconUrl = "https://example.com/icon.png";
    context.agentDisplayName = "";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("iconUrl", ""), "https://example.com/icon.png");
    EXPECT_FALSE(root.Has("primaryColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("agentDisplayName"));
}

TEST_F(CustomComponentTest, should_build_json_with_agent_display_name_when_agent_display_name_not_empty)
{
    ThemeContext context;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = false;
    context.iconUrl = "";
    context.agentDisplayName = "MyAgent";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("agentDisplayName", ""), "MyAgent");
    EXPECT_FALSE(root.Has("primaryColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("iconUrl"));
}

TEST_F(CustomComponentTest, should_build_json_with_all_fields_when_all_fields_set)
{
    ThemeContext context;
    context.colorMode = ThemeMode::DARK;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0xFF111111;
    context.hasDarkPrimaryColor = true;
    context.darkPrimaryColorArgb = 0xFF222222;
    context.hasBrandColor = true;
    context.brandColor = 0xFF333333;
    context.breakpoint = Breakpoint::LG;
    context.iconUrl = "https://test.com/icon.svg";
    context.agentDisplayName = "TestAgent";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_EQ(root.GetString("primaryColor", ""), "#FF111111");
    EXPECT_EQ(root.GetString("darkPrimaryColor", ""), "#FF222222");
    EXPECT_EQ(root.GetString("brandColor", ""), "#FF333333");
    EXPECT_DOUBLE_EQ(root.GetNumber("colorMode", -1.0), static_cast<double>(static_cast<int32_t>(ThemeMode::DARK)));
    EXPECT_DOUBLE_EQ(root.GetNumber("breakpoint", -1.0), static_cast<double>(static_cast<int32_t>(Breakpoint::LG)));
    EXPECT_EQ(root.GetString("iconUrl", ""), "https://test.com/icon.svg");
    EXPECT_EQ(root.GetString("agentDisplayName", ""), "TestAgent");
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
}

TEST_F(CustomComponentTest, should_set_component_theme_property_with_color_mode_when_surface_theme_values_are_absent)
{
    napi_env env = reinterpret_cast<napi_env>(0x100);
    napi_value object = nullptr;
    mockNapiPtr_->CreateObject(env, &object);
    ASSERT_NE(object, nullptr);

    ThemeContext context;
    context.hasPrimaryColor = false;
    context.hasDarkPrimaryColor = false;
    context.iconUrl = "";
    context.agentDisplayName = "";

    SetComponentThemeProperty(env, object, context);

    auto& props = mockNapiPtr_->objectProperties_[object];
    EXPECT_TRUE(props.find("componentTheme") != props.end());
}

TEST_F(CustomComponentTest, should_set_component_theme_property_when_build_returns_valid_json)
{
    napi_env env = reinterpret_cast<napi_env>(0x100);
    napi_value object = nullptr;
    mockNapiPtr_->CreateObject(env, &object);
    ASSERT_NE(object, nullptr);

    ThemeContext context;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0xFF123456;
    context.iconUrl = "https://example.com/icon.png";
    context.agentDisplayName = "AgentName";

    SetComponentThemeProperty(env, object, context);

    auto& props = mockNapiPtr_->objectProperties_[object];
    EXPECT_TRUE(props.find("componentTheme") != props.end());
}

TEST_F(CustomComponentTest, should_build_json_with_partial_fields_when_only_some_fields_set)
{
    ThemeContext context;
    context.colorMode = ThemeMode::LIGHT;
    context.breakpoint = Breakpoint::MD;
    context.hasPrimaryColor = true;
    context.primaryColorArgb = 0xFF123456;
    context.hasDarkPrimaryColor = false;
    context.hasBrandColor = false;
    context.iconUrl = "";
    context.agentDisplayName = "PartialAgent";

    auto result = BuildComponentThemeJson(context);
    ASSERT_NE(result, nullptr);

    JsonValue root = result->GetRoot();
    ASSERT_TRUE(root.IsObject());
    EXPECT_TRUE(root.Has("primaryColor"));
    EXPECT_TRUE(root.Has("colorMode"));
    EXPECT_TRUE(root.Has("breakpoint"));
    EXPECT_FALSE(root.Has("darkPrimaryColor"));
    EXPECT_FALSE(root.Has("brandColor"));
    EXPECT_FALSE(root.Has("primaryColorArgb"));
    EXPECT_FALSE(root.Has("darkPrimaryColorArgb"));
    EXPECT_FALSE(root.Has("brandColorArgb"));
    EXPECT_FALSE(root.Has("iconUrl"));
    EXPECT_TRUE(root.Has("agentDisplayName"));
}

TEST_F(CustomComponentTest, should_cover_choice_picker_dynamic_descriptor_and_listener_edges)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent choice("ChoicePicker");
    auto missingValue = ParseJson(R"({"items":[]})");
    auto scalarValue = ParseJson(R"({"value":"plain"})");
    auto multiValue = ParseJson(R"({"value":[{"path":"/first"},{"path":"/second"}]})");
    ASSERT_NE(missingValue, nullptr);
    ASSERT_NE(scalarValue, nullptr);
    ASSERT_NE(multiValue, nullptr);

    choice.RegisterDataBindings(missingValue->GetRoot());
    EXPECT_TRUE(choice.GetDataBindings().empty());
    choice.RegisterDataBindings(scalarValue->GetRoot());
    EXPECT_TRUE(choice.GetDataBindings().empty());
    choice.RegisterDataBindings(multiValue->GetRoot());
    EXPECT_TRUE(choice.GetDataBindings().empty());

    CustomComponent dateTime("DateTimeInput");
    dateTime.SetComponentId("dateTime");
    dateTime.SetSurfaceId("surface-id");
    dateTime.SetRenderId(810);
    auto dateDescriptor = ParseJson(R"({
        "enableDate": {"path": "/date/enable"},
        "enableTime": {"path": "/date/time"},
        "min": {"path": "/date/min"},
        "max": {"path": "/date/max"}
    })");
    ASSERT_NE(dateDescriptor, nullptr);
    dateTime.ApplyCustomProperties(dateDescriptor->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "dateTime.enableDate"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "dateTime.enableTime"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "dateTime.min"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "dateTime.max"), 1U);

    CustomComponent picker("ChoicePicker");
    picker.SetComponentId("picker");
    picker.SetSurfaceId("surface-id");
    picker.SetRenderId(811);
    auto pickerDescriptor = ParseJson(R"({
        "variant": {"path": "/picker/variant"},
        "displayStyle": {"call": "resolveDisplayStyle"},
        "filterable": {"path": "/picker/filterable"}
    })");
    ASSERT_NE(pickerDescriptor, nullptr);
    picker.ApplyCustomProperties(pickerDescriptor->GetRoot());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "picker.variant"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "picker.displayStyle"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "picker.filterable"), 1U);

    picker.ParseListeners(JsonValue());
    EXPECT_TRUE(picker.eventHandlers_.empty());
}

TEST_F(CustomComponentTest, should_validate_checks_with_invalid_and_valid_default_target_values)
{
    CustomComponent component("Panel");
    component.SetComponentId("checksPanel");
    component.SetSurfaceId("checks-surface");
    component.SetRenderId(812);

    auto descriptor = ParseJson(R"({"checks":[{"condition":{"call":"required"},"message":"required"}]})");
    ASSERT_NE(descriptor, nullptr);
    component.ParseChecks(descriptor->GetRoot());

    std::string failedMessage;
    EXPECT_FALSE(component.ValidateChecks("", &failedMessage));
    EXPECT_FALSE(component.ValidateChecks(R"(["target"])", &failedMessage));
    EXPECT_FALSE(component.currentCheckTargetValue_.IsValid());
}

TEST_F(CustomComponentTest, should_cover_extended_common_style_dynamic_skip_and_invalid_boundaries)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponentProbe component("Web");
    component.SetComponentId("webMoreEdges");
    component.SetSurfaceId("surface-id");
    component.SetRenderId(813);

    auto descriptor = ParseJson(R"({
        "styles": {
            "width": {"path": "/styles/width"},
            "height": {"call": "resolveHeight"},
            "constraintSize": {
                "minWidth": {"path": "/styles/minWidth"},
                "maxWidth": [],
                "minHeight": {}
            },
            "backgroundImageSize": {},
            "backgroundimageSize": {"height": []},
            "margin": {"left": []},
            "padding": {"right": {}},
            "borderRadius": {"bottomRight": []},
            "borderWidth": {"all": []},
            "backgroundColor": [],
            "borderColor": {"top": []},
            "clip": [],
            "linearGradient": {
                "colors": {"path": "/styles/colors"},
                "angle": "",
                "repeating": []
            },
            "layoutWeight": -1,
            "flexShrink": -0.1,
            "shadow": {
                "radius": [],
                "offsetX": -2,
                "offsetY": 3,
                "color": "transparent",
                "fill": true
            },
            "visibility": {}
        }
    })");
    ASSERT_NE(descriptor, nullptr);
    component.ApplyCustomProperties(descriptor->GetRoot());

    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.constraintSize.maxWidth"),
        1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.constraintSize.minHeight"),
        1U);
    EXPECT_EQ(CountWarningRequests(
                  mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.backgroundimageSize.height"),
        1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.margin.left"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.padding.right"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.borderRadius.bottomRight"),
        1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.borderWidth.all"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.backgroundColor"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.borderColor.top"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.clip"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMoreEdges.styles.linearGradient.angle"), 1U);
    EXPECT_EQ(
        CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.linearGradient.repeating"),
        1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMoreEdges.styles.layoutWeight"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMoreEdges.styles.flexShrink"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "webMoreEdges.styles.shadow.radius"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "webMoreEdges.styles.visibility"), 1U);

    auto stylesOpt = component.GetProperty("styles");
    ASSERT_TRUE(stylesOpt.has_value());
    JsonValue styles = stylesOpt.value();
    ASSERT_TRUE(styles.IsObject());
    EXPECT_TRUE(styles.Has("width"));
    EXPECT_TRUE(styles.Has("height"));
    ASSERT_TRUE(styles.Has("backgroundImageSize"));
    EXPECT_EQ(styles.GetItem("backgroundImageSize").GetStringValue(""), "auto");
    EXPECT_TRUE(styles.Has("linearGradient"));
    EXPECT_TRUE(styles.GetItem("linearGradient").Has("colors"));
}

TEST_F(CustomComponentTest, should_cover_tabs_and_row_child_resolution_edge_paths)
{
    CustomComponentProbe tabs("Tabs");
    auto noTabs = ParseJson(R"({"children": []})");
    auto invalidTabs = ParseJson(R"({"tabs":[1, {"child": ""}]})");
    auto staticChildren = ParseJson(R"({"children":["child-a","child-b"]})");
    ASSERT_NE(noTabs, nullptr);
    ASSERT_NE(invalidTabs, nullptr);
    ASSERT_NE(staticChildren, nullptr);

    tabs.RegisterDataBindings(noTabs->GetRoot());
    tabs.RegisterDataBindings(invalidTabs->GetRoot());
    EXPECT_TRUE(tabs.GetDataBindings().empty());
    EXPECT_TRUE(tabs.ResolveTabsChildIds(invalidTabs->GetRoot()).empty());

    std::list<std::string> ids = tabs.ResolveTabsChildIds(staticChildren->GetRoot());
    ASSERT_EQ(ids.size(), 2U);
    EXPECT_EQ(ids.front(), "child-a");

    CustomComponentProbe row("Extended.Row");
    auto invalidRowChildren = ParseJson(R"({"children": 12})");
    ASSERT_NE(invalidRowChildren, nullptr);
    EXPECT_TRUE(row.ResolveRowChildIds(invalidRowChildren->GetRoot()).empty());
}

TEST_F(CustomComponentTest, should_cover_template_child_resolution_empty_missing_and_non_array_paths)
{
    CustomComponentProbe component("Tabs");
    EXPECT_TRUE(component.ResolveTemplateChildIds("", "/items").empty());
    EXPECT_TRUE(component.ResolveTemplateChildIds("tpl", "").empty());

    component.SetSurfaceId("surface-missing-template");
    EXPECT_TRUE(component.ResolveTemplateChildIds("tpl", "/items").empty());

    constexpr int32_t renderId = 814;
    const std::string surfaceId = "surface-template-non-array";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto objectData = ParseJson(R"({"name":"not-array"})");
    ASSERT_NE(objectData, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/items", objectData->GetRoot());

    component.SetSurfaceId(surfaceId);
    EXPECT_TRUE(component.ResolveTemplateChildIds("tpl", "/missing").empty());
    EXPECT_TRUE(component.ResolveTemplateChildIds("tpl", "/items").empty());
}

TEST_F(CustomComponentTest, should_create_and_update_custom_component_with_legacy_child_slots_key)
{
    napi_env env = reinterpret_cast<napi_env>(0x8200);
    napi_value createFunction = CreateManualNapiFunction(mockNapiPtr_, 8201);
    napi_value updateFunction = CreateManualNapiFunction(mockNapiPtr_, 8202);
    NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);

    napi_value content = CreateManualNapiObject(mockNapiPtr_, 8210);
    napi_value legacySlots = CreateManualNapiObject(mockNapiPtr_, 8211);
    napi_value tab0 = CreateManualNapiObject(mockNapiPtr_, 8212);
    mockNapiPtr_->objectProperties_[legacySlots]["tab-0"] = tab0;
    for (int32_t id = 8220; id <= 8420; ++id) {
        napi_value createResult = CreateManualNapiObject(mockNapiPtr_, id);
        mockNapiPtr_->objectProperties_[createResult]["content"] = content;
        mockNapiPtr_->objectProperties_[createResult]["childSlots"] = legacySlots;
    }
    mockNapiPtr_->nextValueId_ = 8220;
    mockArkUIPtr_->SetNodeHandleResult(reinterpret_cast<ArkUI_NodeHandle>(0x8221));
    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x8222));

    CustomComponent component("Tabs");
    component.SetComponentId("tabsLegacySlots");
    component.SetSurfaceId("surface-legacy-slots");
    component.SetRenderId(820);

    EXPECT_TRUE(component.CreateCustomComponent());
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());

    component.childSlotRef_ = nullptr;
    component.childSlotRefs_.clear();
    component.childSlotHandles_.clear();
    for (int32_t id = 8430; id <= 8630; ++id) {
        napi_value updateResult = CreateManualNapiObject(mockNapiPtr_, id);
        mockNapiPtr_->objectProperties_[updateResult]["childSlots"] = legacySlots;
    }
    mockNapiPtr_->nextValueId_ = 8430;
    component.UpdateCustomComponent();
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());
}

TEST_F(CustomComponentTest, should_dispatch_dynamic_values_from_surface_and_report_callback_failures)
{
    constexpr int32_t renderId = 824;
    const std::string surfaceId = "surface-dynamic-values";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);
    auto titleData = JsonAdapter::CreateString("current-title");
    ASSERT_NE(titleData, nullptr);
    surface.GetBindingEngine()->UpdateDataModelByPath(surfaceId, "/form/title", titleData->GetRoot());

    napi_env env = reinterpret_cast<napi_env>(0x8240);
    napi_value callback = nullptr;
    ASSERT_EQ(
        mockNapiPtr_->CreateFunction(env, "dynamicCallback", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback), napi_ok);

    CustomComponent component("Panel");
    component.SetComponentId("dynamicPanel");
    component.SetSurfaceId(surfaceId);
    component.SetRenderId(renderId);

    std::string errorMessage;
    auto validPath = ParseJson(R"({"path":"/form/title"})");
    auto missingPath = ParseJson(R"({"path":"/form/missing"})");
    auto missingCall = ParseJson(R"({"call":"missingFunction"})");
    auto literalValue = JsonAdapter::CreateString("resolved-now");
    ASSERT_NE(validPath, nullptr);
    ASSERT_NE(missingPath, nullptr);
    ASSERT_NE(missingCall, nullptr);
    ASSERT_NE(literalValue, nullptr);

    EXPECT_TRUE(component.RegisterDynamicValueCallback("title", validPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_GE(mockNapiPtr_->callFunctionCallCount_, 1U);

    EXPECT_TRUE(
        component.RegisterDynamicValueCallback("missing", missingPath->GetRoot(), env, callback, &errorMessage));
    EXPECT_TRUE(component.dynamicValueCallbacks_.find("missing") != component.dynamicValueCallbacks_.end());

    EXPECT_FALSE(
        component.RegisterDynamicValueCallback("callMissing", missingCall->GetRoot(), env, callback, &errorMessage));
    EXPECT_FALSE(errorMessage.empty());

    mockNapiPtr_->SetGetGlobalStatus(napi_generic_failure);
    EXPECT_FALSE(
        component.RegisterDynamicValueCallback("literalFail", literalValue->GetRoot(), env, callback, &errorMessage));
    EXPECT_EQ(errorMessage, "failed to dispatch resolved value");
    mockNapiPtr_->ResetGetGlobalStatus();

    component.properties_["invalidClone"] = JsonValue();
    EXPECT_FALSE(component.GetCustomProperty("invalidClone").IsValid());
}

TEST_F(CustomComponentTest, should_sync_checked_value_to_bound_data_model_when_surface_exists)
{
    CustomComponent component("Panel");
    component.SyncCheckedToBoundDataModel("", true);
    component.SyncCheckedToBoundDataModel("/form/checked", true);

    constexpr int32_t renderId = 825;
    const std::string surfaceId = "surface-checked-sync";
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& surface = renderSlot.GetSurfaceManager()->CreateSurface(surfaceId, nullptr);

    component.SetSurfaceId(surfaceId);
    component.SetRenderId(renderId);
    component.SyncCheckedToBoundDataModel("/form/checked", true);

    std::shared_ptr<DataModel> dataModel = surface.GetBindingEngine()->GetOrCreateDataModel(surfaceId);
    ASSERT_NE(dataModel, nullptr);
    std::optional<JsonValue> checkedValue = dataModel->GetNode("/form/checked");
    ASSERT_TRUE(checkedValue.has_value());
    EXPECT_TRUE(checkedValue.value().GetBoolValue(false));
}

TEST_F(CustomComponentTest, should_remove_stale_custom_properties_and_skip_empty_binding_names)
{
    CustomComponentProbe component("Panel");
    auto firstDescriptor = ParseJson(R"({"customA":"a","customB":"b"})");
    auto secondDescriptor = ParseJson(R"({"customA":"new"})");
    ASSERT_NE(firstDescriptor, nullptr);
    ASSERT_NE(secondDescriptor, nullptr);

    component.ApplyCustomProperties(firstDescriptor->GetRoot());
    EXPECT_TRUE(component.GetProperty("customB").has_value());

    component.ApplyCustomProperties(secondDescriptor->GetRoot());
    EXPECT_FALSE(component.GetProperty("customB").has_value());
    ASSERT_TRUE(component.GetProperty("customA").has_value());
    EXPECT_EQ(component.GetProperty("customA")->GetStringValue(""), "new");

    component.AddBinding("", "/ignored");
    component.AddBinding("valid", "/value");
    JsonValue props = component.BuildCustomProps();
    ASSERT_TRUE(props.IsObject());
    ASSERT_TRUE(props.Has("__a2uiBindings"));
    EXPECT_FALSE(props.GetItem("__a2uiBindings").Has(""));
    EXPECT_EQ(props.GetItem("__a2uiBindings").GetString("valid", ""), "/value");
}

TEST_F(CustomComponentTest, should_resolve_template_children_from_default_surface_and_extended_tabs_children)
{
    constexpr int32_t renderId = 826;
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    SurfaceSlot& defaultSurface = renderSlot.GetSurfaceManager()->CreateSurface("default", nullptr);
    auto arrayData = ParseJson(R"([{"name":"one"},{"name":"two"}])");
    ASSERT_NE(arrayData, nullptr);
    defaultSurface.GetBindingEngine()->UpdateDataModelByPath("default", "/items", arrayData->GetRoot());

    CustomComponentProbe templateComponent("Tabs");
    std::list<std::string> generated = templateComponent.ResolveTemplateChildIds("tpl", "/items");
    ASSERT_EQ(generated.size(), 2U);
    EXPECT_EQ(generated.front(), "/itemstpl:0:tpl");

    CustomComponentProbe extendedTabs("Tabs");
    SurfaceContext surfaceContext;
    surfaceContext.catalogId = A2UI_EXTENDED_CATALOG_ID;
    extendedTabs.SetSurfaceContext(surfaceContext);
    auto staticChildren = ParseJson(R"({"children":["tab-a","tab-b"]})");
    auto templateChildren = ParseJson(R"({"children":{"componentId":"tpl","path":"/items"}})");
    ASSERT_NE(staticChildren, nullptr);
    ASSERT_NE(templateChildren, nullptr);
    std::list<std::string> childIds = extendedTabs.ResolveTabsChildIds(staticChildren->GetRoot());
    ASSERT_EQ(childIds.size(), 2U);
    EXPECT_EQ(childIds.front(), "tab-a");

    extendedTabs.SetSurfaceId("default");
    std::list<std::string> templateChildIds = extendedTabs.ResolveTabsChildIds(templateChildren->GetRoot());
    ASSERT_EQ(templateChildIds.size(), 2U);
    EXPECT_EQ(templateChildIds.front(), "/itemstpl:0:tpl");
}

TEST_F(CustomComponentTest, should_cover_tabs_prop_helpers_and_function_call_resolution_failures)
{
    CustomComponentProbe component("Tabs");
    auto tabsArrayAdapter = ParseJson(R"([1])");
    ASSERT_NE(tabsArrayAdapter, nullptr);
    JsonValue tabsArray = tabsArrayAdapter->GetRoot();

    std::map<std::string, JsonValue> malformedProperties;
    malformedProperties["tabs.title"] = CreateStringValue("ignored");
    malformedProperties["tabs[0].title"] = CreateStringValue("ignored-on-non-object");
    component.UpdateTabsWithProperties(tabsArray, malformedProperties);
    EXPECT_EQ(tabsArray.GetArraySize(), 1);
    component.ResolveFunctionCallsInTabsArray(tabsArray);

    JsonValue invalidTabsArray;
    component.UpdateTabsWithProperties(invalidTabsArray, malformedProperties);
    component.ResolveFunctionCallsInTabsArray(invalidTabsArray);

    auto noObjectTitle = ParseJson(R"([{"title":"Home"}])");
    auto missingCallString = ParseJson(R"([{"title":{"path":"/title"}}])");
    auto unresolvedCall = ParseJson(R"([{"title":{"call":"missingTitle"}}])");
    ASSERT_NE(noObjectTitle, nullptr);
    ASSERT_NE(missingCallString, nullptr);
    ASSERT_NE(unresolvedCall, nullptr);
    JsonValue noObjectTitleRoot = noObjectTitle->GetRoot();
    JsonValue missingCallStringRoot = missingCallString->GetRoot();
    JsonValue unresolvedCallRoot = unresolvedCall->GetRoot();
    component.ResolveFunctionCallsInTabsArray(noObjectTitleRoot);
    component.ResolveFunctionCallsInTabsArray(missingCallStringRoot);
    component.ResolveFunctionCallsInTabsArray(unresolvedCallRoot);

    std::ostringstream builder;
    bool hasCustomProp = false;
    component.AddTabsToBuilder(builder, tabsArray, hasCustomProp);
    component.AddOtherPropertiesToBuilder(builder, malformedProperties, hasCustomProp);
    EXPECT_FALSE(hasCustomProp);
}

TEST_F(CustomComponentTest, should_return_from_update_when_referenced_content_is_missing)
{
    napi_env env = reinterpret_cast<napi_env>(0x8270);
    napi_value createFunction = CreateManualNapiFunction(mockNapiPtr_, 8271);
    napi_value updateFunction = CreateManualNapiFunction(mockNapiPtr_, 8272);
    NapiResourceManagerRefGuard callbackGuard(mockNapiPtr_, env, createFunction, updateFunction);

    CustomComponent component("Panel");
    component.env_ = env;
    component.componentContentRef_ = reinterpret_cast<napi_ref>(0x8273);

    component.UpdateCustomComponent();

    EXPECT_TRUE(component.childSlotHandles_.empty());
}

TEST_F(CustomComponentTest, should_cover_sync_child_slots_type_guard_and_existing_ref_path)
{
    napi_env env = reinterpret_cast<napi_env>(0x8280);
    CustomComponent component("Tabs");
    component.env_ = env;

    napi_value notObject = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateStringUtf8(env, "not-object", NAPI_AUTO_LENGTH, &notObject), napi_ok);
    component.SyncChildSlots(notObject);
    EXPECT_TRUE(component.childSlotHandles_.empty());

    napi_value childSlots = CreateManualNapiObject(mockNapiPtr_, 8281);
    napi_value slotValue = CreateManualNapiObject(mockNapiPtr_, 8282);
    mockNapiPtr_->objectProperties_[childSlots]["tab-0"] = slotValue;
    napi_ref existingRef = nullptr;
    ASSERT_EQ(mockNapiPtr_->CreateReference(env, slotValue, 1, &existingRef), napi_ok);
    component.childSlotRefs_["tab-0"] = existingRef;
    mockArkUIPtr_->SetNodeContentHandleResult(reinterpret_cast<ArkUI_NodeContentHandle>(0x8283));

    component.SyncChildSlots(childSlots);

    EXPECT_EQ(component.childSlotRefs_["tab-0"], existingRef);
    EXPECT_TRUE(component.childSlotHandles_.find("tab-0") != component.childSlotHandles_.end());
    mockArkUIPtr_->ResetNodeContentHandleResult();
}

TEST_F(CustomComponentTest, should_cover_sync_dynamic_bindings_default_surface_bad_weak_ptr)
{
    constexpr int32_t renderId = 829;
    RenderSlotCleanupGuard cleanupGuard(renderId);
    RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
    renderSlot.GetSurfaceManager()->CreateSurface("default", nullptr);

    CustomComponent component("Panel");
    component.SyncDynamicValueBindings();
}

TEST_F(CustomComponentTest, should_dispatch_event_entrypoint_with_no_registered_handlers)
{
    CustomComponent component("Panel");
    component.SetSurfaceId("surface-event");
    component.SetComponentId("eventPanel");
    auto extraContext = JsonAdapter::CreateString("payload");
    ASSERT_NE(extraContext, nullptr);

    component.DispatchEvent("onClick", extraContext->GetRoot());
}

// =============================================================================
// Issue #85: Width / Height schema validation in ApplyCommonAttributes
// =============================================================================

// ---------------------------------------------------------------------------
// Group 1: Valid width / height values
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_accept_valid_numeric_width_and_height_in_common_attributes)
{
    CustomComponent component("Panel");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "width": 200,
            "height": 150
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCommonAttributes(descriptor->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 200.0);
    EXPECT_TRUE(component.descriptor_.properties.hasHeight);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.height, 150.0);
    EXPECT_FALSE(component.descriptor_.properties.size.empty());
}

TEST_F(CustomComponentTest, should_accept_valid_string_dimension_width_with_vp_fp_percent_units)
{
    CustomComponent component("Panel");

    // "100vp"
    {
        auto desc = ParseJson(R"({"width": "100vp"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent comp("Panel");
        comp.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(comp.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(comp.descriptor_.properties.width, 100.0);
    }
    // "50fp"
    {
        auto desc = ParseJson(R"({"width": "50fp"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent comp("Panel");
        comp.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(comp.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(comp.descriptor_.properties.width, 50.0);
    }
    // "75%"
    {
        auto desc = ParseJson(R"({"width": "75%"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent comp("Panel");
        comp.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(comp.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(comp.descriptor_.properties.width, 75.0);
    }
}

TEST_F(CustomComponentTest, should_accept_keyword_dimension_width_values)
{
    // "matchParent"
    {
        auto desc = ParseJson(R"({"width": "matchParent"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent component("Panel");
        component.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(component.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 100.0);
    }
    // "wrapContent"
    {
        auto desc = ParseJson(R"({"width": "wrapContent"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent component("Panel");
        component.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(component.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 0.0);
    }
    // "fixAtIdealSize"
    {
        auto desc = ParseJson(R"({"width": "fixAtIdealSize"})");
        ASSERT_NE(desc, nullptr);
        CustomComponent component("Panel");
        component.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_TRUE(component.descriptor_.properties.hasWidth);
        EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 0.0);
    }
}

TEST_F(CustomComponentTest, should_accept_zero_width_and_height_as_valid_boundary_values)
{
    CustomComponent component("Panel");
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(
        R"({
            "width": 0,
            "height": 0
        })");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyCommonAttributes(descriptor->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 0.0);
    EXPECT_TRUE(component.descriptor_.properties.hasHeight);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.height, 0.0);
}

TEST_F(CustomComponentTest, should_set_only_width_when_only_width_is_provided)
{
    CustomComponent component("Panel");
    auto desc = ParseJson(R"({"width": 300})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 300.0);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.height, 0.0);
}

TEST_F(CustomComponentTest, should_set_only_height_when_only_height_is_provided)
{
    CustomComponent component("Panel");
    auto desc = ParseJson(R"({"height": "40vp"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_TRUE(component.descriptor_.properties.hasHeight);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.height, 40.0);
}

// ---------------------------------------------------------------------------
// Group 2: Invalid values — schema warnings dispatched
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_dispatch_invalid_value_warning_for_negative_width_and_height)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("negSize");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8501);

    auto desc = ParseJson(R"({"width": -100, "height": -50})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "negSize.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "negSize.height"), 1U);
}

TEST_F(CustomComponentTest, should_dispatch_invalid_value_warning_for_unsupported_string_unit_px)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("pxUnit");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8502);

    auto desc = ParseJson(R"({"width": "100px", "height": "200px"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "pxUnit.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "pxUnit.height"), 1U);
}

TEST_F(CustomComponentTest, should_dispatch_invalid_value_warning_for_unparseable_string_dimension)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("badStr");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8503);

    auto desc = ParseJson(R"({"width": "abc", "height": ""})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "badStr.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "badStr.height"), 1U);
}

TEST_F(CustomComponentTest, should_dispatch_type_mismatch_warning_for_boolean_width_and_height)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("boolDim");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8504);

    auto desc = ParseJson(R"({"width": true, "height": false})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "boolDim.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "boolDim.height"), 1U);
}

TEST_F(CustomComponentTest, should_dispatch_type_mismatch_warning_for_array_and_object_width)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    // Array width
    {
        auto desc = ParseJson(R"({"width": [1, 2, 3]})");
        ASSERT_NE(desc, nullptr);
        CustomComponent comp("Panel");
        comp.SetComponentId("arrObj");
        comp.SetSurfaceId("surface-85");
        comp.SetRenderId(8505);
        comp.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_FALSE(comp.descriptor_.properties.hasWidth);
    }
    // Plain object width (non-dynamic — no "path" or "call" key)
    {
        auto desc = ParseJson(R"({"width": {"value": 100}})");
        ASSERT_NE(desc, nullptr);
        CustomComponent comp("Panel");
        comp.SetComponentId("arrObj");
        comp.SetSurfaceId("surface-85");
        comp.SetRenderId(8505);
        comp.ApplyCommonAttributes(desc->GetRoot());
        EXPECT_FALSE(comp.descriptor_.properties.hasWidth);
    }

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "arrObj.width"), 2U);
}

TEST_F(CustomComponentTest, should_dispatch_invalid_value_warning_for_negative_float_string_dimension)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("negFloat");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8506);

    // "-10vp" is parsed by strtof → -10.0, then checked < 0 → rejected
    auto desc = ParseJson(R"({"width": "-10vp"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "negFloat.width"), 1U);
}

// ---------------------------------------------------------------------------
// Group 3: Dynamic / expression values — skipped (no validation)
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_skip_validation_for_dynamic_descriptor_width_and_height)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("dynDim");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8507);

    auto desc = ParseJson(
        R"({
            "width": {"path": "data.dynamicWidth"},
            "height": {"call": "computeHeight"}
        })");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    // Dynamic values are skipped — hasWidth/hasHeight should NOT be set.
    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    // No warnings should be dispatched for dynamic values.
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

TEST_F(CustomComponentTest, should_skip_validation_for_expression_string_width_and_height)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("exprDim");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8508);

    auto desc = ParseJson(
        R"({
            "width": "{{data.boundWidth}}",
            "height": "{{data.boundHeight}}"
        })");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    // Expression strings are skipped — hasWidth/hasHeight should NOT be set.
    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    // No warnings should be dispatched for expression values.
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0U);
}

// ---------------------------------------------------------------------------
// Group 4: Mixed validity — partial application
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_apply_valid_width_and_reject_invalid_height_together)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("mixedVal");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8509);

    auto desc = ParseJson(R"({"width": 200, "height": "notValidHeight"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    // Valid width is applied.
    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_DOUBLE_EQ(component.descriptor_.properties.width, 200.0);
    // Invalid height is rejected.
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    // Warning only for height.
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "mixedVal.height"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "mixedVal.width"), 0U);
}

TEST_F(CustomComponentTest, should_reject_both_when_width_and_height_are_invalid)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    CustomComponent component("Panel");
    component.SetComponentId("bothBad");
    component.SetSurfaceId("surface-85");
    component.SetRenderId(8510);

    auto desc = ParseJson(R"({"width": true, "height": []})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_TRUE(component.descriptor_.properties.size.empty());
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "bothBad.width"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, "ERROR_CODE_TYPE_MISMATCH", "bothBad.height"), 1U);
}

// ---------------------------------------------------------------------------
// Group 5: Size JSON string construction
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_build_size_json_string_from_validated_width_and_height)
{
    CustomComponent component("Panel");
    auto desc = ParseJson(R"({"width": 320, "height": 240})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_TRUE(component.descriptor_.properties.hasHeight);
    EXPECT_FALSE(component.descriptor_.properties.size.empty());
    // The size string should contain both width and height.
    std::string size = component.descriptor_.properties.size;
    EXPECT_NE(size.find("\"width\":320"), std::string::npos);
    EXPECT_NE(size.find("\"height\":240"), std::string::npos);
}

TEST_F(CustomComponentTest, should_not_build_size_json_when_width_and_height_are_absent)
{
    CustomComponent component("Panel");
    auto desc = ParseJson(R"({"weight": 2})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_TRUE(component.descriptor_.properties.size.empty());
}

TEST_F(CustomComponentTest, should_build_size_json_with_only_width_when_height_is_absent)
{
    CustomComponent component("Panel");
    auto desc = ParseJson(R"({"width": "200vp"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    EXPECT_TRUE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    EXPECT_FALSE(component.descriptor_.properties.size.empty());
    std::string size = component.descriptor_.properties.size;
    EXPECT_NE(size.find("\"width\":200"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Group 6: No warnings when renderId is not set (guard clause coverage)
// ---------------------------------------------------------------------------

TEST_F(CustomComponentTest, should_not_dispatch_warnings_when_render_id_is_negative)
{
    // No RegisterDispatchCallbacks — renderId_ defaults to -1
    CustomComponent component("Panel");
    // Do NOT set renderId — leave it at the default (-1).

    auto desc = ParseJson(R"({"width": -999, "height": "badPx"})");
    ASSERT_NE(desc, nullptr);

    component.ApplyCommonAttributes(desc->GetRoot());

    // hasWidth/hasHeight are still correctly false.
    EXPECT_FALSE(component.descriptor_.properties.hasWidth);
    EXPECT_FALSE(component.descriptor_.properties.hasHeight);
    // But no warnings were dispatched because renderId_ < 0 triggers the early return.
}

} // namespace
