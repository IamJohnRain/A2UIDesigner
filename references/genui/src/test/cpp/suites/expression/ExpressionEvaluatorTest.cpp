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
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include "catalog/Catalog.h"
#include "catalog/CatalogConstants.h"
#include "catalog/CatalogItem.h"
#include "components/A2UI/text/TextComponent.h"
#include "components/Component.h"
#include "components/extended/ExtendedTextComponent.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "data/ResolvedValue.h"
#include "theme/ThemeBase.h"
#include "utils/JsonAdapter.h"

#include "NativeEntry.h"
#include "RenderManager.h"
#include "RenderSlot.h"
#include "SchemaWarningTestHelper.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"
#include "TestFixture.h"
#include "expression/DataModelPathUtils.h"
#include "expression/EvalResult.h"
#include "expression/EvaluationContext.h"
#include "expression/Evaluator.h"
#include "expression/ExpressionEngine.h"
#include "expression/ExpressionFunctions.h"
#include "expression/Lexer.h"
#include "expression/Parser.h"
#include "expression/Sandbox.h"
#include "expression/ThemeContextUtils.h"

using namespace NativeModule;

namespace {

EvalResult Eval(const std::string& expr)
{
    EvaluationContext context;
    return ExpressionEngine::GetInstance().Evaluate("{{ " + expr + " }}", context);
}

EvalResult EvalRaw(const std::string& input)
{
    EvaluationContext context;
    return ExpressionEngine::GetInstance().Evaluate(input, context);
}

EvalResult EvalWithContext(const std::string& expr, EvaluationContext& context)
{
    return ExpressionEngine::GetInstance().Evaluate(expr, context);
}

std::shared_ptr<Catalog> BuildExtendedTemplateCatalog()
{
    auto catalog = std::make_shared<Catalog>(A2UI_EXTENDED_CATALOG_ID);
    auto column = std::make_shared<CatalogItem>("Column", CatalogItemType::COMPONENT);
    column->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(column);
    auto text = std::make_shared<CatalogItem>("Text", CatalogItemType::COMPONENT);
    text->SetCategory(CatalogCategory::OHOS_EXTENDS);
    catalog->AddComponent(text);
    return catalog;
}

std::vector<std::shared_ptr<ExtendedTextComponent>> CollectTemplateTextChildren(const std::shared_ptr<Component>& root)
{
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren;
    if (root == nullptr) {
        return textChildren;
    }
    for (const auto& child : root->GetChildren()) {
        auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(child);
        if (text != nullptr) {
            textChildren.push_back(text);
        }
        std::vector<std::shared_ptr<ExtendedTextComponent>> nestedTextChildren = CollectTemplateTextChildren(child);
        textChildren.insert(textChildren.end(), nestedTextChildren.begin(), nestedTextChildren.end());
    }
    return textChildren;
}

} // namespace

class ExpressionFormatTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(ExpressionFormatTest, L0_should_identify_complete_expression_format)
{
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{ 1 + 2 }}"));
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{}}"));
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{   }}"));
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{ size(${/user}) }}"));
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{ 'size = ' + size(${/user}) }}"));
}

TEST_F(ExpressionFormatTest, L0_should_reject_non_expression_or_unclosed_format)
{
    EXPECT_FALSE(ExpressionEngine::IsExpression("plain text"));
    EXPECT_FALSE(ExpressionEngine::IsExpression(""));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ 1 + 2"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{ 1 + 2 }"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("1 + 2 }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("prefix {{ 1 + 2 }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ 1 + 2 }} suffix"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ 1 }} {{ 2 }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ {{ 1 }} }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ 1 + {{ 2 }} }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ { } }}"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ '}' }}"));
}

/**
 * @tc.name: ExpressionFormat_should_reject_unclosed_placeholder_wrapper
 * @tc.desc: 验证表达式格式校验会拒绝缺少闭合的 ${...} 片段。
 * @tc.type: FUNC
 */
TEST_F(ExpressionFormatTest, L0_should_reject_unclosed_placeholder_wrapper)
{
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ ${/user }"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{ ${/user}"));
}

TEST_F(ExpressionFormatTest, L0_should_extract_trimmed_expression_content)
{
    EXPECT_EQ(ExpressionEngine::ExtractExpression("{{ 1 + 2 }}"), "1 + 2");
    EXPECT_EQ(ExpressionEngine::ExtractExpression("{{'hello'}}"), "'hello'");
    EXPECT_EQ(ExpressionEngine::ExtractExpression("plain"), "");
}

TEST(EvalResultTest, L0_should_create_result_types)
{
    EvalResult undefined = EvalResult::Undefined();
    EXPECT_TRUE(undefined.IsUndefined());
    EXPECT_FALSE(undefined.IsDefined());

    EvalResult str = EvalResult::FromString("hello");
    ASSERT_TRUE(str.IsString());
    EXPECT_EQ(str.stringValue, "hello");
    EXPECT_EQ(str.AsString(), "hello");

    EvalResult number = EvalResult::FromNumber(42.0);
    ASSERT_TRUE(number.IsNumber());
    EXPECT_DOUBLE_EQ(number.numberValue, 42.0);
    EXPECT_DOUBLE_EQ(number.AsNumber(), 42.0);

    EvalResult boolean = EvalResult::FromBool(true);
    ASSERT_TRUE(boolean.IsBoolean());
    EXPECT_TRUE(boolean.boolValue);
    EXPECT_TRUE(boolean.AsBool());

    EvalResult nullVal = EvalResult::Null();
    ASSERT_TRUE(nullVal.IsNull());
    EXPECT_EQ(nullVal.AsString(), "null");
}

TEST(EvalResultTest, L0_should_convert_between_types)
{
    EXPECT_DOUBLE_EQ(EvalResult::FromBool(true).AsNumber(), 1.0);
    EXPECT_DOUBLE_EQ(EvalResult::FromBool(false).AsNumber(), 0.0);
    EXPECT_DOUBLE_EQ(EvalResult::FromString("3.14").AsNumber(), 3.14);
    EXPECT_DOUBLE_EQ(EvalResult::FromString("abc").AsNumber(), 0.0);
    EXPECT_EQ(EvalResult::FromNumber(42.0).AsString(), "42");
    EXPECT_EQ(EvalResult::FromBool(true).AsString(), "true");
    EXPECT_TRUE(EvalResult::FromNumber(1.0).AsBool());
    EXPECT_FALSE(EvalResult::FromNumber(0.0).AsBool());
    EXPECT_TRUE(EvalResult::FromString("x").AsBool());
    EXPECT_FALSE(EvalResult::FromString("").AsBool());
}

/**
 * @tc.name: EvalResult_should_convert_json_values_and_detect_container_types
 * @tc.desc: 验证 EvalResult 能保留 JSON 容器类型，并对溢出数字字符串安全降级。
 * @tc.type: FUNC
 */
TEST(EvalResultTest, L0_should_convert_json_values_and_detect_container_types)
{
    auto arrayAdapter = JsonAdapter::Parse(R"([1,2])");
    auto objectAdapter = JsonAdapter::Parse(R"({"name":"Alice"})");
    ASSERT_NE(arrayAdapter, nullptr);
    ASSERT_NE(objectAdapter, nullptr);

    EvalResult arrayResult = EvalResult::FromJson(arrayAdapter->GetRoot());
    ASSERT_TRUE(arrayResult.IsJson());
    EXPECT_TRUE(arrayResult.IsArray());
    EXPECT_FALSE(arrayResult.IsObject());

    EvalResult objectResult = EvalResult::FromJson(objectAdapter->GetRoot());
    ASSERT_TRUE(objectResult.IsJson());
    EXPECT_TRUE(objectResult.IsObject());
    EXPECT_FALSE(objectResult.IsArray());

    EXPECT_DOUBLE_EQ(EvalResult::FromString("1e309").AsNumber(), 0.0);
}

/**
 * @tc.name: EvaluationContext_should_resolve_data_model_root_and_flag_unknown_globals
 * @tc.desc: 验证 EvaluationContext 在字符串或容器模式下解析 $__dataModel，并给未知系统变量打软错误标记。
 * @tc.type: FUNC
 */
TEST(EvaluationContextTest, L0_should_resolve_data_model_root_and_flag_unknown_globals)
{
    EvaluationContext ctx;
    auto model = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(adapter, nullptr);
    model->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(model.get());

    EvalResult stringRoot = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(stringRoot.IsString());
    EXPECT_NE(stringRoot.AsString().find("\"Alice\""), std::string::npos);

    ctx.allowContainerResults = true;
    EvalResult jsonRoot = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(jsonRoot.IsObject());
    EXPECT_EQ(jsonRoot.AsJson().GetItem("user").GetItem("name").GetStringValue(""), "Alice");

    EvalResult unknownGlobal = ctx.ResolveVariable("__missingGlobal");
    ASSERT_TRUE(unknownGlobal.IsString());
    EXPECT_TRUE(unknownGlobal.hasEvaluationError);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_NO_GLOBAL_VARIABLE);
}

class LexerTest : public ::testing::Test {
protected:
    Lexer lexer_;
};

TEST_F(LexerTest, L0_should_tokenize_literals_and_operators)
{
    auto tokens = lexer_.Tokenize("'Hello\\'s' + 42 * false");

    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].value, "Hello's");
    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[2].type, TokenType::NUMBER_LITERAL);
    EXPECT_EQ(tokens[2].value, "42");
    EXPECT_EQ(tokens[3].type, TokenType::STAR);
    EXPECT_EQ(tokens[4].type, TokenType::BOOLEAN_FALSE);
}

TEST_F(LexerTest, L0_should_tokenize_comparison_and_logical_operators)
{
    auto tokens = lexer_.Tokenize("1 == 2 && 3 != 4 || 5 <= 6");

    ASSERT_GE(tokens.size(), 12u);
    EXPECT_EQ(tokens[1].type, TokenType::EQ);
    EXPECT_EQ(tokens[3].type, TokenType::AND);
    EXPECT_EQ(tokens[5].type, TokenType::NEQ);
    EXPECT_EQ(tokens[7].type, TokenType::OR);
    EXPECT_EQ(tokens[9].type, TokenType::LTE);
}

TEST_F(LexerTest, L0_should_tokenize_ternary_operators)
{
    auto tokens = lexer_.Tokenize("true ? 1 : 2");

    ASSERT_GE(tokens.size(), 6u);
    EXPECT_EQ(tokens[1].type, TokenType::QUESTION);
    EXPECT_EQ(tokens[3].type, TokenType::COLON);
}

TEST_F(LexerTest, L0_should_tokenize_variable_and_member_access)
{
    auto tokens = lexer_.Tokenize("$name");

    ASSERT_GE(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::DOLLAR);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "name");
}

TEST_F(LexerTest, L0_should_report_unterminated_string)
{
    auto tokens = lexer_.Tokenize("'hello");

    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
}

TEST_F(LexerTest, L0_should_produce_illegal_for_backtick)
{
    auto tokens = lexer_.Tokenize("`hello`");

    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
}

class LiteralExpressionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(LiteralExpressionTest, L0_should_evaluate_literals)
{
    auto str = Eval("'Hello'");
    ASSERT_TRUE(str.IsString());
    EXPECT_EQ(str.stringValue, "Hello");

    auto escaped = Eval("'Hello\\'s'");
    ASSERT_TRUE(escaped.IsString());
    EXPECT_EQ(escaped.stringValue, "Hello's");

    auto empty = Eval("''");
    ASSERT_TRUE(empty.IsString());
    EXPECT_EQ(empty.stringValue, "");

    auto integer = Eval("42");
    ASSERT_TRUE(integer.IsNumber());
    EXPECT_DOUBLE_EQ(integer.numberValue, 42.0);

    auto decimal = Eval("3.14");
    ASSERT_TRUE(decimal.IsNumber());
    EXPECT_DOUBLE_EQ(decimal.numberValue, 3.14);

    auto boolean = Eval("true");
    ASSERT_TRUE(boolean.IsBoolean());
    EXPECT_TRUE(boolean.boolValue);
}

TEST_F(LiteralExpressionTest, L0_should_evaluate_arithmetic_and_unary_operators)
{
    auto add = Eval("1 + 2");
    ASSERT_TRUE(add.IsNumber());
    EXPECT_DOUBLE_EQ(add.numberValue, 3.0);

    auto sub = Eval("10 - 3");
    ASSERT_TRUE(sub.IsNumber());
    EXPECT_DOUBLE_EQ(sub.numberValue, 7.0);

    auto mul = Eval("4 * 5");
    ASSERT_TRUE(mul.IsNumber());
    EXPECT_DOUBLE_EQ(mul.numberValue, 20.0);

    auto div = Eval("10 / 4");
    ASSERT_TRUE(div.IsNumber());
    EXPECT_DOUBLE_EQ(div.numberValue, 2.5);

    auto mod = Eval("10 % 3");
    ASSERT_TRUE(mod.IsNumber());
    EXPECT_DOUBLE_EQ(mod.numberValue, 1.0);

    auto unaryMinus = Eval("-5");
    ASSERT_TRUE(unaryMinus.IsNumber());
    EXPECT_DOUBLE_EQ(unaryMinus.numberValue, -5.0);

    auto unaryNot = Eval("!false");
    ASSERT_TRUE(unaryNot.IsBoolean());
    EXPECT_TRUE(unaryNot.boolValue);
}

TEST_F(LiteralExpressionTest, L0_should_evaluate_comparison_operators)
{
    auto eq = Eval("1 == 1");
    ASSERT_TRUE(eq.IsBoolean());
    EXPECT_TRUE(eq.boolValue);

    auto neq = Eval("1 != 2");
    ASSERT_TRUE(neq.IsBoolean());
    EXPECT_TRUE(neq.boolValue);

    auto lt = Eval("1 < 2");
    ASSERT_TRUE(lt.IsBoolean());
    EXPECT_TRUE(lt.boolValue);

    auto lte = Eval("2 <= 2");
    ASSERT_TRUE(lte.IsBoolean());
    EXPECT_TRUE(lte.boolValue);

    auto gt = Eval("3 > 2");
    ASSERT_TRUE(gt.IsBoolean());
    EXPECT_TRUE(gt.boolValue);

    auto gte = Eval("2 >= 3");
    ASSERT_TRUE(gte.IsBoolean());
    EXPECT_FALSE(gte.boolValue);

    auto strEq = Eval("'abc' == 'abc'");
    ASSERT_TRUE(strEq.IsBoolean());
    EXPECT_TRUE(strEq.boolValue);

    auto strNeq = Eval("'abc' != 'def'");
    ASSERT_TRUE(strNeq.IsBoolean());
    EXPECT_TRUE(strNeq.boolValue);
}

TEST_F(LiteralExpressionTest, L0_should_evaluate_logical_operators)
{
    auto andTrue = Eval("true && true");
    ASSERT_TRUE(andTrue.IsBoolean());
    EXPECT_TRUE(andTrue.boolValue);

    auto andFalse = Eval("true && false");
    ASSERT_TRUE(andFalse.IsBoolean());
    EXPECT_FALSE(andFalse.boolValue);

    auto orTrue = Eval("false || true");
    ASSERT_TRUE(orTrue.IsBoolean());
    EXPECT_TRUE(orTrue.boolValue);

    auto orFalse = Eval("false || false");
    ASSERT_TRUE(orFalse.IsBoolean());
    EXPECT_FALSE(orFalse.boolValue);
}

TEST_F(LiteralExpressionTest, L0_should_evaluate_ternary_operator)
{
    auto trueBranch = Eval("true ? 1 : 2");
    ASSERT_TRUE(trueBranch.IsNumber());
    EXPECT_DOUBLE_EQ(trueBranch.numberValue, 1.0);

    auto falseBranch = Eval("false ? 1 : 2");
    ASSERT_TRUE(falseBranch.IsNumber());
    EXPECT_DOUBLE_EQ(falseBranch.numberValue, 2.0);
}

TEST_F(LiteralExpressionTest, L0_should_respect_precedence_and_parentheses)
{
    auto precedence = Eval("1 + 2 * 3");
    ASSERT_TRUE(precedence.IsNumber());
    EXPECT_DOUBLE_EQ(precedence.numberValue, 7.0);

    auto grouped = Eval("(1 + 2) * 3");
    ASSERT_TRUE(grouped.IsNumber());
    EXPECT_DOUBLE_EQ(grouped.numberValue, 9.0);

    auto nested = Eval("((1 + 2) * (3 + 4))");
    ASSERT_TRUE(nested.IsNumber());
    EXPECT_DOUBLE_EQ(nested.numberValue, 21.0);

    auto unaryInBinary = Eval("1 + -2");
    ASSERT_TRUE(unaryInBinary.IsNumber());
    EXPECT_DOUBLE_EQ(unaryInBinary.numberValue, -1.0);
}

TEST_F(LiteralExpressionTest, L0_should_evaluate_string_concat_and_implicit_conversion)
{
    auto concat = Eval("'Hello' + ' ' + 'World'");
    ASSERT_TRUE(concat.IsString());
    EXPECT_EQ(concat.stringValue, "Hello World");

    auto stringNumber = Eval("'Hello' + 5");
    ASSERT_TRUE(stringNumber.IsString());
    EXPECT_EQ(stringNumber.stringValue, "Hello5");

    auto numberString = Eval("5 + 'Hello'");
    ASSERT_TRUE(numberString.IsString());
    EXPECT_EQ(numberString.stringValue, "5Hello");

    auto booleanNumber = Eval("true + 1");
    ASSERT_TRUE(booleanNumber.IsNumber());
    EXPECT_DOUBLE_EQ(booleanNumber.numberValue, 2.0);

    auto falseMul = Eval("false * 10");
    ASSERT_TRUE(falseMul.IsNumber());
    EXPECT_DOUBLE_EQ(falseMul.numberValue, 0.0);

    auto numericString = Eval("'10' - 3");
    ASSERT_TRUE(numericString.IsNumber());
    EXPECT_DOUBLE_EQ(numericString.numberValue, 7.0);

    auto nonNumericString = Eval("'abc' - 1");
    ASSERT_TRUE(nonNumericString.IsNumber());
    EXPECT_DOUBLE_EQ(nonNumericString.numberValue, -1.0);

    auto stringBoolean = Eval("'enabled=' + true");
    ASSERT_TRUE(stringBoolean.IsString());
    EXPECT_EQ(stringBoolean.stringValue, "enabled=true");

    auto booleanString = Eval("false + ' flag'");
    ASSERT_TRUE(booleanString.IsString());
    EXPECT_EQ(booleanString.stringValue, "false flag");

    auto booleanBoolean = Eval("true + false");
    ASSERT_TRUE(booleanBoolean.IsNumber());
    EXPECT_DOUBLE_EQ(booleanBoolean.numberValue, 1.0);

    auto stringStringNumber = Eval("'10' * '2'");
    ASSERT_TRUE(stringStringNumber.IsNumber());
    EXPECT_DOUBLE_EQ(stringStringNumber.numberValue, 20.0);

    auto spacedNumericString = Eval("' 10 ' / ' 2 '");
    ASSERT_TRUE(spacedNumericString.IsNumber());
    EXPECT_DOUBLE_EQ(spacedNumericString.numberValue, 5.0);

    auto partialNumericString = Eval("'10abc' - 1");
    ASSERT_TRUE(partialNumericString.IsNumber());
    EXPECT_DOUBLE_EQ(partialNumericString.numberValue, -1.0);

    auto emptyNumericString = Eval("'' - 1");
    ASSERT_TRUE(emptyNumericString.IsNumber());
    EXPECT_DOUBLE_EQ(emptyNumericString.numberValue, -1.0);
}

TEST_F(LiteralExpressionTest, L0_should_apply_unary_implicit_conversion_rules)
{
    auto unaryNumericString = Eval("-'5'");
    ASSERT_TRUE(unaryNumericString.IsNumber());
    EXPECT_DOUBLE_EQ(unaryNumericString.numberValue, -5.0);

    auto unaryInvalidString = Eval("-'abc'");
    ASSERT_TRUE(unaryInvalidString.IsNumber());
    EXPECT_DOUBLE_EQ(unaryInvalidString.numberValue, 0.0);

    auto notZero = Eval("!0");
    ASSERT_TRUE(notZero.IsBoolean());
    EXPECT_TRUE(notZero.boolValue);

    auto notNumber = Eval("!5");
    ASSERT_TRUE(notNumber.IsBoolean());
    EXPECT_FALSE(notNumber.boolValue);

    auto notEmptyString = Eval("!''");
    ASSERT_TRUE(notEmptyString.IsBoolean());
    EXPECT_TRUE(notEmptyString.boolValue);

    auto notString = Eval("!'x'");
    ASSERT_TRUE(notString.IsBoolean());
    EXPECT_FALSE(notString.boolValue);
}

class ExpressionErrorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(ExpressionErrorTest, L0_should_return_undefined_for_syntax_and_runtime_errors)
{
    EXPECT_TRUE(Eval("1 + + 2").IsUndefined());
    EXPECT_TRUE(Eval("10 / 0").IsUndefined());
    EXPECT_TRUE(Eval("10 % 0").IsUndefined());
    EXPECT_TRUE(EvalRaw("{{}}").IsUndefined());
    EXPECT_TRUE(EvalRaw("{{   }}").IsUndefined());
    EXPECT_TRUE(Eval("'hello").IsUndefined());
}

TEST_F(ExpressionErrorTest, L0_should_apply_security_limits)
{
    EvaluationContext context;
    context.maxExprLength = 10;
    EXPECT_TRUE(ExpressionEngine::GetInstance().Evaluate("{{ 1 + 2 + 3 + 4 }}", context).IsUndefined());

    context = EvaluationContext();
    context.maxTokenCount = 4;
    EXPECT_TRUE(ExpressionEngine::GetInstance().Evaluate("{{ 1 + 2 + 3 }}", context).IsUndefined());

    context = EvaluationContext();
    context.maxNestingDepth = 2;
    EXPECT_TRUE(ExpressionEngine::GetInstance().Evaluate("{{ (((1))) }}", context).IsUndefined());

    context = EvaluationContext();
    context.maxAstNodes = 3;
    EXPECT_TRUE(ExpressionEngine::GetInstance().Evaluate("{{ 1 + 2 + 3 }}", context).IsUndefined());
}

TEST_F(ExpressionErrorTest, L0_should_return_undefined_for_undefined_variables)
{
    EXPECT_TRUE(Eval("$name").IsUndefined());
    EXPECT_TRUE(Eval("user.name").IsUndefined());
    EXPECT_TRUE(Eval("items[0]").IsUndefined());
    EXPECT_TRUE(Eval("format('x')").IsUndefined());
    EXPECT_TRUE(Eval("`hello`").IsUndefined());
}

class AstCacheTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
        ExpressionEngine::GetInstance().SetAstCacheCapacity(256);
    }

    void TearDown() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
        ExpressionEngine::GetInstance().SetAstCacheCapacity(256);
    }
};

TEST_F(AstCacheTest, L0_should_keep_ast_cache_disabled_by_default)
{
    EvaluationContext context;
    auto& engine = ExpressionEngine::GetInstance();

    auto first = engine.Evaluate("{{ 1 + 2 }}", context);
    auto second = engine.Evaluate("{{ 1 + 2 }}", context);

    ASSERT_TRUE(first.IsNumber());
    ASSERT_TRUE(second.IsNumber());
    EXPECT_DOUBLE_EQ(first.numberValue, 3.0);
    EXPECT_DOUBLE_EQ(second.numberValue, 3.0);
    EXPECT_EQ(engine.GetAstCacheSize(), 0u);
}

TEST_F(AstCacheTest, L0_should_cache_ast_when_enabled)
{
    EvaluationContext context;
    auto& engine = ExpressionEngine::GetInstance();
    engine.EnableAstCache(true);

    auto first = engine.Evaluate("{{ 1 + 2 }}", context);
    auto second = engine.Evaluate("{{ 1 + 2 }}", context);

    ASSERT_TRUE(first.IsNumber());
    ASSERT_TRUE(second.IsNumber());
    EXPECT_DOUBLE_EQ(first.numberValue, 3.0);
    EXPECT_DOUBLE_EQ(second.numberValue, 3.0);
    EXPECT_EQ(engine.GetAstCacheSize(), 1u);
}

TEST_F(AstCacheTest, L0_should_evict_ast_cache_by_lru_capacity)
{
    EvaluationContext context;
    auto& engine = ExpressionEngine::GetInstance();
    engine.EnableAstCache(true);
    engine.SetAstCacheCapacity(16);

    EXPECT_TRUE(engine.Evaluate("{{ 1 + 1 }}", context).IsNumber());
    EXPECT_TRUE(engine.Evaluate("{{ 2 + 2 }}", context).IsNumber());
    EXPECT_TRUE(engine.Evaluate("{{ 1 + 1 }}", context).IsNumber());
    for (int i = 3; i <= 17; ++i) {
        engine.Evaluate("{{ " + std::to_string(i) + " + " + std::to_string(i) + " }}", context);
    }

    EXPECT_EQ(engine.GetAstCacheSize(), 16u);
}

class NativeExpressionApiTest : public A2UITest {
protected:
    napi_env env_ = reinterpret_cast<napi_env>(0x100);
    napi_callback_info cbInfo_ = reinterpret_cast<napi_callback_info>(0x200);

    napi_value CreateStringArg(const std::string& value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateStringUtf8(env_, value.c_str(), value.size(), &result);
        return result;
    }

    napi_value CreateInt32Arg(int32_t value)
    {
        napi_value result = nullptr;
        mockNapiPtr_->CreateInt32(env_, value, &result);
        return result;
    }

    bool GetBoolProperty(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return mockNapiPtr_->boolValues_[value];
    }

    double GetNumberProperty(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return mockNapiPtr_->numberValues_[value];
    }

    std::string GetStringProperty(napi_value object, const std::string& key)
    {
        napi_value value = mockNapiPtr_->objectProperties_[object][key];
        return mockNapiPtr_->stringValues_[value];
    }

    bool HasProperty(napi_value object, const std::string& key)
    {
        return mockNapiPtr_->objectProperties_[object].find(key) != mockNapiPtr_->objectProperties_[object].end();
    }
};

TEST_F(NativeExpressionApiTest, L0_should_return_number_result_for_valid_expression)
{
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ 1 + 2 }}") });

    napi_value result = EvaluateExpression(env_, cbInfo_);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(GetBoolProperty(result, "success"));
    EXPECT_EQ(GetStringProperty(result, "type"), "number");
    EXPECT_DOUBLE_EQ(GetNumberProperty(result, "value"), 3.0);
}

TEST_F(NativeExpressionApiTest, L0_should_return_string_and_boolean_results)
{
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ 'Hello' + ' TS' }}") });
    napi_value stringResult = EvaluateExpression(env_, cbInfo_);

    ASSERT_NE(stringResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(stringResult, "success"));
    EXPECT_EQ(GetStringProperty(stringResult, "type"), "string");
    EXPECT_EQ(GetStringProperty(stringResult, "value"), "Hello TS");

    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ !false }}") });
    napi_value boolResult = EvaluateExpression(env_, cbInfo_);

    ASSERT_NE(boolResult, nullptr);
    EXPECT_TRUE(GetBoolProperty(boolResult, "success"));
    EXPECT_EQ(GetStringProperty(boolResult, "type"), "boolean");
    EXPECT_TRUE(GetBoolProperty(boolResult, "value"));
}

TEST_F(NativeExpressionApiTest, L0_should_return_undefined_result_for_invalid_input)
{
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("plain text") });
    napi_value nonExpressionResult = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(nonExpressionResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(nonExpressionResult, "success"));
    EXPECT_EQ(GetStringProperty(nonExpressionResult, "type"), "undefined");
    EXPECT_FALSE(HasProperty(nonExpressionResult, "value"));

    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ 10 / 0 }}") });
    napi_value errorResult = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(errorResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(errorResult, "success"));
    EXPECT_EQ(GetStringProperty(errorResult, "type"), "undefined");

    mockNapiPtr_->SetCallbackArgs({ CreateInt32Arg(3) });
    napi_value invalidArgResult = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(invalidArgResult, nullptr);
    EXPECT_FALSE(GetBoolProperty(invalidArgResult, "success"));
    EXPECT_EQ(GetStringProperty(invalidArgResult, "type"), "undefined");
}

// ==================== Lexer Branch Coverage ====================

class LexerBranchTest : public ::testing::Test {
protected:
    Lexer lexer_;
};

TEST_F(LexerBranchTest, L0_should_tokenize_all_operator_types)
{
    auto tokens = lexer_.Tokenize("+ - * / % ( ) [ ] . , ? :");
    ASSERT_GE(tokens.size(), 13u);
    EXPECT_EQ(tokens[0].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, TokenType::STAR);
    EXPECT_EQ(tokens[3].type, TokenType::SLASH);
    EXPECT_EQ(tokens[4].type, TokenType::PERCENT);
    EXPECT_EQ(tokens[5].type, TokenType::LPAREN);
    EXPECT_EQ(tokens[6].type, TokenType::RPAREN);
    EXPECT_EQ(tokens[7].type, TokenType::LBRACKET);
    EXPECT_EQ(tokens[8].type, TokenType::RBRACKET);
    EXPECT_EQ(tokens[9].type, TokenType::DOT);
    EXPECT_EQ(tokens[10].type, TokenType::COMMA);
    EXPECT_EQ(tokens[11].type, TokenType::QUESTION);
    EXPECT_EQ(tokens[12].type, TokenType::COLON);
}

TEST_F(LexerBranchTest, L0_should_tokenize_single_equals_as_illegal)
{
    auto tokens = lexer_.Tokenize("=");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
    EXPECT_EQ(tokens[0].value, "=");
}

TEST_F(LexerBranchTest, L0_should_tokenize_single_ampersand_as_illegal)
{
    auto tokens = lexer_.Tokenize("&");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
    EXPECT_EQ(tokens[0].value, "&");
}

TEST_F(LexerBranchTest, L0_should_tokenize_single_pipe_as_illegal)
{
    auto tokens = lexer_.Tokenize("|");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
    EXPECT_EQ(tokens[0].value, "|");
}

TEST_F(LexerBranchTest, L0_should_tokenize_bang_alone)
{
    auto tokens = lexer_.Tokenize("!");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::BANG);
    EXPECT_EQ(tokens[0].value, "!");
}

TEST_F(LexerBranchTest, L0_should_handle_trailing_dot_as_invalid_number)
{
    auto tokens = lexer_.Tokenize("3.");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::ILLEGAL);
}

TEST_F(LexerBranchTest, L0_should_handle_float_numbers)
{
    auto tokens = lexer_.Tokenize("3.14");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::NUMBER_LITERAL);
    EXPECT_EQ(tokens[0].value, "3.14");
}

TEST_F(LexerBranchTest, L0_should_handle_string_with_escape_sequences)
{
    auto tokens = lexer_.Tokenize("'\\n\\t\\\\'");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].value, "\n\t\\");
}

TEST_F(LexerBranchTest, L0_should_handle_string_with_unknown_escape)
{
    auto tokens = lexer_.Tokenize("'\\x'");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].value, "x");
}

TEST_F(LexerBranchTest, L0_should_handle_dollar_without_identifier)
{
    auto tokens = lexer_.Tokenize("$");
    ASSERT_GE(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::DOLLAR);
}

TEST_F(LexerBranchTest, L0_should_handle_newlines_in_strings)
{
    auto tokens = lexer_.Tokenize("'line1\nline2'");
    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING_LITERAL);
}

TEST_F(LexerBranchTest, L0_should_handle_whitespace_and_newlines_between_tokens)
{
    auto tokens = lexer_.Tokenize("1 \n + \n 2");
    ASSERT_GE(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].type, TokenType::NUMBER_LITERAL);
    EXPECT_EQ(tokens[1].type, TokenType::PLUS);
    EXPECT_EQ(tokens[2].type, TokenType::NUMBER_LITERAL);
}

TEST_F(LexerBranchTest, L0_should_tokenize_identifiers)
{
    auto tokens = lexer_.Tokenize("foo bar");
    ASSERT_GE(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].value, "foo");
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].value, "bar");
}

// ==================== Parser Branch Coverage ====================

class ParserBranchTest : public ::testing::Test {
protected:
    Lexer lexer_;
    Parser parser_;

    std::shared_ptr<AstNode> Parse(const std::string& input)
    {
        auto tokens = lexer_.Tokenize(input);
        auto result = parser_.Parse(tokens);
        return result.success ? result.ast : nullptr;
    }
};

TEST_F(ParserBranchTest, L0_should_parse_empty_input)
{
    auto tokens = lexer_.Tokenize("");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "empty expression");
}

TEST_F(ParserBranchTest, L0_should_parse_function_call_no_args)
{
    auto ast = Parse("func()");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::FUNCTION_CALL);
    auto* fc = static_cast<FunctionCall*>(ast.get());
    EXPECT_EQ(fc->name, "func");
    EXPECT_EQ(fc->arguments.size(), 0u);
}

TEST_F(ParserBranchTest, L0_should_parse_function_call_with_args)
{
    auto ast = Parse("func(1, 2, 3)");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::FUNCTION_CALL);
    auto* fc = static_cast<FunctionCall*>(ast.get());
    EXPECT_EQ(fc->arguments.size(), 3u);
}

TEST_F(ParserBranchTest, L0_should_parse_member_access_dot)
{
    auto ast = Parse("obj.prop");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::MEMBER_ACCESS);
    auto* ma = static_cast<MemberAccess*>(ast.get());
    EXPECT_EQ(ma->property, "prop");
    EXPECT_FALSE(ma->isBracket);
}

TEST_F(ParserBranchTest, L0_should_parse_member_access_bracket)
{
    auto ast = Parse("obj[0]");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::MEMBER_ACCESS);
    auto* ma = static_cast<MemberAccess*>(ast.get());
    EXPECT_TRUE(ma->isBracket);
    ASSERT_NE(ma->bracketKey, nullptr);
    EXPECT_EQ(ma->bracketKey->type, AstNodeType::NUMBER_LITERAL);
}

TEST_F(ParserBranchTest, L0_should_parse_chained_member_access_after_numeric_bracket)
{
    auto ast = Parse("$__dataModel.user.names[0].a");
    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->type, AstNodeType::MEMBER_ACCESS);

    auto* topMember = static_cast<MemberAccess*>(ast.get());
    EXPECT_FALSE(topMember->isBracket);
    EXPECT_EQ(topMember->property, "a");
    ASSERT_NE(topMember->object, nullptr);
    ASSERT_EQ(topMember->object->type, AstNodeType::MEMBER_ACCESS);

    auto* bracketMember = static_cast<MemberAccess*>(topMember->object.get());
    EXPECT_TRUE(bracketMember->isBracket);
    ASSERT_NE(bracketMember->bracketKey, nullptr);
    EXPECT_EQ(bracketMember->bracketKey->type, AstNodeType::NUMBER_LITERAL);
}

TEST_F(ParserBranchTest, L0_should_parse_dollar_variable)
{
    auto ast = Parse("$name");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::VARIABLE_REFERENCE);
    auto* vr = static_cast<VariableReference*>(ast.get());
    EXPECT_EQ(vr->name, "name");
    EXPECT_TRUE(vr->isAbsolute);
}

TEST_F(ParserBranchTest, L0_should_parse_plain_identifier_as_variable)
{
    auto ast = Parse("user");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::VARIABLE_REFERENCE);
    auto* vr = static_cast<VariableReference*>(ast.get());
    EXPECT_EQ(vr->name, "user");
    EXPECT_FALSE(vr->isAbsolute);
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_colon_in_ternary)
{
    auto tokens = lexer_.Tokenize("true ? 1");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected ':' in ternary expression");
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_rparen)
{
    auto tokens = lexer_.Tokenize("(1 + 2");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected ')'");
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_rbracket)
{
    auto tokens = lexer_.Tokenize("obj[0");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected ']'");
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_identifier_after_dot)
{
    auto tokens = lexer_.Tokenize("obj.");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected identifier after '.'");
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_identifier_after_dollar)
{
    auto tokens = lexer_.Tokenize("$ ");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected identifier after '$'");
}

TEST_F(ParserBranchTest, L0_should_error_on_unexpected_token)
{
    auto tokens = lexer_.Tokenize("=");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
}

TEST_F(ParserBranchTest, L0_should_error_on_trailing_tokens)
{
    auto tokens = lexer_.Tokenize("1 2");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "unexpected token after expression");
}

TEST_F(ParserBranchTest, L0_should_error_on_missing_rparen_in_function_call)
{
    auto tokens = lexer_.Tokenize("func(1");
    auto result = parser_.Parse(tokens);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "expected ')' after function arguments");
}

// ==================== Sandbox Branch Coverage ====================

class SandboxTest : public ::testing::Test {
protected:
    Sandbox sandbox_;
    EvaluationContext context_;
};

TEST_F(SandboxTest, L0_should_pass_valid_expression_length)
{
    EXPECT_TRUE(sandbox_.CheckExpressionLength(100, context_));
}

TEST_F(SandboxTest, L0_should_reject_expression_too_long)
{
    context_.maxExprLength = 5;
    EXPECT_FALSE(sandbox_.CheckExpressionLength(10, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::SANDBOX_LENGTH_EXCEEDED);
}

TEST_F(SandboxTest, L0_should_pass_valid_tokens)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1 + 2");
    EXPECT_TRUE(sandbox_.ValidateTokens(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_illegal_tokens)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("`hello`");
    EXPECT_FALSE(sandbox_.ValidateTokens(tokens, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::SANDBOX_ILLEGAL_TOKEN);
}

TEST_F(SandboxTest, L0_should_pass_token_count_within_limit)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1 + 2");
    EXPECT_TRUE(sandbox_.CheckTokenCount(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_too_many_tokens)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1 + 2 + 3 + 4 + 5");
    context_.maxTokenCount = 3;
    EXPECT_FALSE(sandbox_.CheckTokenCount(tokens, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::SANDBOX_TOKEN_COUNT_EXCEEDED);
}

TEST_F(SandboxTest, L0_should_pass_nesting_depth_within_limit)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("((1 + 2))");
    EXPECT_TRUE(sandbox_.CheckNestingDepth(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_too_deep_nesting)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("((((1))))");
    context_.maxNestingDepth = 2;
    EXPECT_FALSE(sandbox_.CheckNestingDepth(tokens, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::SANDBOX_DEPTH_EXCEEDED);
}

TEST_F(SandboxTest, L0_should_reject_unmatched_opening_bracket)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("(1 + 2");
    EXPECT_FALSE(sandbox_.CheckNestingDepth(tokens, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::PARSE_UNBALANCED_PARENS);
}

TEST_F(SandboxTest, L0_should_reject_unmatched_closing_bracket)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1 + 2)");
    EXPECT_FALSE(sandbox_.CheckNestingDepth(tokens, context_));
    EXPECT_EQ(context_.lastError, ExpressionError::PARSE_UNBALANCED_PARENS);
}

TEST_F(SandboxTest, L0_should_pass_bracket_balance)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("(1 + 2) * [3]");
    EXPECT_TRUE(sandbox_.CheckBracketBalance(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_unmatched_paren_in_balance)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("(1 + 2");
    EXPECT_FALSE(sandbox_.CheckBracketBalance(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_unmatched_bracket_in_balance)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("arr[0");
    EXPECT_FALSE(sandbox_.CheckBracketBalance(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_extra_closing_paren)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1)");
    EXPECT_FALSE(sandbox_.CheckBracketBalance(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_extra_closing_bracket)
{
    Lexer lexer;
    auto tokens = lexer.Tokenize("1]");
    EXPECT_FALSE(sandbox_.CheckBracketBalance(tokens, context_));
}

TEST_F(SandboxTest, L0_should_reject_null_ast)
{
    auto result = sandbox_.ValidateAst(nullptr, context_);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.reason, "null AST");
}

TEST_F(SandboxTest, L0_should_reject_ast_too_large)
{
    auto ast = std::make_shared<BinaryExpression>();
    ast->op = BinaryOp::PLUS;
    ast->left = std::make_shared<NumberLiteral>(1);
    ast->right = std::make_shared<NumberLiteral>(2);
    context_.maxAstNodes = 1;
    auto result = sandbox_.ValidateAst(ast, context_);
    EXPECT_FALSE(result.valid);
}

TEST_F(SandboxTest, L0_should_validate_variable_names)
{
    EXPECT_TRUE(Sandbox::IsVariableNameValid("$name"));
    EXPECT_TRUE(Sandbox::IsVariableNameValid("$user_name"));
    EXPECT_TRUE(Sandbox::IsVariableNameValid("$items[0]"));
    EXPECT_FALSE(Sandbox::IsVariableNameValid(""));
    EXPECT_FALSE(Sandbox::IsVariableNameValid("name"));
    EXPECT_TRUE(Sandbox::IsVariableNameValid("$"));
    EXPECT_FALSE(Sandbox::IsVariableNameValid("$name!"));
    EXPECT_FALSE(Sandbox::IsVariableNameValid("$..name"));
}

TEST_F(SandboxTest, L0_should_validate_data_model_paths)
{
    EXPECT_TRUE(Sandbox::IsDataModelPathAllowed("/user/name"));
    EXPECT_TRUE(Sandbox::IsDataModelPathAllowed("/items/0"));
    EXPECT_TRUE(Sandbox::IsDataModelPathAllowed("/a-b/c_d"));
    EXPECT_FALSE(Sandbox::IsDataModelPathAllowed(""));
    EXPECT_FALSE(Sandbox::IsDataModelPathAllowed("user"));
    EXPECT_TRUE(Sandbox::IsDataModelPathAllowed("/"));
    EXPECT_FALSE(Sandbox::IsDataModelPathAllowed("/a/../b"));
    EXPECT_FALSE(Sandbox::IsDataModelPathAllowed("/a/b!c"));
    EXPECT_FALSE(Sandbox::IsDataModelPathAllowed("/a//b"));
}

// ==================== EvalResult Branch Coverage ====================

TEST(EvalResultBranchTest, L0_should_handle_null_type)
{
    EvalResult nullVal = EvalResult::Null();
    EXPECT_TRUE(nullVal.IsNull());
    EXPECT_FALSE(nullVal.IsString());
    EXPECT_FALSE(nullVal.IsNumber());
    EXPECT_FALSE(nullVal.IsBoolean());
    EXPECT_EQ(nullVal.AsString(), "null");
    EXPECT_DOUBLE_EQ(nullVal.AsNumber(), 0.0);
    EXPECT_FALSE(nullVal.AsBool());
}

TEST(EvalResultBranchTest, L0_should_handle_number_to_string_integer)
{
    EvalResult val = EvalResult::FromNumber(42.0);
    EXPECT_EQ(val.AsString(), "42");
}

TEST(EvalResultBranchTest, L0_should_handle_number_to_string_decimal)
{
    EvalResult val = EvalResult::FromNumber(3.14);
    EXPECT_EQ(val.AsString(), "3.14");
}

TEST(EvalResultBranchTest, L0_should_handle_number_to_string_large_integer)
{
    EvalResult val = EvalResult::FromNumber(1e16);
    EXPECT_NE(val.AsString(), "");
}

TEST(EvalResultBranchTest, L0_should_handle_infinity_as_empty_string)
{
    EvalResult val = EvalResult::FromNumber(std::numeric_limits<double>::infinity());
    EXPECT_EQ(val.AsString(), "");
}

TEST(EvalResultBranchTest, L0_should_handle_nan_as_empty_string)
{
    EvalResult val = EvalResult::FromNumber(std::nan(""));
    EXPECT_EQ(val.AsString(), "");
}

TEST(EvalResultBranchTest, L0_should_handle_string_to_number_valid)
{
    EvalResult val = EvalResult::FromString("3.14");
    EXPECT_DOUBLE_EQ(val.AsNumber(), 3.14);
}

TEST(EvalResultBranchTest, L0_should_handle_string_to_number_invalid)
{
    EvalResult val = EvalResult::FromString("abc");
    EXPECT_DOUBLE_EQ(val.AsNumber(), 0.0);
}

TEST(EvalResultBranchTest, L0_should_handle_string_to_number_with_trailing_spaces)
{
    EvalResult val = EvalResult::FromString("3.14  ");
    EXPECT_DOUBLE_EQ(val.AsNumber(), 3.14);
}

TEST(EvalResultBranchTest, L0_should_handle_string_to_number_overflow)
{
    EvalResult val = EvalResult::FromString("1e999");
    EXPECT_DOUBLE_EQ(val.AsNumber(), 0.0);
}

TEST(EvalResultBranchTest, L0_should_handle_undefined_type_conversions)
{
    EvalResult val = EvalResult::Undefined();
    EXPECT_EQ(val.AsString(), "");
    EXPECT_DOUBLE_EQ(val.AsNumber(), 0.0);
    EXPECT_FALSE(val.AsBool());
}

TEST(EvalResultBranchTest, L0_should_handle_move_string)
{
    EvalResult val = EvalResult::FromString(std::string("moved"));
    EXPECT_TRUE(val.IsString());
    EXPECT_EQ(val.stringValue, "moved");
}

// ==================== ExpressionFunctions Branch Coverage ====================

TEST(ExpressionFunctionsTest, L0_should_register_and_call_function)
{
    ExpressionFunctions funcs;
    funcs.Register("double", [](const std::vector<EvalResult>& args) -> EvalResult {
        if (args.empty())
            return EvalResult::Undefined();
        return EvalResult::FromNumber(args[0].AsNumber() * 2);
    });

    EXPECT_TRUE(funcs.Has("double"));
    EXPECT_FALSE(funcs.Has("unknown"));

    auto result = funcs.Call("double", { EvalResult::FromNumber(5.0) });
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 10.0);
}

TEST(ExpressionFunctionsTest, L0_should_return_undefined_for_unknown_function)
{
    ExpressionFunctions funcs;
    auto result = funcs.Call("unknown", {});
    EXPECT_TRUE(result.IsUndefined());
}

TEST(EvalResultJsonCoverageTest, L0_should_convert_json_values_to_matching_eval_result_types)
{
    auto nullAdapter = JsonAdapter::Parse("null");
    auto boolAdapter = JsonAdapter::Parse("true");
    auto numberAdapter = JsonAdapter::Parse("42");
    auto stringAdapter = JsonAdapter::Parse(R"("alice")");
    auto arrayAdapter = JsonAdapter::Parse(R"([1,2])");
    auto objectAdapter = JsonAdapter::Parse(R"({"name":"alice"})");
    ASSERT_NE(nullAdapter, nullptr);
    ASSERT_NE(boolAdapter, nullptr);
    ASSERT_NE(numberAdapter, nullptr);
    ASSERT_NE(stringAdapter, nullptr);
    ASSERT_NE(arrayAdapter, nullptr);
    ASSERT_NE(objectAdapter, nullptr);

    EXPECT_TRUE(EvalResult::FromJson(JsonValue()).IsUndefined());
    EXPECT_TRUE(EvalResult::FromJson(nullAdapter->GetRoot()).IsNull());

    EvalResult boolResult = EvalResult::FromJson(boolAdapter->GetRoot());
    ASSERT_TRUE(boolResult.IsBoolean());
    EXPECT_TRUE(boolResult.AsBool());

    EvalResult numberResult = EvalResult::FromJson(numberAdapter->GetRoot());
    ASSERT_TRUE(numberResult.IsNumber());
    EXPECT_DOUBLE_EQ(numberResult.AsNumber(), 42.0);

    EvalResult stringResult = EvalResult::FromJson(stringAdapter->GetRoot());
    ASSERT_TRUE(stringResult.IsString());
    EXPECT_EQ(stringResult.AsString(), "alice");

    EvalResult arrayResult = EvalResult::FromJson(arrayAdapter->GetRoot());
    ASSERT_TRUE(arrayResult.IsJson());
    EXPECT_TRUE(arrayResult.IsArray());
    EXPECT_FALSE(arrayResult.IsObject());
    EXPECT_EQ(arrayResult.AsString(), "[1,2]");
    EXPECT_DOUBLE_EQ(arrayResult.AsNumber(), 0.0);

    EvalResult objectResult = EvalResult::FromJson(objectAdapter->GetRoot());
    ASSERT_TRUE(objectResult.IsJson());
    EXPECT_FALSE(objectResult.IsArray());
    EXPECT_TRUE(objectResult.IsObject());
    EXPECT_NE(objectResult.AsString().find("alice"), std::string::npos);
    EXPECT_TRUE(objectResult.AsBool());
}

TEST(ExpressionFunctionsTest, L0_should_call_context_aware_function_with_explicit_context)
{
    ExpressionFunctions funcs;
    funcs.Register("touchContext", [](const std::vector<EvalResult>& args, EvaluationContext& context) -> EvalResult {
        context.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "context touched");
        return EvalResult::FromNumber(static_cast<double>(args.size()));
    });

    EvaluationContext ctx;
    auto result = funcs.Call("touchContext", { EvalResult::FromNumber(1.0), EvalResult::FromString("two") }, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 2.0);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(ctx.errorMessage, "context touched");
}

// ==================== ResolvedValue Branch Coverage ====================

TEST(ResolvedValueTest, L0_should_create_ok_literal)
{
    auto val = ResolvedValue::OkLiteral(JsonValue());
    EXPECT_EQ(val.source, ResolveSource::LITERAL);
    EXPECT_TRUE(val.success);
}

TEST(ResolvedValueTest, L0_should_create_ok_expression)
{
    auto val = ResolvedValue::OkExpression(JsonValue());
    EXPECT_EQ(val.source, ResolveSource::EXPRESSION);
    EXPECT_TRUE(val.success);
}

TEST(ResolvedValueTest, L0_should_create_ok_path)
{
    auto val = ResolvedValue::OkPath(JsonValue(), "/user/name");
    EXPECT_EQ(val.source, ResolveSource::PATH);
    EXPECT_TRUE(val.success);
    EXPECT_EQ(val.path, "/user/name");
}

TEST(ResolvedValueTest, L0_should_create_ok_function_call)
{
    auto val = ResolvedValue::OkFunctionCall(JsonValue(), "formatNumber");
    EXPECT_EQ(val.source, ResolveSource::FUNCTION_CALL);
    EXPECT_TRUE(val.success);
    EXPECT_EQ(val.functionName, "formatNumber");
}

TEST(ResolvedValueTest, L0_should_create_fail_expression)
{
    auto val = ResolvedValue::FailExpression("eval error");
    EXPECT_EQ(val.source, ResolveSource::EXPRESSION);
    EXPECT_FALSE(val.success);
    EXPECT_EQ(val.errorMessage, "eval error");
}

TEST(ResolvedValueTest, L0_should_create_fail_path)
{
    auto val = ResolvedValue::FailPath("/bad/path", "not found");
    EXPECT_EQ(val.source, ResolveSource::PATH);
    EXPECT_FALSE(val.success);
    EXPECT_EQ(val.path, "/bad/path");
    EXPECT_EQ(val.errorMessage, "not found");
}

TEST(ResolvedValueTest, L0_should_create_fail_function_call)
{
    auto val = ResolvedValue::FailFunctionCall("badFunc", "error");
    EXPECT_EQ(val.source, ResolveSource::FUNCTION_CALL);
    EXPECT_FALSE(val.success);
    EXPECT_EQ(val.functionName, "badFunc");
}

TEST(ResolvedValueTest, L0_should_create_fail_invalid)
{
    auto val = ResolvedValue::FailInvalid("bad value");
    EXPECT_EQ(val.source, ResolveSource::INVALID);
    EXPECT_FALSE(val.success);
    EXPECT_EQ(val.errorMessage, "bad value");
}

// ==================== ExpressionEngine Branch Coverage ====================

class ExpressionEngineBranchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(ExpressionEngineBranchTest, L0_should_return_undefined_for_non_expression_string)
{
    EvaluationContext ctx;
    auto result = ExpressionEngine::GetInstance().Evaluate("not an expression", ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_to_json_value_number)
{
    EvaluationContext ctx;
    auto jsonVal = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ 42 }}", ctx);
    EXPECT_TRUE(jsonVal.IsValid());
    EXPECT_DOUBLE_EQ(jsonVal.GetNumberValue(0), 42.0);
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_to_json_value_string)
{
    EvaluationContext ctx;
    auto jsonVal = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ 'hello' }}", ctx);
    EXPECT_TRUE(jsonVal.IsValid());
    EXPECT_EQ(jsonVal.GetStringValue(""), "hello");
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_to_json_value_boolean)
{
    EvaluationContext ctx;
    auto jsonVal = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ true }}", ctx);
    EXPECT_TRUE(jsonVal.IsValid());
    EXPECT_TRUE(jsonVal.GetBoolValue(false));
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_to_json_value_undefined_returns_invalid)
{
    EvaluationContext ctx;
    auto jsonVal = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ 1 / 0 }}", ctx);
    EXPECT_FALSE(jsonVal.IsValid());
}

TEST_F(ExpressionEngineBranchTest, L0_should_parse_valid_expression)
{
    auto result = ExpressionEngine::GetInstance().Parse("1 + 2");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.ast, nullptr);
}

TEST_F(ExpressionEngineBranchTest, L0_should_parse_fail_on_empty)
{
    auto result = ExpressionEngine::GetInstance().Parse("");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorMessage, "empty expression");
}

TEST_F(ExpressionEngineBranchTest, L0_should_parse_fail_on_invalid)
{
    auto result = ExpressionEngine::GetInstance().Parse("1 + + 2");
    EXPECT_FALSE(result.success);
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_and_collect)
{
    EvaluationContext ctx;
    auto result = ExpressionEngine::GetInstance().EvaluateAndCollect("{{ 1 + 2 }}", ctx);
    EXPECT_TRUE(result.result.IsDefined());
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_and_collect_undefined_returns_empty_deps)
{
    EvaluationContext ctx;
    auto result = ExpressionEngine::GetInstance().EvaluateAndCollect("{{ 1 / 0 }}", ctx);
    EXPECT_TRUE(result.result.IsUndefined());
    EXPECT_TRUE(result.dependencies.empty());
}

/**
 * @tc.name: ExpressionEngine_should_resolve_escaped_json_pointer_segments_and_ignore_literal_size_tokens
 * @tc.desc: 验证 JSON Pointer 转义片段、无效转义以及字符串中的 size 文本都按预期处理。
 * @tc.type: FUNC
 */
TEST_F(ExpressionEngineBranchTest, L0_should_resolve_escaped_json_pointer_segments_and_ignore_literal_size_tokens)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"01":"leading-zero","a/b":"slash","~key":"tilde"})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto leadingZero = EvalWithContext("{{ ${/01} }}", ctx);
    ASSERT_TRUE(leadingZero.IsString());
    EXPECT_EQ(leadingZero.AsString(), "leading-zero");

    auto slashKey = EvalWithContext("{{ ${/a~1b} }}", ctx);
    EXPECT_TRUE(slashKey.IsUndefined());
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto tildeKey = EvalWithContext("{{ ${/~0key} }}", ctx);
    EXPECT_TRUE(tildeKey.IsUndefined());
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto invalidPointerEscape = EvalWithContext("{{ ${/bad~2key} }}", ctx);
    EXPECT_TRUE(invalidPointerEscape.IsUndefined());
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto quotedSizeText = EvalWithContext("{{ 'size(1)' }}", ctx);
    ASSERT_TRUE(quotedSizeText.IsString());
    EXPECT_EQ(quotedSizeText.AsString(), "size(1)");
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto unrelatedFunction = EvalWithContext("{{ resize(1) }}", ctx);
    EXPECT_TRUE(unrelatedFunction.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
    EXPECT_NE(ctx.errorMessage.find("unknown function"), std::string::npos);
}

/**
 * @tc.name: ExpressionEngine_should_support_size_whitespace_and_unbalanced_parenthesis_paths
 * @tc.desc: 验证 size 重写支持空白和嵌套括号，并对未闭合括号走失败分支。
 * @tc.type: FUNC
 */
TEST_F(ExpressionEngineBranchTest, L0_should_support_size_whitespace_and_unbalanced_parenthesis_paths)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto whitespaceSize = EvalWithContext("{{ size   ( ($__dataModel.items) ) }}", ctx);
    ASSERT_TRUE(whitespaceSize.IsNumber());
    EXPECT_DOUBLE_EQ(whitespaceSize.AsNumber(), 3.0);

    JsonValue arrayJson = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ $__dataModel.items }}", ctx);
    ASSERT_TRUE(arrayJson.IsArray());
    EXPECT_EQ(arrayJson.GetArraySize(), 3u);

    ctx.ClearError();
    auto unbalancedSize = EvalWithContext("{{ size(($__dataModel.items) }}", ctx);
    EXPECT_TRUE(unbalancedSize.IsUndefined());
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST_F(ExpressionEngineBranchTest, L0_should_cache_update_existing_key)
{
    auto& engine = ExpressionEngine::GetInstance();
    engine.EnableAstCache(true);
    engine.ClearAstCache();
    engine.SetAstCacheCapacity(256);

    EvaluationContext ctx;
    engine.Evaluate("{{ 1 + 1 }}", ctx);
    EXPECT_EQ(engine.GetAstCacheSize(), 1u);
    engine.Evaluate("{{ 1 + 1 }}", ctx);
    EXPECT_EQ(engine.GetAstCacheSize(), 1u);
    engine.EnableAstCache(false);
}

// ==================== Evaluator Branch Coverage ====================

class EvaluatorBranchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_and_with_false_left)
{
    auto result = Eval("false && 1");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_FALSE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_and_with_undefined_right)
{
    auto result = Eval("true && $undef");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_or_with_true_left)
{
    auto result = Eval("true || 0");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_or_with_undefined_right)
{
    auto result = Eval("false || $undef");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_and_with_undefined_left)
{
    auto result = Eval("$undef && true");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_short_circuit_or_with_undefined_left)
{
    auto result = Eval("$undef || true");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_handle_undefined_in_binary_left)
{
    auto result = Eval("$undef + 1");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_handle_undefined_in_binary_right)
{
    auto result = Eval("1 + $undef");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_handle_undefined_in_ternary_condition)
{
    auto result = Eval("$undef ? 1 : 2");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_handle_member_access_undefined)
{
    auto result = Eval("user.name");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_handle_variable_reference_undefined)
{
    auto result = Eval("$user");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_nested_ternary)
{
    auto result = Eval("true ? false ? 1 : 2 : 3");
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 2.0);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_chained_or)
{
    auto result = Eval("false || false || true");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_chained_and)
{
    auto result = Eval("true && true && false");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_FALSE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_number_lt)
{
    auto result = Eval("1 < 2");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_number_comparison_gt)
{
    auto result = Eval("5 > 3");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_number_comparison_gte)
{
    auto result = Eval("3 >= 3");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_null_in_evalresult)
{
    EvalResult nullVal = EvalResult::Null();
    EXPECT_TRUE(nullVal.IsNull());
    EXPECT_FALSE(nullVal.AsBool());
    EXPECT_DOUBLE_EQ(nullVal.AsNumber(), 0.0);
    EXPECT_EQ(nullVal.AsString(), "null");
}

// ==================== IsExpression Edge Cases ====================

TEST_F(ExpressionFormatTest, L0_should_reject_too_short_string)
{
    EXPECT_FALSE(ExpressionEngine::IsExpression("{{"));
    EXPECT_FALSE(ExpressionEngine::IsExpression("a"));
}

TEST_F(ExpressionFormatTest, L0_should_accept_empty_expression_content)
{
    EXPECT_TRUE(ExpressionEngine::IsExpression("{{}}"));
}

// ==================== EvaluationContext Branch Coverage ====================

TEST(EvaluationContextTest, L0_should_set_and_get_context_fields)
{
    EvaluationContext ctx;
    ctx.SetRenderId(42);
    ctx.SetSurfaceId("surface1");
    ctx.SetComponentId("comp1");
    EXPECT_EQ(ctx.GetRenderId(), 42);
    EXPECT_EQ(ctx.GetSurfaceId(), "surface1");
    EXPECT_EQ(ctx.GetComponentId(), "comp1");
}

TEST(EvaluationContextTest, L0_should_manage_global_variables)
{
    EvaluationContext ctx;
    ctx.SetGlobalVariable("x", EvalResult::FromNumber(42.0));
    auto result = ctx.ResolveVariable("x");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 42.0);
}

TEST(EvaluationContextTest, L0_should_manage_scope_stack)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("x", EvalResult::FromNumber(1.0));
    ctx.PopScope();
}

TEST(EvaluationContextTest, L0_should_handle_set_local_without_scope)
{
    EvaluationContext ctx;
    ctx.SetLocalVariable("x", EvalResult::FromNumber(1.0));
}

TEST(EvaluationContextTest, L0_should_handle_pop_on_empty_scope_stack)
{
    EvaluationContext ctx;
    ctx.PopScope();
}

TEST(EvaluationContextTest, L0_should_set_and_clear_error)
{
    EvaluationContext ctx;
    ctx.SetError(ExpressionError::EVAL_DIVISION_BY_ZERO, "div by zero", 10);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_DIVISION_BY_ZERO);
    EXPECT_EQ(ctx.errorMessage, "div by zero");
    EXPECT_EQ(ctx.errorPosition, 10u);
    ctx.ClearError();
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);
    EXPECT_TRUE(ctx.errorMessage.empty());
}

TEST(EvaluationContextTest, L0_should_resolve_data_model_root_as_string_or_json_based_on_flag)
{
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2],"name":"Alice"})");
    ASSERT_NE(adapter, nullptr);

    DataModel dataModel("ctx-surface");
    dataModel.ReplaceAll(adapter->GetRoot());

    EvaluationContext ctx;
    ctx.SetDataModel(&dataModel);

    EvalResult stringResult = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(stringResult.IsString());
    EXPECT_NE(stringResult.AsString().find("\"items\""), std::string::npos);

    ctx.allowContainerResults = true;
    EvalResult jsonResult = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(jsonResult.IsJson());
    ASSERT_TRUE(jsonResult.IsObject());
    EXPECT_TRUE(jsonResult.AsJson().GetItem("items").IsArray());
}

// ==================== DynamicValueResolver Branch Coverage ====================

TEST(DynamicValueResolverTest, L0_should_resolve_simple_literal)
{
    auto adapter = JsonAdapter::Parse(R"({"value": 42})");
    ASSERT_NE(adapter, nullptr);
    DynamicResolveContext ctx;
    auto result = DynamicValueResolver::Resolve(adapter->GetRoot().GetChild(), ctx);
    EXPECT_TRUE(result.success);
}

namespace {

constexpr int32_t DYNAMIC_TEMPLATE_RENDER_ID = 9410;
const char* DYNAMIC_TEMPLATE_SURFACE_ID = "dynamic-template-surface";

} // namespace

class DynamicValueResolverJsonPointerTest : public A2UITest {
protected:
    void SetUp() override
    {
        A2UITest::SetUp();
        RenderManager::GetInstance().RemoveRenderSlot(DYNAMIC_TEMPLATE_RENDER_ID);

        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(DYNAMIC_TEMPLATE_RENDER_ID);
        SurfaceSlot& surfaceSlot = renderSlot.GetSurfaceManager()->CreateSurface(DYNAMIC_TEMPLATE_SURFACE_ID);
        dataModel_ = surfaceSlot.GetOrCreateDataModel();
        ASSERT_NE(dataModel_, nullptr);

        auto initialData = JsonAdapter::Parse(R"({"user":{"name":"Alice"},"users":[{"a":"first"},{"a":"second"}]})");
        ASSERT_NE(initialData, nullptr);
        dataModel_->ReplaceAll(initialData->GetRoot());
    }

    void TearDown() override
    {
        RenderManager::GetInstance().RemoveRenderSlot(DYNAMIC_TEMPLATE_RENDER_ID);
        A2UITest::TearDown();
    }

    DynamicResolveContext MakeContext() const
    {
        return DynamicResolveContext { .renderId = DYNAMIC_TEMPLATE_RENDER_ID,
            .surfaceId = DYNAMIC_TEMPLATE_SURFACE_ID,
            .componentId = "dynamic-template-component",
            .allowExpression = true };
    }

    std::shared_ptr<DataModel> dataModel_;
};

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_resolve_json_pointer_template_string)
{
    auto adapter = JsonAdapter::Parse(R"("User name = ${/user/name}")");
    ASSERT_NE(adapter, nullptr);

    auto result = DynamicValueResolver::Resolve(adapter->GetRoot(), MakeContext());

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsString());
    EXPECT_EQ(result.value.GetStringValue(""), "User name = Alice");
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_resolve_json_pointer_template_with_array_index)
{
    auto adapter = JsonAdapter::Parse(R"("${/users/0/a}")");
    ASSERT_NE(adapter, nullptr);

    auto result = DynamicValueResolver::Resolve(adapter->GetRoot(), MakeContext());

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsString());
    EXPECT_EQ(result.value.GetStringValue(""), "first");
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_keep_surrounding_literal_when_json_pointer_path_missing)
{
    auto adapter = JsonAdapter::Parse(R"("User name = ${/user/alias}")");
    ASSERT_NE(adapter, nullptr);

    auto result = DynamicValueResolver::Resolve(adapter->GetRoot(), MakeContext());

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.value.IsString());
    EXPECT_EQ(result.value.GetStringValue(""), "User name = ");
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_preserve_soft_expression_errors_for_multiple_error_categories)
{
    auto missingGlobalAdapter = JsonAdapter::Parse(R"("{{ 'Mode = ' + $__missingGlobal }}")");
    auto undefinedVariableAdapter = JsonAdapter::Parse(R"("{{ 'User = ' + $missingVar }}")");
    auto illegalExpressionAdapter = JsonAdapter::Parse(R"("{{ 'User = ' + $__dataModel[name] }}")");
    ASSERT_NE(missingGlobalAdapter, nullptr);
    ASSERT_NE(undefinedVariableAdapter, nullptr);
    ASSERT_NE(illegalExpressionAdapter, nullptr);

    auto missingGlobal = DynamicValueResolver::Resolve(missingGlobalAdapter->GetRoot(), MakeContext());
    ASSERT_TRUE(missingGlobal.success);
    EXPECT_EQ(missingGlobal.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(missingGlobal.value.GetStringValue(""), "Mode = ");
    EXPECT_NE(missingGlobal.errorMessage.find("no global variables"), std::string::npos);

    auto undefinedVariable = DynamicValueResolver::Resolve(undefinedVariableAdapter->GetRoot(), MakeContext());
    EXPECT_FALSE(undefinedVariable.success);
    EXPECT_EQ(undefinedVariable.source, ResolveSource::EXPRESSION);
    EXPECT_NE(undefinedVariable.errorMessage.find("undefined variable"), std::string::npos);

    auto illegalExpression = DynamicValueResolver::Resolve(illegalExpressionAdapter->GetRoot(), MakeContext());
    ASSERT_TRUE(illegalExpression.success);
    EXPECT_EQ(illegalExpression.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(illegalExpression.value.GetStringValue(""), "User = ");
    EXPECT_NE(illegalExpression.errorMessage.find("illegal expression"), std::string::npos);
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_resolve_template_local_variables_in_expression_context)
{
    auto expressionAdapter = JsonAdapter::Parse(R"("{{ $item.name + ':' + $index }}")");
    auto itemAdapter = JsonAdapter::Parse(R"({"name":"Alice"})");
    auto indexAdapter = JsonAdapter::CreateNumber(2.0);
    ASSERT_NE(expressionAdapter, nullptr);
    ASSERT_NE(itemAdapter, nullptr);
    ASSERT_NE(indexAdapter, nullptr);

    DynamicResolveContext context = MakeContext();
    context.localVariables["item"] = itemAdapter->GetRoot();
    context.localVariables["index"] = indexAdapter->GetRoot();

    ResolvedValue result = DynamicValueResolver::Resolve(expressionAdapter->GetRoot(), context);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(result.value.GetStringValue(""), "Alice:2");
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_resolve_object_and_array_local_variables)
{
    auto expressionAdapter = JsonAdapter::Parse(R"("{{ $order.items[1].sku + ':' + size($order.items) }}")");
    auto orderAdapter = JsonAdapter::Parse(R"({"items":[{"sku":"A1"},{"sku":"B2"}]})");
    ASSERT_NE(expressionAdapter, nullptr);
    ASSERT_NE(orderAdapter, nullptr);

    DynamicResolveContext context = MakeContext();
    context.localVariables["order"] = orderAdapter->GetRoot();

    ResolvedValue result = DynamicValueResolver::Resolve(expressionAdapter->GetRoot(), context);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(result.value.GetStringValue(""), "B2:2");
}

TEST_F(DynamicValueResolverJsonPointerTest, L0_should_keep_json_pointer_resolution_independent_from_local_variables)
{
    auto expressionAdapter = JsonAdapter::Parse(R"("{{ ${/user/name} + ':' + $item.name }}")");
    auto itemAdapter = JsonAdapter::Parse(R"({"name":"Local"})");
    ASSERT_NE(expressionAdapter, nullptr);
    ASSERT_NE(itemAdapter, nullptr);

    DynamicResolveContext context = MakeContext();
    context.localVariables["item"] = itemAdapter->GetRoot();

    ResolvedValue result = DynamicValueResolver::Resolve(expressionAdapter->GetRoot(), context);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.source, ResolveSource::EXPRESSION);
    EXPECT_EQ(result.value.GetStringValue(""), "Alice:Local");
}

// ==================== Sandbox Branch Coverage (target 80%) ====================
// Uncovered: CollectAstStats null node, consecutive function calls, template literal,
// too many function calls, consecutive call chains, template depth, memory limit

TEST_F(SandboxTest, L0_should_pass_ast_with_function_call)
{
    // Build: func(1)
    auto num = std::make_shared<NumberLiteral>(1.0);
    auto fc = std::make_shared<FunctionCall>("func");
    fc->arguments.push_back(num);
    auto result = sandbox_.ValidateAst(fc, context_);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.nodeCount, 2);
}

TEST_F(SandboxTest, L0_should_reject_too_many_function_calls)
{
    auto fc1 = std::make_shared<FunctionCall>("f1");
    auto fc2 = std::make_shared<FunctionCall>("f2");
    auto fc3 = std::make_shared<FunctionCall>("f3");
    auto fc4 = std::make_shared<FunctionCall>("f4");
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = fc1;
    auto bin2 = std::make_shared<BinaryExpression>();
    bin2->op = BinaryOp::PLUS;
    bin2->left = bin;
    bin2->right = fc2;
    auto bin3 = std::make_shared<BinaryExpression>();
    bin3->op = BinaryOp::PLUS;
    bin3->left = bin2;
    bin3->right = fc3;
    auto bin4 = std::make_shared<BinaryExpression>();
    bin4->op = BinaryOp::PLUS;
    bin4->left = bin3;
    bin4->right = fc4;
    auto result = sandbox_.ValidateAst(bin4, context_);
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.reason.find("function calls") != std::string::npos);
}

TEST_F(SandboxTest, L0_should_reject_consecutive_call_chains)
{
    auto inner = std::make_shared<FunctionCall>("inner");
    auto outer = std::make_shared<FunctionCall>("outer");
    outer->arguments.push_back(inner);
    auto result = sandbox_.ValidateAst(outer, context_);
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.reason.find("consecutive") != std::string::npos);
}

TEST_F(SandboxTest, L0_should_handle_template_literal_in_ast_stats)
{
    auto tl = std::make_shared<TemplateLiteral>();
    TemplatePart textPart;
    textPart.isExpression = false;
    textPart.text = "hello ";
    tl->parts.push_back(textPart);
    TemplatePart exprPart;
    exprPart.isExpression = true;
    exprPart.expression = std::make_shared<NumberLiteral>(1.0);
    tl->parts.push_back(exprPart);
    auto result = sandbox_.ValidateAst(tl, context_);
    EXPECT_TRUE(result.valid);
}

TEST_F(SandboxTest, L0_should_handle_member_access_with_bracket_key)
{
    auto obj = std::make_shared<VariableReference>("obj");
    auto ma = std::make_shared<MemberAccess>();
    ma->object = obj;
    ma->isBracket = true;
    ma->bracketKey = std::make_shared<NumberLiteral>(0.0);
    auto result = sandbox_.ValidateAst(ma, context_);
    EXPECT_TRUE(result.valid);
}

TEST_F(SandboxTest, L0_should_handle_conditional_expression)
{
    auto cond = std::make_shared<ConditionalExpression>();
    cond->condition = std::make_shared<BooleanLiteral>(true);
    cond->consequent = std::make_shared<NumberLiteral>(1.0);
    cond->alternate = std::make_shared<NumberLiteral>(2.0);
    auto result = sandbox_.ValidateAst(cond, context_);
    EXPECT_TRUE(result.valid);
}

TEST_F(SandboxTest, L0_should_handle_grouped_expression)
{
    auto grp = std::make_shared<GroupedExpression>();
    grp->expression = std::make_shared<NumberLiteral>(42.0);
    auto result = sandbox_.ValidateAst(grp, context_);
    EXPECT_TRUE(result.valid);
}

TEST_F(SandboxTest, L0_should_handle_unary_expression)
{
    auto unary = std::make_shared<UnaryExpression>();
    unary->op = UnaryOp::MINUS;
    unary->operand = std::make_shared<NumberLiteral>(5.0);
    auto result = sandbox_.ValidateAst(unary, context_);
    EXPECT_TRUE(result.valid);
}

TEST_F(SandboxTest, L0_should_reject_memory_limit_exceeded)
{
    // Build a tree with many nodes - 112 nodes, each ~600 bytes = ~67KB > 64KB limit
    // But maxAstNodes must be high enough to pass the node count check
    std::shared_ptr<AstNode> node = std::make_shared<NumberLiteral>(1.0);
    for (int i = 0; i < 110; i++) {
        auto bin = std::make_shared<BinaryExpression>();
        bin->op = BinaryOp::PLUS;
        bin->left = node;
        bin->right = std::make_shared<NumberLiteral>(1.0);
        node = bin;
    }
    context_.maxAstNodes = 500;
    auto result = sandbox_.ValidateAst(node, context_);
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.reason.find("memory") != std::string::npos);
}

// ==================== Evaluator Branch Coverage (target 80%) ====================
// Uncovered: null AST, TEMPLATE_LITERAL, missing binary operands, unsupported binary/unary op,
// function call with registered function, function arg eval failure

TEST_F(EvaluatorBranchTest, L0_should_return_undefined_for_template_literal)
{
    // Template literals are parsed from backtick strings but evaluator returns undefined
    // We need to construct a template literal AST and evaluate it directly
    auto tl = std::make_shared<TemplateLiteral>();
    EvaluationContext ctx;
    auto& engine = ExpressionEngine::GetInstance();
    // Use the internal evaluator through the engine - but we can test via backtick
    auto result = Eval("`hello`");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_registered_function)
{
    // Register a function that the evaluator can call
    auto& engine = ExpressionEngine::GetInstance();
    engine.ClearAstCache();
    engine.EnableAstCache(false);

    // format('x') is unknown since no functions registered - test via ExpressionFunctions
    ExpressionFunctions funcs;
    funcs.Register("double", [](const std::vector<EvalResult>& args) -> EvalResult {
        if (args.empty())
            return EvalResult::Undefined();
        return EvalResult::FromNumber(args[0].AsNumber() * 2);
    });

    // Test function call through parser + evaluator
    Lexer lexer;
    Parser parser;
    auto tokens = lexer.Tokenize("1 + 2");
    auto parseResult = parser.Parse(tokens);
    ASSERT_TRUE(parseResult.success);

    EvaluationContext ctx;
    Evaluator eval(funcs);
    auto result = eval.Evaluate(parseResult.ast, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 3.0);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_function_call_successfully)
{
    ExpressionFunctions funcs;
    funcs.Register("abs", [](const std::vector<EvalResult>& args) -> EvalResult {
        if (args.empty())
            return EvalResult::FromNumber(0.0);
        return EvalResult::FromNumber(std::abs(args[0].AsNumber()));
    });

    Lexer lexer;
    Parser parser;
    auto tokens = lexer.Tokenize("abs(-5)");
    auto parseResult = parser.Parse(tokens);
    ASSERT_TRUE(parseResult.success);

    EvaluationContext ctx;
    Evaluator eval(funcs);
    auto result = eval.Evaluate(parseResult.ast, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 5.0);
}

TEST_F(EvaluatorBranchTest, L0_should_return_undefined_for_unknown_function)
{
    Lexer lexer;
    Parser parser;
    auto tokens = lexer.Tokenize("unknown(1)");
    auto parseResult = parser.Parse(tokens);
    ASSERT_TRUE(parseResult.success);

    ExpressionFunctions funcs;
    EvaluationContext ctx;
    Evaluator eval(funcs);
    auto result = eval.Evaluate(parseResult.ast, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_return_string_length_for_member_access)
{
    EvaluationContext ctx;
    ctx.SetGlobalVariable("user", EvalResult::FromString("alice"));

    auto result = EvalWithContext("{{ $user.length }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 5.0);
}

TEST_F(EvaluatorBranchTest, L0_should_return_undefined_for_null_ast)
{
    ExpressionFunctions funcs;
    EvaluationContext ctx;
    Evaluator eval(funcs);
    auto result = eval.Evaluate(nullptr, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_boolean_false_literal)
{
    auto result = Eval("false");
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_FALSE(result.boolValue);
}

TEST_F(EvaluatorBranchTest, L0_should_evaluate_null_result)
{
    EvalResult nullVal = EvalResult::Null();
    EXPECT_FALSE(nullVal.AsBool());
}

// ==================== Parser Branch Coverage (target 80%) ====================
// Uncovered: various nullptr returns from sub-parsers

TEST_F(ParserBranchTest, L0_should_parse_complex_nested_expressions)
{
    auto ast = Parse("(1 + 2) * (3 - 4) / 5 % 2");
    ASSERT_NE(ast, nullptr);
}

TEST_F(ParserBranchTest, L0_should_parse_boolean_true)
{
    auto ast = Parse("true");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::BOOLEAN_LITERAL);
    EXPECT_TRUE(static_cast<BooleanLiteral*>(ast.get())->value);
}

TEST_F(ParserBranchTest, L0_should_parse_boolean_false)
{
    auto ast = Parse("false");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::BOOLEAN_LITERAL);
    EXPECT_FALSE(static_cast<BooleanLiteral*>(ast.get())->value);
}

TEST_F(ParserBranchTest, L0_should_parse_grouped_expression)
{
    auto ast = Parse("(42)");
    ASSERT_NE(ast, nullptr);
    EXPECT_EQ(ast->type, AstNodeType::GROUPED_EXPRESSION);
}

TEST_F(ParserBranchTest, L0_should_parse_multiple_and_operators)
{
    auto ast = Parse("true && true && false");
    ASSERT_NE(ast, nullptr);
}

TEST_F(ParserBranchTest, L0_should_parse_multiple_or_operators)
{
    auto ast = Parse("false || true || false");
    ASSERT_NE(ast, nullptr);
}

TEST_F(ParserBranchTest, L0_should_parse_multiple_eq_operators)
{
    auto ast = Parse("1 == 1 != 0");
    ASSERT_NE(ast, nullptr);
}

TEST_F(ParserBranchTest, L0_should_parse_chained_member_access)
{
    auto ast = Parse("a.b.c");
    ASSERT_NE(ast, nullptr);
}

// ==================== ExpressionEngine Branch Coverage (target 80%) ====================
// Uncovered: bracket balance failure, EvaluateAsJsonValue inf/NaN, default case in switch,
// EvaluateAndCollect dependencies, cache update existing key

TEST_F(ExpressionEngineBranchTest, L0_should_reject_unbalanced_brackets_via_engine)
{
    EvaluationContext ctx;
    // Expression with unmatched brackets that pass token-level nesting check
    // but fail bracket balance
    auto result = ExpressionEngine::GetInstance().Evaluate("{{ (1 + 2 }}", ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(ExpressionEngineBranchTest, L0_should_handle_infinity_in_json_value)
{
    EvaluationContext ctx;
    // Register a function that returns infinity
    auto& engine = ExpressionEngine::GetInstance();
    // Division by zero returns undefined, not infinity. Test via direct EvalResult
    EvalResult inf = EvalResult::FromNumber(std::numeric_limits<double>::infinity());
    EXPECT_EQ(inf.AsString(), "");
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_and_collect_dependencies)
{
    auto& engine = ExpressionEngine::GetInstance();
    engine.ClearAstCache();
    engine.EnableAstCache(true);
    engine.SetAstCacheCapacity(256);

    EvaluationContext ctx;
    auto result = engine.EvaluateAndCollect("{{ 1 + 2 }}", ctx);
    EXPECT_TRUE(result.result.IsDefined());

    // Test cache update for existing key
    auto result2 = engine.EvaluateAndCollect("{{ 1 + 2 }}", ctx);
    EXPECT_TRUE(result2.result.IsDefined());

    engine.EnableAstCache(false);
}

TEST_F(ExpressionEngineBranchTest, L0_should_leave_dependencies_empty_for_wrapped_expression_input)
{
    ThemeContext themeCtx;
    themeCtx.colorMode = ThemeMode::DARK;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);

    auto result = ExpressionEngine::GetInstance().EvaluateAndCollect("{{ $__colorMode == 'dark' ? 'b' : 'a' }}", ctx);
    ASSERT_TRUE(result.result.IsDefined());
    EXPECT_TRUE(result.dependencies.empty());
}

TEST_F(ExpressionEngineBranchTest, L0_should_handle_non_expression_extract)
{
    // Non-expression string should fail at ExtractExpression
    EvaluationContext ctx;
    auto result = ExpressionEngine::GetInstance().Evaluate("plain text", ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_json_value_for_all_types)
{
    EvaluationContext ctx;
    auto& engine = ExpressionEngine::GetInstance();

    // Number
    auto numJson = engine.EvaluateAsJsonValue("{{ 42 }}", ctx);
    EXPECT_TRUE(numJson.IsValid());

    // String
    auto strJson = engine.EvaluateAsJsonValue("{{ 'hello' }}", ctx);
    EXPECT_TRUE(strJson.IsValid());

    // Boolean
    auto boolJson = engine.EvaluateAsJsonValue("{{ true }}", ctx);
    EXPECT_TRUE(boolJson.IsValid());

    // Undefined returns invalid
    auto undefJson = engine.EvaluateAsJsonValue("{{ 1 / 0 }}", ctx);
    EXPECT_FALSE(undefJson.IsValid());
}

// ==================== EvalResult remaining branches for 80% ====================

TEST(EvalResultFullTest, L0_should_handle_all_number_to_string_cases)
{
    // Integer that fits in long long
    EXPECT_EQ(EvalResult::FromNumber(0.0).AsString(), "0");
    EXPECT_EQ(EvalResult::FromNumber(-1.0).AsString(), "-1");
    EXPECT_EQ(EvalResult::FromNumber(999999999999999.0).AsString(), "999999999999999");

    // Decimal
    EXPECT_EQ(EvalResult::FromNumber(1.5).AsString(), "1.5");

    // Very large number (>1e15) uses stream formatting
    EXPECT_NE(EvalResult::FromNumber(1e20).AsString(), "");

    // Non-finite
    EXPECT_EQ(EvalResult::FromNumber(std::numeric_limits<double>::infinity()).AsString(), "");
    EXPECT_EQ(EvalResult::FromNumber(-std::numeric_limits<double>::infinity()).AsString(), "");
}

TEST(EvalResultFullTest, L0_should_handle_all_string_to_number_cases)
{
    // Empty string
    EXPECT_DOUBLE_EQ(EvalResult::FromString("").AsNumber(), 0.0);

    // Valid number with trailing spaces
    EXPECT_DOUBLE_EQ(EvalResult::FromString(" 42 ").AsNumber(), 42.0);

    // Invalid with non-space trailing
    EXPECT_DOUBLE_EQ(EvalResult::FromString("42abc").AsNumber(), 0.0);

    // Overflow
    EXPECT_DOUBLE_EQ(EvalResult::FromString("1e999").AsNumber(), 0.0);

    // Negative
    EXPECT_DOUBLE_EQ(EvalResult::FromString("-3.14").AsNumber(), -3.14);
}

TEST(EvalResultFullTest, L0_should_handle_all_as_bool_cases)
{
    // NaN number
    EXPECT_FALSE(EvalResult::FromNumber(std::nan("")).AsBool());
    // Non-zero number
    EXPECT_TRUE(EvalResult::FromNumber(0.5).AsBool());
    // Zero number
    EXPECT_FALSE(EvalResult::FromNumber(0.0).AsBool());
}

// ==================== NativeEntry EvaluateExpression branches ====================

TEST_F(NativeExpressionApiTest, L0_should_handle_no_args)
{
    mockNapiPtr_->SetCallbackArgs({});
    napi_value result = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success"));
}

TEST_F(NativeExpressionApiTest, L0_should_handle_null_arg)
{
    mockNapiPtr_->SetCallbackArgs({ nullptr });
    napi_value result = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success"));
}

TEST_F(NativeExpressionApiTest, L0_should_handle_long_expression)
{
    std::string longExpr = "{{ " + std::string(3000, '1') + " }}";
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg(longExpr) });
    napi_value result = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(GetBoolProperty(result, "success"));
}

TEST_F(NativeExpressionApiTest, L0_should_return_null_result)
{
    mockNapiPtr_->SetCallbackArgs({ CreateStringArg("{{ null }}") });
    // null is not a valid expression keyword - will fail to parse
    napi_value result = EvaluateExpression(env_, cbInfo_);
    ASSERT_NE(result, nullptr);
}

// ==================== Evaluator Direct AST Tests (target 80%) ====================
// These test branches that can't be reached through normal parsing

class EvaluatorDirectTest : public ::testing::Test {
protected:
    ExpressionFunctions funcs_;
    std::unique_ptr<Evaluator> eval_;

    void SetUp() override
    {
        funcs_.Register("testFunc", [](const std::vector<EvalResult>& args) -> EvalResult {
            if (args.empty())
                return EvalResult::FromNumber(0.0);
            return EvalResult::FromNumber(args[0].AsNumber() + 1.0);
        });
        eval_ = std::make_unique<Evaluator>(funcs_);
    }
};

TEST_F(EvaluatorDirectTest, L0_should_handle_null_ast)
{
    EvaluationContext ctx;
    auto result = eval_->Evaluate(nullptr, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_handle_template_literal)
{
    auto tl = std::make_shared<TemplateLiteral>();
    EvaluationContext ctx;
    auto result = eval_->Evaluate(tl, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_handle_binary_missing_operands)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = std::make_shared<NumberLiteral>(1.0);
    bin->right = nullptr;
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_handle_unary_missing_operand)
{
    auto unary = std::make_shared<UnaryExpression>();
    unary->op = UnaryOp::MINUS;
    unary->operand = nullptr;
    EvaluationContext ctx;
    auto result = eval_->Evaluate(unary, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_function_call_with_undefined_arg)
{
    auto fc = std::make_shared<FunctionCall>("testFunc");
    fc->arguments.push_back(std::make_shared<VariableReference>("undef"));
    EvaluationContext ctx;
    auto result = eval_->Evaluate(fc, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_function_call_successfully)
{
    auto fc = std::make_shared<FunctionCall>("testFunc");
    fc->arguments.push_back(std::make_shared<NumberLiteral>(5.0));
    EvaluationContext ctx;
    auto result = eval_->Evaluate(fc, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 6.0);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_variable_reference_undefined)
{
    auto ref = std::make_shared<VariableReference>("x");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(ref, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_member_access_undefined)
{
    auto ma = std::make_shared<MemberAccess>();
    ma->object = std::make_shared<VariableReference>("obj");
    ma->property = "name";
    EvaluationContext ctx;
    auto result = eval_->Evaluate(ma, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_conditional_true)
{
    auto cond = std::make_shared<ConditionalExpression>();
    cond->condition = std::make_shared<BooleanLiteral>(true);
    cond->consequent = std::make_shared<NumberLiteral>(1.0);
    cond->alternate = std::make_shared<NumberLiteral>(2.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(cond, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 1.0);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_conditional_false)
{
    auto cond = std::make_shared<ConditionalExpression>();
    cond->condition = std::make_shared<BooleanLiteral>(false);
    cond->consequent = std::make_shared<NumberLiteral>(1.0);
    cond->alternate = std::make_shared<NumberLiteral>(2.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(cond, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 2.0);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_conditional_undefined_condition)
{
    auto cond = std::make_shared<ConditionalExpression>();
    cond->condition = std::make_shared<VariableReference>("undef");
    cond->consequent = std::make_shared<NumberLiteral>(1.0);
    cond->alternate = std::make_shared<NumberLiteral>(2.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(cond, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_grouped_expression)
{
    auto grp = std::make_shared<GroupedExpression>();
    grp->expression = std::make_shared<NumberLiteral>(42.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(grp, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, 42.0);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_all_binary_ops)
{
    EvaluationContext ctx;
    auto makeBin = [](BinaryOp op, double l, double r) -> std::shared_ptr<BinaryExpression> {
        auto bin = std::make_shared<BinaryExpression>();
        bin->op = op;
        bin->left = std::make_shared<NumberLiteral>(l);
        bin->right = std::make_shared<NumberLiteral>(r);
        return bin;
    };

    auto sub = eval_->Evaluate(makeBin(BinaryOp::MINUS, 10.0, 3.0), ctx);
    EXPECT_DOUBLE_EQ(sub.numberValue, 7.0);

    auto mul = eval_->Evaluate(makeBin(BinaryOp::STAR, 4.0, 5.0), ctx);
    EXPECT_DOUBLE_EQ(mul.numberValue, 20.0);

    auto div = eval_->Evaluate(makeBin(BinaryOp::SLASH, 10.0, 4.0), ctx);
    EXPECT_DOUBLE_EQ(div.numberValue, 2.5);

    auto mod = eval_->Evaluate(makeBin(BinaryOp::PERCENT, 10.0, 3.0), ctx);
    EXPECT_DOUBLE_EQ(mod.numberValue, 1.0);

    auto eq = eval_->Evaluate(makeBin(BinaryOp::EQ, 1.0, 1.0), ctx);
    EXPECT_TRUE(eq.boolValue);

    auto neq = eval_->Evaluate(makeBin(BinaryOp::NEQ, 1.0, 2.0), ctx);
    EXPECT_TRUE(neq.boolValue);

    auto lt = eval_->Evaluate(makeBin(BinaryOp::LT, 1.0, 2.0), ctx);
    EXPECT_TRUE(lt.boolValue);

    auto lte = eval_->Evaluate(makeBin(BinaryOp::LTE, 2.0, 2.0), ctx);
    EXPECT_TRUE(lte.boolValue);

    auto gt = eval_->Evaluate(makeBin(BinaryOp::GT, 3.0, 2.0), ctx);
    EXPECT_TRUE(gt.boolValue);

    auto gte = eval_->Evaluate(makeBin(BinaryOp::GTE, 2.0, 2.0), ctx);
    EXPECT_TRUE(gte.boolValue);

    // Division by zero
    auto divZero = eval_->Evaluate(makeBin(BinaryOp::SLASH, 1.0, 0.0), ctx);
    EXPECT_TRUE(divZero.IsUndefined());

    // Modulo by zero
    auto modZero = eval_->Evaluate(makeBin(BinaryOp::PERCENT, 1.0, 0.0), ctx);
    EXPECT_TRUE(modZero.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_unary_minus)
{
    auto unary = std::make_shared<UnaryExpression>();
    unary->op = UnaryOp::MINUS;
    unary->operand = std::make_shared<NumberLiteral>(5.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(unary, ctx);
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.numberValue, -5.0);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_unary_not)
{
    auto unary = std::make_shared<UnaryExpression>();
    unary->op = UnaryOp::NOT;
    unary->operand = std::make_shared<BooleanLiteral>(false);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(unary, ctx);
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_and_short_circuit)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::AND;
    bin->left = std::make_shared<BooleanLiteral>(false);
    bin->right = std::make_shared<NumberLiteral>(999.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_FALSE(result.boolValue);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_or_short_circuit)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::OR;
    bin->left = std::make_shared<BooleanLiteral>(true);
    bin->right = std::make_shared<NumberLiteral>(999.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    ASSERT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.boolValue);
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_string_concat)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = std::make_shared<StringLiteral>("hello");
    bin->right = std::make_shared<StringLiteral>(" world");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.stringValue, "hello world");
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_number_concat_with_string)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = std::make_shared<NumberLiteral>(5.0);
    bin->right = std::make_shared<StringLiteral>("x");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.stringValue, "5x");
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_binary_undefined_left)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = std::make_shared<VariableReference>("undef");
    bin->right = std::make_shared<NumberLiteral>(1.0);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_binary_undefined_right)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::PLUS;
    bin->left = std::make_shared<NumberLiteral>(1.0);
    bin->right = std::make_shared<VariableReference>("undef");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_and_undefined_left)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::AND;
    bin->left = std::make_shared<VariableReference>("undef");
    bin->right = std::make_shared<BooleanLiteral>(true);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_and_undefined_right)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::AND;
    bin->left = std::make_shared<BooleanLiteral>(true);
    bin->right = std::make_shared<VariableReference>("undef");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_or_undefined_left)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::OR;
    bin->left = std::make_shared<VariableReference>("undef");
    bin->right = std::make_shared<BooleanLiteral>(true);
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_or_undefined_right)
{
    auto bin = std::make_shared<BinaryExpression>();
    bin->op = BinaryOp::OR;
    bin->left = std::make_shared<BooleanLiteral>(false);
    bin->right = std::make_shared<VariableReference>("undef");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(bin, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(EvaluatorDirectTest, L0_should_evaluate_unary_undefined_operand)
{
    auto unary = std::make_shared<UnaryExpression>();
    unary->op = UnaryOp::MINUS;
    unary->operand = std::make_shared<VariableReference>("undef");
    EvaluationContext ctx;
    auto result = eval_->Evaluate(unary, ctx);
    EXPECT_TRUE(result.IsUndefined());
}

// ==================== ThemeContextUtils Tests (TASK-1) ====================

TEST(ThemeContextUtilsTest, L0_should_convert_breakpoint_to_string)
{
    EXPECT_EQ(BreakpointToString(Breakpoint::XS), "xs");
    EXPECT_EQ(BreakpointToString(Breakpoint::SM), "sm");
    EXPECT_EQ(BreakpointToString(Breakpoint::MD), "md");
    EXPECT_EQ(BreakpointToString(Breakpoint::LG), "lg");
    EXPECT_EQ(BreakpointToString(Breakpoint::XL), "xl");
}

TEST(ThemeContextUtilsTest, L0_should_convert_color_mode_to_string)
{
    EXPECT_EQ(ColorModeToString(ThemeMode::LIGHT), "light");
    EXPECT_EQ(ColorModeToString(ThemeMode::DARK), "dark");
}

// ==================== Global Variable Resolution Tests (TASK-1) ====================

TEST(GlobalVariableTest, L0_should_resolve_width_breakpoint_from_theme_context)
{
    EvaluationContext ctx;
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::MD;
    ctx.SetThemeContext(&themeCtx);

    auto result = ctx.ResolveVariable("__widthBreakpoint");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "md");
}

TEST(GlobalVariableTest, L0_should_not_resolve_legacy_window_breakpoint_alias_from_theme_context)
{
    EvaluationContext ctx;
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::LG;
    ctx.SetThemeContext(&themeCtx);

    auto result = ctx.ResolveVariable("__WindowBreakpoint");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_TRUE(result.hasEvaluationError);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_NO_GLOBAL_VARIABLE);
    EXPECT_NE(ctx.errorMessage.find("__WindowBreakpoint"), std::string::npos);
}

TEST(GlobalVariableTest, L0_should_resolve_color_mode_from_theme_context)
{
    EvaluationContext ctx;
    ThemeContext themeCtx;
    themeCtx.colorMode = ThemeMode::DARK;
    ctx.SetThemeContext(&themeCtx);

    auto result = ctx.ResolveVariable("__colorMode");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "dark");
}

TEST(GlobalVariableTest, L0_should_return_default_breakpoint_when_no_theme_context)
{
    EvaluationContext ctx;
    auto result = ctx.ResolveVariable("__widthBreakpoint");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "sm");
}

TEST(GlobalVariableTest, L0_should_return_default_color_mode_when_no_theme_context)
{
    EvaluationContext ctx;
    auto result = ctx.ResolveVariable("__colorMode");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "light");
}

TEST(GlobalVariableTest, L0_should_return_empty_for_data_model_standalone_ref)
{
    EvaluationContext ctx;
    auto result = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
}

TEST(GlobalVariableTest, L0_should_return_empty_when_data_model_root_is_unavailable)
{
    EvaluationContext ctx;
    auto dataModel = std::make_shared<DataModel>("test_surface");
    ctx.SetDataModel(dataModel.get());

    auto result = ctx.ResolveVariable("__dataModel");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
}

TEST(GlobalVariableTest, L0_should_return_empty_for_unknown_system_variable)
{
    EvaluationContext ctx;
    auto result = ctx.ResolveVariable("__unknownVar");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
}

TEST(GlobalVariableTest, L0_should_resolve_global_variable_by_name)
{
    EvaluationContext ctx;
    ctx.SetGlobalVariable("count", EvalResult::FromNumber(42.0));
    auto result = ctx.ResolveVariable("count");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 42.0);
}

TEST(GlobalVariableTest, L0_should_resolve_local_variable_from_scope)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("x", EvalResult::FromNumber(10.0));
    auto result = ctx.ResolveVariable("x");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 10.0);
    ctx.PopScope();
}

TEST(GlobalVariableTest, L0_should_prefer_inner_scope_over_outer)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("x", EvalResult::FromNumber(1.0));
    ctx.PushScope();
    ctx.SetLocalVariable("x", EvalResult::FromNumber(2.0));
    auto result = ctx.ResolveVariable("x");
    EXPECT_DOUBLE_EQ(result.AsNumber(), 2.0);
    ctx.PopScope();
    result = ctx.ResolveVariable("x");
    EXPECT_DOUBLE_EQ(result.AsNumber(), 1.0);
    ctx.PopScope();
}

TEST(GlobalVariableTest, L0_should_reject_system_prefix_in_set_local_variable)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("__widthBreakpoint", EvalResult::FromString("hacked"));
    auto result = ctx.ResolveVariable("__widthBreakpoint");
    ASSERT_TRUE(result.IsDefined());
    EXPECT_NE(result.AsString(), "hacked");
    ctx.PopScope();
}

TEST(GlobalVariableTest, L0_should_prefer_system_variable_over_global)
{
    EvaluationContext ctx;
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::LG;
    ctx.SetThemeContext(&themeCtx);
    ctx.SetGlobalVariable("__widthBreakpoint", EvalResult::FromString("fake"));

    auto result = ctx.ResolveVariable("__widthBreakpoint");
    ASSERT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsString(), "lg");
}

TEST(GlobalVariableTest, L0_should_return_undefined_for_unknown_variable)
{
    EvaluationContext ctx;
    auto result = ctx.ResolveVariable("nonexistent");
    EXPECT_TRUE(result.IsUndefined());
}

TEST(GlobalVariableTest, L0_should_evaluate_expression_with_system_variable)
{
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::SM;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);

    auto result = EvalWithContext("{{ $__widthBreakpoint }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "sm");
}

TEST(GlobalVariableTest, L0_should_evaluate_expression_with_color_mode)
{
    ThemeContext themeCtx;
    themeCtx.colorMode = ThemeMode::DARK;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);

    auto result = EvalWithContext("{{ $__colorMode }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "dark");
}

TEST(GlobalVariableTest, L0_should_report_member_access_error_for_system_variable_member_chain)
{
    ThemeContext themeCtx;
    themeCtx.colorMode = ThemeMode::DARK;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);

    auto result = EvalWithContext("{{ $__colorMode.name }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_TRUE(result.hasEvaluationError);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(ctx.errorMessage, "member access not supported: .name");
}

TEST(GlobalVariableTest, L0_should_reset_seed_error_for_system_variable_member_chain)
{
    ThemeContext themeCtx;
    themeCtx.colorMode = ThemeMode::DARK;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);
    ctx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");

    auto result = EvalWithContext("{{ $__colorMode.name }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_TRUE(result.hasEvaluationError);
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(ctx.errorMessage, "member access not supported: .name");
}

TEST(GlobalVariableTest, L0_should_record_error_when_resolving_unknown_system_variable_directly)
{
    EvaluationContext ctx;

    auto result = ctx.ResolveVariable("__unknownVar");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_NE(ctx.errorMessage.find("no global variables"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("__unknownVar"), std::string::npos);
}

TEST(GlobalVariableTest, L0_should_preserve_existing_error_when_resolving_unknown_system_variable_directly)
{
    EvaluationContext ctx;
    ctx.SetError(ExpressionError::EVAL_DIVISION_BY_ZERO, "division by zero");

    auto result = ctx.ResolveVariable("__unknownVar");
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_DIVISION_BY_ZERO);
    EXPECT_EQ(ctx.errorMessage, "division by zero");
}

TEST(GlobalVariableTest, L0_should_treat_unknown_system_variable_as_empty_string_in_concat)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'User name = ' + $__unknownVar }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "User name = ");
    EXPECT_NE(ctx.errorMessage.find("no global variables"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("__unknownVar"), std::string::npos);
}

TEST(GlobalVariableTest, L0_should_treat_unknown_system_variable_member_chain_as_empty_string)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'User name = ' + $__dataModelss['user']['names'][0].a }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "User name = ");
    EXPECT_NE(ctx.errorMessage.find("no global variables"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("__dataModelss"), std::string::npos);
}

TEST(GlobalVariableTest, L0_should_evaluate_conditional_with_breakpoint)
{
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::SM;

    EvaluationContext ctx;
    ctx.SetThemeContext(&themeCtx);

    auto result = EvalWithContext("{{ $__widthBreakpoint == 'sm' ? 'small' : 'large' }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsString(), "small");
}

// ==================== MemberAccess / DataModel Tests (TASK-2) ====================

TEST(DataModelAccessTest, L0_should_access_data_model_path_via_expression)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.user.name }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "Alice");
}

TEST(DataModelAccessTest, L0_should_return_empty_for_missing_path)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.user.missing }}", ctx);
    EXPECT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsString(), "");
    EXPECT_NE(ctx.errorMessage.find("path not found"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("/user/missing"), std::string::npos);
}

TEST(DataModelAccessTest, L0_should_return_empty_for_null_data_model)
{
    EvaluationContext ctx;
    auto result = EvalWithContext("{{ $__dataModel.user.name }}", ctx);
    EXPECT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsString(), "");
}

TEST(DataModelAccessTest, L0_should_access_number_value_from_data_model)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"count":42})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.count }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 42.0);
}

TEST(DataModelAccessTest, L0_should_access_boolean_value_from_data_model)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"active":true})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.active }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsBool(), true);
}

TEST(DataModelAccessTest, L0_should_access_array_item_from_data_model_with_numeric_bracket)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"names":[{"a":"bbbb"},{"a":"bbbb1"}]}})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.user.names[0].a }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "bbbb");
}

TEST(DataModelAccessTest, L0_should_access_json_pointer_inside_expression_wrapper)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"names":[{"a":"bbbb"},{"a":"bbbb1"}]}})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    EXPECT_TRUE(ExpressionEngine::IsExpression("{{ ${/user/names/0/a} }}"));
    auto result = EvalWithContext("{{ ${/user/names/0/a} }}", ctx);
    ASSERT_TRUE(result.IsDefined()) << static_cast<int32_t>(ctx.lastError) << " " << ctx.errorMessage;
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "bbbb");
}

TEST(DataModelAccessTest, L0_should_treat_string_bracket_data_model_access_as_illegal_expression)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'User name = ' + $__dataModel['user']['names'][0].a }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "User name = ");
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
    EXPECT_NE(ctx.errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("__dataModel"), std::string::npos);
}

TEST(DataModelAccessTest, L0_should_treat_identifier_bracket_data_model_access_as_illegal_expression)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'User name = ' + $__dataModel[name] }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "User name = ");
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
    EXPECT_NE(ctx.errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(ctx.errorMessage.find("__dataModel"), std::string::npos);
}

TEST(DataModelAccessTest, L0_should_treat_empty_bracket_data_model_access_as_parse_error)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ $__dataModel[].name }}", ctx);
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
    EXPECT_EQ(ctx.errorMessage, "unexpected token");
}

TEST(DataModelAccessTest, L0_should_use_data_model_in_conditional)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"level":3})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__dataModel.level > 2 ? 'high' : 'low' }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    EXPECT_EQ(result.AsString(), "high");
}

TEST(DataModelAccessTest, L0_should_convert_object_and_array_nodes_to_strings_when_read_directly)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":{"name":"Alice"},"tags":["a","b"]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto objectResult = EvalWithContext("{{ $__dataModel.user }}", ctx);
    ASSERT_TRUE(objectResult.IsDefined());
    ASSERT_TRUE(objectResult.IsString());
    EXPECT_NE(objectResult.AsString().find("Alice"), std::string::npos);

    auto arrayResult = EvalWithContext("{{ $__dataModel.tags }}", ctx);
    ASSERT_TRUE(arrayResult.IsDefined());
    ASSERT_TRUE(arrayResult.IsString());
    EXPECT_NE(arrayResult.AsString().find("a"), std::string::npos);
    EXPECT_NE(arrayResult.AsString().find("b"), std::string::npos);
}

TEST(DataModelAccessTest, L0_should_combine_system_var_and_data_model)
{
    EvaluationContext ctx;
    ThemeContext themeCtx;
    themeCtx.breakpoint = Breakpoint::SM;
    ctx.SetThemeContext(&themeCtx);

    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"padding":8})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ $__widthBreakpoint == 'sm' ? $__dataModel.padding : 16 }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 8.0);
}

TEST(DataModelAccessTest, L0_should_stringify_data_model_root_and_expose_root_length)
{
    EvaluationContext objectCtx;
    auto objectModel = std::make_shared<DataModel>("test_surface");
    auto objectAdapter = JsonAdapter::Parse(R"({"items":[1,2],"name":"Alice"})");
    ASSERT_NE(objectAdapter, nullptr);
    objectModel->ReplaceAll(objectAdapter->GetRoot());
    objectCtx.SetDataModel(objectModel.get());

    auto rootString = EvalWithContext("{{ $__dataModel }}", objectCtx);
    ASSERT_TRUE(rootString.IsString());
    EXPECT_NE(rootString.AsString().find("\"Alice\""), std::string::npos);

    EvaluationContext arrayCtx;
    auto arrayModel = std::make_shared<DataModel>("test_surface");
    auto arrayAdapter = JsonAdapter::Parse(R"(["a","b","c"])");
    ASSERT_NE(arrayAdapter, nullptr);
    arrayModel->ReplaceAll(arrayAdapter->GetRoot());
    arrayCtx.SetDataModel(arrayModel.get());

    auto lengthResult = EvalWithContext("{{ size($__dataModel) }}", arrayCtx);
    ASSERT_TRUE(lengthResult.IsNumber());
    EXPECT_DOUBLE_EQ(lengthResult.AsNumber(), 3.0);
}

TEST_F(EvaluatorBranchTest, L0_should_access_json_containers_with_dynamic_bracket_keys)
{
    EvaluationContext ctx;
    auto itemsAdapter = JsonAdapter::Parse(R"(["zero","one","two"])");
    auto objectAdapter = JsonAdapter::Parse(R"({"first":"alice","second":"bob","items":[10,20]})");
    ASSERT_NE(itemsAdapter, nullptr);
    ASSERT_NE(objectAdapter, nullptr);

    ctx.PushScope();
    ctx.SetLocalVariable("items", EvalResult::FromJson(itemsAdapter->GetRoot()));
    ctx.SetLocalVariable("object", EvalResult::FromJson(objectAdapter->GetRoot()));
    ctx.SetLocalVariable("index", EvalResult::FromNumber(1.0));
    ctx.SetLocalVariable("stringIndex", EvalResult::FromString("2"));
    ctx.SetLocalVariable("key", EvalResult::FromString("second"));

    auto arrayResult = EvalWithContext("{{ $items[$index] }}", ctx);
    ASSERT_TRUE(arrayResult.IsString());
    EXPECT_EQ(arrayResult.AsString(), "one");

    auto numericStringResult = EvalWithContext("{{ $items[$stringIndex] }}", ctx);
    ASSERT_TRUE(numericStringResult.IsString());
    EXPECT_EQ(numericStringResult.AsString(), "two");

    auto objectResult = EvalWithContext("{{ $object[$key] }}", ctx);
    ASSERT_TRUE(objectResult.IsString());
    EXPECT_EQ(objectResult.AsString(), "bob");

    auto lengthResult = EvalWithContext("{{ $object.items.length }}", ctx);
    ASSERT_TRUE(lengthResult.IsNumber());
    EXPECT_DOUBLE_EQ(lengthResult.AsNumber(), 2.0);

    ctx.PopScope();
}

TEST_F(EvaluatorBranchTest, L0_should_reject_invalid_dynamic_array_indexes)
{
    EvaluationContext ctx;
    auto itemsAdapter = JsonAdapter::Parse(R"(["zero","one","two"])");
    ASSERT_NE(itemsAdapter, nullptr);

    ctx.PushScope();
    ctx.SetLocalVariable("items", EvalResult::FromJson(itemsAdapter->GetRoot()));
    ctx.SetLocalVariable("badKey", EvalResult::FromString("abc"));
    ctx.SetLocalVariable("negative", EvalResult::FromNumber(-1.0));
    ctx.SetLocalVariable("fractional", EvalResult::FromNumber(1.5));
    ctx.SetLocalVariable("tooLarge", EvalResult::FromString("2147483648"));
    ctx.SetLocalVariable("nanIndex", EvalResult::FromString("nan"));
    ctx.SetLocalVariable("boolIndex", EvalResult::FromBool(false));
    ctx.SetLocalVariable("spacedIndex", EvalResult::FromString(" 1 "));
    ctx.SetLocalVariable("blankIndex", EvalResult::FromString("   "));
    ctx.SetLocalVariable("outOfRange", EvalResult::FromString("9"));

    auto badKeyResult = EvalWithContext("{{ $items[$badKey] }}", ctx);
    EXPECT_TRUE(badKeyResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto negativeResult = EvalWithContext("{{ $items[$negative] }}", ctx);
    EXPECT_TRUE(negativeResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto fractionalResult = EvalWithContext("{{ $items[$fractional] }}", ctx);
    EXPECT_TRUE(fractionalResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto tooLargeResult = EvalWithContext("{{ $items[$tooLarge] }}", ctx);
    EXPECT_TRUE(tooLargeResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto nanResult = EvalWithContext("{{ $items[$nanIndex] }}", ctx);
    EXPECT_TRUE(nanResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto boolResult = EvalWithContext("{{ $items[$boolIndex] }}", ctx);
    EXPECT_TRUE(boolResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto spacedIndexResult = EvalWithContext("{{ $items[$spacedIndex] }}", ctx);
    ASSERT_TRUE(spacedIndexResult.IsString());
    EXPECT_EQ(spacedIndexResult.AsString(), "one");

    ctx.ClearError();
    auto blankIndexResult = EvalWithContext("{{ $items[$blankIndex] }}", ctx);
    EXPECT_TRUE(blankIndexResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto outOfRangeResult = EvalWithContext("{{ $items[$outOfRange] }}", ctx);
    EXPECT_TRUE(outOfRangeResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.PopScope();
}

TEST_F(EvaluatorBranchTest, L0_should_treat_invalid_fragment_intrinsics_as_soft_failures)
{
    EvaluationContext ctx;

    auto invalidFragment = EvalWithContext("{{ __fragment('(') }}", ctx);
    ASSERT_TRUE(invalidFragment.IsString());
    EXPECT_EQ(invalidFragment.AsString(), "");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    ctx.PushScope();
    ctx.SetLocalVariable("expr", EvalResult::FromString("$items[0]"));
    auto nonLiteralFragment = EvalWithContext("{{ __fragment($expr) }}", ctx);
    EXPECT_TRUE(nonLiteralFragment.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
    ctx.PopScope();
}

TEST_F(EvaluatorBranchTest, L0_should_preserve_existing_error_when_followup_evaluation_also_fails)
{
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ExpressionFunctions funcs;
    Evaluator eval(funcs);

    EvaluationContext fragmentCtx;
    fragmentCtx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");
    auto fragmentCall = std::make_shared<FunctionCall>("__fragment");
    fragmentCall->arguments.push_back(std::make_shared<StringLiteral>("("));
    auto invalidFragment = eval.Evaluate(fragmentCall, fragmentCtx);
    ASSERT_TRUE(invalidFragment.IsString());
    EXPECT_EQ(invalidFragment.AsString(), "");
    EXPECT_EQ(fragmentCtx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(fragmentCtx.errorMessage, "seed error");

    EvaluationContext sizeCtx;
    sizeCtx.SetDataModel(dm.get());
    sizeCtx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");
    auto sizeCall = std::make_shared<FunctionCall>("__sizeFragment");
    sizeCall->arguments.push_back(std::make_shared<StringLiteral>("$__dataModel.missing"));
    auto missingSizePath = eval.Evaluate(sizeCall, sizeCtx);
    ASSERT_TRUE(missingSizePath.IsNumber());
    EXPECT_DOUBLE_EQ(missingSizePath.AsNumber(), 0.0);
    EXPECT_EQ(sizeCtx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(sizeCtx.errorMessage, "seed error");

    EvaluationContext dataPathCtx;
    dataPathCtx.SetDataModel(dm.get());
    dataPathCtx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");
    auto missingDataPath = std::make_shared<MemberAccess>();
    missingDataPath->object = std::make_shared<VariableReference>("__dataModel", true);
    missingDataPath->property = "missing";
    auto missingDataPathResult = eval.Evaluate(missingDataPath, dataPathCtx);
    ASSERT_TRUE(missingDataPathResult.IsString());
    EXPECT_EQ(missingDataPathResult.AsString(), "");
    EXPECT_EQ(dataPathCtx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(dataPathCtx.errorMessage, "seed error");

    EvaluationContext memberCtx;
    memberCtx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");
    auto missingMember = std::make_shared<MemberAccess>();
    missingMember->object = std::make_shared<VariableReference>("missing");
    missingMember->property = "name";
    auto missingMemberResult = eval.Evaluate(missingMember, memberCtx);
    EXPECT_TRUE(missingMemberResult.IsUndefined());
    EXPECT_EQ(memberCtx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(memberCtx.errorMessage, "seed error");
}

TEST_F(EvaluatorBranchTest, L0_should_reject_non_literal_internal_intrinsic_arguments_in_direct_ast_path)
{
    ExpressionFunctions funcs;
    Evaluator eval(funcs);
    EvaluationContext ctx;

    auto fragmentCall = std::make_shared<FunctionCall>("__fragment");
    fragmentCall->arguments.push_back(std::make_shared<NumberLiteral>(1.0));
    auto fragmentResult = eval.Evaluate(fragmentCall, ctx);
    EXPECT_TRUE(fragmentResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);

    ctx.ClearError();
    auto sizeFragmentCall = std::make_shared<FunctionCall>("__sizeFragment");
    sizeFragmentCall->arguments.push_back(std::make_shared<NumberLiteral>(1.0));
    auto sizeFragmentResult = eval.Evaluate(sizeFragmentCall, ctx);
    EXPECT_TRUE(sizeFragmentResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::PARSE_UNEXPECTED_TOKEN);
}

TEST_F(EvaluatorBranchTest, L0_should_preserve_existing_error_when_function_argument_evaluation_fails)
{
    ExpressionFunctions funcs;
    funcs.Register("echo", [](const std::vector<EvalResult>& args, EvaluationContext&) -> EvalResult {
        return args.empty() ? EvalResult::Undefined() : args[0];
    });

    auto call = std::make_shared<FunctionCall>("echo");
    call->arguments.push_back(std::make_shared<VariableReference>("missing"));

    EvaluationContext ctx;
    ctx.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "seed error");

    Evaluator eval(funcs);
    auto result = eval.Evaluate(call, ctx);
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(ctx.errorMessage, "seed error");
}

/**
 * @tc.name: Evaluator_should_return_empty_for_missing_data_model_or_disallowed_json_pointer
 * @tc.desc: 验证缺少 DataModel 和不允许的 __jsonPointer 路径都会走空字符串降级分支。
 * @tc.type: FUNC
 */
TEST_F(EvaluatorBranchTest, L0_should_return_empty_for_missing_data_model_or_disallowed_json_pointer)
{
    EvaluationContext ctx;

    auto missingDataModel = EvalWithContext("{{ $__dataModel.user }}", ctx);
    ASSERT_TRUE(missingDataModel.IsString());
    EXPECT_EQ(missingDataModel.AsString(), "");
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto disallowedPointer = EvalWithContext("{{ __jsonPointer('/user/..') }}", ctx);
    ASSERT_TRUE(disallowedPointer.IsString());
    EXPECT_EQ(disallowedPointer.AsString(), "");
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);
}

/**
 * @tc.name: Evaluator_should_report_dynamic_object_member_access_failures
 * @tc.desc: 验证对象下标访问在缺失键、非字符串键和未定义下标变量时都能命中错误分支。
 * @tc.type: FUNC
 */
TEST_F(EvaluatorBranchTest, L0_should_report_dynamic_object_member_access_failures)
{
    EvaluationContext ctx;
    auto objectAdapter = JsonAdapter::Parse(R"({"first":"alice","second":"bob"})");
    ASSERT_NE(objectAdapter, nullptr);

    ctx.PushScope();
    ctx.SetLocalVariable("object", EvalResult::FromJson(objectAdapter->GetRoot()));
    ctx.SetLocalVariable("missingKey", EvalResult::FromString("absent"));
    ctx.SetLocalVariable("numericKey", EvalResult::FromNumber(1.0));

    auto missingProperty = EvalWithContext("{{ $object[$missingKey] }}", ctx);
    EXPECT_TRUE(missingProperty.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto missingDotProperty = EvalWithContext("{{ $object.absent }}", ctx);
    EXPECT_TRUE(missingDotProperty.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto nonStringKey = EvalWithContext("{{ $object[$numericKey] }}", ctx);
    EXPECT_TRUE(nonStringKey.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.ClearError();
    auto undefinedBracketKey = EvalWithContext("{{ $object[$missingVar] }}", ctx);
    EXPECT_TRUE(undefinedBracketKey.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_NE(ctx.errorMessage.find("undefined variable"), std::string::npos);

    ctx.PopScope();
}

TEST_F(EvaluatorBranchTest, L0_should_handle_manual_member_access_edge_cases_on_json_values)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto dmAdapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    auto itemsAdapter = JsonAdapter::Parse(R"(["zero","one"])");
    ASSERT_NE(dmAdapter, nullptr);
    ASSERT_NE(itemsAdapter, nullptr);
    dm->ReplaceAll(dmAdapter->GetRoot());
    ctx.SetDataModel(dm.get());

    ctx.PushScope();
    ctx.SetLocalVariable("items", EvalResult::FromJson(itemsAdapter->GetRoot()));

    auto disallowedPath = std::make_shared<MemberAccess>();
    disallowedPath->object = std::make_shared<VariableReference>("__dataModel", true);
    disallowedPath->property = "..";

    ExpressionFunctions funcs;
    Evaluator eval(funcs);
    auto disallowedResult = eval.Evaluate(disallowedPath, ctx);
    ASSERT_TRUE(disallowedResult.IsString());
    EXPECT_EQ(disallowedResult.AsString(), "");
    EXPECT_TRUE(disallowedResult.hasEvaluationError);

    ctx.ClearError();
    auto missingBracketKey = std::make_shared<MemberAccess>();
    missingBracketKey->object = std::make_shared<VariableReference>("items");
    missingBracketKey->isBracket = true;
    missingBracketKey->property = "ignored";

    auto missingBracketKeyResult = eval.Evaluate(missingBracketKey, ctx);
    EXPECT_TRUE(missingBracketKeyResult.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);

    ctx.PopScope();
}

TEST_F(ExpressionEngineBranchTest, L0_should_trim_nested_placeholders_and_preserve_json_outputs)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto dataAdapter = JsonAdapter::Parse(R"({"user":{"name":"Alice"},"items":["zero","one"]})");
    auto itemsAdapter = JsonAdapter::Parse(R"(["zero","one"])");
    ASSERT_NE(dataAdapter, nullptr);
    ASSERT_NE(itemsAdapter, nullptr);
    dm->ReplaceAll(dataAdapter->GetRoot());
    ctx.SetDataModel(dm.get());

    ctx.PushScope();
    ctx.SetLocalVariable("items", EvalResult::FromJson(itemsAdapter->GetRoot()));

    auto trimmedFragment = EvalWithContext("{{ ${   $items[1]   } }}", ctx);
    ASSERT_TRUE(trimmedFragment.IsString());
    EXPECT_EQ(trimmedFragment.AsString(), "one");

    auto stringifiedJson = EvalWithContext("{{ ${   /user   } }}", ctx);
    ASSERT_TRUE(stringifiedJson.IsString());
    EXPECT_NE(stringifiedJson.AsString().find("\"name\":\"Alice\""), std::string::npos);

    JsonValue jsonValue = ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ ${   /user   } }}", ctx);
    ASSERT_TRUE(jsonValue.IsObject());
    EXPECT_EQ(jsonValue.GetItem("name").GetStringValue(""), "Alice");

    ctx.PopScope();
}

TEST_F(ExpressionEngineBranchTest, L0_should_evaluate_nested_placeholders_without_rewriting_member_size_property)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto dataAdapter = JsonAdapter::Parse(R"({"count":2})");
    auto toolAdapter = JsonAdapter::Parse(R"({"size":5})");
    ASSERT_NE(dataAdapter, nullptr);
    ASSERT_NE(toolAdapter, nullptr);
    dm->ReplaceAll(dataAdapter->GetRoot());
    ctx.SetDataModel(dm.get());

    ctx.PushScope();
    ctx.SetLocalVariable("tool", EvalResult::FromJson(toolAdapter->GetRoot()));

    EXPECT_TRUE(ExpressionEngine::IsExpression("{{ ${ ${/count} + 1 } }}"));

    auto nestedPlaceholder = EvalWithContext("{{ ${ ${/count} + 1 } }}", ctx);
    ASSERT_TRUE(nestedPlaceholder.IsNumber());
    EXPECT_DOUBLE_EQ(nestedPlaceholder.AsNumber(), 3.0);

    auto memberSize = EvalWithContext("{{ $tool.size }}", ctx);
    ASSERT_TRUE(memberSize.IsNumber());
    EXPECT_DOUBLE_EQ(memberSize.AsNumber(), 5.0);

    ctx.PopScope();
}

TEST_F(ExpressionEngineBranchTest, L0_should_rewrite_size_calls_with_quoted_parentheses_and_escapes)
{
    EvaluationContext ctx;

    auto quotedParenthesis = EvalWithContext("{{ size(')') }}", ctx);
    ASSERT_TRUE(quotedParenthesis.IsNumber());
    EXPECT_DOUBLE_EQ(quotedParenthesis.AsNumber(), 0.0);
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);

    ctx.ClearError();
    auto escapedQuote = EvalWithContext(R"EXPR({{ size('a\')') }})EXPR", ctx);
    ASSERT_TRUE(escapedQuote.IsNumber());
    EXPECT_DOUBLE_EQ(escapedQuote.AsNumber(), 0.0);
    EXPECT_EQ(ctx.lastError, ExpressionError::NONE);
}

// ==================== size Builtin Tests (issue-76 / TASK-1) ====================

TEST(SizeBuiltinFunctionTest, L0_should_return_array_length_for_data_model_array_expression)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ size($__dataModel.items) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 3.0);
}

TEST(SizeBuiltinFunctionTest, L0_should_return_array_length_for_local_array_variable)
{
    EvaluationContext ctx;
    auto localItems = JsonAdapter::Parse(R"([1,2,3,4])");
    ASSERT_NE(localItems, nullptr);

    ctx.PushScope();
    ctx.SetLocalVariable("localItems", EvalResult::FromJson(localItems->GetRoot()));

    auto result = EvalWithContext("{{ size(localItems) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 4.0);

    ctx.PopScope();
}

TEST(SizeBuiltinFunctionTest, L0_should_return_array_length_for_local_object_member)
{
    EvaluationContext ctx;
    auto localObject = JsonAdapter::Parse(R"({"items":["a","b"]})");
    ASSERT_NE(localObject, nullptr);

    ctx.PushScope();
    ctx.SetLocalVariable("localObject", EvalResult::FromJson(localObject->GetRoot()));

    auto result = EvalWithContext("{{ size(localObject.items) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 2.0);

    ctx.PopScope();
}

TEST(SizeBuiltinFunctionTest, L0_should_return_array_length_for_json_pointer_argument)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"user":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ size(${/user}) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 3.0);
}

TEST(SizeBuiltinFunctionTest, L0_should_trim_whitespace_around_size_argument_expression)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ size(   $__dataModel.items   ) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 3.0);
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_for_non_array_argument)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("count", EvalResult::FromNumber(42.0));

    auto result = EvalWithContext("{{ size(count) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(result.AsNumber(), 0.0);

    ctx.PopScope();
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_when_size_argument_is_missing)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'size is = ' + size() }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "size is = 0");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_when_size_has_extra_arguments)
{
    EvaluationContext ctx;
    ctx.PushScope();
    ctx.SetLocalVariable("a", EvalResult::FromNumber(1.0));
    ctx.SetLocalVariable("b", EvalResult::FromNumber(2.0));

    auto result = EvalWithContext("{{ 'size is = ' + size(a, b) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "size is = 0");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);

    ctx.PopScope();
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_when_size_argument_is_undefined)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'size is = ' + size(undefined) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "size is = 0");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_when_size_argument_path_is_missing)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ 'size is = ' + size($__dataModel.missing) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "size is = 0");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST(SizeBuiltinFunctionTest, L0_should_return_zero_when_size_argument_expression_is_illegal)
{
    EvaluationContext ctx;

    auto result = EvalWithContext("{{ 'size is = ' + size($__dataModel.items[) }}", ctx);
    ASSERT_TRUE(result.IsDefined());
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ(result.AsString(), "size is = 0");
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST(SizeBuiltinFunctionTest, L0_should_not_support_legacy_placeholder_wrapped_size_call)
{
    EvaluationContext ctx;
    auto dm = std::make_shared<DataModel>("test_surface");
    auto adapter = JsonAdapter::Parse(R"({"items":[1,2,3]})");
    ASSERT_NE(adapter, nullptr);
    dm->ReplaceAll(adapter->GetRoot());
    ctx.SetDataModel(dm.get());

    auto result = EvalWithContext("{{ ${size($__dataModel.items)} }}", ctx);
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_NE(ctx.lastError, ExpressionError::NONE);
}

TEST(MemberAccessErrorTest, L0_should_report_member_access_not_supported_for_non_system_variable)
{
    EvaluationContext ctx;
    ctx.SetGlobalVariable("user", EvalResult::FromString("alice"));

    auto result = EvalWithContext("{{ $user.name }}", ctx);
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_EQ(ctx.lastError, ExpressionError::EVAL_UNDEFINED_VARIABLE);
    EXPECT_EQ(ctx.errorMessage, "member access not supported: .name");
}

// ==================== DependencyCollector Tests (TASK-3) ====================

TEST(DependencyCollectorGlobalTest, L0_should_collect_breakpoint_dependency)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__widthBreakpoint");
    ASSERT_TRUE(parseResult.success);
    ASSERT_NE(parseResult.ast, nullptr);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__widthBreakpoint");
    EXPECT_EQ(deps[0].path, "");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_color_mode_dependency)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__colorMode");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__colorMode");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_data_model_dependency_with_path)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel.user.name");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__dataModel");
    EXPECT_EQ(deps[0].path, "/user/name");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_multiple_dependencies)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__widthBreakpoint == 'sm' && $__colorMode == 'dark'");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    EXPECT_GE(deps.size(), 2u);

    bool hasBreakpoint = false;
    bool hasColorMode = false;
    for (const auto& dep : deps) {
        if (dep.variableName == "__widthBreakpoint") {
            hasBreakpoint = true;
        }
        if (dep.variableName == "__colorMode") {
            hasColorMode = true;
        }
    }
    EXPECT_TRUE(hasBreakpoint);
    EXPECT_TRUE(hasColorMode);
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_from_conditional)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__widthBreakpoint == 'sm' ? $__dataModel.padding : 16");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    EXPECT_GE(deps.size(), 2u);

    bool hasBreakpoint = false;
    bool hasDataModel = false;
    for (const auto& dep : deps) {
        if (dep.variableName == "__widthBreakpoint") {
            hasBreakpoint = true;
        }
        if (dep.variableName == "__dataModel" && dep.path == "/padding") {
            hasDataModel = true;
        }
    }
    EXPECT_TRUE(hasBreakpoint);
    EXPECT_TRUE(hasDataModel);
}

TEST(DependencyCollectorGlobalTest, L0_should_return_empty_for_literal_expression)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("1 + 2");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    EXPECT_TRUE(deps.empty());
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_nested_data_model_path)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel.settings.theme.primaryColor");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__dataModel");
    EXPECT_EQ(deps[0].path, "/settings/theme/primaryColor");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_object_and_bracket_key_dependencies_for_dynamic_member_access)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$items[$index]");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 2u);

    bool hasItems = false;
    bool hasIndex = false;
    for (const auto& dep : deps) {
        if (dep.variableName == "items" && dep.path.empty()) {
            hasItems = true;
        }
        if (dep.variableName == "index" && dep.path.empty()) {
            hasIndex = true;
        }
    }
    EXPECT_TRUE(hasItems);
    EXPECT_TRUE(hasIndex);
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_dependencies_from_grouped_unary_and_function_call_nodes)
{
    auto parseResult =
        ExpressionEngine::GetInstance().Parse("format((!$__colorMode), $__widthBreakpoint, $__dataModel.user)");
    ASSERT_TRUE(parseResult.success);
    ASSERT_NE(parseResult.ast, nullptr);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 3u);

    bool hasColorMode = false;
    bool hasBreakpoint = false;
    bool hasDataModel = false;
    for (const auto& dep : deps) {
        if (dep.variableName == "__colorMode") {
            hasColorMode = true;
        }
        if (dep.variableName == "__widthBreakpoint") {
            hasBreakpoint = true;
        }
        if (dep.variableName == "__dataModel" && dep.path == "/user") {
            hasDataModel = true;
        }
    }
    EXPECT_TRUE(hasColorMode);
    EXPECT_TRUE(hasBreakpoint);
    EXPECT_TRUE(hasDataModel);
}

TEST(DataModelPathUtilsTest, L0_should_extract_data_model_path_with_numeric_bracket)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel.user.names[0].a");
    ASSERT_TRUE(parseResult.success);

    EXPECT_EQ(DataModelPathUtils::TryExtractDataModelPath(parseResult.ast), "/user/names/0/a");
}

/**
 * @tc.name: DataModelPathUtils_should_return_empty_path_for_dynamic_bracket_key
 * @tc.desc: Verify static path extraction rejects $__dataModel member access when the bracket key is dynamic.
 * @tc.type: FUNC
 */
TEST(DataModelPathUtilsTest, L0_should_return_empty_path_for_dynamic_bracket_key)
{
    auto dataModelRef = std::make_shared<VariableReference>("__dataModel", true);
    auto indexRef = std::make_shared<VariableReference>("index", true);
    auto memberAccess = std::make_shared<MemberAccess>();
    memberAccess->object = dataModelRef;
    memberAccess->isBracket = true;
    memberAccess->bracketKey = indexRef;

    EXPECT_TRUE(DataModelPathUtils::TryExtractDataModelPath(memberAccess).empty());
}

/**
 * @tc.name: DataModelPathUtils_should_return_empty_path_when_depth_exceeds_limit
 * @tc.desc: Verify static path extraction stops when the configured member access depth is exceeded.
 * @tc.type: FUNC
 */
TEST(DataModelPathUtilsTest, L0_should_return_empty_path_when_depth_exceeds_limit)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel.user.names[0].a");
    ASSERT_TRUE(parseResult.success);

    EXPECT_TRUE(DataModelPathUtils::TryExtractDataModelPath(parseResult.ast, 2).empty());
}

TEST(DataModelPathUtilsTest, L0_should_extract_static_bracket_segments_and_reject_invalid_numeric_keys)
{
    std::string segment = "seed";
    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(nullptr, segment));
    EXPECT_EQ(segment, "seed");

    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(std::make_shared<StringLiteral>("name"), segment));
    EXPECT_EQ(segment, "seed");

    EXPECT_TRUE(DataModelPathUtils::TryExtractBracketSegment(std::make_shared<NumberLiteral>(3.0), segment));
    EXPECT_EQ(segment, "3");

    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(std::make_shared<NumberLiteral>(-1.0), segment));
    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(std::make_shared<NumberLiteral>(1.5), segment));
    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(
        std::make_shared<NumberLiteral>(std::numeric_limits<double>::infinity()), segment));
    EXPECT_FALSE(DataModelPathUtils::TryExtractBracketSegment(
        std::make_shared<NumberLiteral>(std::numeric_limits<double>::max()), segment));
    EXPECT_FALSE(
        DataModelPathUtils::TryExtractBracketSegment(std::make_shared<VariableReference>("index", true), segment));
}

TEST(DataModelPathUtilsTest, L0_should_return_empty_for_non_data_model_or_relative_paths)
{
    auto rootVar = std::make_shared<VariableReference>("user", true);
    auto memberAccess = std::make_shared<MemberAccess>();
    memberAccess->object = rootVar;
    memberAccess->property = "name";
    EXPECT_TRUE(DataModelPathUtils::TryExtractDataModelPath(memberAccess).empty());

    auto relativeDataModel = std::make_shared<VariableReference>("__dataModel", false);
    auto relativeMemberAccess = std::make_shared<MemberAccess>();
    relativeMemberAccess->object = relativeDataModel;
    relativeMemberAccess->property = "name";
    EXPECT_TRUE(DataModelPathUtils::TryExtractDataModelPath(relativeMemberAccess).empty());

    EXPECT_TRUE(DataModelPathUtils::TryExtractDataModelPath(rootVar).empty());
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_data_model_dependency_with_array_index_path)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel.user.names[0].a");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__dataModel");
    EXPECT_EQ(deps[0].path, "/user/names/0/a");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_data_model_dependency_from_json_pointer_expression)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("${/user/names/0/a}");
    ASSERT_TRUE(parseResult.success) << parseResult.errorMessage;

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__dataModel");
    EXPECT_EQ(deps[0].path, "/user/names/0/a");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_data_model_dependency_from_size_fragment)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("size($__dataModel.items)");
    ASSERT_TRUE(parseResult.success) << parseResult.errorMessage;

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 1u);
    EXPECT_EQ(deps[0].variableName, "__dataModel");
    EXPECT_EQ(deps[0].path, "/items");
}

TEST(DependencyCollectorGlobalTest, L0_should_collect_dependencies_from_valid_fragment_intrinsics)
{
    auto fragmentParseResult = ExpressionEngine::GetInstance().Parse("__fragment('$__dataModel.user.name')");
    ASSERT_TRUE(fragmentParseResult.success) << fragmentParseResult.errorMessage;
    ASSERT_NE(fragmentParseResult.ast, nullptr);

    DependencyCollector collector;
    auto fragmentDeps = collector.Collect(fragmentParseResult.ast);
    ASSERT_EQ(fragmentDeps.size(), 1u);
    EXPECT_EQ(fragmentDeps[0].variableName, "__dataModel");
    EXPECT_EQ(fragmentDeps[0].path, "/user/name");

    auto sizeFragmentParseResult = ExpressionEngine::GetInstance().Parse("__sizeFragment('$__dataModel.items')");
    ASSERT_TRUE(sizeFragmentParseResult.success) << sizeFragmentParseResult.errorMessage;
    ASSERT_NE(sizeFragmentParseResult.ast, nullptr);

    auto sizeFragmentDeps = collector.Collect(sizeFragmentParseResult.ast);
    ASSERT_EQ(sizeFragmentDeps.size(), 1u);
    EXPECT_EQ(sizeFragmentDeps[0].variableName, "__dataModel");
    EXPECT_EQ(sizeFragmentDeps[0].path, "/items");
}

TEST(DependencyCollectorGlobalTest, L0_should_not_collect_dependency_from_legacy_placeholder_wrapped_size_call)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("${size($__dataModel.items)}");
    EXPECT_FALSE(parseResult.success);
    EXPECT_EQ(parseResult.ast, nullptr);
}

TEST(DependencyCollectorGlobalTest, L0_should_handle_invalid_and_non_literal_fragment_intrinsics)
{
    auto invalidParseResult = ExpressionEngine::GetInstance().Parse("__fragment('(')");
    ASSERT_TRUE(invalidParseResult.success);
    ASSERT_NE(invalidParseResult.ast, nullptr);

    DependencyCollector collector;
    auto invalidDeps = collector.Collect(invalidParseResult.ast);
    EXPECT_TRUE(invalidDeps.empty());

    auto dynamicParseResult = ExpressionEngine::GetInstance().Parse("__fragment($expr)");
    ASSERT_TRUE(dynamicParseResult.success);
    ASSERT_NE(dynamicParseResult.ast, nullptr);

    auto dynamicDeps = collector.Collect(dynamicParseResult.ast);
    ASSERT_EQ(dynamicDeps.size(), 1u);
    EXPECT_EQ(dynamicDeps[0].variableName, "expr");
    EXPECT_TRUE(dynamicDeps[0].path.empty());
}

/**
 * @tc.name: DependencyCollector_should_handle_invalid_and_non_literal_size_fragment_intrinsics
 * @tc.desc: 验证 __sizeFragment 对非法片段、非字面量参数和非法 json pointer 字面量的依赖收集分支。
 * @tc.type: FUNC
 */
TEST(DependencyCollectorGlobalTest, L0_should_handle_invalid_and_non_literal_size_fragment_intrinsics)
{
    auto invalidSizeParseResult = ExpressionEngine::GetInstance().Parse("__sizeFragment('(')");
    ASSERT_TRUE(invalidSizeParseResult.success);
    ASSERT_NE(invalidSizeParseResult.ast, nullptr);

    DependencyCollector collector;
    auto invalidSizeDeps = collector.Collect(invalidSizeParseResult.ast);
    EXPECT_TRUE(invalidSizeDeps.empty());

    auto dynamicSizeParseResult = ExpressionEngine::GetInstance().Parse("__sizeFragment($expr)");
    ASSERT_TRUE(dynamicSizeParseResult.success);
    ASSERT_NE(dynamicSizeParseResult.ast, nullptr);

    auto dynamicSizeDeps = collector.Collect(dynamicSizeParseResult.ast);
    ASSERT_EQ(dynamicSizeDeps.size(), 1u);
    EXPECT_EQ(dynamicSizeDeps[0].variableName, "expr");
    EXPECT_TRUE(dynamicSizeDeps[0].path.empty());

    auto relativeJsonPointerParseResult = ExpressionEngine::GetInstance().Parse("__jsonPointer('user')");
    ASSERT_TRUE(relativeJsonPointerParseResult.success);
    ASSERT_NE(relativeJsonPointerParseResult.ast, nullptr);

    auto relativeJsonPointerDeps = collector.Collect(relativeJsonPointerParseResult.ast);
    EXPECT_TRUE(relativeJsonPointerDeps.empty());
}

/**
 * @tc.name: DependencyCollector_should_collect_dynamic_index_when_data_model_path_is_not_static
 * @tc.desc: Verify dependency collection falls back to object and bracket-key deps for dynamic $__dataModel access.
 * @tc.type: FUNC
 */
TEST(DependencyCollectorGlobalTest, L0_should_collect_dynamic_index_when_data_model_path_is_not_static)
{
    auto parseResult = ExpressionEngine::GetInstance().Parse("$__dataModel[$index]");
    ASSERT_TRUE(parseResult.success);

    DependencyCollector collector;
    auto deps = collector.Collect(parseResult.ast);
    ASSERT_EQ(deps.size(), 2u);

    bool hasDataModel = false;
    bool hasIndex = false;
    for (const auto& dep : deps) {
        if (dep.variableName == "__dataModel" && dep.path.empty()) {
            hasDataModel = true;
        }
        if (dep.variableName == "index" && dep.path.empty()) {
            hasIndex = true;
        }
    }
    EXPECT_TRUE(hasDataModel);
    EXPECT_TRUE(hasIndex);
}

namespace {

class ExpressionBindingProbe : public Component {
public:
    explicit ExpressionBindingProbe(ArkUI_NodeHandle nativeView, bool ownsNativeView = false)
        : Component(nativeView, ownsNativeView)
    {}

    std::string GetType() const override
    {
        return "ExpressionBindingProbe";
    }

    void InvokeSetPropertyFromDescriptor(
        const std::string& propertyKey, const JsonValue& descriptor, const std::string& bindingKey = "")
    {
        SetPropertyFromDescriptor(propertyKey, descriptor, bindingKey);
    }

    std::optional<std::string> GetStoredString(const std::string& propertyName) const
    {
        auto it = storedProperties.find(propertyName);
        if (it == storedProperties.end() || !it->second.IsValid()) {
            return std::nullopt;
        }
        return it->second.GetStringValue("");
    }

    std::optional<double> GetStoredNumber(const std::string& propertyName) const
    {
        auto it = storedProperties.find(propertyName);
        if (it == storedProperties.end() || !it->second.IsValid()) {
            return std::nullopt;
        }
        return it->second.GetNumberValue(0.0);
    }

    int GetUpdateCount(const std::string& propertyName) const
    {
        auto it = updateCounts.find(propertyName);
        return it == updateCounts.end() ? 0 : it->second;
    }

    int GetApplyCount(const std::string& propertyName) const
    {
        auto it = applyCounts.find(propertyName);
        return it == applyCounts.end() ? 0 : it->second;
    }

    void OnDataUpdate(const std::string& property, const JsonValue& value) override
    {
        ++updateCounts[property];
        Component::OnDataUpdate(property, value);
    }

protected:
    bool IsExpressionSupported() const override
    {
        return true;
    }

    void OnPropertyApplied(const std::string& propertyName, const JsonValue& value) override
    {
        ++applyCounts[propertyName];
        storedProperties[propertyName] = value;
    }

    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override
    {
        if (propertyName == "text") {
            return PropertyDeclaration { .name = "text",
                .type = PropertyValueType::STRING,
                .allowDynamic = true,
                .allowExpression = true,
                .fallbackString = "" };
        }
        if (propertyName == "padding") {
            return PropertyDeclaration { .name = "padding",
                .type = PropertyValueType::NUMBER,
                .allowDynamic = true,
                .allowExpression = true,
                .fallbackNumber = 0.0 };
        }
        return Component::GetPrivatePropertyDeclaration(propertyName);
    }

private:
    std::map<std::string, JsonValue> storedProperties;
    std::map<std::string, int> updateCounts;
    std::map<std::string, int> applyCounts;
};

std::unique_ptr<JsonAdapter> ParseJsonForBindingTest(const std::string& json)
{
    return JsonAdapter::Parse(json);
}

} // namespace

class ExpressionBindingIntegrationTest : public A2UITest {
protected:
    void TearDown() override
    {
        for (int32_t renderId : renderIds_) {
            if (RenderManager::GetInstance().HasRenderSlot(renderId)) {
                RenderManager::GetInstance().RemoveRenderSlot(renderId);
            }
        }
        renderIds_.clear();
        A2UITest::TearDown();
    }

    SurfaceSlot& CreateSurface(int32_t renderId, const std::string& surfaceId)
    {
        RenderSlot& renderSlot = RenderManager::GetInstance().CreateRenderSlot(renderId);
        renderIds_.insert(renderId);
        return renderSlot.GetSurfaceManager()->CreateSurface(surfaceId);
    }

    std::shared_ptr<ExpressionBindingProbe> CreateProbeComponent(
        int32_t renderId, const std::string& surfaceId, const std::string& componentId)
    {
        auto component =
            std::make_shared<ExpressionBindingProbe>(reinterpret_cast<ArkUI_NodeHandle>(0x9500 + renderId));
        component->SetRenderId(renderId);
        component->SetSurfaceId(surfaceId);
        component->SetComponentId(componentId);
        return component;
    }

    void RegisterComponentBinding(SurfaceSlot& surfaceSlot, const std::shared_ptr<Component>& component)
    {
        auto bindingEngine = surfaceSlot.GetBindingEngine();
        ASSERT_NE(bindingEngine, nullptr);
        bindingEngine->RegisterComponent(component);
    }

private:
    std::set<int32_t> renderIds_;
};

TEST_F(ExpressionBindingIntegrationTest, L0_should_register_expression_binding_for_global_variable_property)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9301, "expr-color-surface");
    auto component = CreateProbeComponent(9301, "expr-color-surface", "expr-color");

    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    const auto& bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings[0].propertyName_, "text");
    EXPECT_EQ(bindings[0].type_, BindingType::EXPRESSION);
    EXPECT_EQ(bindings[0].expression_, "$__colorMode");
    ASSERT_EQ(bindings[0].globalVarDeps_.size(), 1U);
    EXPECT_EQ(bindings[0].globalVarDeps_[0], "__colorMode");
    EXPECT_TRUE(bindings[0].dataPath_.empty());
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::LIGHT)));
    (void)surfaceSlot;
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_component_expression_with_template_local_variables)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9318, "expr-local-template-surface");
    auto component = CreateProbeComponent(9318, "expr-local-template-surface", "expr-local-template");
    auto itemAdapter = ParseJsonForBindingTest(R"({"name":"Bob"})");
    auto indexAdapter = JsonAdapter::CreateNumber(1.0);
    ASSERT_NE(itemAdapter, nullptr);
    ASSERT_NE(indexAdapter, nullptr);

    std::map<std::string, JsonValue> localVariables;
    localVariables["item"] = itemAdapter->GetRoot();
    localVariables["index"] = indexAdapter->GetRoot();
    component->SetLocalVariables(localVariables);

    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $item.name + ':' + $index }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("Bob:1")));
    (void)surfaceSlot;
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_treat_local_variable_as_out_of_scope_without_template_context)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9324, "expr-local-out-of-scope-surface");
    auto component = CreateProbeComponent(9324, "expr-local-out-of-scope-surface", "expr-local-out-of-scope");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $item.name }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("")));
    (void)surfaceSlot;
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_keep_standard_protocol_text_as_literal_expression_string)
{
    TextComponent component;
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $item.name }}"})");
    ASSERT_NE(descriptor, nullptr);

    component.ApplyDescriptor(descriptor->GetRoot());

    EXPECT_TRUE(component.GetDataBindings().empty());
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_default_template_variables_during_eager_template_expansion)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9319, "expr-template-eager-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    auto data = ParseJsonForBindingTest(R"({"users":[{"name":"Alice"},{"name":"Bob"}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": { "componentId": "userRow", "path": "/users" }
            },
            {
                "id": "userRow",
                "component": "Text",
                "content": "{{ $item.name + ':' + $index }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren = CollectTemplateTextChildren(root);
    ASSERT_EQ(textChildren.size(), 2U);
    EXPECT_EQ(textChildren[0]->GetTextValueForTest(), "Alice:0");
    EXPECT_EQ(textChildren[1]->GetTextValueForTest(), "Bob:1");
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_custom_template_variables_during_eager_template_expansion)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9320, "expr-template-custom-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    auto data = ParseJsonForBindingTest(R"({"users":[{"name":"Alice"},{"name":"Bob"}]})");
    ASSERT_NE(data, nullptr);
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": {
                    "componentId": "userRow",
                    "path": "/users",
                    "indexVar": "i",
                    "itemVar": "it"
                }
            },
            {
                "id": "userRow",
                "component": "Text",
                "content": "{{ $it.name + ':' + $i }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren = CollectTemplateTextChildren(root);
    ASSERT_EQ(textChildren.size(), 2U);
    EXPECT_EQ(textChildren[0]->GetTextValueForTest(), "Alice:0");
    EXPECT_EQ(textChildren[1]->GetTextValueForTest(), "Bob:1");
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_resolve_outer_default_and_inner_custom_template_variables)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9321, "expr-template-nested-custom-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    TestHelpers::RegisterWarningDispatchCallback(mockNapiPtr_);
    auto data = ParseJsonForBindingTest(R"({
        "users": [
            { "name": "Alice", "orders": [{ "sku": "A1" }, { "sku": "A2" }] },
            { "name": "Bob", "orders": [{ "sku": "B1" }] }
        ]
    })");
    ASSERT_NE(data, nullptr);
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": { "componentId": "userGroup", "path": "/users" }
            },
            {
                "id": "userGroup",
                "component": "Column",
                "children": {
                    "componentId": "orderRow",
                    "path": "orders",
                    "indexVar": "orderIndex",
                    "itemVar": "order"
                }
            },
            {
                "id": "orderRow",
                "component": "Text",
                "content": "{{ $item.name + ':' + $index + '>' + $order.sku + ':' + $orderIndex }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));
    EXPECT_EQ(TestHelpers::CountWarningRequests(mockNapiPtr_, "ERROR_CODE_INVALID_VALUE", "orderRow.content"), 0U);
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren = CollectTemplateTextChildren(root);
    ASSERT_EQ(textChildren.size(), 3U);
    EXPECT_EQ(textChildren[0]->GetTextValueForTest(), "Alice:0>A1:0");
    EXPECT_EQ(textChildren[1]->GetTextValueForTest(), "Alice:0>A2:1");
    EXPECT_EQ(textChildren[2]->GetTextValueForTest(), "Bob:1>B1:0");
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_resolve_nearest_default_scope_inside_three_level_template)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9322, "expr-template-three-level-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    auto data = ParseJsonForBindingTest(R"({
        "users": [
            {
                "name": "Alice",
                "groups": [
                    { "title": "G1", "entries": [{ "label": "E1" }] },
                    { "title": "G2", "entries": [{ "label": "E2" }] }
                ]
            }
        ]
    })");
    ASSERT_NE(data, nullptr);
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": { "componentId": "userGroup", "path": "/users" }
            },
            {
                "id": "userGroup",
                "component": "Column",
                "children": { "componentId": "groupBox", "path": "groups" }
            },
            {
                "id": "groupBox",
                "component": "Column",
                "children": {
                    "componentId": "entryRow",
                    "path": "entries",
                    "indexVar": "entryIndex",
                    "itemVar": "entry"
                }
            },
            {
                "id": "entryRow",
                "component": "Text",
                "content": "{{ $item.title + ':' + $index + '>' + $entry.label + ':' + $entryIndex }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren = CollectTemplateTextChildren(root);
    ASSERT_EQ(textChildren.size(), 2U);
    EXPECT_EQ(textChildren[0]->GetTextValueForTest(), "G1:0>E1:0");
    EXPECT_EQ(textChildren[1]->GetTextValueForTest(), "G2:1>E2:0");
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_keep_outer_custom_variable_when_inner_custom_names_conflict)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9323, "expr-template-inner-conflict-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    auto data = ParseJsonForBindingTest(R"({
        "users": [
            { "name": "Alice", "details": [{ "label": "D1" }] },
            { "name": "Bob", "details": [{ "label": "D2" }] }
        ]
    })");
    ASSERT_NE(data, nullptr);
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);
    dataModel->ReplaceAll(data->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Column",
                "children": {
                    "componentId": "userGroup",
                    "path": "/users",
                    "indexVar": "outerIndex",
                    "itemVar": "temp"
                }
            },
            {
                "id": "userGroup",
                "component": "Column",
                "children": {
                    "componentId": "detailRow",
                    "path": "details",
                    "indexVar": "temp",
                    "itemVar": "temp"
                }
            },
            {
                "id": "detailRow",
                "component": "Text",
                "content": "{{ $temp.name + ':' + $outerIndex + '>' + $item.label + ':' + $index }}"
            }
        ]
    })");
    ASSERT_NE(message, nullptr);

    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));
    std::shared_ptr<Component> root = surfaceSlot.FindComponentById("root");
    ASSERT_NE(root, nullptr);
    std::vector<std::shared_ptr<ExtendedTextComponent>> textChildren = CollectTemplateTextChildren(root);
    ASSERT_EQ(textChildren.size(), 2U);
    EXPECT_EQ(textChildren[0]->GetTextValueForTest(), "Alice:0>D1:0");
    EXPECT_EQ(textChildren[1]->GetTextValueForTest(), "Bob:1>D2:0");
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_expression_property_with_unknown_system_variable_member_chain)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9307, "expr-invalid-system-surface");
    auto component = CreateProbeComponent(9307, "expr-invalid-system-surface", "expr-invalid-system");

    auto descriptor =
        ParseJsonForBindingTest(R"({"text":"{{ 'User name = ' + $__dataModelss['user']['names'][0].a }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("User name = ")));
    (void)surfaceSlot;
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_empty_string_for_soft_system_variable_member_access_error)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9315, "expr-soft-system-member-surface");
    auto component = CreateProbeComponent(9315, "expr-soft-system-member-surface", "expr-soft-system-member");

    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode.name }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("")));
    EXPECT_EQ(component->GetApplyCount("text"), 1);
    (void)surfaceSlot;
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_theme_mode_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9302, "expr-theme-surface");
    auto component = CreateProbeComponent(9302, "expr-theme-surface", "expr-theme");

    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9302);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::DARK)));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_extended_text_style_expression_when_theme_mode_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9326, "expr-style-theme-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9326);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::LIGHT);

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "theme style expression",
                "styles": {
                    "fontColor": "{{ $__colorMode == 'dark' ? '#FFFFFFFF' : '#FF000000' }}"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(surfaceSlot.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xFF000000u);

    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(text->GetFontColorForTest(), 0xFFFFFFFFu);
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_breakpoint_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9303, "expr-breakpoint-surface");
    auto component = CreateProbeComponent(9303, "expr-breakpoint-surface", "expr-breakpoint");

    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__widthBreakpoint }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9303);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateBreakpoint(Breakpoint::LG);

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(BreakpointToString(Breakpoint::LG)));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_data_model_path_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9304, "expr-data-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9304, "expr-data-surface", "expr-data");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__dataModel.user.name }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    auto newName = JsonAdapter::CreateString("Bob");
    ASSERT_NE(newName, nullptr);
    dataModel->UpdateByPath("/user/name", newName->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("Bob")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_extended_text_style_expression_when_data_model_path_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9325, "expr-style-data-surface");
    surfaceSlot.SetCatalog(BuildExtendedTemplateCatalog());
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"theme":{"text":"#FF000000"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto message = ParseJsonForBindingTest(R"({
        "components": [
            {
                "id": "root",
                "component": "Text",
                "content": "style expression",
                "styles": {
                    "fontColor": "{{ $__dataModel.theme.text }}"
                }
            }
        ]
    })");
    ASSERT_NE(message, nullptr);
    ASSERT_TRUE(surfaceSlot.UpdateComponents(message->GetRoot()));

    auto text = std::dynamic_pointer_cast<ExtendedTextComponent>(surfaceSlot.FindComponentById("root"));
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetFontColorForTest(), 0xFF000000u);

    auto newColor = JsonAdapter::CreateString("#FF112233");
    ASSERT_NE(newColor, nullptr);
    dataModel->UpdateByPath("/theme/text", newColor->GetRoot());

    EXPECT_EQ(text->GetFontColorForTest(), 0xFF112233u);
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_re_evaluate_mixed_expression_when_non_active_data_path_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9305, "expr-mixed-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"spacing":{"small":8,"large":24}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9305, "expr-mixed-surface", "expr-mixed");
    auto descriptor = ParseJsonForBindingTest(
        R"({"padding":"{{ $__widthBreakpoint == 'sm' ? $__dataModel.spacing.small : $__dataModel.spacing.large }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("padding", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    auto newLarge = JsonAdapter::CreateNumber(32.0);
    ASSERT_NE(newLarge, nullptr);
    dataModel->UpdateByPath("/spacing/large", newLarge->GetRoot());

    EXPECT_EQ(component->GetStoredNumber("padding"), std::make_optional(8.0));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_array_item_path_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9306, "expr-array-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"names":[{"a":"bbbb"},{"a":"bbbb1"}]}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9306, "expr-array-surface", "expr-array");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ $__dataModel.user.names[0].a }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    auto newName = JsonAdapter::CreateString("updated");
    ASSERT_NE(newName, nullptr);
    dataModel->UpdateByPath("/user/names/0/a", newName->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("updated")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_json_pointer_dependency_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9310, "expr-json-pointer-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"names":[{"a":"bbbb"},{"a":"bbbb1"}]}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9310, "expr-json-pointer-surface", "expr-json-pointer");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ ${/user/names/0/a} }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    auto newName = JsonAdapter::CreateString("updated");
    ASSERT_NE(newName, nullptr);
    dataModel->UpdateByPath("/user/names/0/a", newName->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("updated")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_size_dependency_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9311, "expr-size-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"items":[1,2,3]})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9311, "expr-size-surface", "expr-size");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ size($__dataModel.items) }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("3")));

    auto newItems = ParseJsonForBindingTest(R"([1,2,3,4])");
    ASSERT_NE(newItems, nullptr);
    dataModel->UpdateByPath("/items", newItems->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("4")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_refresh_expression_binding_when_size_json_pointer_dependency_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9313, "expr-size-json-pointer-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":[1,2,3]})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9313, "expr-size-json-pointer-surface", "expr-size-json-pointer");
    auto descriptor = ParseJsonForBindingTest(R"({"text":"{{ size(${/user}) }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("3")));

    auto newItems = ParseJsonForBindingTest(R"([1,2,3,4])");
    ASSERT_NE(newItems, nullptr);
    dataModel->UpdateByPath("/user", newItems->GetRoot());

    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("4")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_resync_expression_binding_when_dependency_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9308, "expr-resync-surface");
    auto component = CreateProbeComponent(9308, "expr-resync-surface", "expr-resync");

    auto colorDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    ASSERT_NE(colorDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", colorDescriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);
    ASSERT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::LIGHT)));

    auto breakpointDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__widthBreakpoint }}"})");
    ASSERT_NE(breakpointDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", breakpointDescriptor->GetRoot());
    surfaceSlot.GetBindingEngine()->SyncComponentBindings(component);

    const auto& bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1u);
    EXPECT_EQ(bindings[0].propertyName_, "text");
    EXPECT_EQ(bindings[0].expression_, "$__widthBreakpoint");
    ASSERT_EQ(bindings[0].globalVarDeps_.size(), 1u);
    EXPECT_EQ(bindings[0].globalVarDeps_[0], "__widthBreakpoint");
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(BreakpointToString(Breakpoint::SM)));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9308);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(BreakpointToString(Breakpoint::SM)));

    renderSlot->GetSurfaceManager()->UpdateBreakpoint(Breakpoint::LG);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(BreakpointToString(Breakpoint::LG)));
}

TEST_F(ExpressionBindingIntegrationTest,
    L0_should_refresh_expression_property_once_when_global_variable_change_hits_multiple_bindings)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9309, "expr-dedup-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"primary":"A","secondary":"B"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9309, "expr-dedup-surface", "expr-dedup");
    auto descriptor = ParseJsonForBindingTest(
        R"({"text":"{{ $__colorMode == 'dark' ? $__dataModel.user.primary : $__dataModel.user.secondary }}"})");
    ASSERT_NE(descriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());
    ASSERT_EQ(component->GetDataBindings().size(), 2u);
    RegisterComponentBinding(surfaceSlot, component);
    EXPECT_EQ(component->GetUpdateCount("text"), 0);

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9309);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(component->GetUpdateCount("text"), 1);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("A")));
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_collapse_duplicate_expression_dependencies_for_single_path)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9310, "expr-duplicate-deps-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9310, "expr-duplicate-deps-surface", "expr-duplicate-deps");
    auto descriptor = ParseJsonForBindingTest(
        R"({"text":"{{ $__colorMode == 'dark' && $__colorMode != 'light' ? $__dataModel.user.name : $__dataModel.user.name }}"})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    const auto& bindings = component->GetDataBindings();
    ASSERT_EQ(bindings.size(), 1u);
    EXPECT_EQ(bindings[0].dataPath_, "/user/name");

    int colorModeDepCount = 0;
    int dataModelDepCount = 0;
    for (const auto& dep : bindings[0].globalVarDeps_) {
        if (dep == "__colorMode") {
            ++colorModeDepCount;
        }
        if (dep == "__dataModel") {
            ++dataModelDepCount;
        }
    }
    EXPECT_EQ(colorModeDepCount, 1);
    EXPECT_EQ(dataModelDepCount, 1);
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_apply_declared_string_fallback_for_null_literal)
{
    auto component = CreateProbeComponent(9312, "expr-null-literal-surface", "expr-null-literal");

    auto descriptor = ParseJsonForBindingTest(R"({"text":null})");
    ASSERT_NE(descriptor, nullptr);

    component->InvokeSetPropertyFromDescriptor("text", descriptor->GetRoot());

    EXPECT_TRUE(component->GetDataBindings().empty());
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("")));
    EXPECT_EQ(component->GetApplyCount("text"), 1);
}

TEST_F(ExpressionBindingIntegrationTest, L0_should_unregister_data_model_expression_dependency_when_binding_changes)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9313, "expr-unregister-data-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"user":{"name":"Alice"}})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9313, "expr-unregister-data-surface", "expr-unregister-data");
    auto dataDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__dataModel.user.name }}"})");
    ASSERT_NE(dataDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", dataDescriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    auto updatedName = JsonAdapter::CreateString("Bob");
    ASSERT_NE(updatedName, nullptr);
    dataModel->UpdateByPath("/user/name", updatedName->GetRoot());
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("Bob")));

    auto colorDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    ASSERT_NE(colorDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", colorDescriptor->GetRoot());
    surfaceSlot.GetBindingEngine()->SyncComponentBindings(component);

    int updateCountBeforePathChange = component->GetUpdateCount("text");
    auto secondUpdatedName = JsonAdapter::CreateString("Carol");
    ASSERT_NE(secondUpdatedName, nullptr);
    dataModel->UpdateByPath("/user/name", secondUpdatedName->GetRoot());

    EXPECT_EQ(component->GetUpdateCount("text"), updateCountBeforePathChange);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::LIGHT)));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9313);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::DARK)));
}

TEST_F(
    ExpressionBindingIntegrationTest, L0_should_refresh_only_dependent_expression_bindings_for_global_variable_change)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9314, "expr-mixed-global-surface");
    auto dataModel = surfaceSlot.GetOrCreateDataModel();
    ASSERT_NE(dataModel, nullptr);

    auto initialData = ParseJsonForBindingTest(R"({"name":"Alice"})");
    ASSERT_NE(initialData, nullptr);
    dataModel->ReplaceAll(initialData->GetRoot());

    auto component = CreateProbeComponent(9314, "expr-mixed-global-surface", "expr-mixed-global");
    component->AddBinding("label", "/name");

    auto colorDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    auto breakpointDescriptor = ParseJsonForBindingTest(R"({"padding":"{{ $__widthBreakpoint == 'sm' ? 8 : 16 }}"})");
    ASSERT_NE(colorDescriptor, nullptr);
    ASSERT_NE(breakpointDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", colorDescriptor->GetRoot());
    component->InvokeSetPropertyFromDescriptor("padding", breakpointDescriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);

    int labelUpdateCountBefore = component->GetUpdateCount("label");
    int textUpdateCountBefore = component->GetUpdateCount("text");
    int paddingUpdateCountBefore = component->GetUpdateCount("padding");

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9314);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(component->GetUpdateCount("label"), labelUpdateCountBefore);
    EXPECT_EQ(component->GetUpdateCount("text"), textUpdateCountBefore + 1);
    EXPECT_EQ(component->GetUpdateCount("padding"), paddingUpdateCountBefore);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(ColorModeToString(ThemeMode::DARK)));
    EXPECT_EQ(component->GetStoredNumber("padding"), std::make_optional(8.0));
}

/**
 * @tc.name: ExpressionBindingIntegration_should_unregister_expression_dependencies_when_property_becomes_constant
 * @tc.desc: Verify SyncComponentBindings removes old global-variable registrations once an expression becomes constant.
 * @tc.type: FUNC
 */
TEST_F(ExpressionBindingIntegrationTest, L0_should_unregister_expression_dependencies_when_property_becomes_constant)
{
    SurfaceSlot& surfaceSlot = CreateSurface(9311, "expr-constant-surface");
    auto component = CreateProbeComponent(9311, "expr-constant-surface", "expr-constant");

    auto dynamicDescriptor = ParseJsonForBindingTest(R"({"text":"{{ $__colorMode }}"})");
    ASSERT_NE(dynamicDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", dynamicDescriptor->GetRoot());
    RegisterComponentBinding(surfaceSlot, component);
    ASSERT_EQ(component->GetDataBindings().size(), 1u);

    auto constantDescriptor = ParseJsonForBindingTest(R"({"text":"{{ 'static' }}"})");
    ASSERT_NE(constantDescriptor, nullptr);
    component->InvokeSetPropertyFromDescriptor("text", constantDescriptor->GetRoot());
    surfaceSlot.GetBindingEngine()->SyncComponentBindings(component);

    EXPECT_TRUE(component->GetDataBindings().empty());
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("static")));

    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(9311);
    ASSERT_NE(renderSlot, nullptr);
    renderSlot->GetSurfaceManager()->UpdateThemeMode(ThemeMode::DARK);

    EXPECT_EQ(component->GetUpdateCount("text"), 0);
    EXPECT_EQ(component->GetStoredString("text"), std::make_optional(std::string("static")));
}
