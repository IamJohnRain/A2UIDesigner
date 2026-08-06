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

#include "Lexer.h"

#include <cctype>
#include <unordered_map>

namespace NativeModule {

namespace {

bool IsIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool IsIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool TrySetDoubleOperatorToken(char current, char next, Token& token)
{
    if (current == '!' && next == '=') {
        token.type = TokenType::NEQ;
        token.value = "!=";
        return true;
    }
    if (current == '=' && next == '=') {
        token.type = TokenType::EQ;
        token.value = "==";
        return true;
    }
    if (current == '<' && next == '=') {
        token.type = TokenType::LTE;
        token.value = "<=";
        return true;
    }
    if (current == '>' && next == '=') {
        token.type = TokenType::GTE;
        token.value = ">=";
        return true;
    }
    if (current == '&' && next == '&') {
        token.type = TokenType::AND;
        token.value = "&&";
        return true;
    }
    if (current == '|' && next == '|') {
        token.type = TokenType::OR;
        token.value = "||";
        return true;
    }
    return false;
}

void SetSingleOperatorToken(char current, Token& token)
{
    static const std::unordered_map<char, TokenType> simpleOps = { { '+', TokenType::PLUS }, { '-', TokenType::MINUS },
        { '*', TokenType::STAR }, { '/', TokenType::SLASH }, { '%', TokenType::PERCENT }, { '(', TokenType::LPAREN },
        { ')', TokenType::RPAREN }, { '[', TokenType::LBRACKET }, { ']', TokenType::RBRACKET }, { '.', TokenType::DOT },
        { ',', TokenType::COMMA }, { '?', TokenType::QUESTION }, { ':', TokenType::COLON }, { '!', TokenType::BANG },
        { '<', TokenType::LT }, { '>', TokenType::GT } };

    auto it = simpleOps.find(current);
    if (it != simpleOps.end()) {
        token.type = it->second;
    }
    token.value = std::string(1, current);
}

} // namespace

Token Lexer::ScanString(const std::string& input, size_t& pos, int32_t& line, int32_t& column) const
{
    size_t start = pos;
    int32_t startCol = column;
    ++pos;
    ++column;
    std::string value;
    bool terminated = false;
    while (pos < input.size()) {
        if (input[pos] == '\\' && pos + 1 < input.size()) {
            char escaped = input[pos + 1];
            if (escaped == '\'' || escaped == '\\') {
                value += escaped;
            } else if (escaped == 'n') {
                value += '\n';
            } else if (escaped == 't') {
                value += '\t';
            } else {
                value += escaped;
            }
            pos += 2;
            column += 2;
            continue;
        }
        if (input[pos] == '\'') {
            terminated = true;
            ++pos;
            ++column;
            break;
        }
        if (input[pos] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
        value += input[pos];
        ++pos;
    }
    Token token;
    token.line = line;
    token.column = startCol;
    if (!terminated) {
        token.type = TokenType::ILLEGAL;
        token.value = input.substr(start, pos - start);
    } else {
        token.type = TokenType::STRING_LITERAL;
        token.value = value;
    }
    return token;
}

Token Lexer::ScanNumber(const std::string& input, size_t& pos, int32_t& line, int32_t& column) const
{
    size_t start = pos;
    while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos])) != 0) {
        ++pos;
        ++column;
    }
    if (pos < input.size() && input[pos] == '.') {
        ++pos;
        ++column;
        if (pos >= input.size() || std::isdigit(static_cast<unsigned char>(input[pos])) == 0) {
            Token token;
            token.type = TokenType::ILLEGAL;
            token.value = input.substr(start, pos - start);
            token.line = line;
            token.column = column;
            return token;
        }
        while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos])) != 0) {
            ++pos;
            ++column;
        }
    }
    Token token;
    token.type = TokenType::NUMBER_LITERAL;
    token.value = input.substr(start, pos - start);
    token.line = line;
    token.column = column;
    return token;
}

void Lexer::ScanDollar(
    const std::string& input, size_t& pos, int32_t& line, int32_t& column, std::vector<Token>& tokens) const
{
    size_t start = pos;
    int32_t startCol = column;
    ++pos;
    ++column;
    while (pos < input.size() && IsIdentifierChar(input[pos])) {
        ++pos;
        ++column;
    }
    Token dollarToken;
    dollarToken.type = TokenType::DOLLAR;
    dollarToken.value = "$";
    dollarToken.line = line;
    dollarToken.column = startCol;
    tokens.push_back(dollarToken);
    if (pos > start + 1) {
        Token identToken;
        identToken.type = TokenType::IDENTIFIER;
        identToken.value = input.substr(start + 1, pos - start - 1);
        identToken.line = line;
        identToken.column = startCol + 1;
        tokens.push_back(identToken);
    }
}

Token Lexer::ScanIdentifier(const std::string& input, size_t& pos, int32_t& line, int32_t& column) const
{
    size_t start = pos;
    while (pos < input.size() && IsIdentifierChar(input[pos])) {
        ++pos;
        ++column;
    }
    std::string word = input.substr(start, pos - start);
    Token token;
    token.line = line;
    token.column = column;
    if (word == "true") {
        token.type = TokenType::BOOLEAN_TRUE;
    } else if (word == "false") {
        token.type = TokenType::BOOLEAN_FALSE;
    } else {
        token.type = TokenType::IDENTIFIER;
    }
    token.value = word;
    return token;
}

Token Lexer::ScanOperator(const std::string& input, size_t& pos, int32_t& line, int32_t& column) const
{
    char current = input[pos];
    Token token;
    token.line = line;
    token.column = column;

    if (pos + 1 >= input.size() || !TrySetDoubleOperatorToken(current, input[pos + 1], token)) {
        SetSingleOperatorToken(current, token);
    }

    pos += token.value.size();
    column += static_cast<int32_t>(token.value.size());
    return token;
}

void Lexer::ScanNextToken(
    const std::string& input, size_t& pos, int32_t& line, int32_t& column, std::vector<Token>& tokens) const
{
    char current = input[pos];
    if (current == '\n') {
        ++line;
        column = 1;
        ++pos;
        return;
    }
    if (std::isspace(static_cast<unsigned char>(current)) != 0) {
        ++pos;
        ++column;
        return;
    }
    if (current == '\'') {
        tokens.push_back(ScanString(input, pos, line, column));
        return;
    }
    if (current == '`') {
        tokens.push_back(Token { .type = TokenType::ILLEGAL, .value = "`", .line = line, .column = column });
        ++pos;
        ++column;
        return;
    }
    if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
        tokens.push_back(ScanNumber(input, pos, line, column));
        return;
    }
    if (current == '$') {
        ScanDollar(input, pos, line, column, tokens);
        return;
    }
    if (IsIdentifierStart(current)) {
        tokens.push_back(ScanIdentifier(input, pos, line, column));
        return;
    }
    tokens.push_back(ScanOperator(input, pos, line, column));
}

std::vector<Token> Lexer::Tokenize(const std::string& input) const
{
    std::vector<Token> tokens;
    int32_t line = 1;
    int32_t column = 1;
    size_t pos = 0;

    while (pos < input.size()) {
        ScanNextToken(input, pos, line, column, tokens);
    }

    Token eof;
    eof.type = TokenType::EOF_TOKEN;
    eof.value = "";
    eof.line = line;
    eof.column = column;
    tokens.push_back(eof);
    return tokens;
}

} // namespace NativeModule
