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

#include "Sandbox.h"

#include <cctype>

#include "utils/LogA2UI.h"

namespace NativeModule {

bool Sandbox::CheckExpressionLength(size_t length, EvaluationContext& context)
{
    if (length > context.maxExprLength) {
        context.SetError(ExpressionError::SANDBOX_LENGTH_EXCEEDED, "expression too long: " + std::to_string(length));
        return false;
    }
    return true;
}

bool Sandbox::ValidateTokens(const std::vector<Token>& tokens, EvaluationContext& context)
{
    for (const auto& token : tokens) {
        if (token.type == TokenType::ILLEGAL) {
            context.SetError(ExpressionError::SANDBOX_ILLEGAL_TOKEN, "illegal token: " + token.value,
                static_cast<size_t>(token.column));
            return false;
        }
    }
    return true;
}

bool Sandbox::CheckTokenCount(const std::vector<Token>& tokens, EvaluationContext& context)
{
    int32_t count = 0;
    for (const auto& token : tokens) {
        if (token.type != TokenType::EOF_TOKEN) {
            ++count;
        }
    }
    if (count > static_cast<int32_t>(context.maxTokenCount)) {
        context.SetError(ExpressionError::SANDBOX_TOKEN_COUNT_EXCEEDED, "too many tokens: " + std::to_string(count));
        return false;
    }
    return true;
}

bool Sandbox::CheckNestingDepth(const std::vector<Token>& tokens, EvaluationContext& context)
{
    int32_t depth = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::LPAREN || token.type == TokenType::LBRACKET) {
            ++depth;
            if (depth > static_cast<int32_t>(context.maxNestingDepth)) {
                context.SetError(ExpressionError::SANDBOX_DEPTH_EXCEEDED, "nesting too deep: " + std::to_string(depth));
                return false;
            }
        } else if (token.type == TokenType::RPAREN || token.type == TokenType::RBRACKET) {
            --depth;
            if (depth < 0) {
                context.SetError(ExpressionError::PARSE_UNBALANCED_PARENS, "unmatched closing bracket");
                return false;
            }
        }
    }
    if (depth != 0) {
        context.SetError(ExpressionError::PARSE_UNBALANCED_PARENS, "unmatched opening bracket");
        return false;
    }
    return true;
}

bool Sandbox::CheckBracketBalance(const std::vector<Token>& tokens, EvaluationContext& context)
{
    int32_t parenDepth = 0;
    int32_t bracketDepth = 0;
    for (const auto& token : tokens) {
        if (token.type == TokenType::LPAREN) {
            ++parenDepth;
        } else if (token.type == TokenType::RPAREN) {
            --parenDepth;
            if (parenDepth < 0) {
                context.SetError(ExpressionError::PARSE_UNBALANCED_PARENS, "unmatched ')'");
                return false;
            }
        } else if (token.type == TokenType::LBRACKET) {
            ++bracketDepth;
        } else if (token.type == TokenType::RBRACKET) {
            --bracketDepth;
            if (bracketDepth < 0) {
                context.SetError(ExpressionError::PARSE_UNBALANCED_PARENS, "unmatched ']'");
                return false;
            }
        }
    }
    if (parenDepth != 0 || bracketDepth != 0) {
        context.SetError(ExpressionError::PARSE_UNBALANCED_PARENS, "unmatched brackets");
        return false;
    }
    return true;
}

namespace {

struct AstStats {
    int32_t totalNodes = 0;
    int32_t functionCallCount = 0;
    int32_t maxConsecutiveCalls = 0;
    int32_t maxTemplateInterpDepth = 0;
};

void CollectAstStats(const std::shared_ptr<AstNode>& node, AstStats& stats);

void AccumulateFunctionCallStats(FunctionCall* fc, AstStats& stats)
{
    stats.functionCallCount++;
    int32_t consecutive = 1;
    for (const auto& arg : fc->arguments) {
        CollectAstStats(arg, stats);
        if (arg && arg->type == AstNodeType::FUNCTION_CALL) {
            ++consecutive;
            stats.maxConsecutiveCalls = std::max(stats.maxConsecutiveCalls, consecutive);
        } else {
            consecutive = 0;
        }
    }
}

void CollectBinaryAstStats(const BinaryExpression* bin, AstStats& stats)
{
    CollectAstStats(bin->left, stats);
    CollectAstStats(bin->right, stats);
}

void CollectUnaryAstStats(const UnaryExpression* unary, AstStats& stats)
{
    CollectAstStats(unary->operand, stats);
}

void CollectConditionalAstStats(const ConditionalExpression* cond, AstStats& stats)
{
    CollectAstStats(cond->condition, stats);
    CollectAstStats(cond->consequent, stats);
    CollectAstStats(cond->alternate, stats);
}

void CollectGroupedAstStats(const GroupedExpression* grp, AstStats& stats)
{
    CollectAstStats(grp->expression, stats);
}

void CollectMemberAccessAstStats(const MemberAccess* ma, AstStats& stats)
{
    CollectAstStats(ma->object, stats);
    if (ma->isBracket) {
        CollectAstStats(ma->bracketKey, stats);
    }
}

void CollectTemplateLiteralAstStats(const TemplateLiteral* tmpl, AstStats& stats)
{
    for (const auto& part : tmpl->parts) {
        if (part.isExpression) {
            CollectAstStats(part.expression, stats);
        }
    }
}

void CollectAstStats(const std::shared_ptr<AstNode>& node, AstStats& stats)
{
    if (node == nullptr) {
        return;
    }
    stats.totalNodes++;

    switch (node->type) {
        case AstNodeType::BINARY_EXPRESSION: {
            CollectBinaryAstStats(static_cast<BinaryExpression*>(node.get()), stats);
            break;
        }
        case AstNodeType::UNARY_EXPRESSION: {
            CollectUnaryAstStats(static_cast<UnaryExpression*>(node.get()), stats);
            break;
        }
        case AstNodeType::CONDITIONAL_EXPRESSION: {
            CollectConditionalAstStats(static_cast<ConditionalExpression*>(node.get()), stats);
            break;
        }
        case AstNodeType::GROUPED_EXPRESSION: {
            CollectGroupedAstStats(static_cast<GroupedExpression*>(node.get()), stats);
            break;
        }
        case AstNodeType::FUNCTION_CALL: {
            AccumulateFunctionCallStats(static_cast<FunctionCall*>(node.get()), stats);
            break;
        }
        case AstNodeType::MEMBER_ACCESS: {
            CollectMemberAccessAstStats(static_cast<MemberAccess*>(node.get()), stats);
            break;
        }
        case AstNodeType::TEMPLATE_LITERAL:
            CollectTemplateLiteralAstStats(static_cast<TemplateLiteral*>(node.get()), stats);
            break;
        default:
            break;
    }
}

} // namespace

Sandbox::AstValidationResult Sandbox::ValidateAst(const std::shared_ptr<AstNode>& ast, EvaluationContext& context)
{
    AstValidationResult result;
    if (ast == nullptr) {
        result.valid = false;
        result.reason = "null AST";
        return result;
    }

    AstStats stats;
    CollectAstStats(ast, stats);
    result.nodeCount = stats.totalNodes;

    if (stats.totalNodes > static_cast<int32_t>(context.maxAstNodes)) {
        result.valid = false;
        result.reason = "AST too large: " + std::to_string(stats.totalNodes);
        return result;
    }

    if (stats.functionCallCount > MAX_FUNCTION_CALLS_PER_EXPR) {
        result.valid = false;
        result.reason = "too many function calls: " + std::to_string(stats.functionCallCount);
        return result;
    }

    if (stats.maxConsecutiveCalls > MAX_CONSECUTIVE_CALLS) {
        result.valid = false;
        result.reason = "consecutive call chain too long";
        return result;
    }

    if (stats.maxTemplateInterpDepth > MAX_TEMPLATE_INTERP_DEPTH) {
        result.valid = false;
        result.reason = "template interpolation too deep";
        return result;
    }

    size_t estimatedMemory = static_cast<size_t>(stats.totalNodes) * BYTES_PER_NODE_ESTIMATE;
    if (estimatedMemory > MAX_MEMORY_PER_EVAL) {
        result.valid = false;
        result.reason = "estimated memory exceeds limit";
        return result;
    }

    return result;
}

bool Sandbox::IsVariableNameValid(const std::string& name)
{
    if (name.empty()) {
        return false;
    }
    if (name[0] != '$') {
        return false;
    }
    for (size_t i = 1; i < name.size(); ++i) {
        char ch = name[i];
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '.' && ch != '[' && ch != ']' &&
            ch != '-' && ch != '\'') {
            return false;
        }
    }
    if (name.find("..") != std::string::npos) {
        return false;
    }
    return true;
}

bool Sandbox::IsDataModelPathAllowed(const std::string& path)
{
    if (path.empty()) {
        return false;
    }
    if (path[0] != '/') {
        return false;
    }
    size_t start = 1;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.size();
        }
        std::string segment = path.substr(start, end - start);
        if (segment.empty() || segment == "..") {
            return false;
        }
        for (char ch : segment) {
            if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_' && ch != '-' && ch != '.') {
                return false;
            }
        }
        start = end + 1;
    }
    return true;
}

} // namespace NativeModule
