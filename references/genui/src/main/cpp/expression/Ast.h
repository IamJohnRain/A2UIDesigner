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

#ifndef A2UI_EXPRESSION_AST_H
#define A2UI_EXPRESSION_AST_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "EvalResult.h"

namespace NativeModule {

enum class AstNodeType {
    NUMBER_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    TEMPLATE_LITERAL,
    VARIABLE_REFERENCE,
    MEMBER_ACCESS,
    BINARY_EXPRESSION,
    UNARY_EXPRESSION,
    CONDITIONAL_EXPRESSION,
    FUNCTION_CALL,
    GROUPED_EXPRESSION,
};

enum class BinaryOp {
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    AND,
    OR,
    EQ,
    NEQ,
    LT,
    LTE,
    GT,
    GTE,
};

enum class UnaryOp {
    MINUS,
    NOT,
};

struct AstNode {
    AstNodeType type;
    int32_t line = 0;
    int32_t column = 0;

    virtual ~AstNode() = default;
};

struct NumberLiteral final : AstNode {
    double value = 0.0;

    explicit NumberLiteral(double val) : value(val)
    {
        type = AstNodeType::NUMBER_LITERAL;
    }
};

struct StringLiteral final : AstNode {
    std::string value;

    explicit StringLiteral(std::string val) : value(std::move(val))
    {
        type = AstNodeType::STRING_LITERAL;
    }
};

struct BooleanLiteral final : AstNode {
    bool value = false;

    explicit BooleanLiteral(bool val) : value(val)
    {
        type = AstNodeType::BOOLEAN_LITERAL;
    }
};

struct TemplatePart {
    bool isExpression = false;
    std::string text;
    std::shared_ptr<AstNode> expression;
};

struct TemplateLiteral final : AstNode {
    std::vector<TemplatePart> parts;

    TemplateLiteral()
    {
        type = AstNodeType::TEMPLATE_LITERAL;
    }
};

struct VariableReference final : AstNode {
    std::string name;
    bool isAbsolute = false;

    explicit VariableReference(std::string n, bool abs = false) : name(std::move(n)), isAbsolute(abs)
    {
        type = AstNodeType::VARIABLE_REFERENCE;
    }
};

struct MemberAccess final : AstNode {
    std::shared_ptr<AstNode> object;
    std::string property;
    bool isBracket = false;
    std::shared_ptr<AstNode> bracketKey;

    MemberAccess()
    {
        type = AstNodeType::MEMBER_ACCESS;
    }
};

struct BinaryExpression final : AstNode {
    BinaryOp op = BinaryOp::PLUS;
    std::shared_ptr<AstNode> left;
    std::shared_ptr<AstNode> right;

    BinaryExpression()
    {
        type = AstNodeType::BINARY_EXPRESSION;
    }
};

struct UnaryExpression final : AstNode {
    UnaryOp op = UnaryOp::MINUS;
    std::shared_ptr<AstNode> operand;

    UnaryExpression()
    {
        type = AstNodeType::UNARY_EXPRESSION;
    }
};

struct ConditionalExpression final : AstNode {
    std::shared_ptr<AstNode> condition;
    std::shared_ptr<AstNode> consequent;
    std::shared_ptr<AstNode> alternate;

    ConditionalExpression()
    {
        type = AstNodeType::CONDITIONAL_EXPRESSION;
    }
};

struct FunctionCall final : AstNode {
    std::string name;
    std::vector<std::shared_ptr<AstNode>> arguments;

    explicit FunctionCall(std::string n) : name(std::move(n))
    {
        type = AstNodeType::FUNCTION_CALL;
    }
};

struct GroupedExpression final : AstNode {
    std::shared_ptr<AstNode> expression;

    GroupedExpression()
    {
        type = AstNodeType::GROUPED_EXPRESSION;
    }
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_AST_H
