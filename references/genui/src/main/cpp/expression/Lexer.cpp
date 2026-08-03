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

} // namespace

Token Lexer::ScanString(const std::string& input, size_t& pos, int32_t& line, int32_t& column)
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

Token Lexer::ScanNumber(const std::string& input, size_t& pos, int32_t& line, int32_t& column)
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
    const std::string& input, size_t& pos, int32_t& line, int32_t& column, std::vector<Token>& tokens)
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

Token Lexer::ScanIdentifier(const std::string& input, size_t& pos, int32_t& line, int32_t& column)
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

Token Lexer::ScanOperator(const std::string& input, size_t& pos, int32_t& line, int32_t& column)
{
    char current = input[pos];
    Token token;
    token.line = line;
    token.column = column;

    switch (current) {
        case '+':
            token.type = TokenType::PLUS;
            token.value = "+";
            break;
        case '-':
            token.type = TokenType::MINUS;
            token.value = "-";
            break;
        case '*':
            token.type = TokenType::STAR;
            token.value = "*";
            break;
        case '/':
            token.type = TokenType::SLASH;
            token.value = "/";
            break;
        case '%':
            token.type = TokenType::PERCENT;
            token.value = "%";
            break;
        case '(':
            token.type = TokenType::LPAREN;
            token.value = "(";
            break;
        case ')':
            token.type = TokenType::RPAREN;
            token.value = ")";
            break;
        case '[':
            token.type = TokenType::LBRACKET;
            token.value = "[";
            break;
        case ']':
            token.type = TokenType::RBRACKET;
            token.value = "]";
            break;
        case '.':
            token.type = TokenType::DOT;
            token.value = ".";
            break;
        case ',':
            token.type = TokenType::COMMA;
            token.value = ",";
            break;
        case '?':
            token.type = TokenType::QUESTION;
            token.value = "?";
            break;
        case ':':
            token.type = TokenType::COLON;
            token.value = ":";
            break;
        case '!':
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                token.type = TokenType::NEQ;
                token.value = "!=";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::BANG;
                token.value = "!";
            }
            break;
        case '=':
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                token.type = TokenType::EQ;
                token.value = "==";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::ILLEGAL;
                token.value = "=";
            }
            break;
        case '<':
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                token.type = TokenType::LTE;
                token.value = "<=";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::LT;
                token.value = "<";
            }
            break;
        case '>':
            if (pos + 1 < input.size() && input[pos + 1] == '=') {
                token.type = TokenType::GTE;
                token.value = ">=";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::GT;
                token.value = ">";
            }
            break;
        case '&':
            if (pos + 1 < input.size() && input[pos + 1] == '&') {
                token.type = TokenType::AND;
                token.value = "&&";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::ILLEGAL;
                token.value = "&";
            }
            break;
        case '|':
            if (pos + 1 < input.size() && input[pos + 1] == '|') {
                token.type = TokenType::OR;
                token.value = "||";
                ++pos;
                ++column;
            } else {
                token.type = TokenType::ILLEGAL;
                token.value = "|";
            }
            break;
        default:
            token.type = TokenType::ILLEGAL;
            token.value = std::string(1, current);
            break;
    }
    ++pos;
    ++column;
    return token;
}

std::vector<Token> Lexer::Tokenize(const std::string& input)
{
    std::vector<Token> tokens;
    int32_t line = 1;
    int32_t column = 1;
    size_t pos = 0;

    while (pos < input.size()) {
        char current = input[pos];

        if (current == '\n') {
            ++line;
            column = 1;
            ++pos;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(current)) != 0) {
            ++pos;
            ++column;
            continue;
        }

        if (current == '\'') {
            tokens.push_back(ScanString(input, pos, line, column));
            continue;
        }

        if (current == '`') {
            Token token;
            token.type = TokenType::ILLEGAL;
            token.value = "`";
            token.line = line;
            token.column = column;
            tokens.push_back(token);
            ++pos;
            ++column;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
            tokens.push_back(ScanNumber(input, pos, line, column));
            continue;
        }

        if (current == '$') {
            ScanDollar(input, pos, line, column, tokens);
            continue;
        }

        if (IsIdentifierStart(current)) {
            tokens.push_back(ScanIdentifier(input, pos, line, column));
            continue;
        }

        tokens.push_back(ScanOperator(input, pos, line, column));
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
