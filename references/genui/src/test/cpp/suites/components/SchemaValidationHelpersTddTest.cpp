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
#include <initializer_list>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/ChildListSchemaValidationUtils.h"
#include "components/custom/ExtendedTabsPrebuildHelper.h"
#include "functions/WarningDispatchBridge.h"
#include "utils/RequiredStringPropertyUtils.h"

#include "SchemaErrorCodes.h"
#include "SurfaceSlotSchemaValidation.h"
#include "TestFixture.h"

#define private public
#define protected public
#include "SurfaceSlot.h"
#undef protected
#undef private

using namespace NativeModule;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

std::shared_ptr<Catalog> BuildCatalog(
    const std::string& catalogId, std::initializer_list<std::pair<const char*, bool>> components)
{
    auto catalog = std::make_shared<Catalog>(catalogId);
    for (const auto& entry : components) {
        if (entry.first == nullptr || entry.first[0] == '\0') {
            continue;
        }
        auto item = std::make_shared<CatalogItem>(entry.first, CatalogItemType::COMPONENT);
        item->SetInnerNative(entry.second);
        catalog->AddComponent(item);
    }
    return catalog;
}

void RegisterDispatchCallbacks(MockNapiProvider* mockNapi)
{
    ASSERT_NE(mockNapi, nullptr);
    napi_env env = reinterpret_cast<napi_env>(0x1501);
    napi_value warningCallback = nullptr;
    mockNapi->CreateFunction(env, "dispatchWarning", NAPI_AUTO_LENGTH, nullptr, nullptr, &warningCallback);
    WarningDispatchBridge::GetInstance().RegisterDispatchWarning(env, warningCallback);
    mockNapi->callFunctionCallCount_ = 0;
    mockNapi->lastCallFunctionRecv_ = nullptr;
    mockNapi->lastCallFunctionFunc_ = nullptr;
    mockNapi->lastCallFunctionArgs_.clear();
    mockNapi->callFunctionArgsHistory_.clear();
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
    auto propertyIt = objectIt->second.find(key);
    return propertyIt == objectIt->second.end() ? nullptr : propertyIt->second;
}

std::string GetStringValue(const MockNapiProvider* mockNapi, napi_value value)
{
    if (mockNapi == nullptr || value == nullptr) {
        return "";
    }
    auto it = mockNapi->stringValues_.find(value);
    return it == mockNapi->stringValues_.end() ? "" : it->second;
}

napi_value GetLastWarningRequest(const MockNapiProvider* mockNapi)
{
    if (mockNapi == nullptr || mockNapi->callFunctionArgsHistory_.empty()) {
        return nullptr;
    }
    const auto& args = mockNapi->callFunctionArgsHistory_.back();
    return args.empty() ? nullptr : args.front();
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
        std::string warningCode = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "code"));
        std::string warningPath = GetStringValue(mockNapi, GetRequestProperty(mockNapi, request, "path"));
        if (warningCode == code && warningPath.find(pathFragment) != std::string::npos) {
            ++count;
        }
    }
    return count;
}

class SchemaValidationHelpersTddTest : public A2UITest {};

} // namespace

TEST_F(SchemaValidationHelpersTddTest, should_classify_required_string_property_states)
{
    std::unique_ptr<JsonAdapter> descriptor = ParseJson(R"({
        "valid": "alpha",
        "empty": "",
        "number": 1
    })");
    ASSERT_NE(descriptor, nullptr);

    EXPECT_EQ(GetRequiredStringPropertyState(descriptor->GetRoot(), "valid"), RequiredStringPropertyState::VALID);
    EXPECT_EQ(GetRequiredStringPropertyState(descriptor->GetRoot(), "empty"), RequiredStringPropertyState::EMPTY);
    EXPECT_EQ(
        GetRequiredStringPropertyState(descriptor->GetRoot(), "number"), RequiredStringPropertyState::TYPE_MISMATCH);
    EXPECT_EQ(GetRequiredStringPropertyState(descriptor->GetRoot(), "missing"), RequiredStringPropertyState::MISSING);
    EXPECT_EQ(GetRequiredStringPropertyState(descriptor->GetRoot(), nullptr), RequiredStringPropertyState::MISSING);
    EXPECT_EQ(GetRequiredStringPropertyState(JsonValue(), "valid"), RequiredStringPropertyState::MISSING);
}

TEST_F(SchemaValidationHelpersTddTest, should_validate_child_list_schema_for_array_and_template_forms)
{
    EXPECT_TRUE(ValidateChildListSchema(JsonValue(), "children", ChildListEmptyArrayPolicy::ALLOW).empty());

    std::unique_ptr<JsonAdapter> typeMismatchDescriptor = ParseJson(R"({"children": 1})");
    ASSERT_NE(typeMismatchDescriptor, nullptr);
    std::vector<SchemaValidationIssue> mismatchIssues =
        ValidateChildListSchema(typeMismatchDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(mismatchIssues.size(), 1U);
    EXPECT_EQ(mismatchIssues[0].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(mismatchIssues[0].propertyPath, "children");

    std::unique_ptr<JsonAdapter> arrayDescriptor = ParseJson(R"({"children": ["first", "", 2]})");
    ASSERT_NE(arrayDescriptor, nullptr);
    std::vector<SchemaValidationIssue> arrayIssues =
        ValidateChildListSchema(arrayDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(arrayIssues.size(), 2U);
    EXPECT_EQ(arrayIssues[0].code, SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(arrayIssues[0].propertyPath, "children[1]");
    EXPECT_EQ(arrayIssues[1].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(arrayIssues[1].propertyPath, "children[2]");

    std::unique_ptr<JsonAdapter> emptyArrayDescriptor = ParseJson(R"({"children": []})");
    ASSERT_NE(emptyArrayDescriptor, nullptr);
    EXPECT_TRUE(
        ValidateChildListSchema(emptyArrayDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW).empty());
    std::vector<SchemaValidationIssue> emptyArrayIssues = ValidateChildListSchema(
        emptyArrayDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::WARN_INVALID_VALUE);
    ASSERT_EQ(emptyArrayIssues.size(), 1U);
    EXPECT_EQ(emptyArrayIssues[0].code, SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(emptyArrayIssues[0].propertyPath, "children");

    std::unique_ptr<JsonAdapter> templateDescriptor = ParseJson(R"({
        "children": {
            "componentId": 1
        }
    })");
    ASSERT_NE(templateDescriptor, nullptr);
    std::vector<SchemaValidationIssue> templateIssues =
        ValidateChildListSchema(templateDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(templateIssues.size(), 2U);
    EXPECT_EQ(templateIssues[0].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(templateIssues[0].propertyPath, "children.componentId");
    EXPECT_EQ(templateIssues[1].code, SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(templateIssues[1].propertyPath, "children.path");
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_schema_warning_requests_with_expected_paths)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    DispatchComponentSchemaWarning(
        -1, "surface", "root", "Column", SCHEMA_ERROR_CODE_REQUIRED_MISS, "ignored", "children");
    EXPECT_TRUE(mockNapiPtr_->callFunctionArgsHistory_.empty());

    DispatchComponentSchemaWarning(1, "surface", "", "", SCHEMA_ERROR_CODE_REQUIRED_MISS, "id required", "id");
    ASSERT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 1U);
    napi_value firstRequest = GetLastWarningRequest(mockNapiPtr_);
    ASSERT_NE(firstRequest, nullptr);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, firstRequest, "path")), "id");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, firstRequest, "itemType")), "component");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, firstRequest, "itemName")), "unknown");

    DispatchComponentSchemaWarning(1, "surface", "root", "Column", SCHEMA_ERROR_CODE_INVALID_VALUE, "invalid", "");
    napi_value secondRequest = GetLastWarningRequest(mockNapiPtr_);
    ASSERT_NE(secondRequest, nullptr);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, secondRequest, "path")), "root");

    DispatchComponentSchemaWarning(
        1, "surface", "root", "Column", SCHEMA_ERROR_CODE_TYPE_MISMATCH, "wrong type", "children[0]");
    napi_value thirdRequest = GetLastWarningRequest(mockNapiPtr_);
    ASSERT_NE(thirdRequest, nullptr);
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, thirdRequest, "path")), "root.children[0]");
    EXPECT_EQ(GetStringValue(mockNapiPtr_, GetRequestProperty(mockNapiPtr_, thirdRequest, "code")),
        SCHEMA_ERROR_CODE_TYPE_MISMATCH);
}

TEST_F(SchemaValidationHelpersTddTest, should_validate_descriptor_and_creation_fields)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    std::unique_ptr<JsonAdapter> validDescriptor = ParseJson(R"({"id":"root","component":"Column"})");
    std::unique_ptr<JsonAdapter> missingIdDescriptor = ParseJson(R"({"component":"Column"})");
    std::unique_ptr<JsonAdapter> mismatchIdDescriptor = ParseJson(R"({"id":1,"component":"Column"})");
    std::unique_ptr<JsonAdapter> creationDescriptor = ParseJson(R"({"id":1,"component":2})");
    ASSERT_NE(validDescriptor, nullptr);
    ASSERT_NE(missingIdDescriptor, nullptr);
    ASSERT_NE(mismatchIdDescriptor, nullptr);
    ASSERT_NE(creationDescriptor, nullptr);

    EXPECT_TRUE(ValidateDescriptorIdForPreparation(validDescriptor->GetRoot(), 2, "surface"));
    EXPECT_FALSE(ValidateDescriptorIdForPreparation(missingIdDescriptor->GetRoot(), 2, "surface"));
    EXPECT_FALSE(ValidateDescriptorIdForPreparation(mismatchIdDescriptor->GetRoot(), 2, "surface"));

    ValidateRequiredCreationFields(creationDescriptor->GetRoot(), 2, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "id"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "id"), 2U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "component"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_validate_required_structural_fields_and_shapes)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    std::unique_ptr<JsonAdapter> missingButtonChild = ParseJson(R"({"id":"btn","component":"Button"})");
    std::unique_ptr<JsonAdapter> emptyButtonChild = ParseJson(R"({"id":"btn2","component":"Button","child":""})");
    std::unique_ptr<JsonAdapter> missingColumnChildren = ParseJson(R"({"id":"col","component":"Column"})");
    std::unique_ptr<JsonAdapter> invalidColumnChildren = ParseJson(R"({"id":"row","component":"Row","children":[1]})");
    std::unique_ptr<JsonAdapter> emptyListChildren = ParseJson(R"({"id":"list","component":"List","children":[]})");
    std::unique_ptr<JsonAdapter> extendedColumnChildren = ParseJson(R"({"id":"ext","component":"Column"})");
    ASSERT_NE(missingButtonChild, nullptr);
    ASSERT_NE(emptyButtonChild, nullptr);
    ASSERT_NE(missingColumnChildren, nullptr);
    ASSERT_NE(invalidColumnChildren, nullptr);
    ASSERT_NE(emptyListChildren, nullptr);
    ASSERT_NE(extendedColumnChildren, nullptr);

    ValidateRequiredStructuralFields(missingButtonChild->GetRoot(), 3, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(emptyButtonChild->GetRoot(), 3, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(
        missingColumnChildren->GetRoot(), 3, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(
        extendedColumnChildren->GetRoot(), 3, "surface", SurfaceProtocolMode::EXTENDED_PROTOCOL);

    ValidateStructuralFieldShapes(invalidColumnChildren->GetRoot(), 3, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateStructuralFieldShapes(emptyListChildren->GetRoot(), 3, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateStructuralFieldShapes(
        invalidColumnChildren->GetRoot(), 3, "surface", SurfaceProtocolMode::EXTENDED_PROTOCOL);

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.child"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn2.child"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "col.children"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "row.children[0]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "list.children"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "ext.children"), 0U);
}

// ===== EventHandler DFX: 通过 AppendIssue → DispatchComponentSchemaWarning → WarningDispatchBridge 链路 =====

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_for_empty_handler_object)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            { "call": "fn1" },
            {},
            { "call": "fn2" }
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.onClick[1].call"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.onClick[0]"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.onClick[2]"), 0U);
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_for_handler_with_condition_but_no_call)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            { "call": "fn1" },
            { "condition": "{{ true }}" },
            { "call": "fn2" }
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.onClick[1].call"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_for_handler_with_args_but_no_call)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            { "args": { "componentId": "s1" } }
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "btn.onClick[0].call"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_for_empty_call_string)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            { "call": "" }
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "btn.onClick[0].call"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_for_non_object_handler)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            "notAnObject",
            123,
            null
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "btn.onClick[0]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "btn.onClick[1]"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "btn.onClick[2]"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_dispatch_warning_when_event_property_is_not_array)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": { "call": "fn1" }
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "btn.onClick"), 1U);
}

TEST_F(SchemaValidationHelpersTddTest, should_not_dispatch_warning_for_valid_event_handlers)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    auto descriptor = ParseJson(R"({
        "id": "btn",
        "component": "Button",
        "onClick": [
            { "call": "setAttributes", "args": { "componentId": "s1" } },
            { "call": "setAttributes", "args": { "componentId": "s2" }, "condition": "{{ true }}", "as": "result" }
        ],
        "onAppear": [
            { "call": "fn1" }
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(mockNapiPtr_->callFunctionArgsHistory_.size(), 0U);
}

TEST_F(SchemaValidationHelpersTddTest, should_cover_extended_tabs_helper_parsing_and_merge_branches)
{
    EXPECT_TRUE(IsExtendedTabsComponentType("Extended.Tabs"));
    EXPECT_TRUE(IsExtendedTabsChildComponentType("Extended.Tabs"));
    EXPECT_TRUE(IsExtendedTabsChildComponentType("Extended.TabContent"));
    EXPECT_TRUE(IsExtendedTabsChildComponentType("TabContent"));
    EXPECT_FALSE(IsExtendedTabsChildComponentType("Column"));

    std::unique_ptr<JsonAdapter> staticDescriptor = ParseJson(R"({
        "children": ["tabHome", "tabOrders"]
    })");
    ASSERT_NE(staticDescriptor, nullptr);
    ChildListDescriptor staticChildList = ParseExtendedTabsChildList(staticDescriptor->GetRoot());
    ASSERT_EQ(staticChildList.type, ChildListType::STATIC_IDS);
    ASSERT_EQ(staticChildList.staticChildIds.size(), 2U);
    EXPECT_EQ(staticChildList.staticChildIds.front(), "tabHome");

    std::list<std::string> staticIds = ResolveExtendedTabsChildIds(staticDescriptor->GetRoot(),
        [](const std::string&, const std::string&) { return std::list<std::string> { "ignored" }; });
    ASSERT_EQ(staticIds.size(), 2U);

    std::unique_ptr<JsonAdapter> templateDescriptor = ParseJson(R"({
        "children": {
            "componentId": "tabTemplate",
            "path": "/tabs"
        }
    })");
    ASSERT_NE(templateDescriptor, nullptr);
    std::list<std::string> templateIds = ResolveExtendedTabsChildIds(
        templateDescriptor->GetRoot(), [](const std::string& componentId, const std::string& path) {
            EXPECT_EQ(componentId, "tabTemplate");
            EXPECT_EQ(path, "/tabs");
            return std::list<std::string> { "generated-0", "generated-1" };
        });
    ASSERT_EQ(templateIds.size(), 2U);
    EXPECT_EQ(templateIds.front(), "generated-0");
    EXPECT_TRUE(ResolveExtendedTabsChildIds(templateDescriptor->GetRoot(), nullptr).empty());

    std::unique_ptr<JsonAdapter> childrenArray = ParseJson("[]");
    ASSERT_NE(childrenArray, nullptr);
    JsonValue emptyChildrenArray = childrenArray->GetRoot();
    MergeExtendedTabsChildIds({ "tabHome", "", "tabOrders" }, emptyChildrenArray);
    ASSERT_TRUE(emptyChildrenArray.IsArray());
    ASSERT_EQ(emptyChildrenArray.GetArraySize(), 2);
    EXPECT_EQ(emptyChildrenArray.GetArrayItem(0).GetStringValue(""), "tabHome");
    EXPECT_EQ(emptyChildrenArray.GetArrayItem(1).GetStringValue(""), "tabOrders");

    std::unique_ptr<JsonAdapter> prefilledChildren = ParseJson(R"(["keep"])");
    ASSERT_NE(prefilledChildren, nullptr);
    JsonValue prefilledChildrenArray = prefilledChildren->GetRoot();
    MergeExtendedTabsChildIds({ "tabMore" }, prefilledChildrenArray);
    ASSERT_EQ(prefilledChildrenArray.GetArraySize(), 1);
    EXPECT_EQ(prefilledChildrenArray.GetArrayItem(0).GetStringValue(""), "keep");
}

TEST_F(SchemaValidationHelpersTddTest, should_prebuild_extended_tabs_children_for_static_and_non_array_template_inputs)
{
    SurfaceSlot staticSlot;
    staticSlot.SetSurfaceId("tabs_static_surface");
    staticSlot.SetRenderId(41);
    staticSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true } }));

    std::unique_ptr<JsonAdapter> childDescriptors = ParseJson(R"([
        {"id":"tabHome","component":"Text","text":"Home"},
        {"id":"tabOrders","component":"Text","text":"Orders"}
    ])");
    ASSERT_NE(childDescriptors, nullptr);
    staticSlot.PrepareDescriptorById(childDescriptors->GetRoot());

    std::unique_ptr<JsonAdapter> staticTabsDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Extended.Tabs",
        "children": ["tabHome", "tabOrders", "tabHome"]
    })");
    ASSERT_NE(staticTabsDescriptor, nullptr);
    PrebuildExtendedTabsChildren(staticSlot, staticTabsDescriptor->GetRoot());
    EXPECT_NE(staticSlot.FindComponentById("tabHome"), nullptr);
    EXPECT_NE(staticSlot.FindComponentById("tabOrders"), nullptr);

    SurfaceSlot templateSlot;
    templateSlot.SetSurfaceId("tabs_template_surface");
    templateSlot.SetRenderId(42);
    templateSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true } }));
    std::unique_ptr<JsonAdapter> dataModel = ParseJson(R"({
        "value": {
            "tabs": {
                "name": "single-object"
            }
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(templateSlot.UpdateDataModel(dataModel->GetRoot()));

    std::unique_ptr<JsonAdapter> templateTabsDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Extended.Tabs",
        "children": {
            "componentId": "tabTemplate",
            "path": "/tabs"
        }
    })");
    ASSERT_NE(templateTabsDescriptor, nullptr);
    PrebuildExtendedTabsChildren(templateSlot, templateTabsDescriptor->GetRoot());
    EXPECT_TRUE(templateSlot.GetAllComponents().empty());

    PrebuildExtendedTabsChildren(templateSlot, JsonValue());
}

/**
 * @tc.name: 扩展Tabs短类型预构建与merge回退
 * @tc.desc: 覆盖短类型Tabs仅在扩展协议surface下触发预构建，以及MergeExtendedTabsChildIds的早退分支。
 * @tc.type: FUNC
 */
TEST_F(
    SchemaValidationHelpersTddTest, should_only_prebuild_short_tabs_on_extended_surface_and_keep_merge_inputs_unchanged)
{
    std::unique_ptr<JsonAdapter> childDescriptors = ParseJson(R"([
        {"id":"tabHome","component":"Text","content":"Home"}
    ])");
    std::unique_ptr<JsonAdapter> shortTabsDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Tabs",
        "children": ["tabHome"]
    })");
    ASSERT_NE(childDescriptors, nullptr);
    ASSERT_NE(shortTabsDescriptor, nullptr);

    SurfaceSlot standardSlot;
    standardSlot.SetSurfaceId("tabs_standard_surface");
    standardSlot.SetRenderId(43);
    standardSlot.SetCatalog(BuildCatalog(A2UI_BASIC_CATALOG_ID, { { "Text", true } }));
    standardSlot.PrepareDescriptorById(childDescriptors->GetRoot());
    PrebuildExtendedTabsChildren(standardSlot, shortTabsDescriptor->GetRoot());
    EXPECT_EQ(standardSlot.FindComponentById("tabHome"), nullptr);

    SurfaceSlot extendedSlot;
    extendedSlot.SetSurfaceId("tabs_extended_surface");
    extendedSlot.SetRenderId(44);
    extendedSlot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", true } }));
    extendedSlot.PrepareDescriptorById(childDescriptors->GetRoot());
    PrebuildExtendedTabsChildren(extendedSlot, shortTabsDescriptor->GetRoot());
    EXPECT_NE(extendedSlot.FindComponentById("tabHome"), nullptr);

    std::unique_ptr<JsonAdapter> objectChildren = ParseJson(R"({"keep":"value"})");
    ASSERT_NE(objectChildren, nullptr);
    JsonValue objectChildrenValue = objectChildren->GetRoot();
    MergeExtendedTabsChildIds({ "tabIgnored" }, objectChildrenValue);
    EXPECT_TRUE(objectChildrenValue.IsObject());
    EXPECT_EQ(objectChildrenValue.GetString("keep", ""), "value");

    std::unique_ptr<JsonAdapter> emptyChildren = ParseJson("[]");
    ASSERT_NE(emptyChildren, nullptr);
    JsonValue emptyChildrenValue = emptyChildren->GetRoot();
    MergeExtendedTabsChildIds({}, emptyChildrenValue);
    EXPECT_EQ(emptyChildrenValue.GetArraySize(), 0);

    std::unique_ptr<JsonAdapter> prefilledChildren = ParseJson(R"(["keep"])");
    ASSERT_NE(prefilledChildren, nullptr);
    JsonValue prefilledChildrenValue = prefilledChildren->GetRoot();
    MergeExtendedTabsChildIds({ "tabHome" }, prefilledChildrenValue);
    ASSERT_EQ(prefilledChildrenValue.GetArraySize(), 1);
    EXPECT_EQ(prefilledChildrenValue.GetArrayItem(0).GetStringValue(""), "keep");
}

/**
 * @tc.name: 扩展Tabs模板children预构建
 * @tc.desc: 覆盖模板数组展开、生成实例节点，以及模板根缺失时的跳过分支。
 * @tc.type: FUNC
 */
TEST_F(SchemaValidationHelpersTddTest,
    should_prebuild_extended_tabs_template_children_for_array_data_and_skip_missing_template_root)
{
    SurfaceSlot templateSlot;
    templateSlot.SetSurfaceId("tabs_template_array_surface");
    templateSlot.SetRenderId(45);
    templateSlot.SetCatalog(
        BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Extended.TabContent", false }, { "Text", true } }));

    std::unique_ptr<JsonAdapter> dataModel = ParseJson(R"({
        "value": {
            "tabs": [
                { "title": "Home", "body": "Alpha" },
                { "title": "Orders", "body": "Beta" }
            ]
        }
    })");
    ASSERT_NE(dataModel, nullptr);
    ASSERT_TRUE(templateSlot.UpdateDataModel(dataModel->GetRoot()));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {
            "id": "tabTemplate",
            "component": "Extended.TabContent",
            "title": { "path": "title" },
            "children": ["pageText"]
        },
        {
            "id": "pageText",
            "component": "Text",
            "content": { "path": "body" }
        }
    ])");
    ASSERT_NE(descriptors, nullptr);
    templateSlot.PrepareDescriptorById(descriptors->GetRoot());

    std::unique_ptr<JsonAdapter> templateTabsDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Tabs",
        "children": {
            "componentId": "tabTemplate",
            "path": "/tabs",
            "itemVar": "tab",
            "indexVar": "tabIndex"
        }
    })");
    ASSERT_NE(templateTabsDescriptor, nullptr);
    PrebuildExtendedTabsChildren(templateSlot, templateTabsDescriptor->GetRoot());

    EXPECT_NE(templateSlot.FindComponentById("/tabstabTemplate:0:tabTemplate"), nullptr);
    EXPECT_NE(templateSlot.FindComponentById("/tabstabTemplate:1:tabTemplate"), nullptr);
    EXPECT_NE(templateSlot.FindComponentById("/tabstabTemplate:0:pageText"), nullptr);
    EXPECT_NE(templateSlot.FindComponentById("/tabstabTemplate:1:pageText"), nullptr);

    SurfaceSlot missingTemplateSlot;
    missingTemplateSlot.SetSurfaceId("tabs_template_missing_surface");
    missingTemplateSlot.SetRenderId(46);
    missingTemplateSlot.SetCatalog(
        BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Extended.TabContent", false }, { "Text", true } }));
    ASSERT_TRUE(missingTemplateSlot.UpdateDataModel(dataModel->GetRoot()));
    missingTemplateSlot.PrepareDescriptorById(descriptors->GetRoot());

    std::unique_ptr<JsonAdapter> missingTemplateDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Tabs",
        "children": {
            "componentId": "missingTemplate",
            "path": "/tabs"
        }
    })");
    ASSERT_NE(missingTemplateDescriptor, nullptr);
    PrebuildExtendedTabsChildren(missingTemplateSlot, missingTemplateDescriptor->GetRoot());
    EXPECT_EQ(missingTemplateSlot.FindComponentById("/tabsmissingTemplate:0:missingTemplate"), nullptr);
}

/**
 * @tc.name: 子列表与结构字段校验边界
 * @tc.desc: 覆盖模板children合法/非法形态、非对象早退，以及Card和Row结构字段缺失告警。
 * @tc.type: FUNC
 */
TEST_F(SchemaValidationHelpersTddTest, should_cover_template_child_list_and_structural_validation_edge_cases)
{
    RegisterDispatchCallbacks(mockNapiPtr_);

    std::unique_ptr<JsonAdapter> validTemplateDescriptor = ParseJson(R"({
        "children": {
            "componentId": "tabTemplate",
            "path": "/tabs"
        }
    })");
    std::unique_ptr<JsonAdapter> pathMismatchDescriptor = ParseJson(R"({
        "children": {
            "componentId": "tabTemplate",
            "path": 1
        }
    })");
    std::unique_ptr<JsonAdapter> emptyTemplateDescriptor = ParseJson(R"({
        "children": {
            "componentId": "",
            "path": ""
        }
    })");
    std::unique_ptr<JsonAdapter> missingCardChild = ParseJson(R"({"id":"card","component":"Card"})");
    std::unique_ptr<JsonAdapter> missingRowChildren = ParseJson(R"({"id":"row","component":"Row"})");
    std::unique_ptr<JsonAdapter> textDescriptor = ParseJson(R"({"id":"text","component":"Text","children":["x"]})");
    ASSERT_NE(validTemplateDescriptor, nullptr);
    ASSERT_NE(pathMismatchDescriptor, nullptr);
    ASSERT_NE(emptyTemplateDescriptor, nullptr);
    ASSERT_NE(missingCardChild, nullptr);
    ASSERT_NE(missingRowChildren, nullptr);
    ASSERT_NE(textDescriptor, nullptr);

    EXPECT_TRUE(
        ValidateChildListSchema(validTemplateDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW)
            .empty());
    EXPECT_TRUE(
        ValidateChildListSchema(validTemplateDescriptor->GetRoot(), "", ChildListEmptyArrayPolicy::WARN_INVALID_VALUE)
            .empty());

    std::vector<SchemaValidationIssue> pathMismatchIssues =
        ValidateChildListSchema(pathMismatchDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(pathMismatchIssues.size(), 1U);
    EXPECT_EQ(pathMismatchIssues[0].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(pathMismatchIssues[0].propertyPath, "children.path");

    std::vector<SchemaValidationIssue> emptyTemplateIssues =
        ValidateChildListSchema(emptyTemplateDescriptor->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(emptyTemplateIssues.size(), 2U);
    EXPECT_EQ(emptyTemplateIssues[0].code, SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(emptyTemplateIssues[0].propertyPath, "children.componentId");
    EXPECT_EQ(emptyTemplateIssues[1].code, SCHEMA_ERROR_CODE_REQUIRED_MISS);
    EXPECT_EQ(emptyTemplateIssues[1].propertyPath, "children.path");

    EXPECT_FALSE(ValidateDescriptorIdForPreparation(JsonValue(), 4, "surface"));
    ValidateRequiredCreationFields(JsonValue(), 4, "surface");
    ValidateRequiredStructuralFields(JsonValue(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(missingCardChild->GetRoot(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(missingRowChildren->GetRoot(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateRequiredStructuralFields(textDescriptor->GetRoot(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateStructuralFieldShapes(textDescriptor->GetRoot(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);
    ValidateStructuralFieldShapes(JsonValue(), 4, "surface", SurfaceProtocolMode::A2UI_STANDARD);

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "card.child"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "row.children"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_REQUIRED_MISS, "text.children"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "text.children"), 0U);
}

TEST_F(SchemaValidationHelpersTddTest, should_skip_extended_tabs_resolution_and_prebuild_when_children_shape_is_invalid)
{
    SurfaceSlot slot;
    slot.SetSurfaceId("tabs_invalid_shape_surface");
    slot.SetRenderId(47);
    slot.SetCatalog(BuildCatalog(A2UI_EXTENDED_CATALOG_ID, { { "Text", true } }));

    std::unique_ptr<JsonAdapter> descriptors = ParseJson(R"([
        {"id":"tabTemplate","component":"Text","content":"ignored"}
    ])");
    ASSERT_NE(descriptors, nullptr);
    slot.PrepareDescriptorById(descriptors->GetRoot());

    std::unique_ptr<JsonAdapter> invalidTabsDescriptor = ParseJson(R"({
        "id": "tabs",
        "component": "Tabs",
        "children": true
    })");
    ASSERT_NE(invalidTabsDescriptor, nullptr);

    bool resolveCallbackCalled = false;
    std::list<std::string> childIds = ResolveExtendedTabsChildIds(
        invalidTabsDescriptor->GetRoot(), [&resolveCallbackCalled](const std::string&, const std::string&) {
            resolveCallbackCalled = true;
            return std::list<std::string> { "unexpected" };
        });
    EXPECT_TRUE(childIds.empty());
    EXPECT_FALSE(resolveCallbackCalled);

    PrebuildExtendedTabsChildren(slot, invalidTabsDescriptor->GetRoot());
    EXPECT_EQ(slot.FindComponentById("tabTemplate"), nullptr);
}
