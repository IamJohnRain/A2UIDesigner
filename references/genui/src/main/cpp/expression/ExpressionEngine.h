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

#ifndef A2UI_EXPRESSION_ENGINE_H
#define A2UI_EXPRESSION_ENGINE_H

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "../utils/JsonAdapter.h"
#include "Ast.h"
#include "DependencyCollector.h"
#include "EvalResult.h"
#include "EvaluationContext.h"
#include "Evaluator.h"
#include "ExpressionFunctions.h"
#include "Lexer.h"
#include "Parser.h"
#include "Sandbox.h"

namespace NativeModule {

struct ExprParseResult {
    std::shared_ptr<AstNode> ast;
    bool success = false;
    std::string errorMessage;
    int32_t errorLine = 0;
    int32_t errorColumn = 0;
};

struct EvaluateAndCollectResult {
    EvalResult result;
    std::vector<Dependency> dependencies;
};

struct CacheEntry {
    std::string key;
    std::shared_ptr<AstNode> ast;
};

class ExpressionEngine {
public:
    static ExpressionEngine& GetInstance();

    EvalResult Evaluate(const std::string& exprStr, EvaluationContext& context);
    JsonValue EvaluateAsJsonValue(const std::string& exprStr, EvaluationContext& context);

    ExprParseResult Parse(const std::string& exprStr);

    EvaluateAndCollectResult EvaluateAndCollect(const std::string& exprStr, EvaluationContext& context);

    static bool IsExpression(const std::string& value);
    static std::string ExtractExpression(const std::string& value);

    void EnableAstCache(bool enabled);
    void SetAstCacheCapacity(size_t maxSize);
    void ClearAstCache();
    size_t GetAstCacheSize() const;

private:
    ExpressionEngine();
    EvalResult EvaluateInternal(const std::string& exprStr, EvaluationContext& context, bool stringifyJsonResult);

    static std::string Trim(const std::string& input);

    std::shared_ptr<AstNode> GetCachedAst(const std::string& key);
    void PutCachedAst(const std::string& key, std::shared_ptr<AstNode> ast);
    void EvictOverflow();

    Lexer lexer_;
    Parser parser_;
    std::unique_ptr<Evaluator> evaluator_;
    DependencyCollector dependencyCollector_;
    ExpressionFunctions expressionFunctions_;
    Sandbox sandbox_;

    bool astCacheEnabled_ = false;
    size_t maxAstCacheSize_ = 256;
    static constexpr size_t MIN_AST_CACHE_SIZE = 16;
    static constexpr size_t MAX_AST_CACHE_SIZE_LIMIT = 1024;
    std::list<CacheEntry> cacheList_;
    std::unordered_map<std::string, std::list<CacheEntry>::iterator> cacheMap_;
};

} // namespace NativeModule

#endif // A2UI_EXPRESSION_ENGINE_H
