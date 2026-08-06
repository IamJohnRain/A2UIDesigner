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

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "catalog/Catalog.h"
#include "catalog/CatalogItem.h"
#include "components/custom/CustomComponent.h"
#include "data/DataModel.h"
#include "data/ResolvedValue.h"
#include "functions/FunctionBridge.h"
#include "functions/FunctionResult.h"
#include "functions/NativeAndFunction.h"
#include "functions/NativeEmailFunction.h"
#include "functions/NativeFormatCurrencyFunction.h"
#include "functions/NativeFormatDateFunction.h"
#include "functions/NativeFormatNumberFunction.h"
#include "functions/NativeFormatStringFunction.h"
#include "functions/NativeFunctionBase.h"
#include "functions/NativeFunctionRegistry.h"
#include "functions/NativeLengthFunction.h"
#include "functions/NativeNotFunction.h"
#include "functions/NativeNumericFunction.h"
#include "functions/NativeOrFunction.h"
#include "functions/NativePluralizeFunction.h"
#include "functions/NativeRegexFunction.h"
#include "functions/NativeRequiredFunction.h"
#include "utils/JsonAdapter.h"

#include "../RenderManager.h"
#include "../RenderSlot.h"
#include "../SurfaceManager.h"
#include "../SurfaceSlot.h"
#include "TestFixture.h"

using namespace NativeModule;

namespace {

std::string ExecuteAsLiteral(NativeFunctionBase& function, const std::string& argsJson)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(argsJson);
    if (adapter == nullptr) {
        return "null";
    }
    return function.Execute(adapter->GetRoot()).ToJsonLiteral();
}

FunctionResult ExecuteAsResult(NativeFunctionBase& function, const std::string& argsJson)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Parse(argsJson);
    if (adapter == nullptr) {
        return FunctionResult();
    }
    return function.Execute(adapter->GetRoot());
}

struct PluralLocaleManagerLayout {
    napi_env env_;
    napi_ref providerRef_;
    std::string cachedLocale_;
    std::chrono::steady_clock::time_point lastCallTime_;
};

struct FunctionResultLayout {
    FunctionResultType type_;
    bool boolValue_;
    int32_t intValue_;
    double doubleValue_;
    std::string stringValue_;
    JsonValue jsonValue_;
};

void ResetPluralLocaleManagerState(const std::string& locale = "en")
{
    auto* layout = reinterpret_cast<PluralLocaleManagerLayout*>(&PluralLocaleManager::GetInstance());
    layout->env_ = nullptr;
    layout->providerRef_ = nullptr;
    layout->cachedLocale_ = locale;
    layout->lastCallTime_ = std::chrono::steady_clock::time_point();
}

} // namespace

// ==================== NativeAndFunction ====================

class NativeAndFunctionTest : public ::testing::Test {
protected:
    NativeAndFunction function_;
};

TEST_F(NativeAndFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "and");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeAndFunctionTest, should_return_false_when_values_not_array)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":true})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_values_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_array_has_less_than_two_items)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_array_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_one_item_is_not_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,"hello"]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_one_item_is_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,false,true]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_true_when_all_items_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,true,true]})"), "true");
}

TEST_F(NativeAndFunctionTest, should_return_true_with_exactly_two_true_items)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,true]})"), "true");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_item_is_number_instead_of_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,1,true]})"), "false");
}

// ==================== NativeEmailFunction ====================

class NativeEmailFunctionTest : public ::testing::Test {
protected:
    NativeEmailFunction function_;
};

TEST_F(NativeEmailFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "email");
}

TEST_F(NativeEmailFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("not_object")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeEmailFunctionTest, should_return_false_when_value_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_return_false_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_return_false_when_value_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":""})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_validate_common_email)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@example.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_validate_email_with_dots_in_local)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"first.last@example.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_validate_email_with_plus_in_local)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user+tag@example.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_validate_email_with_hyphen_in_domain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@my-domain.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_validate_email_with_subdomain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@mail.example.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_without_at)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"userexample.com"})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_without_domain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@"})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_without_local)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"@example.com"})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_with_spaces)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user @example.com"})"), "false");
}

// ==================== NativeFormatCurrencyFunction ====================

class NativeFormatCurrencyFunctionTest : public ::testing::Test {
protected:
    NativeFormatCurrencyFunction function_;
};

TEST_F(NativeFormatCurrencyFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "formatCurrency");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_value_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","currency":"USD"})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_currency_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":123})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_currency_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":""})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_basic_currency)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":99.99,"currency":"USD"})"), "\"USD 99.99\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_integer_currency)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":"EUR"})"), "\"EUR 100.00\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_with_grouping)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234567.89,"currency":"USD","grouping":true})"),
        "\"USD 1,234,567.89\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_grouping_not_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":"USD","grouping":"yes"})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_decimals_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":"USD","decimals":"two"})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_decimals_too_large)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"currency":"USD","decimals":25})"), "\"\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_with_custom_decimals)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100.123,"currency":"USD","decimals":3})"), "\"USD 100.123\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_with_zero_decimals)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":99.99,"currency":"JPY","decimals":0})"), "\"JPY 100\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_without_grouping_by_default)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234567.89,"currency":"USD"})"), "\"USD 1234567.89\"");
}

// ==================== NativeFormatDateFunction ====================

class NativeFormatDateFunctionTest : public ::testing::Test {
protected:
    NativeFormatDateFunction function_;
};

TEST_F(NativeFormatDateFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "formatDate");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"([1])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_value_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123,"format":"yyyy-MM-dd"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_format_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":123})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_value_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"","format":"yyyy-MM-dd"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_format_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":""})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_invalid_iso_too_short)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01","format":"yyyy-MM-dd"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_invalid_iso_bad_separator)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026/01/16","format":"yyyy-MM-dd"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_yyyy_MM_dd)
{
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:00Z","format":"yyyy-MM-dd"})"), "\"2026-01-16\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_yy)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":"yy"})"), "\"26\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_MMMM_full_month_name)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":"MMMM"})"), "\"January\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_MMM_abbreviated_month)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-06-15T00:00:00Z","format":"MMM"})"), "\"Jun\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_MM_zero_padded_month)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-03-05T00:00:00Z","format":"MM"})"), "\"03\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_M_single_digit_month)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-03-05T00:00:00Z","format":"M"})"), "\"3\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_dd_zero_padded_day)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-05T00:00:00Z","format":"dd"})"), "\"05\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_d_single_digit_day)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-05T00:00:00Z","format":"d"})"), "\"5\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_HH_24h_zero_padded)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T09:05:00Z","format":"HH"})"), "\"09\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_H_24h_single_digit)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T09:05:00Z","format":"H"})"), "\"9\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_hh_12h_zero_padded)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:00Z","format":"hh"})"), "\"02\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_h_12h_single_digit)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:00Z","format":"h"})"), "\"2\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_h12_as_12_for_midnight)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":"h"})"), "\"12\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_mm_zero_padded_minute)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:05:00Z","format":"mm"})"), "\"05\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_m_single_digit_minute)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:05:00Z","format":"m"})"), "\"5\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_ss_zero_padded_second)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:07Z","format":"ss"})"), "\"07\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_s_single_digit_second)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:07Z","format":"s"})"), "\"7\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_EEEE_full_day_name)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":"EEEE"})"), "\"Friday\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_EE_abbreviated_day)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T00:00:00Z","format":"EE"})"), "\"Fri\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_a_am_for_morning)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T09:30:00Z","format":"a"})"), "\"AM\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_a_pm_for_afternoon)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:00Z","format":"a"})"), "\"PM\"");
}

TEST_F(NativeFormatDateFunctionTest, should_include_literal_chars_in_pattern)
{
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:00Z","format":"yyyy/MM/dd"})"), "\"2026/01/16\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_full_datetime_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30:07Z","format":"yyyy-MM-dd HH:mm:ss"})"),
        "\"2026-01-16 14:30:07\"");
}

TEST_F(NativeFormatDateFunctionTest, should_parse_date_only_without_time)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16","format":"yyyy-MM-dd"})"), "\"2026-01-16\"");
}

TEST_F(NativeFormatDateFunctionTest, should_parse_iso_with_hours_minutes_only)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T14:30","format":"HH:mm"})"), "\"14:30\"");
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_reject_too_short_input)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_FALSE(NativeFormatDateFunction::ParseISO8601("short", parts));
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_reject_bad_separators)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_FALSE(NativeFormatDateFunction::ParseISO8601("2026/01/16", parts));
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_parse_valid_date)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_TRUE(NativeFormatDateFunction::ParseISO8601("2026-01-16", parts));
    EXPECT_EQ(parts.year, 2026);
    EXPECT_EQ(parts.month, 1);
    EXPECT_EQ(parts.day, 16);
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_parse_full_datetime)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_TRUE(NativeFormatDateFunction::ParseISO8601("2026-01-16T14:30:07", parts));
    EXPECT_EQ(parts.year, 2026);
    EXPECT_EQ(parts.month, 1);
    EXPECT_EQ(parts.day, 16);
    EXPECT_EQ(parts.hour, 14);
    EXPECT_EQ(parts.minute, 30);
    EXPECT_EQ(parts.second, 7);
}

TEST_F(NativeFormatDateFunctionTest, ApplyPattern_should_handle_various_patterns)
{
    NativeFormatDateFunction::DateTimeParts parts = {
        .year = 2026, .month = 1, .day = 16, .hour = 14, .minute = 30, .second = 7
    };
    std::string result = NativeFormatDateFunction::ApplyPattern(parts, "yyyy/MM/dd HH:mm:ss");
    EXPECT_EQ(result, "2026/01/16 14:30:07");
}

// ==================== NativeFormatNumberFunction ====================

class NativeFormatNumberFunctionTest : public ::testing::Test {
protected:
    NativeFormatNumberFunction function_;
};

TEST_F(NativeFormatNumberFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "formatNumber");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_value_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc"})"), "\"\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_default_two_decimals)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234.5})"), "\"1234.50\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_with_custom_decimals)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234.567,"decimals":3})"), "\"1234.567\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_with_zero_decimals)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234.5,"decimals":0})"), "\"1235\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_with_grouping)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1000000,"grouping":true})"), "\"1,000,000.00\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_negative_with_grouping)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":-1234567.89,"grouping":true})"), "\"-1,234,567.89\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_without_grouping_by_default)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1234567.89})"), "\"1234567.89\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_grouping_not_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"grouping":"yes"})"), "\"\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_decimals_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"decimals":"two"})"), "\"\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_decimals_too_large)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"decimals":25})"), "\"\"");
}

TEST_F(NativeFormatNumberFunctionTest, FormatDecimal_should_round_correctly)
{
    EXPECT_EQ(NativeFormatNumberFunction::FormatDecimal(2.345, 2), "2.35");
}

TEST_F(NativeFormatNumberFunctionTest, FormatDecimal_should_format_integer_value)
{
    EXPECT_EQ(NativeFormatNumberFunction::FormatDecimal(100.0, 0), "100");
}

TEST_F(NativeFormatNumberFunctionTest, FormatWithGrouping_should_handle_small_number)
{
    EXPECT_EQ(NativeFormatNumberFunction::FormatWithGrouping(123.0, 2), "123.00");
}

TEST_F(NativeFormatNumberFunctionTest, FormatWithGrouping_should_handle_large_number)
{
    EXPECT_EQ(NativeFormatNumberFunction::FormatWithGrouping(1000000.0, 2), "1,000,000.00");
}

TEST_F(NativeFormatNumberFunctionTest, FormatWithGrouping_should_handle_negative)
{
    EXPECT_EQ(NativeFormatNumberFunction::FormatWithGrouping(-1234.56, 2), "-1,234.56");
}

TEST_F(NativeFormatNumberFunctionTest, FormatWithGrouping_should_handle_number_without_decimal_part)
{
    std::string result = NativeFormatNumberFunction::FormatWithGrouping(1000.0, 0);
    EXPECT_EQ(result, "1,000");
}

// ==================== NativeFormatStringFunction ====================

class NativeFormatStringFunctionTest : public ::testing::Test {
protected:
    NativeFormatStringFunction function_;
};

TEST_F(NativeFormatStringFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "formatString");
}

TEST_F(NativeFormatStringFunctionTest, should_return_empty_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatStringFunctionTest, should_return_empty_when_value_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123})"), "\"\"");
}

TEST_F(NativeFormatStringFunctionTest, should_return_empty_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "\"\"");
}

TEST_F(NativeFormatStringFunctionTest, should_return_original_text)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"Hello World"})"), "\"Hello World\"");
}

TEST_F(NativeFormatStringFunctionTest, should_return_empty_string_value)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":""})"), "\"\"");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_return_plain_text_without_template)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"plain text"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "plain text");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_return_empty_when_args_not_object)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_return_empty_when_value_not_string)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":123})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_escaped_dollar_brace)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"\\${literal"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "${literal");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unmatched_brace)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${unmatched"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "${unmatched");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_plain_text)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"no templates here"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "no templates here");
}

// ==================== NativeFunctionBase ====================

class NativeFunctionBaseTest : public ::testing::Test {};

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_empty_type)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_TRUE(function.ValidateReturnType("", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_void_when_null)
{
    NativeRequiredFunction function;
    auto nullAdapter = JsonAdapter::CreateNull();
    ASSERT_NE(nullAdapter, nullptr);
    EXPECT_TRUE(function.ValidateReturnType("void", nullAdapter->GetRoot()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_string_type)
{
    NativeFormatStringFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_TRUE(function.ValidateReturnType("string", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_number_type)
{
    NativeRequiredFunction function;
    auto numAdapter = JsonAdapter::CreateNumber(42.0);
    ASSERT_NE(numAdapter, nullptr);
    EXPECT_TRUE(function.ValidateReturnType("number", numAdapter->GetRoot()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_boolean_type)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_TRUE(function.ValidateReturnType("boolean", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_null_type)
{
    NativeRequiredFunction function;
    auto nullAdapter = JsonAdapter::CreateNull();
    ASSERT_NE(nullAdapter, nullptr);
    EXPECT_TRUE(function.ValidateReturnType("null", nullAdapter->GetRoot()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_array_type)
{
    NativeRequiredFunction function;
    auto arrAdapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(arrAdapter, nullptr);
    EXPECT_TRUE(function.ValidateReturnType("array", arrAdapter->GetRoot()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_true_for_object_type)
{
    NativeRequiredFunction function;
    auto objAdapter = JsonAdapter::Parse(R"({"key":"value"})");
    ASSERT_NE(objAdapter, nullptr);
    EXPECT_TRUE(function.ValidateReturnType("object", objAdapter->GetRoot()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_unknown_type)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("unknown_type", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ExecuteWithContext_should_delegate_to_execute_by_default)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(adapter, nullptr);
    DynamicResolveContext context;
    FunctionResult result = function.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_TRUE(result.GetBoolValue(false));
}

// ==================== NativeFunctionRegistry ====================

class NativeFunctionRegistryTest : public ::testing::Test {};

class NativeComponentValueFunctionContractTest : public A2UITest {
protected:
    static constexpr int32_t RENDER_ID = 1880;
    static constexpr const char* SURFACE_ID = "component_value_contract_surface";

    void SetUp() override
    {
        A2UITest::SetUp();
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(RENDER_ID);
        renderSlot.GetSurfaceManager()->CreateSurface(SURFACE_ID);
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(RENDER_ID);
        A2UITest::TearDown();
    }

    DynamicResolveContext BuildContext() const
    {
        DynamicResolveContext context;
        context.renderId = RENDER_ID;
        context.surfaceId = SURFACE_ID;
        context.componentId = "source";
        return context;
    }

    SurfaceSlot* GetSurface() const
    {
        return RenderManager::GetInstance().FindSurface(SURFACE_ID);
    }
};

namespace {

class ThrowingNativeFunction : public NativeFunctionBase {
public:
    std::string GetName() const override
    {
        return "throwing";
    }

    FunctionResult Execute(const JsonValue& resolvedArgs) override
    {
        (void)resolvedArgs;
        throw std::runtime_error("boom");
    }
};

class InvalidResultNativeFunction : public NativeFunctionBase {
public:
    std::string GetName() const override
    {
        return "invalid_result";
    }

    FunctionResult Execute(const JsonValue& resolvedArgs) override
    {
        (void)resolvedArgs;
        FunctionResult result;
        auto* layout = reinterpret_cast<FunctionResultLayout*>(&result);
        layout->type_ = static_cast<FunctionResultType>(-1);
        return result;
    }
};

} // namespace

TEST_F(NativeFunctionRegistryTest, should_have_all_standard_functions)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    EXPECT_TRUE(registry.HasFunction("required"));
    EXPECT_TRUE(registry.HasFunction("regex"));
    EXPECT_TRUE(registry.HasFunction("length"));
    EXPECT_TRUE(registry.HasFunction("numeric"));
    EXPECT_TRUE(registry.HasFunction("email"));
    EXPECT_TRUE(registry.HasFunction("formatString"));
    EXPECT_TRUE(registry.HasFunction("formatNumber"));
    EXPECT_TRUE(registry.HasFunction("formatCurrency"));
    EXPECT_TRUE(registry.HasFunction("formatDate"));
    EXPECT_TRUE(registry.HasFunction("pluralize"));
    EXPECT_TRUE(registry.HasFunction("and"));
    EXPECT_TRUE(registry.HasFunction("or"));
    EXPECT_TRUE(registry.HasFunction("not"));
}

TEST_F(NativeFunctionRegistryTest, should_not_have_unknown_function)
{
    EXPECT_FALSE(NativeFunctionRegistry::GetInstance().HasFunction("unknown_func"));
}

TEST_F(NativeFunctionRegistryTest, should_execute_known_function_successfully)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"user@example.com"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("email", argsAdapter->GetRoot(), context);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.value.GetBoolValue(false));
}

TEST_F(NativeFunctionRegistryTest, should_fail_for_unknown_function)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("nonexistent", argsAdapter->GetRoot(), context);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.functionName, "nonexistent");
}

TEST_F(NativeFunctionRegistryTest, should_validate_return_type_when_specified)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;

    ResolvedValue boolResult = registry.Execute("required", argsAdapter->GetRoot(), context, "boolean");
    EXPECT_TRUE(boolResult.success);

    ResolvedValue stringResult = registry.Execute("required", argsAdapter->GetRoot(), context, "string");
    EXPECT_FALSE(stringResult.success);
}

TEST_F(NativeFunctionRegistryTest, Register_should_not_register_empty_name)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto func = std::make_shared<NativeRequiredFunction>();
    registry.Register("", func);
    EXPECT_FALSE(registry.HasFunction(""));
}

TEST_F(NativeFunctionRegistryTest, Register_should_not_register_null_handler)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("test_null", nullptr);
    EXPECT_FALSE(registry.HasFunction("test_null"));
}

TEST_F(NativeFunctionRegistryTest, Execute_should_return_function_name_on_success)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("required", argsAdapter->GetRoot(), context);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.functionName, "required");
}

TEST_F(NativeFunctionRegistryTest, Execute_should_fail_when_function_throws)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("throwing_for_test", std::make_shared<ThrowingNativeFunction>());
    auto argsAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("throwing_for_test", argsAdapter->GetRoot(), context);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.functionName, "throwing_for_test");
}

TEST_F(NativeFunctionRegistryTest, Execute_should_fail_when_result_json_is_invalid)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    registry.Register("invalid_result_for_test", std::make_shared<InvalidResultNativeFunction>());
    auto argsAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("invalid_result_for_test", argsAdapter->GetRoot(), context);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.functionName, "invalid_result_for_test");
}

// ==================== NativeLengthFunction ====================

class NativeLengthFunctionTest : public ::testing::Test {
protected:
    NativeLengthFunction function_;
};

TEST_F(NativeLengthFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "length");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_value_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123,"min":1})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_no_min_or_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello"})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_min_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","min":"five"})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_max_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","max":"ten"})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_length_meets_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","min":5})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_length_below_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hi","min":5})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_length_within_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hi","max":5})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_length_above_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello world","max":5})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_length_within_min_and_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","min":3,"max":10})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_length_outside_range)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello world","min":3,"max":5})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_length_equals_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","min":3})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_length_equals_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","max":3})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_true_when_valid_min_only)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","min":1})");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(function_.Execute(adapter->GetRoot()).GetBoolValue(false));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"min":1,"max":10})"), "false");
}

// ==================== NativeNotFunction ====================

class NativeNotFunctionTest : public ::testing::Test {
protected:
    NativeNotFunction function_;
};

TEST_F(NativeNotFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "not");
}

TEST_F(NativeNotFunctionTest, should_return_true_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_TRUE(result.GetBoolValue(false));
}

TEST_F(NativeNotFunctionTest, should_return_true_when_args_is_string)
{
    auto adapter = JsonAdapter::Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_TRUE(result.GetBoolValue(false));
}

TEST_F(NativeNotFunctionTest, should_return_true_when_value_not_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123})"), "true");
}

TEST_F(NativeNotFunctionTest, should_return_true_when_value_is_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello"})"), "true");
}

TEST_F(NativeNotFunctionTest, should_return_true_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "true");
}

TEST_F(NativeNotFunctionTest, should_return_false_when_value_is_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":true})"), "false");
}

TEST_F(NativeNotFunctionTest, should_return_true_when_value_is_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":false})"), "true");
}

TEST_F(NativeNotFunctionTest, should_return_true_when_value_is_null)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":null})"), "true");
}

// ==================== NativeNumericFunction ====================

class NativeNumericFunctionTest : public ::testing::Test {
protected:
    NativeNumericFunction function_;
};

TEST_F(NativeNumericFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "numeric");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_args_is_array)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","min":1})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"min":1})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_is_nan)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":null,"min":1})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_no_min_and_no_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_min_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"min":"small"})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_min_is_nan)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"min":null})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_below_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":3,"min":5})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_equals_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"min":5})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_above_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"min":5})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_max_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"max":"big"})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_max_is_nan)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"max":null})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_above_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"max":5})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_equals_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"max":5})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_below_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":3,"max":5})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_within_min_and_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"min":1,"max":10})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_false_when_value_outside_range)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":15,"min":1,"max":10})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_true_when_value_at_range_boundaries)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"min":1,"max":10})"), "true");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"min":1,"max":10})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_true_with_negative_value_and_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":-3,"min":-5})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_false_with_negative_value_below_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":-10,"min":-5})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_true_with_negative_value_and_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":-3,"max":0})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_false_with_negative_value_above_max)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"max":-1})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_return_true_with_decimal_value_in_range)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":2.5,"min":1.0,"max":3.0})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_return_false_with_decimal_value_below_min)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0.5,"min":1.0})"), "false");
}

TEST_F(NativeNumericFunctionTest, should_work_with_min_only_max_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"min":10})"), "true");
}

TEST_F(NativeNumericFunctionTest, should_work_with_max_only_min_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"max":10})"), "true");
}

// ==================== NativeOrFunction ====================

class NativeOrFunctionTest : public ::testing::Test {
protected:
    NativeOrFunction function_;
};

TEST_F(NativeOrFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "or");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeOrFunctionTest, should_return_false_when_args_is_string)
{
    auto adapter = JsonAdapter::Parse(R"("hello")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeOrFunctionTest, should_return_false_when_values_not_array)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":true})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_values_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_array_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_array_has_one_item)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_true_when_any_item_is_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[false,true,false]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_return_true_when_first_item_is_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,false,false]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_return_true_when_last_item_is_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[false,false,true]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_return_true_when_all_items_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,true,true]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_return_true_with_exactly_two_true_items)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,true]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_all_items_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[false,false,false]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_all_items_false_with_two)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[false,false]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_skip_non_bool_items_and_return_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":["hello",true,123]})"), "true");
}

TEST_F(NativeOrFunctionTest, should_skip_non_bool_items_and_return_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":["hello",123,false]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_skip_all_non_bool_items_and_return_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":["a","b","c"]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_item_is_number_instead_of_bool)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[0,1,2]})"), "false");
}

TEST_F(NativeOrFunctionTest, should_return_false_when_mixed_non_bool_and_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[null,false,"str",false]})"), "false");
}

// ==================== NativePluralizeFunction ====================

class NativePluralizeFunctionTest : public ::testing::Test {
protected:
    NativePluralizeFunction function_;

    void SetUp() override
    {
        PluralLocaleManager::GetInstance().SetLocale("en");
    }
};

TEST_F(NativePluralizeFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "pluralize");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("x"), "");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_args_is_array)
{
    auto adapter = JsonAdapter::Parse(R"([1,2])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("x"), "");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_value_not_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","one":"item","other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"one":"item","other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_one_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":123,"other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_two_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"two":123,"other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_few_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"few":123,"other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_many_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"many":123,"other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_zero_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"zero":123,"other":"items"})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_when_other_key_is_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"other":123})"), "\"\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_category_string_when_matched)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"single","other":"multi"})"), "\"single\"");
}

TEST_F(NativePluralizeFunctionTest, should_fallback_to_other_when_category_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"single","other":"multi"})"), "\"multi\"");
}

TEST_F(NativePluralizeFunctionTest, should_return_empty_other_when_no_keys_provided)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5})"), "\"\"");
}

// --- Default (en): PluralOneOther ---
TEST_F(NativePluralizeFunctionTest, en_should_return_one_when_n_is_1)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, en_should_return_other_when_n_is_0)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, en_should_return_other_when_decimal)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, en_should_parse_locale_with_dash)
{
    PluralLocaleManager::GetInstance().SetLocale("en-US");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","other":"other"})"), "\"one\"");
}

// --- French (fr): PluralFrenchOneOther ---
TEST_F(NativePluralizeFunctionTest, fr_should_return_one_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, fr_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, fr_should_return_other_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":2,"one":"one","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, fr_should_return_one_when_decimal_intpart_0)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0.5,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, fr_should_return_one_when_decimal_intpart_1)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, fr_should_return_other_when_decimal_intpart_2)
{
    PluralLocaleManager::GetInstance().SetLocale("fr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":2.5,"one":"one","other":"other"})"), "\"other\"");
}

// --- Portuguese (pt): also PluralFrenchOneOther ---
TEST_F(NativePluralizeFunctionTest, pt_should_return_one_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("pt-BR");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","other":"other"})"), "\"one\"");
}

// --- Russian (ru): PluralOneFewManyOther ---
TEST_F(NativePluralizeFunctionTest, ru_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_one_when_mod10_1_mod100_not_11)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":21,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_few_when_mod10_2_to_4_mod100_not_12_14)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":22,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_many_when_mod10_0)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_many_when_mod10_5_to_9)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_many_when_mod100_11_to_14)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, ru_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("ru");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","many":"many","other":"other"})"), "\"one\"");
}

// --- Ukrainian (uk): same as ru ---
TEST_F(NativePluralizeFunctionTest, uk_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("uk");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","many":"many","other":"other"})"), "\"one\"");
}

// --- Belarusian (be): same as ru ---
TEST_F(NativePluralizeFunctionTest, be_should_return_few_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("be");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","few":"few","many":"many","other":"other"})"), "\"few\"");
}

// --- Bosnian (bs): same as ru ---
TEST_F(NativePluralizeFunctionTest, bs_should_return_one_when_21)
{
    PluralLocaleManager::GetInstance().SetLocale("bs");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":21,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

// --- Polish (pl): PluralPolish ---
TEST_F(NativePluralizeFunctionTest, pl_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("pl");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, pl_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("pl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","many":"many","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, pl_should_return_few_when_mod10_2_to_4_mod100_not_12_14)
{
    PluralLocaleManager::GetInstance().SetLocale("pl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","few":"few","many":"many","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, pl_should_return_many_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("pl");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, pl_should_return_many_when_mod100_12_to_14)
{
    PluralLocaleManager::GetInstance().SetLocale("pl");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":12,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

// --- Czech (cs): PluralOneFewOther ---
TEST_F(NativePluralizeFunctionTest, cs_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("cs");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, cs_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("cs");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, cs_should_return_few_when_n_is_2_to_4)
{
    PluralLocaleManager::GetInstance().SetLocale("cs");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":3,"one":"one","few":"few","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, cs_should_return_other_when_n_is_5)
{
    PluralLocaleManager::GetInstance().SetLocale("cs");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

// --- Slovak (sk): same as cs ---
TEST_F(NativePluralizeFunctionTest, sk_should_return_few_when_n_is_4)
{
    PluralLocaleManager::GetInstance().SetLocale("sk");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":4,"one":"one","few":"few","other":"other"})"), "\"few\"");
}

// --- Lithuanian (lt): PluralLithuanian ---
TEST_F(NativePluralizeFunctionTest, lt_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, lt_should_return_one_when_mod10_1_mod100_not_11)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, lt_should_return_few_when_mod10_2_to_9_mod100_not_12_19)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":2,"one":"one","few":"few","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, lt_should_return_other_when_mod10_1_mod100_11)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, lt_should_return_other_when_mod10_2_mod100_12)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":12,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, lt_should_return_other_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("lt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","few":"few","other":"other"})"), "\"other\"");
}

// --- Latvian (lv): PluralLatvian ---
TEST_F(NativePluralizeFunctionTest, lv_should_return_zero_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("lv");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"zero":"zero","one":"one","other":"other"})"), "\"zero\"");
}

TEST_F(NativePluralizeFunctionTest, lv_should_return_zero_when_mod10_0)
{
    PluralLocaleManager::GetInstance().SetLocale("lv");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"zero":"zero","one":"one","other":"other"})"), "\"zero\"");
}

TEST_F(NativePluralizeFunctionTest, lv_should_return_zero_when_mod100_11_to_19)
{
    PluralLocaleManager::GetInstance().SetLocale("lv");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"zero":"zero","one":"one","other":"other"})"), "\"zero\"");
}

TEST_F(NativePluralizeFunctionTest, lv_should_return_one_when_mod10_1_mod100_not_11)
{
    PluralLocaleManager::GetInstance().SetLocale("lv");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"zero":"zero","one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, lv_should_return_other_when_mod10_2)
{
    PluralLocaleManager::GetInstance().SetLocale("lv");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":2,"zero":"zero","one":"one","other":"other"})"), "\"other\"");
}

// --- Welsh (cy): PluralWelsh ---
TEST_F(NativePluralizeFunctionTest, cy_should_return_zero_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":0,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"zero\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":1,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_two_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":2,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_few_when_n_is_3)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":3,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_many_when_n_is_6)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":6,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_other_when_n_is_4)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":4,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, cy_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("cy");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":1.5,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Irish (ga): PluralIrish ---
TEST_F(NativePluralizeFunctionTest, ga_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("ga");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, ga_should_return_two_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("ga");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, ga_should_return_few_when_n_is_3_to_6)
{
    PluralLocaleManager::GetInstance().SetLocale("ga");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":5,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, ga_should_return_many_when_n_is_7_to_10)
{
    PluralLocaleManager::GetInstance().SetLocale("ga");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":7,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, ga_should_return_other_when_n_is_11)
{
    PluralLocaleManager::GetInstance().SetLocale("ga");
    EXPECT_EQ(ExecuteAsLiteral(
                  function_, R"({"value":11,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Breton (br): PluralBreton ---
TEST_F(NativePluralizeFunctionTest, br_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("br");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, br_should_return_two_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("br");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, br_should_return_few_when_n_is_3)
{
    PluralLocaleManager::GetInstance().SetLocale("br");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":3,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, br_should_return_many_when_n_is_6)
{
    PluralLocaleManager::GetInstance().SetLocale("br");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":6,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, br_should_return_other_when_n_is_4)
{
    PluralLocaleManager::GetInstance().SetLocale("br");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":4,"one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Hebrew (he): PluralHebrew ---
TEST_F(NativePluralizeFunctionTest, he_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","two":"two","many":"many","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_two_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","two":"two","many":"many","other":"other"})"), "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_many_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"one":"one","two":"two","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_many_when_n_10_to_20)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":10,"one":"one","two":"two","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_many_when_n_gt_20_mod10_0)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":30,"one":"one","two":"two","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_other_when_n_is_3)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":3,"one":"one","two":"two","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, he_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("he");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","two":"two","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Icelandic (is): PluralIcelandic ---
TEST_F(NativePluralizeFunctionTest, is_should_return_one_when_mod10_1_mod100_not_11)
{
    PluralLocaleManager::GetInstance().SetLocale("is");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, is_should_return_other_when_mod100_11)
{
    PluralLocaleManager::GetInstance().SetLocale("is");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"one":"one","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, is_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("is");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","other":"other"})"), "\"other\"");
}

// --- Macedonian (mk): PluralMacedonian ---
TEST_F(NativePluralizeFunctionTest, mk_should_return_one_when_mod10_1_n_not_11)
{
    PluralLocaleManager::GetInstance().SetLocale("mk");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, mk_should_return_other_when_n_is_11)
{
    PluralLocaleManager::GetInstance().SetLocale("mk");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"one":"one","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, mk_should_return_one_when_21)
{
    PluralLocaleManager::GetInstance().SetLocale("mk");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":21,"one":"one","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, mk_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("mk");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","other":"other"})"), "\"other\"");
}

// --- Slovenian (sl): PluralSlovenian ---
TEST_F(NativePluralizeFunctionTest, sl_should_return_one_when_mod100_1)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","two":"two","few":"few","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, sl_should_return_two_when_mod100_2)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","two":"two","few":"few","other":"other"})"), "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, sl_should_return_few_when_mod100_3)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":3,"one":"one","two":"two","few":"few","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, sl_should_return_few_when_mod100_4)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":4,"one":"one","two":"two","few":"few","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, sl_should_return_other_when_mod100_0)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":0,"one":"one","two":"two","few":"few","other":"other"})"), "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, sl_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("sl");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","two":"two","few":"few","other":"other"})"),
        "\"other\"");
}

// --- Maltese (mt): PluralMaltese ---
TEST_F(NativePluralizeFunctionTest, mt_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":1,"one":"one","few":"few","many":"many","other":"other"})"), "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, mt_should_return_few_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":0,"one":"one","few":"few","many":"many","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, mt_should_return_few_when_mod100_2_to_10)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","few":"few","many":"many","other":"other"})"), "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, mt_should_return_many_when_mod100_11_to_19)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":11,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, mt_should_return_other_when_mod100_0_and_n_not_0_not_1)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, mt_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("mt");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":1.5,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Arabic (ar): PluralArabic ---
TEST_F(NativePluralizeFunctionTest, ar_should_return_zero_when_n_is_0)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":0,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"zero\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_one_when_n_is_1)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":1,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_two_when_n_is_2)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":2,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"two\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_few_when_mod100_3_to_10)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":3,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"few\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_many_when_mod100_11_to_99)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":99,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"many\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_other_when_mod100_0_and_n_gt_100)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":100,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

TEST_F(NativePluralizeFunctionTest, ar_should_return_other_when_decimal)
{
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function_,
                  R"({"value":1.5,"zero":"zero","one":"one","two":"two","few":"few","many":"many","other":"other"})"),
        "\"other\"");
}

// --- Croatian (hr): same as ru ---
TEST_F(NativePluralizeFunctionTest, hr_should_return_one_when_21)
{
    PluralLocaleManager::GetInstance().SetLocale("hr");
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":21,"one":"one","few":"few","many":"many","other":"other"})"),
        "\"one\"");
}

// --- Serbian (sr): same as ru ---
TEST_F(NativePluralizeFunctionTest, sr_should_return_few_when_2)
{
    PluralLocaleManager::GetInstance().SetLocale("sr");
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":2,"one":"one","few":"few","many":"many","other":"other"})"), "\"few\"");
}

// --- ValidateOptionalString: missing key is valid ---
TEST_F(NativePluralizeFunctionTest, should_pass_when_plural_keys_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":5,"one":"item"})"), "\"\"");
}

class PluralLocaleManagerProviderTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x620100);

    void SetUp() override
    {
        A2UITest::SetUp();
        ResetPluralLocaleManagerState();
    }

    void TearDown() override
    {
        ResetPluralLocaleManagerState();
        A2UITest::TearDown();
    }

    napi_value CreateCallback()
    {
        napi_value callback = nullptr;
        mockNapiPtr_->CreateFunction(env_, "localeProvider", NAPI_AUTO_LENGTH, nullptr, nullptr, &callback);
        return callback;
    }

    void RegisterProvider()
    {
        PluralLocaleManager::GetInstance().RegisterLocaleProvider(env_, CreateCallback());
    }

    napi_value PredictNextCallFunctionResult() const
    {
        return reinterpret_cast<napi_value>(static_cast<intptr_t>(mockNapiPtr_->nextValueId_));
    }

    void SetNextProviderResultString(const std::string& locale)
    {
        napi_value result = PredictNextCallFunctionResult();
        mockNapiPtr_->valueTypes_[result] = napi_string;
        mockNapiPtr_->stringValues_[result] = locale;
    }
};

TEST_F(PluralLocaleManagerProviderTest, RegisterLocaleProvider_should_ignore_invalid_args)
{
    PluralLocaleManager::GetInstance().SetLocale("cached");
    PluralLocaleManager::GetInstance().RegisterLocaleProvider(nullptr, nullptr);

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_fallback_to_cached_locale_when_create_reference_fails)
{
    PluralLocaleManager::GetInstance().SetLocale("cached");
    mockNapiPtr_->SetCreateReferenceStatus(napi_invalid_arg);
    PluralLocaleManager::GetInstance().RegisterLocaleProvider(env_, CreateCallback());
    mockNapiPtr_->ResetCreateReferenceStatus();

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_use_provider_result_and_cache_it_before_ttl)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    SetNextProviderResultString("fr");

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "fr");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1u);

    SetNextProviderResultString("de");
    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "fr");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_refresh_provider_after_ttl_expires)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    SetNextProviderResultString("fr");
    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "fr");

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    SetNextProviderResultString("de");
    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "de");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 2u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_fallback_to_cached_locale_when_get_reference_value_fails)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    mockNapiPtr_->SetGetReferenceValueStatus(napi_invalid_arg);

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_fallback_to_cached_locale_when_call_function_fails)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    mockNapiPtr_->SetCallFunctionStatus(napi_invalid_arg);

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 0u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_fallback_to_cached_locale_when_provider_returns_empty_string)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    SetNextProviderResultString("");

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1u);
}

TEST_F(PluralLocaleManagerProviderTest, GetLocale_should_fallback_to_cached_locale_when_string_read_fails)
{
    RegisterProvider();
    PluralLocaleManager::GetInstance().SetLocale("cached");
    SetNextProviderResultString("fr");
    mockNapiPtr_->SetGetValueStringUtf8Status(napi_invalid_arg);

    EXPECT_EQ(PluralLocaleManager::GetInstance().GetLocale(), "cached");
    EXPECT_EQ(mockNapiPtr_->callFunctionCallCount_, 1u);
}

// ==================== NativeRegexFunction ====================

class NativeRegexFunctionTest : public ::testing::Test {
protected:
    NativeRegexFunction function_;
};

TEST_F(NativeRegexFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "regex");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_args_is_array)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_value_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123,"pattern":"\\d+"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"pattern":"\\d+"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_pattern_not_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","pattern":123})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_pattern_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_pattern_empty)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","pattern":""})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_true_when_value_matches_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"12345","pattern":"\\d+"})"), "true");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_value_does_not_match)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"abc","pattern":"\\d+"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_match_full_string_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello","pattern":"hello"})"), "true");
}

TEST_F(NativeRegexFunctionTest, should_not_match_partial)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello world","pattern":"hello"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_return_false_when_invalid_regex_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"test","pattern":"[invalid"})"), "false");
}

TEST_F(NativeRegexFunctionTest, should_match_email_like_pattern)
{
    EXPECT_EQ(
        ExecuteAsLiteral(function_, R"({"value":"user@example.com","pattern":"[a-z]+@[a-z]+\\.[a-z]+"})"), "true");
}

TEST_F(NativeRegexFunctionTest, should_return_true_with_empty_value_and_matching_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"","pattern":".*"})"), "true");
}

TEST_F(NativeRegexFunctionTest, should_return_false_with_empty_value_and_non_empty_pattern)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"","pattern":"\\d+"})"), "false");
}

// ==================== NativeRequiredFunction ====================

class NativeRequiredFunctionTest : public ::testing::Test {
protected:
    NativeRequiredFunction function_;
};

TEST_F(NativeRequiredFunctionTest, should_return_correct_name)
{
    EXPECT_EQ(function_.GetName(), "required");
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_args_not_object)
{
    auto adapter = JsonAdapter::Parse(R"("string")");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_args_is_array)
{
    auto adapter = JsonAdapter::Parse(R"([1,2,3])");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_value_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({})"), "false");
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_value_is_null)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":null})"), "false");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_non_empty_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello"})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_value_is_empty_string)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":""})"), "false");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_number)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":42})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_zero)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_bool_true)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":true})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_bool_false)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":false})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_non_empty_array)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":[1,2,3]})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_value_is_empty_array)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":[]})"), "false");
}

TEST_F(NativeRequiredFunctionTest, should_return_true_when_value_is_non_empty_object)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":{"key":"val"}})"), "true");
}

TEST_F(NativeRequiredFunctionTest, should_return_false_when_value_is_empty_object)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":{}})"), "false");
}

TEST_F(NativeRequiredFunctionTest, IsPresent_should_return_false_when_invalid)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    JsonValue invalidVal = adapter->GetRoot().GetItem("nonexistent");
    EXPECT_FALSE(NativeRequiredFunction::IsPresent(invalidVal));
}

// ==================== Branch Coverage Supplement: NativeAndFunction ====================

TEST_F(NativeAndFunctionTest, should_return_false_when_array_contains_null)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,null,true]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_array_contains_object)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,{},true]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_false_when_array_contains_array)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,[],true]})"), "false");
}

TEST_F(NativeAndFunctionTest, should_return_true_with_many_true_items)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"values":[true,true,true,true,true]})"), "true");
}

// ==================== Branch Coverage Supplement: NativeEmailFunction ====================

TEST_F(NativeEmailFunctionTest, should_reject_email_with_double_at_sign)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@@example.com"})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_with_trailing_dot_in_domain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@example.com."})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_validate_simple_two_part_domain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"a@b.cd"})"), "true");
}

// ==================== Branch Coverage Supplement: NativeFormatCurrencyFunction ====================

TEST_F(NativeFormatCurrencyFunctionTest, should_use_default_decimals_when_missing)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100.456,"currency":"USD"})"), "\"USD 100.46\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_decimals_is_nan)
{
    auto adapter = JsonAdapter::Parse(R"({"value":100,"currency":"USD","decimals":null})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_return_empty_when_decimals_is_inf)
{
    auto adapter = JsonAdapter::Parse(R"({"value":100,"currency":"USD","decimals":1e308})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_use_negative_decimals_as_abs)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":100.123,"currency":"USD","decimals":-2})"), "\"USD 100.12\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_with_zero_value)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0,"currency":"EUR"})"), "\"EUR 0.00\"");
}

TEST_F(NativeFormatCurrencyFunctionTest, should_format_negative_value)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":-99.99,"currency":"USD"})"), "\"USD -99.99\"");
}

// ==================== Branch Coverage Supplement: NativeFormatDateFunction ====================

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_iso_has_non_numeric_year)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"XXXX-01-16","format":"yyyy"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_iso_has_non_numeric_time)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16TAB:cd:ef","format":"HH"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_return_empty_when_iso_time_partials_invalid)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16TAB:cd:ef","format":"HH"})"), "\"\"");
}

TEST_F(NativeFormatDateFunctionTest, should_handle_iso_with_T_and_partial_time_11_to_15)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16TAB","format":"yyyy-MM-dd"})"), "\"2026-01-16\"");
}

TEST_F(NativeFormatDateFunctionTest, should_parse_iso_with_11_chars_T_but_no_time)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-16T","format":"yyyy-MM-dd"})"), "\"2026-01-16\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_yyyy_with_short_year)
{
    NativeFormatDateFunction::DateTimeParts parts = { .year = 26 };
    std::string result = NativeFormatDateFunction::ApplyPattern(parts, "yyyy");
    EXPECT_EQ(result, "0026");
}

TEST_F(NativeFormatDateFunctionTest, should_format_day_name_for_monday)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-19","format":"EEEE"})"), "\"Monday\"");
}

TEST_F(NativeFormatDateFunctionTest, should_format_day_abbrev_for_monday)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"2026-01-19","format":"E"})"), "\"Mon\"");
}

TEST_F(NativeFormatDateFunctionTest, should_handle_literal_text_in_pattern)
{
    NativeFormatDateFunction::DateTimeParts parts = {
        .year = 2026, .month = 1, .day = 16, .hour = 14, .minute = 30, .second = 7
    };
    std::string result = NativeFormatDateFunction::ApplyPattern(parts, "|yyyy|");
    EXPECT_EQ(result, "|2026|");
}

// ==================== Branch Coverage Supplement: NativeFormatNumberFunction ====================

TEST_F(NativeFormatNumberFunctionTest, should_use_default_decimals_when_missing)
{
    auto adapter = JsonAdapter::Parse(R"({"value":123.456})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue(""), "123.46");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_decimals_is_nan)
{
    auto adapter = JsonAdapter::Parse(R"({"value":100,"decimals":null})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatNumberFunctionTest, should_return_empty_when_decimals_is_inf)
{
    auto adapter = JsonAdapter::Parse(R"({"value":100,"decimals":1e308})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_EQ(result.GetStringValue("fallback"), "");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_with_negative_decimals_as_abs)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":123.456,"decimals":-2})"), "\"123.46\"");
}

TEST_F(NativeFormatNumberFunctionTest, should_format_zero_value)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":0})"), "\"0.00\"");
}

// ==================== Branch Coverage Supplement: NativeFormatStringFunction ====================

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_resolve_function_call)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"Result: ${formatNumber(value:42)}!"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "Result: 42.00!");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_resolve_function_with_quoted_string_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${email(value:'test@example.com')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "true");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_unquoted_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:42, decimals:0)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "42");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_bool_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:1000, grouping:true)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "1,000.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_null_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:null)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_unclosed_quote)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${formatNumber(value:'unclosed)}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_no_colon_in_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${formatNumber(nocol)}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_empty_arg_name)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${formatNumber(:42)}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_no_value_after_colon)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${formatNumber(value:)}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_inner_brace_in_func_args_resolved_empty)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:${x})}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_inner_brace_unclosed_in_args)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:${x)})"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_NE(result.GetStringValue(""), "42.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_non_function_template)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${somePath}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unknown_function)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${unknownFunc(value:'x')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_escaped_dollar_at_end_of_string)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"price is $10"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "price is $10");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_dollar_without_brace)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"$10 off"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "$10 off");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_resolve_formatCurrency)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${formatCurrency(value:99.99, currency:'USD')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "USD 99.99");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_multiple_args)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:1234.5, decimals:3, grouping:true)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "1,234.500");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_no_parens_as_path)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${myValue}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_with_parens_but_no_closing)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${func(arg:x}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_nested_dollar_brace_in_func_args)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:${somePath})}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_multiple_templates)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"a=${formatNumber(value:1)} b=${formatNumber(value:2)}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "a=1.00 b=2.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_put_arg_on_object)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${required(value:'hello')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "true");
}

// ==================== Branch Coverage Supplement: NativeFunctionRegistry ====================

TEST_F(NativeFunctionRegistryTest, should_have_getToggleValue_function)
{
    EXPECT_TRUE(NativeFunctionRegistry::GetInstance().HasFunction("getToggleValue"));
}

TEST_F(NativeFunctionRegistryTest, should_have_getRadioValue_function)
{
    EXPECT_TRUE(NativeFunctionRegistry::GetInstance().HasFunction("getRadioValue"));
}

TEST_F(NativeFunctionRegistryTest, should_have_getCheckboxGroupValues_function)
{
    EXPECT_TRUE(NativeFunctionRegistry::GetInstance().HasFunction("getCheckboxGroupValues"));
}

TEST_F(NativeFunctionRegistryTest, should_fail_getRadioValue_when_group_missing)
{
    auto argsAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    context.surfaceId = "surface";
    context.componentId = "component";
    ResolvedValue result =
        NativeFunctionRegistry::GetInstance().Execute("getRadioValue", argsAdapter->GetRoot(), context, "string");
    EXPECT_FALSE(result.success);
}

TEST_F(NativeFunctionRegistryTest, should_fail_getToggleValue_when_component_id_missing)
{
    auto argsAdapter = JsonAdapter::Parse(R"({})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    context.surfaceId = "surface";
    context.componentId = "component";
    ResolvedValue result =
        NativeFunctionRegistry::GetInstance().Execute("getToggleValue", argsAdapter->GetRoot(), context, "object");
    EXPECT_FALSE(result.success);
}

TEST_F(NativeFunctionRegistryTest, should_fail_getToggleValue_when_using_uppercase_component_id_arg)
{
    auto argsAdapter = JsonAdapter::Parse(R"({"componentID":"toggleA"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    context.surfaceId = "surface";
    context.componentId = "component";
    ResolvedValue result =
        NativeFunctionRegistry::GetInstance().Execute("getToggleValue", argsAdapter->GetRoot(), context, "object");
    EXPECT_FALSE(result.success);
}

TEST_F(NativeComponentValueFunctionContractTest, should_return_empty_string_when_radio_group_missing)
{
    auto argsAdapter = JsonAdapter::Parse(R"({"group":"missing"})");
    ASSERT_NE(argsAdapter, nullptr);
    ResolvedValue result = NativeFunctionRegistry::GetInstance().Execute(
        "getRadioValue", argsAdapter->GetRoot(), BuildContext(), "string");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsString());
    EXPECT_EQ(result.value.GetStringValue("fallback"), "");
}

TEST_F(NativeComponentValueFunctionContractTest, should_return_empty_string_when_radio_group_has_no_selected_item)
{
    SurfaceSlot* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    auto radio = std::make_shared<CustomComponent>("Extended.Radio");
    radio->SetRuntimeCustomProperty("group", JsonAdapter::Parse(R"("g1")")->GetRoot());
    radio->SetRuntimeCustomProperty("checked", JsonAdapter::Parse(R"(false)")->GetRoot());
    radio->SetRuntimeCustomProperty("value", JsonAdapter::Parse(R"("radioA")")->GetRoot());
    surface->GetAllComponents()["radioA"] = radio;

    auto argsAdapter = JsonAdapter::Parse(R"({"group":"g1"})");
    ASSERT_NE(argsAdapter, nullptr);
    ResolvedValue result = NativeFunctionRegistry::GetInstance().Execute(
        "getRadioValue", argsAdapter->GetRoot(), BuildContext(), "string");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsString());
    EXPECT_EQ(result.value.GetStringValue("fallback"), "");
}

TEST_F(NativeComponentValueFunctionContractTest, should_return_empty_object_when_toggle_component_missing)
{
    auto argsAdapter = JsonAdapter::Parse(R"({"componentId":"missingToggle"})");
    ASSERT_NE(argsAdapter, nullptr);
    ResolvedValue result = NativeFunctionRegistry::GetInstance().Execute(
        "getToggleValue", argsAdapter->GetRoot(), BuildContext(), "object");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsObject());
    EXPECT_EQ(result.value.ToJsonLiteral(), "{}");
}

TEST_F(NativeComponentValueFunctionContractTest, should_return_empty_object_when_component_is_not_toggle)
{
    SurfaceSlot* surface = GetSurface();
    ASSERT_NE(surface, nullptr);
    surface->GetAllComponents()["notToggle"] = std::make_shared<CustomComponent>("Extended.Text");

    auto argsAdapter = JsonAdapter::Parse(R"({"componentId":"notToggle"})");
    ASSERT_NE(argsAdapter, nullptr);
    ResolvedValue result = NativeFunctionRegistry::GetInstance().Execute(
        "getToggleValue", argsAdapter->GetRoot(), BuildContext(), "object");
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsObject());
    EXPECT_EQ(result.value.ToJsonLiteral(), "{}");
}

TEST_F(NativeFunctionRegistryTest, Register_should_overwrite_existing_function)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto func = std::make_shared<NativeRequiredFunction>();
    registry.Register("required", func);
    EXPECT_TRUE(registry.HasFunction("required"));
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("required", argsAdapter->GetRoot(), context);
    EXPECT_TRUE(result.success);
}

TEST_F(NativeFunctionRegistryTest, should_execute_with_return_type_validation_pass)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("formatString", argsAdapter->GetRoot(), context, "string");
    EXPECT_TRUE(result.success);
}

TEST_F(NativeFunctionRegistryTest, should_execute_with_return_type_validation_fail)
{
    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    auto argsAdapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(argsAdapter, nullptr);
    DynamicResolveContext context;
    ResolvedValue result = registry.Execute("formatString", argsAdapter->GetRoot(), context, "number");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.functionName, "formatString");
}

// ==================== Branch Coverage Supplement: NativeFunctionBase ====================

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_void_when_not_null)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("void", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_string_when_not_string)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("string", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_number_when_not_number)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("number", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_boolean_when_not_bool)
{
    NativeFormatStringFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("boolean", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_null_when_not_null)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("null", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_array_when_not_array)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("array", result.ToJsonValue()));
}

TEST_F(NativeFunctionBaseTest, ValidateReturnType_should_return_false_for_object_when_not_object)
{
    NativeRequiredFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"test"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function.Execute(adapter->GetRoot());
    EXPECT_FALSE(function.ValidateReturnType("object", result.ToJsonValue()));
}

// ==================== Branch Coverage Supplement: NativeLengthFunction ====================

TEST_F(NativeLengthFunctionTest, should_return_false_when_min_is_nan)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","min":null})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_max_is_nan)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","max":null})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_min_is_outside_int32_range)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","min":2.147483648e9})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_max_is_outside_int32_range)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","max":2.147483648e9})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_true_with_max_only)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hi","max":5})"), "true");
}

TEST_F(NativeLengthFunctionTest, should_return_false_with_max_only_when_exceeded)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"hello world","max":5})"), "false");
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_min_below_int32_min)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","min":-2.147483649e9})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

TEST_F(NativeLengthFunctionTest, should_return_false_when_max_below_int32_min)
{
    auto adapter = JsonAdapter::Parse(R"({"value":"hello","max":-2.147483649e9})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.Execute(adapter->GetRoot());
    EXPECT_FALSE(result.GetBoolValue(true));
}

// ==================== Deep Branch Coverage: FormatString Data Model ====================

static constexpr int32_t FS_TEST_RENDER_ID = 9991;
static const std::string FS_TEST_SURFACE_ID = "fs-test-surface";

class FormatStringDataModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto& rm = RenderManager::GetInstance();
        rm.RemoveRenderSlot(FS_TEST_RENDER_ID);
        RenderSlot& slot = rm.CreateRenderSlot(FS_TEST_RENDER_ID);
        auto sm = slot.GetSurfaceManager();
        SurfaceSlot* surface = &sm->CreateSurface(FS_TEST_SURFACE_ID);
        surface->SetSurfaceId(FS_TEST_SURFACE_ID);
        dataModel_ = surface->GetOrCreateDataModel();
        auto root = JsonAdapter::Parse(R"({"user":{"name":"Alice","age":30},"active":true,"count":5})");
        ASSERT_NE(root, nullptr);
        dataModel_->ReplaceAll(root->GetRoot());
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(FS_TEST_RENDER_ID);
    }

    DynamicResolveContext MakeContext() const
    {
        DynamicResolveContext ctx;
        ctx.renderId = FS_TEST_RENDER_ID;
        ctx.surfaceId = FS_TEST_SURFACE_ID;
        return ctx;
    }

    std::shared_ptr<DataModel> dataModel_;
    NativeFormatStringFunction function_;
};

TEST_F(FormatStringDataModelTest, should_resolve_string_from_data_model)
{
    DynamicResolveContext ctx = MakeContext();
    auto adapter = JsonAdapter::Parse(R"({"value":"Hello ${user/name}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Hello Alice");
}

TEST_F(FormatStringDataModelTest, should_resolve_number_from_data_model)
{
    DynamicResolveContext ctx = MakeContext();
    auto adapter = JsonAdapter::Parse(R"({"value":"Age: ${user/age}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Age: 30");
}

TEST_F(FormatStringDataModelTest, should_resolve_bool_from_data_model)
{
    DynamicResolveContext ctx = MakeContext();
    auto adapter = JsonAdapter::Parse(R"({"value":"Active: ${active}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Active: true");
}

TEST_F(FormatStringDataModelTest, should_resolve_missing_path_as_empty)
{
    DynamicResolveContext ctx = MakeContext();
    ctx.renderId = -1;
    auto adapter = JsonAdapter::Parse(R"({"value":"Missing: ${nonexistent}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Missing: ");
}

TEST_F(FormatStringDataModelTest, should_resolve_path_without_leading_slash)
{
    DynamicResolveContext ctx = MakeContext();
    auto adapter = JsonAdapter::Parse(R"({"value":"Count: ${count}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Count: 5");
}

TEST_F(FormatStringDataModelTest, should_resolve_with_leading_slash)
{
    DynamicResolveContext ctx = MakeContext();
    auto adapter = JsonAdapter::Parse(R"({"value":"Count: ${/count}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), ctx);
    EXPECT_EQ(result.GetStringValue(""), "Count: 5");
}

// ==================== Deep Branch Coverage: FormatString Edge Cases ====================

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_output_bool_true_from_email)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"Valid: ${email(value:'a@b.cd')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "Valid: true");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_output_bool_false_from_email)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"Valid: ${email(value:'invalid')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "Valid: false");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_output_string_from_formatCurrency)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"Price: ${formatCurrency(value:9.99, currency:'USD')}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "Price: USD 9.99");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unquoted_true_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:1000, grouping:true)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "1,000.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unquoted_false_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:1000, grouping:false)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "1000.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unquoted_null_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:null)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_unquoted_string_arg)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${email(value:test@example.com)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "true");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_function_failing)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:'notanumber')}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_spaces_around_arg_name)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value : 42, decimals:0)}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "42");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_trailing_comma_in_args)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:42, )}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "42.00");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_empty_template)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":""})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_dollar_at_end)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"price$"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "price$");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_backslash_dollar_without_brace)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"\\$10"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "\\$10");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_resolve_required_true)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"Has: ${required(value:'hello')}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "Has: true");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_resolve_and_with_array)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"({"value":"${and(values:${inner})}"})");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "false");
}

TEST_F(NativeFormatStringFunctionTest, ExecuteWithContext_should_handle_nested_resolve_number)
{
    DynamicResolveContext context;
    auto adapter = JsonAdapter::Parse(R"xyz({"value":"${formatNumber(value:${num})}"})xyz");
    ASSERT_NE(adapter, nullptr);
    FunctionResult result = function_.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "");
}

// ==================== Deep Branch Coverage: NativeEmailFunction ====================

TEST_F(NativeEmailFunctionTest, should_reject_email_with_only_at_and_dot)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"@."})"), "false");
}

TEST_F(NativeEmailFunctionTest, should_validate_email_with_numbers_in_local)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user123@example.com"})"), "true");
}

TEST_F(NativeEmailFunctionTest, should_reject_email_with_space_in_domain)
{
    EXPECT_EQ(ExecuteAsLiteral(function_, R"({"value":"user@exa mple.com"})"), "false");
}

// ==================== Deep Branch Coverage: NativeFormatDateFunction ====================

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_parse_T_with_hours_only_12_chars)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_TRUE(NativeFormatDateFunction::ParseISO8601("2026-01-16T14", parts));
    EXPECT_EQ(parts.year, 2026);
    EXPECT_EQ(parts.month, 1);
    EXPECT_EQ(parts.day, 16);
    EXPECT_EQ(parts.hour, 0);
    EXPECT_EQ(parts.minute, 0);
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_parse_hours_minutes_only)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_TRUE(NativeFormatDateFunction::ParseISO8601("2026-01-16T14:30", parts));
    EXPECT_EQ(parts.hour, 14);
    EXPECT_EQ(parts.minute, 30);
    EXPECT_EQ(parts.second, 0);
}

TEST_F(NativeFormatDateFunctionTest, ParseISO8601_should_reject_invalid_hour_minute)
{
    NativeFormatDateFunction::DateTimeParts parts;
    EXPECT_FALSE(NativeFormatDateFunction::ParseISO8601("2026-01-16TAB:cd", parts));
}

TEST_F(NativeFormatDateFunctionTest, should_format_yyyy_for_year_26)
{
    NativeFormatDateFunction::DateTimeParts parts = {
        .year = 26, .month = 1, .day = 16, .hour = 14, .minute = 30, .second = 7
    };
    std::string result = NativeFormatDateFunction::ApplyPattern(parts, "yyyy");
    EXPECT_EQ(result, "0026");
}

// ==================== Deep Branch Coverage: NativeFunctionBase ====================

TEST_F(NativeFunctionBaseTest, ExecuteWithContext_should_pass_context_to_overriding_function)
{
    NativeFormatStringFunction function;
    auto adapter = JsonAdapter::Parse(R"({"value":"hello"})");
    ASSERT_NE(adapter, nullptr);
    DynamicResolveContext context;
    context.renderId = 42;
    FunctionResult result = function.ExecuteWithContext(adapter->GetRoot(), context);
    EXPECT_EQ(result.GetStringValue(""), "hello");
}
