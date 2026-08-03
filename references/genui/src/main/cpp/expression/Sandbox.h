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

#ifndef A2UI_EXPRESSION_SANDBOX_H
#define A2UI_EXPRESSION_SANDBOX_H

#include <cstdint>
#include <string>
#include <vector>

#include "Ast.h"
#include "EvaluationContext.h"
#include "Lexer.h"

namespace NativeModule {

class Sandbox {
public:
    // Phase 1: input precheck
    bool CheckExpressionLength(size_t length, EvaluationContext& context);

    // Phase 2: token-level validation
    bool ValidateTokens(const std::vector<Token>& tokens, EvaluationContext& context);
    bool CheckNestingDepth(const std::vector<Token>& tokens, EvaluationContext& context);
    bool CheckTokenCount(const std::vector<Token>& tokens, EvaluationContext& context);
    bool CheckBracketBalance(const std::vector<Token>& tokens, EvaluationContext& context);

    // Phase 3: AST-level validation
    struct AstValidationResult {
        bool valid = true;
        std::string reason;
        int32_t nodeCount = 0;
    };
    AstValidationResult ValidateAst(const std::shared_ptr<AstNode>& ast, EvaluationContext& context);

    // Phase 4: runtime safety
    static bool IsVariableNameValid(const std::string& name);
    static bool IsDataModelPathAllowed(const std::string& path);

    static constexpr int32_t MAX_FUNCTION_CALLS_PER_EXPR = 3;
    static constexpr int32_t MAX_CONSECUTIVE_CALLS = 1;
    static constexpr int32_t MAX_TEMPLATE_INTERP_DEPTH = 3;
    static constexpr size_t MAX_MEMORY_PER_EVAL = 64 * 1024;
    static constexpr size_t BYTES_PER_NODE_ESTIMATE = 600;
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_SANDBOX_H
