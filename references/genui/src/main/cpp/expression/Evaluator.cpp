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

#include "Evaluator.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <utility>

#include "data/DataModel.h"
#include "utils/JsonAdapter.h"
#include "utils/LogA2UI.h"

#include "DataModelPathUtils.h"
#include "Sandbox.h"
#include "expression/ExpressionEngine.h"
#include "expression/ExpressionIntrinsics.h"

namespace NativeModule {

Evaluator::Evaluator(ExpressionFunctions& functions) : functions_(functions) {}

namespace {

bool TryGetJsonPointerIntrinsicPath(const std::shared_ptr<FunctionCall>& node, std::string& path)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::JSON_POINTER || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr) {
        return false;
    }

    path = literal->value;
    return true;
}

bool TryGetFragmentIntrinsicExpression(const std::shared_ptr<FunctionCall>& node, std::string& expression)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::FRAGMENT || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr) {
        return false;
    }

    expression = literal->value;
    return true;
}

bool TryGetSizeFragmentIntrinsicExpression(const std::shared_ptr<FunctionCall>& node, std::string& expression)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::SIZE_FRAGMENT || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr) {
        return false;
    }

    expression = literal->value;
    return true;
}

std::string TrimWhitespace(const std::string& input)
{
    size_t begin = 0;
    while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    size_t end = input.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }
    return input.substr(begin, end - begin);
}

size_t FindMatchingParenthesis(const std::string& expression, size_t openIndex)
{
    if (openIndex >= expression.size() || expression[openIndex] != '(') {
        return std::string::npos;
    }

    size_t depth = 1u;
    char quote = '\0';
    bool escaping = false;
    for (size_t index = openIndex + 1u; index < expression.size(); ++index) {
        char current = expression[index];
        if (quote != '\0') {
            if (escaping) {
                escaping = false;
                continue;
            }
            if (current == '\\') {
                escaping = true;
                continue;
            }
            if (current == quote) {
                quote = '\0';
            }
            continue;
        }

        if (current == '\'' || current == '"') {
            quote = current;
            continue;
        }
        if (current == '(') {
            ++depth;
            continue;
        }
        if (current != ')') {
            continue;
        }
        --depth;
        if (depth == 0u) {
            return index;
        }
    }
    return std::string::npos;
}

EvalResult ResolveDataModelPath(const std::string& path, EvaluationContext& context);

bool TryParseStringArrayIndex(const std::string& rawValue, double& numericIndex)
{
    size_t begin = 0;
    while (begin < rawValue.size() && std::isspace(static_cast<unsigned char>(rawValue[begin])) != 0) {
        ++begin;
    }
    if (begin == rawValue.size()) {
        return false;
    }

    std::string trimmed = rawValue.substr(begin);
    char* endPtr = nullptr;
    errno = 0;
    numericIndex = std::strtod(trimmed.c_str(), &endPtr);
    if (endPtr == trimmed.c_str() || errno == ERANGE) {
        return false;
    }

    while (*endPtr != '\0' && std::isspace(static_cast<unsigned char>(*endPtr)) != 0) {
        ++endPtr;
    }
    return *endPtr == '\0';
}

bool TryConvertToArrayIndex(const EvalResult& value, int& index)
{
    if (value.IsUndefined()) {
        return false;
    }

    double numericIndex = 0.0;
    if (value.IsNumber()) {
        numericIndex = value.AsNumber();
    } else if (value.IsString()) {
        if (!TryParseStringArrayIndex(value.AsString(), numericIndex)) {
            return false;
        }
    } else {
        return false;
    }

    if (!std::isfinite(numericIndex) || numericIndex < 0.0) {
        return false;
    }

    double integerPart = 0.0;
    if (std::modf(numericIndex, &integerPart) != 0.0 ||
        integerPart > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }

    index = static_cast<int>(integerPart);
    return true;
}

} // namespace

EvalResult Evaluator::Evaluate(const std::shared_ptr<AstNode>& node, EvaluationContext& context)
{
    if (node == nullptr) {
        context.SetError(ExpressionError::PARSE_MISSING_OPERAND, "null AST node");
        return EvalResult::Undefined();
    }

    switch (node->type) {
        case AstNodeType::NUMBER_LITERAL:
            return EvaluateNumberLiteral(static_cast<const NumberLiteral&>(*node));
        case AstNodeType::STRING_LITERAL:
            return EvaluateStringLiteral(static_cast<const StringLiteral&>(*node));
        case AstNodeType::BOOLEAN_LITERAL:
            return EvaluateBooleanLiteral(static_cast<const BooleanLiteral&>(*node));
        case AstNodeType::BINARY_EXPRESSION:
            return EvaluateBinary(std::static_pointer_cast<BinaryExpression>(node), context);
        case AstNodeType::UNARY_EXPRESSION:
            return EvaluateUnary(std::static_pointer_cast<UnaryExpression>(node), context);
        case AstNodeType::CONDITIONAL_EXPRESSION:
            return EvaluateConditional(std::static_pointer_cast<ConditionalExpression>(node), context);
        case AstNodeType::GROUPED_EXPRESSION:
            return EvaluateGrouped(std::static_pointer_cast<GroupedExpression>(node), context);
        case AstNodeType::FUNCTION_CALL:
            return EvaluateFunctionCall(std::static_pointer_cast<FunctionCall>(node), context);
        case AstNodeType::VARIABLE_REFERENCE:
            return EvaluateVariableReference(std::static_pointer_cast<VariableReference>(node), context);
        case AstNodeType::MEMBER_ACCESS:
            return EvaluateMemberAccess(std::static_pointer_cast<MemberAccess>(node), context);
        case AstNodeType::TEMPLATE_LITERAL:
            // stub
            context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "template literals not yet supported");
            return EvalResult::Undefined();
        default:
            context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unsupported AST node type");
            return EvalResult::Undefined();
    }
}

EvalResult Evaluator::EvaluateNumberLiteral(const NumberLiteral& node)
{
    return EvalResult::FromNumber(node.value);
}

EvalResult Evaluator::EvaluateStringLiteral(const StringLiteral& node)
{
    return EvalResult::FromString(node.value);
}

EvalResult Evaluator::EvaluateBooleanLiteral(const BooleanLiteral& node)
{
    return EvalResult::FromBool(node.value);
}

EvalResult Evaluator::EvaluateBinary(const std::shared_ptr<BinaryExpression>& node, EvaluationContext& context)
{
    if (node->left == nullptr || node->right == nullptr) {
        context.SetError(ExpressionError::PARSE_MISSING_OPERAND, "binary expression missing operand");
        return EvalResult::Undefined();
    }

    if (node->op == BinaryOp::AND || node->op == BinaryOp::OR) {
        return EvaluateLogicalBinary(node, context);
    }

    auto left = Evaluate(node->left, context);
    if (left.IsUndefined()) {
        return left;
    }
    auto right = Evaluate(node->right, context);
    if (right.IsUndefined()) {
        return right;
    }

    return EvaluateResolvedBinary(node->op, left, right, context);
}

EvalResult Evaluator::EvaluateLogicalBinary(const std::shared_ptr<BinaryExpression>& node, EvaluationContext& context)
{
    auto left = Evaluate(node->left, context);
    if (left.IsUndefined()) {
        return left;
    }

    if (node->op == BinaryOp::AND && !left.AsBool()) {
        return left;
    }
    if (node->op == BinaryOp::OR && left.AsBool()) {
        return left;
    }

    auto right = Evaluate(node->right, context);
    if (right.IsUndefined()) {
        return right;
    }
    return right;
}

EvalResult Evaluator::EvaluateResolvedBinary(
    BinaryOp op, const EvalResult& left, const EvalResult& right, EvaluationContext& context)
{
    switch (op) {
        case BinaryOp::PLUS:
            if (left.IsString() || right.IsString()) {
                return EvalResult::FromString(left.AsString() + right.AsString());
            }
            return EvalResult::FromNumber(left.AsNumber() + right.AsNumber());
        case BinaryOp::MINUS:
            return EvalResult::FromNumber(left.AsNumber() - right.AsNumber());
        case BinaryOp::STAR:
            return EvalResult::FromNumber(left.AsNumber() * right.AsNumber());
        case BinaryOp::SLASH: {
            double divisor = right.AsNumber();
            if (divisor == 0.0) {
                context.SetError(ExpressionError::EVAL_DIVISION_BY_ZERO, "division by zero");
                return EvalResult::Undefined();
            }
            return EvalResult::FromNumber(left.AsNumber() / divisor);
        }
        case BinaryOp::PERCENT: {
            double divisor = right.AsNumber();
            if (divisor == 0.0) {
                context.SetError(ExpressionError::EVAL_DIVISION_BY_ZERO, "modulo by zero");
                return EvalResult::Undefined();
            }
            return EvalResult::FromNumber(std::fmod(left.AsNumber(), divisor));
        }
        case BinaryOp::EQ:
            return EvalResult::FromBool(left.AsString() == right.AsString());
        case BinaryOp::NEQ:
            return EvalResult::FromBool(left.AsString() != right.AsString());
        case BinaryOp::LT:
            return EvalResult::FromBool(left.AsNumber() < right.AsNumber());
        case BinaryOp::LTE:
            return EvalResult::FromBool(left.AsNumber() <= right.AsNumber());
        case BinaryOp::GT:
            return EvalResult::FromBool(left.AsNumber() > right.AsNumber());
        case BinaryOp::GTE:
            return EvalResult::FromBool(left.AsNumber() >= right.AsNumber());
        default:
            context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unsupported binary operator");
            return EvalResult::Undefined();
    }
}

EvalResult Evaluator::EvaluateUnary(const std::shared_ptr<UnaryExpression>& node, EvaluationContext& context)
{
    if (node->operand == nullptr) {
        context.SetError(ExpressionError::PARSE_MISSING_OPERAND, "unary expression missing operand");
        return EvalResult::Undefined();
    }

    auto operand = Evaluate(node->operand, context);
    if (operand.IsUndefined()) {
        return operand;
    }

    switch (node->op) {
        case UnaryOp::MINUS:
            return EvalResult::FromNumber(-operand.AsNumber());
        case UnaryOp::NOT:
            return EvalResult::FromBool(!operand.AsBool());
        default:
            context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unsupported unary operator");
            return EvalResult::Undefined();
    }
}

EvalResult Evaluator::EvaluateConditional(
    const std::shared_ptr<ConditionalExpression>& node, EvaluationContext& context)
{
    auto condition = Evaluate(node->condition, context);
    if (condition.IsUndefined()) {
        return condition;
    }
    if (condition.AsBool()) {
        return Evaluate(node->consequent, context);
    }
    return Evaluate(node->alternate, context);
}

EvalResult Evaluator::EvaluateGrouped(const std::shared_ptr<GroupedExpression>& node, EvaluationContext& context)
{
    return Evaluate(node->expression, context);
}

EvalResult Evaluator::EvaluateFunctionCall(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context)
{
    if (node->name == ExpressionIntrinsics::JSON_POINTER) {
        return EvaluateJsonPointerIntrinsic(node, context);
    }
    if (node->name == ExpressionIntrinsics::FRAGMENT) {
        return EvaluateFragmentIntrinsic(node, context);
    }
    if (node->name == ExpressionIntrinsics::SIZE_FRAGMENT) {
        return EvaluateSizeFragmentIntrinsic(node, context);
    }

    if (!functions_.Has(node->name)) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unknown function: " + node->name);
        return EvalResult::Undefined();
    }

    bool hasUndefinedArgument = false;
    std::vector<EvalResult> args = EvaluateFunctionArguments(node, context, hasUndefinedArgument);
    if (hasUndefinedArgument) {
        return EvalResult::Undefined();
    }
    return functions_.Call(node->name, args, context);
}

EvalResult Evaluator::EvaluateJsonPointerIntrinsic(
    const std::shared_ptr<FunctionCall>& node, EvaluationContext& context)
{
    std::string dataPath;
    if (!TryGetJsonPointerIntrinsicPath(node, dataPath)) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unknown function: " + node->name);
        return EvalResult::Undefined();
    }
    if (!Sandbox::IsDataModelPathAllowed(dataPath)) {
        return EvalResult::FromString("");
    }
    return ResolveDataModelPath(dataPath, context);
}

EvalResult Evaluator::EvaluateFragmentIntrinsic(const std::shared_ptr<FunctionCall>& node, EvaluationContext& context)
{
    std::string fragmentExpression;
    if (!TryGetFragmentIntrinsicExpression(node, fragmentExpression)) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unknown function: " + node->name);
        return EvalResult::Undefined();
    }

    EvaluationContext fragmentContext = context;
    EvalResult result = ExpressionEngine::GetInstance().Evaluate("{{ " + fragmentExpression + " }}", fragmentContext);
    if (fragmentContext.lastError != ExpressionError::NONE && context.lastError == ExpressionError::NONE) {
        context.SetError(fragmentContext.lastError, fragmentContext.errorMessage, fragmentContext.errorPosition);
    }
    if (result.IsUndefined()) {
        return EvalResult::FromString("");
    }
    return result;
}

EvalResult Evaluator::EvaluateSizeFragmentIntrinsic(
    const std::shared_ptr<FunctionCall>& node, EvaluationContext& context)
{
    std::string sizeArgumentExpression;
    if (!TryGetSizeFragmentIntrinsicExpression(node, sizeArgumentExpression)) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "unknown function: " + node->name);
        return EvalResult::Undefined();
    }

    std::string trimmedExpression = TrimWhitespace(sizeArgumentExpression);
    if (trimmedExpression.empty()) {
        context.SetError(
            ExpressionError::PARSE_UNEXPECTED_TOKEN, "illegal expression: size() requires exactly one argument");
        return EvalResult::FromNumber(0.0);
    }

    EvaluationContext sizeContext = context;
    JsonValue sizeValue =
        ExpressionEngine::GetInstance().EvaluateAsJsonValue("{{ " + trimmedExpression + " }}", sizeContext);
    if (sizeContext.lastError != ExpressionError::NONE && context.lastError == ExpressionError::NONE) {
        context.SetError(sizeContext.lastError, sizeContext.errorMessage, sizeContext.errorPosition);
    }
    if (sizeValue.IsArray()) {
        return EvalResult::FromNumber(static_cast<double>(sizeValue.GetArraySize()));
    }
    if (context.lastError == ExpressionError::NONE) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "Built-in function size() expects an array argument");
    }
    return EvalResult::FromNumber(0.0);
}

std::vector<EvalResult> Evaluator::EvaluateFunctionArguments(
    const std::shared_ptr<FunctionCall>& node, EvaluationContext& context, bool& hasUndefinedArgument)
{
    std::vector<EvalResult> args;
    hasUndefinedArgument = false;
    args.reserve(node->arguments.size());
    for (const auto& arg : node->arguments) {
        EvaluationContext argContext = context;
        argContext.allowContainerResults = true;
        EvalResult result = Evaluate(arg, argContext);
        if (argContext.lastError != ExpressionError::NONE && context.lastError == ExpressionError::NONE) {
            context.SetError(argContext.lastError, argContext.errorMessage, argContext.errorPosition);
        }
        if (result.IsUndefined()) {
            hasUndefinedArgument = true;
            return args;
        }
        args.push_back(std::move(result));
    }
    return args;
}

EvalResult Evaluator::EvaluateVariableReference(
    const std::shared_ptr<VariableReference>& node, EvaluationContext& context)
{
    if (!node->isAbsolute) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "illegal expression: unquoted string: " + node->name);
        EvalResult fallback = EvalResult::FromString("");
        fallback.hasEvaluationError = true;
        return fallback;
    }

    EvalResult result = context.ResolveVariable(node->name);
    if (result.IsDefined()) {
        return result;
    }

    context.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "undefined variable: " + node->name);
    return EvalResult::Undefined();
}

namespace {

static constexpr size_t MAX_MEMBER_ACCESS_DEPTH = 128;

static bool IsAbsoluteDataModelVariableReference(const std::shared_ptr<AstNode>& node)
{
    if (node == nullptr || node->type != AstNodeType::VARIABLE_REFERENCE) {
        return false;
    }
    auto variableReference = std::static_pointer_cast<VariableReference>(node);
    return variableReference->isAbsolute && variableReference->name == "__dataModel";
}

bool IsRecoveredJsonPointerBracket(const std::shared_ptr<AstNode>& bracketKey)
{
    if (bracketKey == nullptr || bracketKey->type != AstNodeType::STRING_LITERAL) {
        return false;
    }
    const auto& value = static_cast<const StringLiteral&>(*bracketKey).value;
    return !value.empty() && value[0] == '/';
}

bool IsIllegalDataModelPathSyntax(const std::shared_ptr<AstNode>& node, size_t maxDepth)
{
    std::shared_ptr<AstNode> current = node;
    size_t depth = 0;
    bool sawMemberAccess = false;
    while (current != nullptr && current->type == AstNodeType::MEMBER_ACCESS) {
        if (depth >= maxDepth) {
            return false;
        }

        sawMemberAccess = true;
        auto member = std::static_pointer_cast<MemberAccess>(current);
        if (member->isBracket && member->bracketKey == nullptr) {
            return false;
        }
        if (member->isBracket) {
            std::string segment;
            if (!DataModelPathUtils::TryExtractBracketSegment(member->bracketKey, segment)) {
                return IsRecoveredJsonPointerBracket(member->bracketKey) ||
                       IsAbsoluteDataModelVariableReference(member->object);
            }
        }

        current = member->object;
        ++depth;
    }

    if (!sawMemberAccess || current == nullptr || current->type != AstNodeType::VARIABLE_REFERENCE) {
        return false;
    }

    auto variableReference = std::static_pointer_cast<VariableReference>(current);
    return variableReference->isAbsolute && variableReference->name == "__dataModel";
}

const VariableReference* FindRootAbsoluteVariableReference(const std::shared_ptr<AstNode>& node)
{
    std::shared_ptr<AstNode> current = node;
    while (current != nullptr && current->type == AstNodeType::MEMBER_ACCESS) {
        current = std::static_pointer_cast<MemberAccess>(current)->object;
    }

    if (current == nullptr || current->type != AstNodeType::VARIABLE_REFERENCE) {
        return nullptr;
    }

    const auto* variableReference = static_cast<const VariableReference*>(current.get());
    if (!variableReference->isAbsolute) {
        return nullptr;
    }

    return variableReference;
}

EvalResult ResolveDataModelPath(const std::string& path, EvaluationContext& context)
{
    DataModel* dataModel = context.GetDataModel();
    if (dataModel == nullptr) {
        EvalResult result = EvalResult::FromString("");
        result.hasEvaluationError = true;
        return result;
    }

    auto nodeOpt = dataModel->GetNode(path);
    if (!nodeOpt.has_value()) {
        if (context.lastError == ExpressionError::NONE) {
            context.SetError(ExpressionError::EVAL_PATH_NOT_FOUND, "path not found: " + path);
        }
        EvalResult result = EvalResult::FromString("");
        result.hasEvaluationError = true;
        return result;
    }

    JsonValue value = nodeOpt.value();
    if (!context.allowContainerResults && (value.IsArray() || value.IsObject())) {
        return EvalResult::FromString(value.ToString());
    }
    return EvalResult::FromJson(value);
}

JsonValue ResolveBracketMember(const JsonValue& objectValue, const EvalResult& bracketKey)
{
    if (objectValue.IsArray()) {
        int index = -1;
        if (!TryConvertToArrayIndex(bracketKey, index)) {
            return JsonValue();
        }
        return objectValue.GetArrayItem(index);
    }
    if (objectValue.IsObject() && bracketKey.IsString()) {
        return objectValue.GetItem(bracketKey.AsString());
    }
    return JsonValue();
}

EvalResult MakeMemberAccessEvaluationErrorResult()
{
    EvalResult result = EvalResult::FromString("");
    result.hasEvaluationError = true;
    return result;
}

} // namespace

EvalResult Evaluator::EvaluateMemberAccess(const std::shared_ptr<MemberAccess>& node, EvaluationContext& context)
{
    bool handled = false;
    EvalResult dataModelResult = TryEvaluateDataModelMemberAccess(node, context, handled);
    if (handled) {
        return dataModelResult;
    }

    auto object = EvaluateMemberObject(node, context);
    if (object.IsUndefined()) {
        return object;
    }

    if (node->property == "length" && object.IsString()) {
        return EvalResult::FromNumber(static_cast<double>(object.AsString().size()));
    }

    if (node->property == "length" && object.IsArray()) {
        return EvalResult::FromNumber(static_cast<double>(object.AsJson().GetArraySize()));
    }

    if (object.IsJson()) {
        bool jsonMemberHandled = false;
        EvalResult jsonMember = EvaluateJsonMemberValue(node, object.AsJson(), context, jsonMemberHandled);
        if (jsonMemberHandled) {
            return jsonMember;
        }
    }

    return EvaluateUnsupportedMemberAccess(node, context);
}

EvalResult Evaluator::TryEvaluateDataModelMemberAccess(
    const std::shared_ptr<MemberAccess>& node, EvaluationContext& context, bool& handled)
{
    handled = false;
    std::string dataPath = DataModelPathUtils::TryExtractDataModelPath(node, MAX_MEMBER_ACCESS_DEPTH);
    if (!dataPath.empty()) {
        handled = true;
        if (!Sandbox::IsDataModelPathAllowed(dataPath)) {
            return MakeMemberAccessEvaluationErrorResult();
        }
        return ResolveDataModelPath(dataPath, context);
    }

    if (context.lastError == ExpressionError::NONE && IsIllegalDataModelPathSyntax(node, MAX_MEMBER_ACCESS_DEPTH)) {
        handled = true;
        context.SetError(
            ExpressionError::PARSE_UNEXPECTED_TOKEN, "illegal expression: unsupported $__dataModel path syntax");
        return MakeMemberAccessEvaluationErrorResult();
    }
    return EvalResult::Undefined();
}

EvalResult Evaluator::EvaluateMemberObject(const std::shared_ptr<MemberAccess>& node, EvaluationContext& context)
{
    EvaluationContext objectContext = context;
    objectContext.allowContainerResults = true;
    auto object = Evaluate(node->object, objectContext);
    if (objectContext.lastError != ExpressionError::NONE && context.lastError == ExpressionError::NONE) {
        context.SetError(objectContext.lastError, objectContext.errorMessage, objectContext.errorPosition);
    }
    return object;
}

EvalResult Evaluator::EvaluateJsonMemberValue(
    const std::shared_ptr<MemberAccess>& node, const JsonValue& objectValue, EvaluationContext& context, bool& handled)
{
    handled = false;
    if (!node->isBracket && objectValue.IsObject()) {
        handled = true;
        JsonValue memberValue = objectValue.GetItem(node->property);
        if (memberValue.IsValid()) {
            return EvalResult::FromJson(memberValue);
        }
        context.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "member access not supported: ." + node->property);
        return EvalResult::Undefined();
    }

    if (node->isBracket && node->bracketKey != nullptr) {
        handled = true;
        EvalResult bracketKey = Evaluate(node->bracketKey, context);
        if (bracketKey.IsUndefined()) {
            return bracketKey;
        }

        JsonValue memberValue = ResolveBracketMember(objectValue, bracketKey);
        if (memberValue.IsValid()) {
            return EvalResult::FromJson(memberValue);
        }
    }

    context.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "member access not supported: ." + node->property);
    return EvalResult::Undefined();
}

EvalResult Evaluator::EvaluateUnsupportedMemberAccess(
    const std::shared_ptr<MemberAccess>& node, EvaluationContext& context) const
{
    const VariableReference* rootVariable = FindRootAbsoluteVariableReference(node);
    if (rootVariable != nullptr && rootVariable->name.size() >= 2 && rootVariable->name[0] == '_' &&
        rootVariable->name[1] == '_') {
        if (context.lastError == ExpressionError::NONE) {
            context.SetError(
                ExpressionError::EVAL_UNDEFINED_VARIABLE, "member access not supported: ." + node->property);
        }
        return MakeMemberAccessEvaluationErrorResult();
    }

    context.SetError(ExpressionError::EVAL_UNDEFINED_VARIABLE, "member access not supported: ." + node->property);
    return EvalResult::Undefined();
}

} // namespace NativeModule
