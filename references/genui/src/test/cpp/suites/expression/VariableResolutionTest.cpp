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
#include <string>

#include "expression/DependencyCollector.h"
#include "expression/EvalResult.h"
#include "expression/EvaluationContext.h"
#include "expression/Evaluator.h"
#include "expression/ExpressionEngine.h"
#include "expression/ExpressionFunctions.h"
#include "expression/Lexer.h"
#include "expression/Parser.h"
#include "expression/Sandbox.h"

using namespace NativeModule;

namespace {

EvalResult EvalWithGlobalVar(const std::string& expr, const std::string& varName, const EvalResult& varValue)
{
    EvaluationContext context;
    context.SetGlobalVariable(varName, varValue);
    return ExpressionEngine::GetInstance().Evaluate("{{ " + expr + " }}", context);
}

EvalResult EvalWithLocalVar(const std::string& expr, const std::string& varName, const EvalResult& varValue)
{
    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable(varName, varValue);
    auto result = ExpressionEngine::GetInstance().Evaluate("{{ " + expr + " }}", context);
    context.PopScope();
    return result;
}

EvalResult EvalRawWithContext(EvaluationContext& context, const std::string& expr)
{
    return ExpressionEngine::GetInstance().Evaluate("{{ " + expr + " }}", context);
}

std::vector<Dependency> CollectDeps(const std::string& expr)
{
    ExpressionEngine::GetInstance().ClearAstCache();
    ExpressionEngine::GetInstance().EnableAstCache(false);
    auto parseResult = ExpressionEngine::GetInstance().Parse(expr);
    if (!parseResult.success || parseResult.ast == nullptr) {
        return {};
    }
    DependencyCollector collector;
    return collector.Collect(parseResult.ast);
}

} // namespace

class VariableResolutionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(VariableResolutionTest, should_returnUndefined_when_resolveVariableInEmptyContext)
{
    EvaluationContext context;
    auto result = context.ResolveVariable("unknown");
    EXPECT_TRUE(result.IsUndefined());
}

TEST_F(VariableResolutionTest, should_returnGlobalValue_when_resolveGlobalVariable)
{
    EvaluationContext context;
    context.SetGlobalVariable("myVar", EvalResult::FromNumber(42.0));
    auto result = context.ResolveVariable("myVar");
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(42.0, result.AsNumber());
}

TEST_F(VariableResolutionTest, should_returnLocalValue_when_resolveLocalVariable)
{
    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("localVar", EvalResult::FromString("hello"));
    auto result = context.ResolveVariable("localVar");
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("hello", result.AsString());
    context.PopScope();
}

TEST_F(VariableResolutionTest, should_preferGlobalOverLocal_when_bothSet)
{
    EvaluationContext context;
    context.SetGlobalVariable("x", EvalResult::FromNumber(1.0));
    context.PushScope();
    context.SetLocalVariable("x", EvalResult::FromNumber(2.0));
    auto result = context.ResolveVariable("x");
    EXPECT_DOUBLE_EQ(1.0, result.AsNumber());
    context.PopScope();
}

TEST_F(VariableResolutionTest, should_fallBackToGlobal_when_localScopeIsEmpty)
{
    EvaluationContext context;
    context.SetGlobalVariable("g", EvalResult::FromBool(true));
    context.PushScope();
    auto result = context.ResolveVariable("g");
    EXPECT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.AsBool());
    context.PopScope();
}

TEST_F(VariableResolutionTest, should_preferGlobalOverNestedScopes_when_globalSet)
{
    EvaluationContext context;
    context.SetGlobalVariable("v", EvalResult::FromNumber(0.0));
    context.PushScope();
    context.SetLocalVariable("v", EvalResult::FromNumber(10.0));
    context.PushScope();
    context.SetLocalVariable("v", EvalResult::FromNumber(20.0));
    EXPECT_DOUBLE_EQ(0.0, context.ResolveVariable("v").AsNumber());
    context.PopScope();
    EXPECT_DOUBLE_EQ(0.0, context.ResolveVariable("v").AsNumber());
    context.PopScope();
    EXPECT_DOUBLE_EQ(0.0, context.ResolveVariable("v").AsNumber());
}

TEST_F(VariableResolutionTest, should_findInNearestScope_when_noGlobalSet)
{
    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("v", EvalResult::FromNumber(10.0));
    context.PushScope();
    context.SetLocalVariable("v", EvalResult::FromNumber(20.0));
    EXPECT_DOUBLE_EQ(20.0, context.ResolveVariable("v").AsNumber());
    context.PopScope();
    EXPECT_DOUBLE_EQ(10.0, context.ResolveVariable("v").AsNumber());
    context.PopScope();
    EXPECT_TRUE(context.ResolveVariable("v").IsUndefined());
}

TEST_F(VariableResolutionTest, should_evaluateVariableReference_inExpression)
{
    auto result = EvalWithGlobalVar("$myVar", "myVar", EvalResult::FromNumber(7.0));
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(7.0, result.AsNumber());
}

TEST_F(VariableResolutionTest, should_evaluateVariableInBinaryExpr_inExpression)
{
    auto result = EvalWithGlobalVar("$x + 3", "x", EvalResult::FromNumber(5.0));
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(8.0, result.AsNumber());
}

TEST_F(VariableResolutionTest, should_evaluateVariableComparison_inExpression)
{
    auto result = EvalWithGlobalVar("$flag == true", "flag", EvalResult::FromBool(true));
    EXPECT_TRUE(result.IsBoolean());
    EXPECT_TRUE(result.AsBool());
}

TEST_F(VariableResolutionTest, should_returnEmptyString_when_unquotedVariableNotFoundInExpression)
{
    EvaluationContext context;
    auto result = EvalRawWithContext(context, "unknownVar");
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("", result.AsString());
    EXPECT_EQ(ExpressionError::PARSE_UNEXPECTED_TOKEN, context.lastError);
    EXPECT_NE(context.errorMessage.find("illegal expression"), std::string::npos);
}

TEST_F(VariableResolutionTest, should_resolveGlobalVariable_stringType)
{
    auto result = EvalWithGlobalVar("$name", "name", EvalResult::FromString("test"));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("test", result.AsString());
}

TEST_F(VariableResolutionTest, should_resolveLocalVariable_inExpression)
{
    auto result = EvalWithLocalVar("$val", "val", EvalResult::FromNumber(99.0));
    EXPECT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(99.0, result.AsNumber());
}

TEST_F(VariableResolutionTest, should_reject_bare_identifier_when_variable_is_defined_in_scope)
{
    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("item", EvalResult::FromNumber(42.0));

    auto result = EvalRawWithContext(context, "item");
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("", result.AsString());
    EXPECT_EQ(ExpressionError::PARSE_UNEXPECTED_TOKEN, context.lastError);
    EXPECT_NE(context.errorMessage.find("illegal expression"), std::string::npos);
    EXPECT_NE(context.errorMessage.find("item"), std::string::npos);

    context.PopScope();
}

TEST_F(VariableResolutionTest, should_reject_bare_identifier_when_variable_is_defined_as_global)
{
    auto result = EvalWithGlobalVar("myVar", "myVar", EvalResult::FromNumber(7.0));
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("", result.AsString());
}

TEST_F(VariableResolutionTest, should_reject_bare_identifier_in_function_argument)
{
    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("arr", EvalResult::FromJson(JsonAdapter::Parse(R"([1,2,3])")->GetRoot()));

    auto result = EvalRawWithContext(context, "size(arr)");
    ASSERT_TRUE(result.IsNumber());
    EXPECT_DOUBLE_EQ(0.0, result.AsNumber());
    EXPECT_EQ(ExpressionError::PARSE_UNEXPECTED_TOKEN, context.lastError);
    EXPECT_NE(context.errorMessage.find("illegal expression"), std::string::npos);

    context.PopScope();
}

TEST_F(VariableResolutionTest, should_return_empty_string_when_dollar_prefixed_variable_not_found)
{
    EvaluationContext context;

    auto result = EvalRawWithContext(context, "$missingVar");
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_EQ(ExpressionError::EVAL_UNDEFINED_VARIABLE, context.lastError);
    EXPECT_NE(context.errorMessage.find("undefined variable"), std::string::npos);
    EXPECT_NE(context.errorMessage.find("missingVar"), std::string::npos);
}

TEST_F(VariableResolutionTest, should_return_empty_string_with_evaluation_error_when_bare_identifier_rejected)
{
    EvaluationContext context;

    auto result = EvalRawWithContext(context, "bareName");
    EXPECT_TRUE(result.IsString());
    EXPECT_EQ("", result.AsString());
    EXPECT_EQ(ExpressionError::PARSE_UNEXPECTED_TOKEN, context.lastError);
}

TEST_F(VariableResolutionTest, should_resolve_dollar_prefixed_variable_in_nested_member_access)
{
    auto adapter = JsonAdapter::Parse(R"({"name":"Alice"})");
    ASSERT_NE(adapter, nullptr);

    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("user", EvalResult::FromJson(adapter->GetRoot()));

    auto result = EvalRawWithContext(context, "$user.name");
    ASSERT_TRUE(result.IsString());
    EXPECT_EQ("Alice", result.AsString());

    context.PopScope();
}

TEST_F(VariableResolutionTest, should_reject_bare_identifier_in_nested_member_access)
{
    auto adapter = JsonAdapter::Parse(R"({"name":"Alice"})");
    ASSERT_NE(adapter, nullptr);

    EvaluationContext context;
    context.PushScope();
    context.SetLocalVariable("user", EvalResult::FromJson(adapter->GetRoot()));

    auto result = EvalRawWithContext(context, "user.name");
    EXPECT_TRUE(result.IsUndefined());
    EXPECT_NE(context.lastError, ExpressionError::NONE);

    context.PopScope();
}

class DependencyCollectorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ExpressionEngine::GetInstance().ClearAstCache();
        ExpressionEngine::GetInstance().EnableAstCache(false);
    }
};

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariableName_fromSingleVariable)
{
    auto deps = CollectDeps("myVar");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectNoDependencies_fromLiteral)
{
    auto deps = CollectDeps("42");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromBinaryExpression)
{
    auto deps = CollectDeps("a + b");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromUnaryExpression)
{
    auto deps = CollectDeps("!flag");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromComparison)
{
    auto deps = CollectDeps("x > 10");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromComplexExpression)
{
    auto deps = CollectDeps("a + b > c && d < e");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromConditionalExpression)
{
    auto deps = CollectDeps("cond ? a : b");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromGroupedExpression)
{
    auto deps = CollectDeps("(x + y)");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectNull_fromNullNode)
{
    DependencyCollector collector;
    auto deps = collector.Collect(nullptr);
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectFromEqualityExpression)
{
    auto deps = CollectDeps("$__widthBreakpoint == 'lg'");
    ASSERT_EQ(1u, deps.size());
    EXPECT_EQ("__widthBreakpoint", deps[0].variableName);
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromLogicalAnd)
{
    auto deps = CollectDeps("a && b");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeVariables_fromLogicalOr)
{
    auto deps = CollectDeps("a || b");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectStringLiteral_noDeps)
{
    auto deps = CollectDeps("'hello'");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectBooleanLiteral_noDeps)
{
    auto deps = CollectDeps("true");
    EXPECT_TRUE(deps.empty());
}

// ==================== B8: MEMBER_ACCESS collection path ====================

TEST_F(DependencyCollectorTest, should_notCollectRelativeMemberAccess_fromSingleLevelDottedAccess)
{
    auto deps = CollectDeps("user.name");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeMemberAccess_fromChainedDottedAccess)
{
    auto deps = CollectDeps("user.address.city");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeMemberAccess_fromComparisonWithMemberAccess)
{
    auto deps = CollectDeps("user.name == 'test'");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_collectMemberAccess_fromDataModelPath)
{
    auto deps = CollectDeps("$__dataModel.flag == true");
    ASSERT_EQ(1u, deps.size());
    EXPECT_EQ("__dataModel", deps[0].variableName);
    EXPECT_EQ("/flag", deps[0].path);
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeMemberAccess_when_binaryExprWithMemberAccess)
{
    auto deps = CollectDeps("a.name == b.value");
    EXPECT_TRUE(deps.empty());
}

TEST_F(DependencyCollectorTest, should_notCollectRelativeBracketKey_when_bracketAccessWithVariable)
{
    auto deps = CollectDeps("arr[idx]");
    EXPECT_TRUE(deps.empty());
}
