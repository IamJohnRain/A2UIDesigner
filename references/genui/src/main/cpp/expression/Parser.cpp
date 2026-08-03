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

#include "Parser.h"

#include <memory>
#include <string>
#include <vector>

namespace NativeModule {

class Parser::ParserImpl {
public:
    explicit ParserImpl(const std::vector<Token>& tokens) : tokens_(tokens) {}

    ParseResult Parse()
    {
        ParseResult result;
        if (tokens_.empty() || Current().type == TokenType::EOF_TOKEN) {
            result.errorMessage = "empty expression";
            return result;
        }

        auto node = ParseExpression();
        if (node == nullptr) {
            result.errorMessage = errorMessage_;
            result.errorLine = errorLine_;
            result.errorColumn = errorColumn_;
            return result;
        }

        if (Current().type != TokenType::EOF_TOKEN) {
            result.errorMessage = "unexpected token after expression";
            result.errorLine = Current().line;
            result.errorColumn = Current().column;
            return result;
        }

        result.ast = std::move(node);
        result.success = true;
        return result;
    }

private:
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;
    std::string errorMessage_;
    int32_t errorLine_ = 0;
    int32_t errorColumn_ = 0;

    const Token& Current() const
    {
        return tokens_[pos_];
    }

    const Token& Advance()
    {
        return tokens_[pos_++];
    }

    bool Match(TokenType type) const
    {
        return Current().type == type;
    }

    void SetError(const std::string& message)
    {
        errorMessage_ = message;
        errorLine_ = Current().line;
        errorColumn_ = Current().column;
    }

    std::shared_ptr<AstNode> ParseExpression()
    {
        return ParseTernary();
    }

    std::shared_ptr<AstNode> ParseTernary()
    {
        auto node = ParseOr();
        if (node == nullptr) {
            return nullptr;
        }
        if (Match(TokenType::QUESTION)) {
            Advance();
            auto consequent = ParseExpression();
            if (consequent == nullptr) {
                return nullptr;
            }
            if (!Match(TokenType::COLON)) {
                SetError("expected ':' in ternary expression");
                return nullptr;
            }
            Advance();
            auto alternate = ParseExpression();
            if (alternate == nullptr) {
                return nullptr;
            }
            auto cond = std::make_shared<ConditionalExpression>();
            cond->condition = std::move(node);
            cond->consequent = std::move(consequent);
            cond->alternate = std::move(alternate);
            return cond;
        }
        return node;
    }

    std::shared_ptr<AstNode> ParseOr()
    {
        auto left = ParseAnd();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::OR)) {
            Advance();
            auto right = ParseAnd();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = BinaryOp::OR;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseAnd()
    {
        auto left = ParseEquality();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::AND)) {
            Advance();
            auto right = ParseEquality();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = BinaryOp::AND;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseEquality()
    {
        auto left = ParseComparison();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::EQ) || Match(TokenType::NEQ)) {
            BinaryOp op = Match(TokenType::EQ) ? BinaryOp::EQ : BinaryOp::NEQ;
            Advance();
            auto right = ParseComparison();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = op;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseComparison()
    {
        auto left = ParseAdditive();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::LT) || Match(TokenType::LTE) || Match(TokenType::GT) || Match(TokenType::GTE)) {
            BinaryOp op;
            if (Match(TokenType::LT)) {
                op = BinaryOp::LT;
            } else if (Match(TokenType::LTE)) {
                op = BinaryOp::LTE;
            } else if (Match(TokenType::GT)) {
                op = BinaryOp::GT;
            } else {
                op = BinaryOp::GTE;
            }
            Advance();
            auto right = ParseAdditive();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = op;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseAdditive()
    {
        auto left = ParseMultiplicative();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::PLUS) || Match(TokenType::MINUS)) {
            BinaryOp op = Match(TokenType::PLUS) ? BinaryOp::PLUS : BinaryOp::MINUS;
            Advance();
            auto right = ParseMultiplicative();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = op;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseMultiplicative()
    {
        auto left = ParseUnary();
        if (left == nullptr) {
            return nullptr;
        }
        while (Match(TokenType::STAR) || Match(TokenType::SLASH) || Match(TokenType::PERCENT)) {
            BinaryOp op;
            if (Match(TokenType::STAR)) {
                op = BinaryOp::STAR;
            } else if (Match(TokenType::SLASH)) {
                op = BinaryOp::SLASH;
            } else {
                op = BinaryOp::PERCENT;
            }
            Advance();
            auto right = ParseUnary();
            if (right == nullptr) {
                return nullptr;
            }
            auto bin = std::make_shared<BinaryExpression>();
            bin->op = op;
            bin->left = std::move(left);
            bin->right = std::move(right);
            left = bin;
        }
        return left;
    }

    std::shared_ptr<AstNode> ParseUnary()
    {
        if (Match(TokenType::MINUS)) {
            Advance();
            auto operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            auto unary = std::make_shared<UnaryExpression>();
            unary->op = UnaryOp::MINUS;
            unary->operand = std::move(operand);
            return unary;
        }
        if (Match(TokenType::BANG)) {
            Advance();
            auto operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            auto unary = std::make_shared<UnaryExpression>();
            unary->op = UnaryOp::NOT;
            unary->operand = std::move(operand);
            return unary;
        }
        return ParsePostfix();
    }

    std::shared_ptr<AstNode> ParsePostfix()
    {
        auto node = ParsePrimary();
        if (node == nullptr) {
            return nullptr;
        }
        while (true) {
            if (Match(TokenType::DOT)) {
                Advance();
                if (!Match(TokenType::IDENTIFIER)) {
                    SetError("expected identifier after '.'");
                    return nullptr;
                }
                auto member = std::make_shared<MemberAccess>();
                member->object = std::move(node);
                member->property = Advance().value;
                member->isBracket = false;
                node = member;
                continue;
            }
            if (Match(TokenType::LBRACKET)) {
                Advance();
                auto key = ParseExpression();
                if (key == nullptr) {
                    return nullptr;
                }
                if (!Match(TokenType::RBRACKET)) {
                    SetError("expected ']'");
                    return nullptr;
                }
                Advance();
                auto member = std::make_shared<MemberAccess>();
                member->object = std::move(node);
                member->isBracket = true;
                member->bracketKey = std::move(key);
                node = member;
                continue;
            }
            break;
        }
        return node;
    }

    std::shared_ptr<AstNode> ParsePrimary()
    {
        if (Match(TokenType::NUMBER_LITERAL)) {
            auto token = Advance();
            errno = 0;
            double value = std::strtod(token.value.c_str(), nullptr);
            auto node = std::make_shared<NumberLiteral>(value);
            node->line = token.line;
            node->column = token.column;
            return node;
        }
        if (Match(TokenType::STRING_LITERAL)) {
            auto token = Advance();
            auto node = std::make_shared<StringLiteral>(token.value);
            node->line = token.line;
            node->column = token.column;
            return node;
        }
        if (Match(TokenType::BOOLEAN_TRUE)) {
            auto token = Advance();
            auto node = std::make_shared<BooleanLiteral>(true);
            node->line = token.line;
            node->column = token.column;
            return node;
        }
        if (Match(TokenType::BOOLEAN_FALSE)) {
            auto token = Advance();
            auto node = std::make_shared<BooleanLiteral>(false);
            node->line = token.line;
            node->column = token.column;
            return node;
        }
        if (Match(TokenType::LPAREN)) {
            auto parenToken = Advance();
            auto inner = ParseExpression();
            if (inner == nullptr) {
                return nullptr;
            }
            if (!Match(TokenType::RPAREN)) {
                SetError("expected ')'");
                return nullptr;
            }
            Advance();
            auto grouped = std::make_shared<GroupedExpression>();
            grouped->expression = std::move(inner);
            grouped->line = parenToken.line;
            grouped->column = parenToken.column;
            return grouped;
        }
        if (Match(TokenType::IDENTIFIER)) {
            auto identToken = Advance();
            if (Match(TokenType::LPAREN)) {
                Advance();
                auto call = std::make_shared<FunctionCall>(identToken.value);
                call->line = identToken.line;
                call->column = identToken.column;
                if (!Match(TokenType::RPAREN)) {
                    auto arg = ParseExpression();
                    if (arg == nullptr) {
                        return nullptr;
                    }
                    call->arguments.push_back(std::move(arg));
                    while (Match(TokenType::COMMA)) {
                        Advance();
                        arg = ParseExpression();
                        if (arg == nullptr) {
                            return nullptr;
                        }
                        call->arguments.push_back(std::move(arg));
                    }
                }
                if (!Match(TokenType::RPAREN)) {
                    SetError("expected ')' after function arguments");
                    return nullptr;
                }
                Advance();
                return call;
            }
            auto ref = std::make_shared<VariableReference>(identToken.value);
            ref->line = identToken.line;
            ref->column = identToken.column;
            return ref;
        }
        if (Match(TokenType::DOLLAR)) {
            auto dollarToken = Advance();
            if (!Match(TokenType::IDENTIFIER)) {
                SetError("expected identifier after '$'");
                return nullptr;
            }
            auto identToken = Advance();
            auto ref = std::make_shared<VariableReference>(identToken.value, true);
            ref->line = dollarToken.line;
            ref->column = dollarToken.column;
            return ref;
        }
        SetError("unexpected token");
        return nullptr;
    }
};

ParseResult Parser::Parse(const std::vector<Token>& tokens)
{
    ParserImpl impl(tokens);
    return impl.Parse();
}

} // namespace NativeModule
