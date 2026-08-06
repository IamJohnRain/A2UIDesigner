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

#ifndef A2UI_EXPRESSION_EVALUATOR_H
#define A2UI_EXPRESSION_EVALUATOR_H

#include <memory>

#include "Ast.h"
#include "EvaluationContext.h"
#include "ExpressionFunctions.h"

namespace NativeModule {

class Evaluator {
public:
    explicit Evaluator(ExpressionFunctions& functions);

    EvalResult Evaluate(const std::shared_ptr<AstNode>& node, EvaluationContext& context);

private:
    EvalResult EvaluateNumberLiteral(const NumberLiteral& node);
    EvalResult EvaluateStringLiteral(const StringLiteral& node);
    EvalResult EvaluateBooleanLiteral(const BooleanLiteral& node);
    EvalResult EvaluateBinary(const std::shared_ptr<BinaryExpression>& node, EvaluationContext& context);
    EvalResult EvaluateLogicalBinary(const std::shared_ptr<BinaryExpression>& node, EvaluationContext& context);
    EvalResult EvaluateResolvedBinary(
        BinaryOp op, const EvalResult& left, const EvalResult& right, EvaluationContext& context);
    EvalResult EvaluateUnary(const std::shared_ptr<UnaryExpression>& node, EvaluationContext& context);
    EvalResult EvaluateConditional(const std::shared_ptr<ConditionalExpression>& node, EvaluationContext& context);
    EvalResult EvaluateGrouped(const std::shared_ptr<GroupedExpression>& node, EvaluationContext& context);
    EvalResult EvaluateFunctionCall(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context);
    EvalResult EvaluateJsonPointerIntrinsic(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context);
    EvalResult EvaluateFragmentIntrinsic(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context);
    EvalResult EvaluateSizeFragmentIntrinsic(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context);
    std::vector<EvalResult> EvaluateFunctionArguments(
        const std::shared_ptr<FunctionCall>& node, EvaluationContext& context, bool& hasUndefinedArgument);
    EvalResult EvaluateVariableReference(const std::shared_ptr<VariableReference>& node, EvaluationContext& context);
    EvalResult EvaluateMemberAccess(const std::shared_ptr<MemberAccess>& node, EvaluationContext& context);
    EvalResult TryEvaluateDataModelMemberAccess(
        const std::shared_ptr<MemberAccess>& node, EvaluationContext& context, bool& handled);
    EvalResult EvaluateMemberObject(const std::shared_ptr<MemberAccess>& node, EvaluationContext& context);
    EvalResult EvaluateJsonMemberValue(const std::shared_ptr<MemberAccess>& node, const JsonValue& objectValue,
        EvaluationContext& context, bool& handled);
    EvalResult EvaluateUnsupportedMemberAccess(
        const std::shared_ptr<MemberAccess>& node, EvaluationContext& context) const;

    ExpressionFunctions& functions_;
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_EVALUATOR_H
