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
#include <memory>
#include <string>
#include <vector>

#include "components/ChildListSchemaValidationUtils.h"
#include "functions/WarningDispatchBridge.h"

#include "SchemaErrorCodes.h"
#include "SurfaceSlot.h"
#include "SurfaceSlotSchemaValidation.h"
#include "TestFixture.h"
#include "include/SchemaWarningTestHelper.h"

using namespace NativeModule;
using namespace NativeModule::TestHelpers;

namespace {

std::unique_ptr<JsonAdapter> ParseJson(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

class LocalVariableSchemaValidationTddTest : public A2UITest {};

} // namespace

TEST_F(LocalVariableSchemaValidationTddTest, should_validate_template_local_variable_names_and_conflicts)
{
    const auto valid = ParseJson(
        R"({"children":{"componentId":"template","path":"/items","indexVar":"entryIndex","itemVar":"entry"}})");
    const auto missing = ParseJson(R"({"children":{"componentId":"template","path":"/items"}})");
    const auto invalid = ParseJson(
        R"({"children":{"componentId":"template","path":"/items","indexVar":"1index","itemVar":"__dataModel"}})");
    const auto invalidTypes =
        ParseJson(R"({"children":{"componentId":"template","path":"/items","indexVar":1,"itemVar":null}})");
    const auto conflict =
        ParseJson(R"({"children":{"componentId":"template","path":"/items","indexVar":"entry","itemVar":"entry"}})");
    ASSERT_NE(valid, nullptr);
    ASSERT_NE(missing, nullptr);
    ASSERT_NE(invalid, nullptr);
    ASSERT_NE(invalidTypes, nullptr);
    ASSERT_NE(conflict, nullptr);

    EXPECT_TRUE(ValidateChildListSchema(valid->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW).empty());
    EXPECT_TRUE(ValidateChildListSchema(missing->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW).empty());

    const std::vector<SchemaValidationIssue> invalidIssues =
        ValidateChildListSchema(invalid->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(invalidIssues.size(), 2U);
    EXPECT_EQ(invalidIssues[0].code, SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(invalidIssues[0].propertyPath, "children.indexVar");
    EXPECT_EQ(invalidIssues[1].code, SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(invalidIssues[1].propertyPath, "children.itemVar");

    const std::vector<SchemaValidationIssue> typeIssues =
        ValidateChildListSchema(invalidTypes->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(typeIssues.size(), 2U);
    EXPECT_EQ(typeIssues[0].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(typeIssues[1].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);

    const std::vector<SchemaValidationIssue> conflictIssues =
        ValidateChildListSchema(conflict->GetRoot(), "children", ChildListEmptyArrayPolicy::ALLOW);
    ASSERT_EQ(conflictIssues.size(), 1U);
    EXPECT_EQ(conflictIssues[0].code, SCHEMA_ERROR_CODE_INVALID_VALUE);
    EXPECT_EQ(conflictIssues[0].propertyPath, "children");
}

TEST_F(LocalVariableSchemaValidationTddTest, should_dispatch_schema_warnings_for_invalid_as_names)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    const auto descriptor = ParseJson(R"({
        "id":"button",
        "component":"Button",
        "onClick":[
            {"call":"fn0","as":1},
            {"call":"fn1","as":null},
            {"call":"fn2","as":""},
            {"call":"fn3","as":"bad-name"},
            {"call":"fn4","as":"__dataModel"},
            {"call":"fn5"},
            {"call":"fn6","as":"context"}
        ]
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateEventHandlerFields(descriptor->GetRoot(), 5, "surface");

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "button.onClick[0].as"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_TYPE_MISMATCH, "button.onClick[1].as"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "button.onClick[2].as"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "button.onClick[3].as"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "button.onClick[4].as"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "button.onClick[5].as"), 0U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "button.onClick[6].as"), 0U);
}

TEST_F(LocalVariableSchemaValidationTddTest, should_dispatch_schema_warnings_for_invalid_template_variable_names)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    const auto descriptor = ParseJson(R"({
        "id":"root",
        "component":"Column",
        "children":{
            "componentId":"template",
            "path":"/items",
            "indexVar":"1index",
            "itemVar":"bad-name"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateStructuralFieldShapes(descriptor->GetRoot(), 5, "surface", SurfaceProtocolMode::A2UI_STANDARD);

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.children.indexVar"), 1U);
    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.children.itemVar"), 1U);
}

TEST_F(LocalVariableSchemaValidationTddTest, should_dispatch_schema_warning_for_conflicting_template_variable_names)
{
    RegisterWarningDispatchCallback(mockNapiPtr_);
    const auto descriptor = ParseJson(R"({
        "id":"root",
        "component":"Column",
        "children":{
            "componentId":"template",
            "path":"/items",
            "indexVar":"entry",
            "itemVar":"entry"
        }
    })");
    ASSERT_NE(descriptor, nullptr);

    ValidateStructuralFieldShapes(descriptor->GetRoot(), 5, "surface", SurfaceProtocolMode::A2UI_STANDARD);

    EXPECT_EQ(CountWarningRequests(mockNapiPtr_, SCHEMA_ERROR_CODE_INVALID_VALUE, "root.children"), 1U);
}
