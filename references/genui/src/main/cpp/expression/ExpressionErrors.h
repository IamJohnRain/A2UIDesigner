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

#ifndef A2UI_EXPRESSION_ERRORS_H
#define A2UI_EXPRESSION_ERRORS_H

namespace NativeModule {

enum class ExpressionError {
    NONE = 0,
    LEX_ILLEGAL_CHARACTER,
    LEX_UNTERMINATED_STRING,
    PARSE_UNEXPECTED_TOKEN,
    PARSE_MISSING_OPERAND,
    PARSE_UNBALANCED_PARENS,
    SANDBOX_LENGTH_EXCEEDED,
    SANDBOX_DEPTH_EXCEEDED,
    SANDBOX_TOKEN_COUNT_EXCEEDED,
    SANDBOX_AST_TOO_LARGE,
    SANDBOX_FUNCTION_CALL_EXCEEDED,
    SANDBOX_ILLEGAL_TOKEN,
    EVAL_DIVISION_BY_ZERO,
    EVAL_UNDEFINED_VARIABLE,
    EVAL_PATH_NOT_FOUND,
    EVAL_NO_GLOBAL_VARIABLE,
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_ERRORS_H
