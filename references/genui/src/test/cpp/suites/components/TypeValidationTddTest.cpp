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
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "components/TypeValidation.h"
#include "utils/JsonAdapter.h"

#include "TestFixture.h"

using namespace NativeModule;

namespace {

JsonValue MakeNull()
{
    return JsonValue();
}

std::unique_ptr<JsonAdapter> Parse(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

// Helper: returns a non-dynamic JsonValue from a JSON literal.
JsonValue ValueFrom(const std::string& json)
{
    auto adapter = Parse(json);
    return adapter ? adapter->GetRoot() : MakeNull();
}

// Helper: build an object with a single key/value from JSON strings.
JsonValue ObjWith(const std::string& key, const std::string& valueJson)
{
    auto obj = JsonAdapter::CreateObject();
    if (obj == nullptr) {
        return MakeNull();
    }
    JsonValue root = obj->GetRoot();
    auto valAdapter = Parse(valueJson);
    if (valAdapter != nullptr) {
        root.Put(key.c_str(), valAdapter->GetRoot());
    }
    return root;
}

// ---------------------------------------------------------------------------
// Report capture for testing ReportTypeMismatchAndReset
// ---------------------------------------------------------------------------

struct ReportRecord {
    std::string code;
    std::string message;
    std::string path;
};

struct ReportCapture {
    std::vector<ReportRecord> calls;

    void operator()(const std::string& code, const std::string& message, const std::string& path)
    {
        calls.push_back({ code, message, path });
    }

    void Clear()
    {
        calls.clear();
    }

    const ReportRecord& Last() const
    {
        return calls.back();
    }

    size_t Count() const
    {
        return calls.size();
    }
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class TypeValidationTddTest : public A2UITest {};

// ===========================================================================
// IsDynamicDescriptorObject
// ===========================================================================

TEST_F(TypeValidationTddTest, dynamic_descriptor_is_object_with_path_key)
{
    auto adapter = Parse(R"({"path": "/data/name"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_is_object_with_call_key)
{
    auto adapter = Parse(R"({"call": "myFunction"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_object_without_path_or_call)
{
    auto adapter = Parse(R"({"color": "red", "size": 12})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_empty_object)
{
    auto adapter = Parse("{}");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_string)
{
    auto adapter = Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_number)
{
    auto adapter = Parse("42");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_boolean)
{
    auto adapter = Parse("true");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicDescriptorObject(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_descriptor_rejects_invalid_json_value)
{
    EXPECT_FALSE(IsDynamicDescriptorObject(MakeNull()));
}

// ===========================================================================
// IsDynamicValue
// ===========================================================================

TEST_F(TypeValidationTddTest, dynamic_value_accepts_descriptor_object)
{
    auto adapter = Parse(R"({"path": "/x"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsDynamicValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_value_rejects_plain_string)
{
    auto adapter = Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_value_rejects_number)
{
    auto adapter = Parse("3.14");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_value_rejects_boolean)
{
    auto adapter = Parse("false");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_value_rejects_plain_object)
{
    auto adapter = Parse(R"({"key": "value"})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, dynamic_value_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsDynamicValue(adapter->GetRoot()));
}

// ===========================================================================
// IsEmptyStringValue
// ===========================================================================

TEST_F(TypeValidationTddTest, empty_string_detects_truly_empty)
{
    auto adapter = Parse(R"("")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_detects_whitespace_only)
{
    auto adapter = Parse(R"("   ")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_detects_tabs_and_newlines)
{
    auto adapter = Parse(R"("\t \n")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_rejects_non_empty)
{
    auto adapter = Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_rejects_number)
{
    auto adapter = Parse("0");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_rejects_boolean)
{
    auto adapter = Parse("true");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_rejects_object)
{
    auto adapter = Parse("{}");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsEmptyStringValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, empty_string_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsEmptyStringValue(adapter->GetRoot()));
}

// ===========================================================================
// IsNonEmptyLiteralString
// ===========================================================================

TEST_F(TypeValidationTddTest, non_empty_literal_accepts_content_string)
{
    auto adapter = Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, non_empty_literal_rejects_empty_string)
{
    auto adapter = Parse(R"("")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, non_empty_literal_rejects_whitespace_only)
{
    auto adapter = Parse(R"("   ")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, non_empty_literal_rejects_number)
{
    auto adapter = Parse("123");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, non_empty_literal_rejects_boolean)
{
    auto adapter = Parse("false");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, non_empty_literal_rejects_object)
{
    auto adapter = Parse(R"({"a":1})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsNonEmptyLiteralString(adapter->GetRoot()));
}

// ===========================================================================
// IsLiteralBool
// ===========================================================================

TEST_F(TypeValidationTddTest, literal_bool_accepts_true)
{
    auto adapter = Parse("true");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralBool(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_bool_accepts_false)
{
    auto adapter = Parse("false");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralBool(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_bool_rejects_string_true)
{
    auto adapter = Parse(R"("true")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralBool(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_bool_rejects_number)
{
    auto adapter = Parse("1");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralBool(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_bool_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralBool(adapter->GetRoot()));
}

// ===========================================================================
// IsLiteralNumber
// ===========================================================================

TEST_F(TypeValidationTddTest, literal_number_accepts_positive_integer)
{
    auto adapter = Parse("42");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_accepts_negative_integer)
{
    auto adapter = Parse("-7");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_accepts_zero)
{
    auto adapter = Parse("0");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_accepts_floating_point)
{
    auto adapter = Parse("3.14159");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_rejects_string)
{
    auto adapter = Parse(R"("42")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_rejects_boolean)
{
    auto adapter = Parse("true");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralNumber(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, literal_number_rejects_object)
{
    auto adapter = Parse(R"({"n": 1})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsLiteralNumber(adapter->GetRoot()));
}

// ===========================================================================
// IsValidColorValue
// ===========================================================================

TEST_F(TypeValidationTddTest, valid_color_accepts_six_digit_hex)
{
    auto adapter = Parse(R"("#FFFFFF")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_three_digit_hex)
{
    auto adapter = Parse(R"("#FFF")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_accepts_eight_digit_hex_with_alpha)
{
    auto adapter = Parse(R"("#FF0000FF")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_accepts_lowercase_hex)
{
    auto adapter = Parse(R"("#aabbcc")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_accepts_transparent_keyword)
{
    auto adapter = Parse(R"("transparent")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_accepts_transparent_with_whitespace)
{
    auto adapter = Parse(R"("  transparent  ")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_invalid_hex_format)
{
    auto adapter = Parse(R"("#GGGGGG")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_arbitrary_string)
{
    auto adapter = Parse(R"("not-a-color")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_empty_string)
{
    auto adapter = Parse(R"("")");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_number)
{
    auto adapter = Parse("123");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_boolean)
{
    auto adapter = Parse("true");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

TEST_F(TypeValidationTddTest, valid_color_rejects_null)
{
    auto adapter = Parse("null");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(IsValidColorValue(adapter->GetRoot()));
}

// ===========================================================================
// BuildTypeMismatchMessage
// ===========================================================================

TEST_F(TypeValidationTddTest, build_type_mismatch_message_formats_correctly)
{
    auto adapter = Parse("123");
    ASSERT_NE(adapter, nullptr);

    std::string msg = BuildTypeMismatchMessage("scrollable", "boolean", adapter->GetRoot());
    EXPECT_NE(msg.find("Property scrollable"), std::string::npos);
    EXPECT_NE(msg.find("expects boolean value"), std::string::npos);
    EXPECT_NE(msg.find("got type 'number'"), std::string::npos);
    EXPECT_NE(msg.find("fallback/reset has been applied"), std::string::npos);
}

TEST_F(TypeValidationTddTest, build_type_mismatch_message_handles_string_value)
{
    auto adapter = Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);

    std::string msg = BuildTypeMismatchMessage("tabType", "string", adapter->GetRoot());
    EXPECT_NE(msg.find("expects string value"), std::string::npos);
    EXPECT_NE(msg.find("got type 'string'"), std::string::npos);
}

TEST_F(TypeValidationTddTest, build_type_mismatch_message_handles_null_value)
{
    std::string msg = BuildTypeMismatchMessage("prop", "number", MakeNull());
    EXPECT_NE(msg.find("got type '"), std::string::npos);
}

// ===========================================================================
// ReportTypeMismatchAndReset — simple reset (value = JsonValue())
// ===========================================================================

TEST_F(TypeValidationTddTest, report_mismatch_simple_calls_report_callback)
{
    ReportCapture captured;
    auto adapter = Parse(R"("true")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    ReportTypeMismatchAndReset(captured, val, "boolean", "scrollable");

    ASSERT_EQ(captured.Count(), 1U);
    EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_NE(captured.Last().message.find("Property scrollable"), std::string::npos);
    EXPECT_NE(captured.Last().message.find("expects boolean value"), std::string::npos);
    EXPECT_EQ(captured.Last().path, "scrollable");
}

TEST_F(TypeValidationTddTest, report_mismatch_simple_resets_value_to_empty)
{
    ReportCapture captured;
    auto adapter = Parse(R"("true")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();
    EXPECT_TRUE(val.IsString()); // precondition

    ReportTypeMismatchAndReset(captured, val, "boolean", "scrollable");

    EXPECT_FALSE(val.IsString());
    EXPECT_FALSE(val.IsValid());
}

TEST_F(TypeValidationTddTest, report_mismatch_simple_with_number_value)
{
    ReportCapture captured;
    auto adapter = Parse("42");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    ReportTypeMismatchAndReset(captured, val, "string", "barPosition");

    ASSERT_EQ(captured.Count(), 1U);
    EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_NE(captured.Last().message.find("got type 'number'"), std::string::npos);
}

// ===========================================================================
// ReportTypeMismatchAndReset — custom reset
// ===========================================================================

TEST_F(TypeValidationTddTest, report_mismatch_custom_calls_report_callback)
{
    ReportCapture captured;
    auto adapter = Parse(R"("not-a-color")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    bool resetCalled = false;
    ReportTypeMismatchAndReset(
        captured, [&resetCalled]() { resetCalled = true; }, val, "non-empty string", "styles.selectedColor");

    ASSERT_EQ(captured.Count(), 1U);
    EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_NE(captured.Last().message.find("styles.selectedColor"), std::string::npos);
    EXPECT_TRUE(resetCalled);
}

TEST_F(TypeValidationTddTest, report_mismatch_custom_does_not_reset_value_itself)
{
    ReportCapture captured;
    auto adapter = Parse("123");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();
    EXPECT_TRUE(val.IsNumber()); // precondition

    ReportTypeMismatchAndReset(
        captured, []() { /* custom reset does nothing to val */ }, val, "number", "styles.fontSize");

    // The value itself is NOT reset by the template — only the custom action runs.
    EXPECT_TRUE(val.IsNumber());
}

// ===========================================================================
// ReportTypeMismatchAndReset — multiple calls in sequence
// ===========================================================================

TEST_F(TypeValidationTddTest, report_mismatch_accumulates_multiple_calls)
{
    ReportCapture captured;

    auto a1 = Parse(R"("x")");
    auto a2 = Parse("5");
    ASSERT_NE(a1, nullptr);
    ASSERT_NE(a2, nullptr);
    JsonValue v1 = a1->GetRoot();
    JsonValue v2 = a2->GetRoot();

    ReportTypeMismatchAndReset(captured, v1, "boolean", "propA");
    ReportTypeMismatchAndReset(captured, v2, "string", "propB");

    ASSERT_EQ(captured.Count(), 2U);
    EXPECT_EQ(captured.calls[0].path, "propA");
    EXPECT_EQ(captured.calls[1].path, "propB");
    EXPECT_EQ(captured.calls[0].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_EQ(captured.calls[1].code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
}

// ===========================================================================
// Combined scenario: dynamic check → type check → mismatch report
// (mimics the exact pattern used in CustomComponentTabsValidation)
// ===========================================================================

TEST_F(TypeValidationTddTest, full_validate_boolean_property_scenario)
{
    ReportCapture captured;

    // Scenario: scrollable receives a string "true" instead of boolean true.
    auto adapter = Parse(R"("true")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    // Step 1: check dynamic
    if (IsDynamicValue(val)) {
        FAIL() << "Should not be dynamic for a plain string";
    }
    // Step 2: check literal type
    if (IsLiteralBool(val)) {
        FAIL() << "String 'true' should not pass strict boolean check";
    }
    // Step 3: report
    ReportTypeMismatchAndReset(captured, val, "boolean", "scrollable");

    ASSERT_EQ(captured.Count(), 1U);
    EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
    EXPECT_FALSE(val.IsValid());
}

TEST_F(TypeValidationTddTest, full_validate_color_property_scenario_valid)
{
    auto adapter = Parse(R"("#2563EB")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    // Valid color path
    if (IsDynamicValue(val)) {
        FAIL() << "Should not be dynamic";
    }
    if (IsNonEmptyLiteralString(val)) {
        if (IsValidColorValue(val)) {
            SUCCEED(); // correct path
            return;
        }
    }
    FAIL() << "Valid color should have passed all checks";
}

TEST_F(TypeValidationTddTest, full_validate_color_property_scenario_invalid)
{
    ReportCapture captured;
    auto adapter = Parse(R"("not-a-color")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    if (IsDynamicValue(val)) {
        FAIL() << "Should not be dynamic";
    }
    if (IsNonEmptyLiteralString(val)) {
        EXPECT_TRUE(IsNonEmptyLiteralString(val));
        if (!IsValidColorValue(val)) {
            // Expected: non-empty string but invalid color → report INVALID_VALUE
            captured(SCHEMA_ERROR_CODE_INVALID_VALUE,
                "Property styles.selectedColor has invalid value and has been reset to default",
                "styles.selectedColor");
            ASSERT_EQ(captured.Count(), 1U);
            EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_INVALID_VALUE);
            return;
        }
    }
    FAIL() << "Invalid color should have been caught";
}

TEST_F(TypeValidationTddTest, full_validate_number_property_scenario)
{
    ReportCapture captured;
    auto adapter = Parse(R"("not-a-number")");
    ASSERT_NE(adapter, nullptr);
    JsonValue val = adapter->GetRoot();

    if (IsDynamicValue(val)) {
        FAIL() << "Should not be dynamic";
    }
    if (IsLiteralNumber(val)) {
        FAIL() << "String should not pass number check";
    }
    ReportTypeMismatchAndReset(captured, val, "number", "fontSize");

    ASSERT_EQ(captured.Count(), 1U);
    EXPECT_EQ(captured.Last().code, SCHEMA_ERROR_CODE_TYPE_MISMATCH);
}

} // namespace
