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
#include <limits>
#include <string>

#include "adapter/NapiBridge.h"
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

#include "mock_napi_provider.h"

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

std::string ExecuteAsLiteral(NativeFunctionBase& function, const JsonValue& args)
{
    return function.Execute(args).ToJsonLiteral();
}

std::string ExecutePluralizeCategory(const std::string& locale, double value)
{
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateObject();
    if (adapter == nullptr) {
        return "null";
    }

    JsonValue root = adapter->GetRoot();
    if (!root.PutNumber("value", value) || !root.PutString("zero", "zero") || !root.PutString("one", "one") ||
        !root.PutString("two", "two") || !root.PutString("few", "few") || !root.PutString("many", "many") ||
        !root.PutString("other", "other")) {
        return "null";
    }

    PluralLocaleManager::GetInstance().SetLocale(locale);
    NativePluralizeFunction function;
    return function.Execute(root).ToJsonLiteral();
}

} // namespace

class NativeFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto mockNapi = MockNapiProvider::Create();
        mockNapiPtr_ = mockNapi.get();
        NapiBridge::SetProvider(std::move(mockNapi));
    }

    void TearDown() override
    {
        NapiBridge::SetProvider(nullptr);
        mockNapiPtr_ = nullptr;
    }

    MockNapiProvider* mockNapiPtr_ = nullptr;
};

/**
 * @tc.name: NativeFunctionsTest001
 * @tc.desc: Verify the following native function registry behavior: contain core functions in registry.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest001)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeFunctionRegistry& registry = NativeFunctionRegistry::GetInstance();
    EXPECT_TRUE(registry.HasFunction("required"));
    EXPECT_TRUE(registry.HasFunction("regex"));
    EXPECT_TRUE(registry.HasFunction("length"));
    EXPECT_TRUE(registry.HasFunction("numeric"));
    EXPECT_TRUE(registry.HasFunction("email"));
    EXPECT_TRUE(registry.HasFunction("and"));
    EXPECT_TRUE(registry.HasFunction("or"));
    EXPECT_TRUE(registry.HasFunction("not"));
    EXPECT_TRUE(registry.HasFunction("getToggleValue"));
    EXPECT_TRUE(registry.HasFunction("getRadioValue"));
    EXPECT_FALSE(registry.HasFunction("nonexistent"));
}

/**
 * @tc.name: NativeFunctionsTest002
 * @tc.desc: Verify the following NativeRequiredFunction behavior: return true for non-empty value.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest002)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeRequiredFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"hello"})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest003
 * @tc.desc: Verify the following NativeRegexFunction behavior: match valid pattern.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest003)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeRegexFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"12345","pattern":"^[0-9]{5}$"})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest004
 * @tc.desc: Verify the following NativeLengthFunction behavior: fail when below min.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest004)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeLengthFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"hi","min":5})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest005
 * @tc.desc: Verify the following NativeNumericFunction behavior: fail when above max.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest005)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":101,"max":100})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest006
 * @tc.desc: Verify the following NativeEmailFunction behavior: validate common address.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest006)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeEmailFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"user@example.com"})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest007
 * @tc.desc: Verify the following NativeAndFunction behavior: return false when one item is false.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest007)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeAndFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"values":[true,false,true]})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest008
 * @tc.desc: Verify the following NativeOrFunction behavior: return true when one item is true.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest008)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"values":[false,true,false]})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest009
 * @tc.desc: Verify the following NativeNotFunction behavior: report function name.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest009)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNotFunction function;
    EXPECT_EQ(function.GetName(), "not");
}

/**
 * @tc.name: NativeFunctionsTest010
 * @tc.desc: Verify the following NativeNotFunction behavior: invert boolean.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest010)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNotFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":true})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest011
 * @tc.desc: Verify the following NativeNotFunction behavior: return true for non-object args.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest011)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNotFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, "true"), "true");
}

/**
 * @tc.name: NativeFunctionsTest012
 * @tc.desc: Verify the following NativeNotFunction behavior: return true when value is not boolean.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest012)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNotFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"true"})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest013
 * @tc.desc: Verify the following NativeNotFunction behavior: invert false to true.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest013)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNotFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":false})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest014
 * @tc.desc: Verify the following NativeFormatNumberFunction behavior: group integer value.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest014)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeFormatNumberFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":1000000,"grouping":true})"), "\"1,000,000.00\"");
}

/**
 * @tc.name: NativeFunctionsTest015
 * @tc.desc: Verify the following NativeFormatCurrencyFunction behavior: format code and value.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest015)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeFormatCurrencyFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":99.99,"currency":"USD"})"), "\"USD 99.99\"");
}

/**
 * @tc.name: NativeFunctionsTest016
 * @tc.desc: Verify the following NativeFormatDateFunction behavior: support yyyy-MM-dd format.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest016)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeFormatDateFunction function;
    EXPECT_EQ(
        ExecuteAsLiteral(function, R"({"value":"2026-01-16T14:30:00Z","format":"yyyy-MM-dd"})"), "\"2026-01-16\"");
}

/**
 * @tc.name: NativeFunctionsTest017
 * @tc.desc: Verify the following NativePluralizeFunction behavior: use zero branch.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest017)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativePluralizeFunction function;
    PluralLocaleManager::GetInstance().SetLocale("ar");
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":0,"zero":"no items","one":"1 item","other":"%{count} items"})"),
        "\"no items\"");
}

/**
 * @tc.name: NativeFunctionsTest018
 * @tc.desc: Verify the following NativeFormatStringFunction behavior: return original text.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest018)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeFormatStringFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"Hello World"})"), "\"Hello World\"");
}

/**
 * @tc.name: NativeFunctionsTest019
 * @tc.desc: Verify the following NativeNumericFunction behavior: report function name.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest019)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(function.GetName(), "numeric");
}

/**
 * @tc.name: NativeFunctionsTest020
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false for non-object args.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest020)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, "true"), "false");
}

/**
 * @tc.name: NativeFunctionsTest021
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when value is not numeric.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest021)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"101","max":100})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest022
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when value is not finite.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest022)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    auto args = JsonAdapter::CreateObject();
    ASSERT_NE(args, nullptr);
    JsonValue root = args->GetRoot();
    ASSERT_TRUE(root.PutNumber("value", std::numeric_limits<double>::infinity()));
    ASSERT_TRUE(root.PutNumber("max", 100.0));

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, root), "false");
}

/**
 * @tc.name: NativeFunctionsTest023
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when range limits are missing.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest023)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":100})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest024
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when min is not numeric.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest024)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":100,"min":"50"})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest025
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when min is not finite.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest025)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    auto args = JsonAdapter::CreateObject();
    ASSERT_NE(args, nullptr);
    JsonValue root = args->GetRoot();
    ASSERT_TRUE(root.PutNumber("value", 100.0));
    ASSERT_TRUE(root.PutNumber("min", std::numeric_limits<double>::infinity()));

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, root), "false");
}

/**
 * @tc.name: NativeFunctionsTest026
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when value is below min.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest026)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":49,"min":50})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest027
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when max is not numeric.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest027)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":50,"max":"100"})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest028
 * @tc.desc: Verify the following NativeNumericFunction behavior: return false when max is not finite.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest028)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    auto args = JsonAdapter::CreateObject();
    ASSERT_NE(args, nullptr);
    JsonValue root = args->GetRoot();
    ASSERT_TRUE(root.PutNumber("value", 100.0));
    ASSERT_TRUE(root.PutNumber("max", std::numeric_limits<double>::infinity()));

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, root), "false");
}

/**
 * @tc.name: NativeFunctionsTest029
 * @tc.desc: Verify the following NativeNumericFunction behavior: return true when value is within range.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest029)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeNumericFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":75,"min":50,"max":100})"), "true");
}

/**
 * @tc.name: NativeFunctionsTest035
 * @tc.desc: Verify the following NativeOrFunction behavior: report function name.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest035)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(function.GetName(), "or");
}

/**
 * @tc.name: NativeFunctionsTest036
 * @tc.desc: Verify the following NativeOrFunction behavior: return false for non-object args.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest036)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, "true"), "false");
}

/**
 * @tc.name: NativeFunctionsTest037
 * @tc.desc: Verify the following NativeOrFunction behavior: return false when values is not an array.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest037)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"values":true})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest038
 * @tc.desc: Verify the following NativeOrFunction behavior: return false when values count is below two.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest038)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"values":[false]})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest039
 * @tc.desc: Verify the following NativeOrFunction behavior: ignore non-boolean items and return false when no true
 * value exists.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest039)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativeOrFunction function;
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"values":["x",0,false]})"), "false");
}

/**
 * @tc.name: NativeFunctionsTest040
 * @tc.desc: Verify the following NativePluralizeFunction behavior: report function name and reject invalid args.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest040)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    NativePluralizeFunction function;
    EXPECT_EQ(function.GetName(), "pluralize");
    EXPECT_EQ(ExecuteAsLiteral(function, "true"), "\"\"");
    EXPECT_EQ(ExecuteAsLiteral(function, R"({"value":"1","other":"other"})"), "\"\"");
}

/**
 * @tc.name: NativeFunctionsTest041
 * @tc.desc: Verify the following NativePluralizeFunction behavior: reject invalid category values and fallback to
 * other.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest041)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    auto invalidAdapter = JsonAdapter::CreateObject();
    ASSERT_NE(invalidAdapter, nullptr);
    JsonValue invalidRoot = invalidAdapter->GetRoot();
    ASSERT_TRUE(invalidRoot.PutNumber("value", 1.0));
    ASSERT_TRUE(invalidRoot.PutBool("one", true));

    auto fallbackAdapter = JsonAdapter::CreateObject();
    ASSERT_NE(fallbackAdapter, nullptr);
    JsonValue fallbackRoot = fallbackAdapter->GetRoot();
    ASSERT_TRUE(fallbackRoot.PutNumber("value", 1.0));
    ASSERT_TRUE(fallbackRoot.PutString("other", "fallback"));

    NativePluralizeFunction function;
    PluralLocaleManager::GetInstance().SetLocale("en");
    EXPECT_EQ(ExecuteAsLiteral(function, invalidRoot), "\"\"");
    EXPECT_EQ(ExecuteAsLiteral(function, fallbackRoot), "\"fallback\"");
}

/**
 * @tc.name: NativeFunctionsTest042
 * @tc.desc: Verify the following NativePluralizeFunction behavior: resolve one-other and french-style categories.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest042)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    EXPECT_EQ(ExecutePluralizeCategory("en", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("en", 2.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("en", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("pt-BR", 0.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("pt-BR", 0.5), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("pt-BR", 1.5), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("pt-BR", 2.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("pt-BR", 2.5), "\"other\"");
}

/**
 * @tc.name: NativeFunctionsTest043
 * @tc.desc: Verify the following NativePluralizeFunction behavior: resolve slavic and baltic categories.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest043)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    EXPECT_EQ(ExecutePluralizeCategory("ru", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("ru", 3.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("ru", 5.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("ru", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("pl", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("pl", 3.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("pl", 5.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("pl", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("cs", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("cs", 3.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("cs", 5.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("cs", 1.2), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("lt", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("lt", 2.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("lt", 11.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("lt", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("lv", 0.0), "\"zero\"");
    EXPECT_EQ(ExecutePluralizeCategory("lv", 11.0), "\"zero\"");
    EXPECT_EQ(ExecutePluralizeCategory("lv", 21.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("lv", 2.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("lv", 1.5), "\"zero\"");
}

/**
 * @tc.name: NativeFunctionsTest044
 * @tc.desc: Verify the following NativePluralizeFunction behavior: resolve breton, macedonian, and icelandic
 * categories.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest044)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    EXPECT_EQ(ExecutePluralizeCategory("br", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("br", 2.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("br", 3.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("br", 6.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("br", 4.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("br", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("mk", 21.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("mk", 11.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("mk", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("is", 21.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("is", 11.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("is", 1.5), "\"other\"");
}

/**
 * @tc.name: NativeFunctionsTest045
 * @tc.desc: Verify the following NativePluralizeFunction behavior: resolve arabic and hebrew categories.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest045)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    EXPECT_EQ(ExecutePluralizeCategory("ar", 0.0), "\"zero\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 2.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 7.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 11.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 100.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("ar", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("he", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 2.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 0.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 15.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 30.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 3.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("he", 1.5), "\"other\"");
}

/**
 * @tc.name: NativeFunctionsTest046
 * @tc.desc: Verify the following NativePluralizeFunction behavior: resolve welsh, irish, slovenian, and maltese
 * categories.
 * @tc.type: FUNC
 */
TEST_F(NativeFunctionsTest, NativeFunctionsTest046)
{
    /**
     * @tc.steps: step1. Invoke the target native function interface with the current test input.
     * @tc.expected: The returned value or registry state matches the expectation.
     */

    EXPECT_EQ(ExecutePluralizeCategory("cy", 0.0), "\"zero\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 2.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 3.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 6.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 4.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("cy", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("ga", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("ga", 2.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("ga", 4.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("ga", 8.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("ga", 11.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("ga", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("sl", 101.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("sl", 102.0), "\"two\"");
    EXPECT_EQ(ExecutePluralizeCategory("sl", 103.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("sl", 105.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("sl", 1.5), "\"other\"");

    EXPECT_EQ(ExecutePluralizeCategory("mt", 1.0), "\"one\"");
    EXPECT_EQ(ExecutePluralizeCategory("mt", 0.0), "\"few\"");
    EXPECT_EQ(ExecutePluralizeCategory("mt", 11.0), "\"many\"");
    EXPECT_EQ(ExecutePluralizeCategory("mt", 20.0), "\"other\"");
    EXPECT_EQ(ExecutePluralizeCategory("mt", 1.5), "\"other\"");
}
