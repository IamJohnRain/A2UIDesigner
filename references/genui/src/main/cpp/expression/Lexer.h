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

#ifndef A2UI_EXPRESSION_LEXER_H
#define A2UI_EXPRESSION_LEXER_H

#include <cstdint>
#include <string>
#include <vector>

namespace NativeModule {

enum class TokenType {
    ILLEGAL,
    EOF_TOKEN,
    DOLLAR,
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER_LITERAL,
    TEMPLATE_START,
    TEMPLATE_END,
    TEMPLATE_INTERP_START,
    TEMPLATE_INTERP_END,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    DOT,
    COMMA,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    BANG,
    AND,
    OR,
    EQ,
    NEQ,
    LT,
    LTE,
    GT,
    GTE,
    QUESTION,
    COLON,
    BOOLEAN_TRUE,
    BOOLEAN_FALSE,
};

struct Token {
    TokenType type = TokenType::ILLEGAL;
    std::string value;
    int32_t line = 0;
    int32_t column = 0;
};

class Lexer {
public:
    std::vector<Token> Tokenize(const std::string& input);

private:
    Token ScanString(const std::string& input, size_t& pos, int32_t& line, int32_t& column);
    Token ScanNumber(const std::string& input, size_t& pos, int32_t& line, int32_t& column);
    void ScanDollar(const std::string& input, size_t& pos, int32_t& line, int32_t& column, std::vector<Token>& tokens);
    Token ScanIdentifier(const std::string& input, size_t& pos, int32_t& line, int32_t& column);
    Token ScanOperator(const std::string& input, size_t& pos, int32_t& line, int32_t& column);
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_LEXER_H
