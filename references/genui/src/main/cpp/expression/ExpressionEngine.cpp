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

#include "ExpressionEngine.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>

#include "utils/LogA2UI.h"

#include "../data/PathValidator.h"
#include "ExpressionIntrinsics.h"

namespace NativeModule {

namespace {

void LogExpressionWarn(const char* stage, const EvaluationContext& context)
{
    LOG_A2UI(LOG_WARN,
        "ExpressionEngine: %{public}s failed, error=%{public}d, position=%{public}zu, "
        "surface=%{public}s, component=%{public}s",
        stage, static_cast<int32_t>(context.lastError), context.errorPosition, context.GetSurfaceId().c_str(),
        context.GetComponentId().c_str());
}

const VariableReference* AsTopLevelRelativeVariableReference(const std::shared_ptr<AstNode>& ast)
{
    if (ast == nullptr || ast->type != AstNodeType::VARIABLE_REFERENCE) {
        return nullptr;
    }
    const auto* variableReference = static_cast<const VariableReference*>(ast.get());
    return variableReference->isAbsolute ? nullptr : variableReference;
}

bool IsIdentifierStart(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool IsIdentifierChar(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool IsExpressionIdentifier(const std::string& segment)
{
    if (segment.empty() || !IsIdentifierStart(segment[0])) {
        return false;
    }
    for (size_t index = 1; index < segment.size(); ++index) {
        if (!IsIdentifierChar(segment[index])) {
            return false;
        }
    }
    return true;
}

bool IsArrayIndexSegment(const std::string& segment)
{
    if (segment.empty()) {
        return false;
    }
    if (segment == "0") {
        return true;
    }
    if (segment[0] == '0') {
        return false;
    }
    for (char ch : segment) {
        if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
            return false;
        }
    }
    return true;
}

std::string TrimPlaceholderToken(const std::string& input)
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

std::string EscapeExpressionStringLiteral(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        if (ch == '\\' || ch == '\'') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

bool DecodeJsonPointerSegment(const std::string& token, std::string& decoded)
{
    decoded.clear();
    decoded.reserve(token.size());
    for (size_t index = 0; index < token.size(); ++index) {
        if (token[index] != '~') {
            decoded.push_back(token[index]);
            continue;
        }
        if (index + 1 >= token.size()) {
            return false;
        }
        char escaped = token[index + 1];
        if (escaped == '0') {
            decoded.push_back('~');
        } else if (escaped == '1') {
            decoded.push_back('/');
        } else {
            return false;
        }
        ++index;
    }
    return true;
}

std::string BuildDataModelExpressionFromJsonPointer(const std::string& path)
{
    if (!IsValidDataPath(path)) {
        return path;
    }

    std::string rewritten = "$__dataModel";
    size_t segmentStart = 1;
    while (segmentStart <= path.size()) {
        size_t segmentEnd = path.find('/', segmentStart);
        std::string segment = segmentEnd == std::string::npos ? path.substr(segmentStart)
                                                              : path.substr(segmentStart, segmentEnd - segmentStart);
        std::string decodedSegment;
        if (!DecodeJsonPointerSegment(segment, decodedSegment)) {
            return path;
        }

        if (IsExpressionIdentifier(decodedSegment)) {
            rewritten.append(".").append(decodedSegment);
        } else if (IsArrayIndexSegment(decodedSegment)) {
            rewritten.append("[").append(decodedSegment).append("]");
        } else {
            return std::string(ExpressionIntrinsics::JSON_POINTER) + "('" + EscapeExpressionStringLiteral(path) + "')";
        }

        if (segmentEnd == std::string::npos) {
            break;
        }
        segmentStart = segmentEnd + 1;
    }

    return rewritten;
}

std::string BuildExpressionFragmentIntrinsic(const std::string& token)
{
    return std::string(ExpressionIntrinsics::FRAGMENT) + "('" +
           EscapeExpressionStringLiteral(TrimPlaceholderToken(token)) + "')";
}

std::string BuildSizeFragmentIntrinsic(const std::string& token)
{
    return std::string(ExpressionIntrinsics::SIZE_FRAGMENT) + "('" +
           EscapeExpressionStringLiteral(TrimPlaceholderToken(token)) + "')";
}

size_t FindPlaceholderClose(const std::string& expression, size_t openIndex)
{
    if (openIndex + 1 >= expression.size() || expression[openIndex] != '$' || expression[openIndex + 1] != '{') {
        return std::string::npos;
    }

    size_t depth = 1;
    for (size_t index = openIndex + 2; index < expression.size(); ++index) {
        if (index + 1 < expression.size() && expression[index] == '$' && expression[index + 1] == '{') {
            ++depth;
            ++index;
            continue;
        }
        if (expression[index] != '}') {
            continue;
        }
        --depth;
        if (depth == 0u) {
            return index;
        }
    }
    return std::string::npos;
}

size_t FindMatchingParenthesis(const std::string& expression, size_t openIndex)
{
    if (openIndex >= expression.size() || expression[openIndex] != '(') {
        return std::string::npos;
    }

    size_t depth = 1;
    char quote = '\0';
    bool escaping = false;
    for (size_t index = openIndex + 1; index < expression.size(); ++index) {
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

bool IsBareSizeCallStart(const std::string& expression, size_t index, size_t& openParenIndex)
{
    static constexpr char kSizeName[] = "size";
    static constexpr size_t kSizeNameLength = sizeof(kSizeName) - 1u;

    if (index + kSizeNameLength > expression.size() || expression.compare(index, kSizeNameLength, kSizeName) != 0) {
        return false;
    }
    if (index > 0u) {
        char previous = expression[index - 1u];
        if (IsIdentifierChar(previous) || previous == '.') {
            return false;
        }
    }

    openParenIndex = index + kSizeNameLength;
    while (openParenIndex < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[openParenIndex])) != 0) {
        ++openParenIndex;
    }
    return openParenIndex < expression.size() && expression[openParenIndex] == '(';
}

bool IsLegacyPlaceholderWrappedSizeCall(const std::string& token)
{
    std::string trimmed = TrimPlaceholderToken(token);
    size_t openParenIndex = std::string::npos;
    if (!IsBareSizeCallStart(trimmed, 0u, openParenIndex)) {
        return false;
    }
    size_t closeParenIndex = FindMatchingParenthesis(trimmed, openParenIndex);
    return closeParenIndex == trimmed.size() - 1u;
}

std::string RewriteSizeFunctionCalls(const std::string& expression)
{
    if (expression.find("size") == std::string::npos) {
        return expression;
    }

    std::string rewritten;
    rewritten.reserve(expression.size());
    size_t index = 0;
    char quote = '\0';
    bool escaping = false;
    while (index < expression.size()) {
        char current = expression[index];
        if (quote != '\0') {
            rewritten.push_back(current);
            if (escaping) {
                escaping = false;
            } else if (current == '\\') {
                escaping = true;
            } else if (current == quote) {
                quote = '\0';
            }
            ++index;
            continue;
        }

        if (current == '\'' || current == '"') {
            quote = current;
            rewritten.push_back(current);
            ++index;
            continue;
        }

        size_t openParenIndex = std::string::npos;
        if (IsBareSizeCallStart(expression, index, openParenIndex)) {
            size_t closeParenIndex = FindMatchingParenthesis(expression, openParenIndex);
            if (closeParenIndex == std::string::npos) {
                rewritten.append(expression.substr(index));
                break;
            }

            rewritten.append(BuildSizeFragmentIntrinsic(
                expression.substr(openParenIndex + 1u, closeParenIndex - openParenIndex - 1u)));
            index = closeParenIndex + 1u;
            continue;
        }

        rewritten.push_back(current);
        ++index;
    }
    return rewritten;
}

std::string RewriteExpressionPlaceholders(const std::string& expression)
{
    if (expression.find("${") == std::string::npos) {
        return RewriteSizeFunctionCalls(expression);
    }

    std::string rewritten;
    rewritten.reserve(expression.size());
    size_t index = 0;
    while (index < expression.size()) {
        if (index + 1 < expression.size() && expression[index] == '$' && expression[index + 1] == '{') {
            size_t closePos = FindPlaceholderClose(expression, index);
            if (closePos == std::string::npos) {
                rewritten.append(expression.substr(index));
                break;
            }

            std::string token = expression.substr(index + 2, closePos - index - 2);
            std::string trimmedToken = TrimPlaceholderToken(token);
            if (!trimmedToken.empty() && trimmedToken[0] == '/') {
                rewritten.append(BuildDataModelExpressionFromJsonPointer(trimmedToken));
                index = closePos + 1;
                continue;
            }

            if (IsLegacyPlaceholderWrappedSizeCall(token)) {
                // Keep the legacy wrapped form as-is so parser/token validation rejects it.
                rewritten.append("${").append(token).append("}");
                index = closePos + 1;
                continue;
            }

            rewritten.append(BuildExpressionFragmentIntrinsic(token));
            index = closePos + 1;
            continue;
        }

        rewritten.push_back(expression[index]);
        ++index;
    }
    return RewriteSizeFunctionCalls(rewritten);
}

} // namespace

ExpressionEngine::ExpressionEngine()
{
    expressionFunctions_.Register(
        "size", [](const std::vector<EvalResult>& args, EvaluationContext& context) -> EvalResult {
            if (args.size() != 1u) {
                context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN,
                    "illegal expression: size() requires exactly one argument");
                return EvalResult::Undefined();
            }
            if (args[0].hasEvaluationError || context.lastError != ExpressionError::NONE) {
                return EvalResult::Undefined();
            }
            if (args[0].IsArray()) {
                return EvalResult::FromNumber(static_cast<double>(args[0].AsJson().GetArraySize()));
            }
            context.SetError(
                ExpressionError::PARSE_UNEXPECTED_TOKEN, "Built-in function size() expects an array argument");
            return EvalResult::FromNumber(0.0);
        });
}

ExpressionEngine& ExpressionEngine::GetInstance()
{
    static ExpressionEngine instance;
    if (instance.evaluator_ == nullptr) {
        instance.evaluator_ = std::make_unique<Evaluator>(instance.expressionFunctions_);
    }
    return instance;
}

bool ExpressionEngine::IsExpression(const std::string& value)
{
    const size_t len = value.size();
    if (len < 4) {
        return false;
    }
    if (value[0] != '{' || value[1] != '{' || value[len - 2] != '}' || value[len - 1] != '}') {
        return false;
    }
    const size_t bodyEnd = len - 2;
    size_t index = 2;
    while (index < bodyEnd) {
        if (index + 1 < bodyEnd && value[index] == '$' && value[index + 1] == '{') {
            const size_t closePos = FindPlaceholderClose(value, index);
            if (closePos == std::string::npos || closePos >= bodyEnd) {
                return false;
            }
            index = closePos + 1;
            continue;
        }
        if (value[index] == '{' || value[index] == '}') {
            return false;
        }
        ++index;
    }
    return true;
}

std::string ExpressionEngine::ExtractExpression(const std::string& value)
{
    if (!IsExpression(value)) {
        return "";
    }
    return Trim(value.substr(2, value.size() - 4));
}

std::string ExpressionEngine::Trim(const std::string& input)
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

EvalResult ExpressionEngine::EvaluateInternal(
    const std::string& exprStr, EvaluationContext& context, bool stringifyJsonResult)
{
    context.ClearError();
    std::string innerExpr;
    if (!PrepareExpression(exprStr, context, innerExpr)) {
        return EvalResult::Undefined();
    }

    std::vector<Token> tokens = lexer_.Tokenize(innerExpr);
    if (!ValidatePreparedTokens(innerExpr, tokens, context)) {
        return EvalResult::Undefined();
    }

    std::shared_ptr<AstNode> ast = GetOrParseAst(innerExpr, tokens, context);
    if (ast == nullptr) {
        return EvalResult::Undefined();
    }

    EvalResult result = evaluator_->Evaluate(ast, context);
    return FinalizeEvaluationResult(ast, std::move(result), context, stringifyJsonResult);
}

bool ExpressionEngine::PrepareExpression(const std::string& exprStr, EvaluationContext& context, std::string& innerExpr)
{
    innerExpr = ExtractExpression(exprStr);
    if (innerExpr.empty() && !exprStr.empty()) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, "invalid expression format");
        LogExpressionWarn("extract", context);
        return false;
    }

    innerExpr = RewriteExpressionPlaceholders(innerExpr);
    if (innerExpr.size() > context.maxExprLength) {
        context.SetError(ExpressionError::SANDBOX_LENGTH_EXCEEDED, "expression too long");
        LogExpressionWarn("length", context);
        return false;
    }
    return true;
}

bool ExpressionEngine::ValidatePreparedTokens(
    const std::string& innerExpr, const std::vector<Token>& tokens, EvaluationContext& context)
{
    (void)innerExpr;
    if (!sandbox_.ValidateTokens(tokens, context)) {
        LogExpressionWarn("tokens", context);
        return false;
    }
    if (tokens.size() > context.maxTokenCount + 1) { // +1 for EOF
        context.SetError(ExpressionError::SANDBOX_TOKEN_COUNT_EXCEEDED, "too many tokens");
        LogExpressionWarn("token-count", context);
        return false;
    }
    if (!sandbox_.CheckNestingDepth(tokens, context)) {
        LogExpressionWarn("nesting", context);
        return false;
    }
    if (!sandbox_.CheckBracketBalance(tokens, context)) {
        LogExpressionWarn("brackets", context);
        return false;
    }
    return true;
}

std::shared_ptr<AstNode> ExpressionEngine::GetOrParseAst(
    const std::string& cacheKey, const std::vector<Token>& tokens, EvaluationContext& context)
{
    std::shared_ptr<AstNode> ast = GetCachedAst(cacheKey);
    if (ast != nullptr) {
        return ast;
    }

    ParseResult parseResult = parser_.Parse(tokens);
    if (!parseResult.success || parseResult.ast == nullptr) {
        context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN, parseResult.errorMessage);
        LogExpressionWarn("parser", context);
        return nullptr;
    }

    auto astResult = sandbox_.ValidateAst(parseResult.ast, context);
    if (!astResult.valid) {
        context.SetError(ExpressionError::SANDBOX_AST_TOO_LARGE, astResult.reason);
        LogExpressionWarn("ast", context);
        return nullptr;
    }

    ast = std::move(parseResult.ast);
    PutCachedAst(cacheKey, ast);
    return ast;
}

EvalResult ExpressionEngine::FinalizeEvaluationResult(
    const std::shared_ptr<AstNode>& ast, EvalResult result, EvaluationContext& context, bool stringifyJsonResult)
{
    if (result.IsUndefined()) {
        const VariableReference* variableReference = AsTopLevelRelativeVariableReference(ast);
        if (variableReference != nullptr && context.lastError == ExpressionError::EVAL_UNDEFINED_VARIABLE) {
            context.SetError(ExpressionError::PARSE_UNEXPECTED_TOKEN,
                "illegal expression: unquoted string: " + variableReference->name);
            EvalResult fallback = EvalResult::FromString("");
            fallback.hasEvaluationError = true;
            return fallback;
        }
        LogExpressionWarn("evaluator", context);
        return result;
    }
    if (stringifyJsonResult && result.IsJson()) {
        return EvalResult::FromString(result.AsString());
    }
    return result;
}

EvalResult ExpressionEngine::Evaluate(const std::string& exprStr, EvaluationContext& context)
{
    return EvaluateInternal(exprStr, context, true);
}

JsonValue ExpressionEngine::EvaluateAsJsonValue(
    const std::string& exprStr, EvaluationContext& context, bool preserveNullResult)
{
    EvaluationContext jsonContext = context;
    jsonContext.allowContainerResults = true;
    EvalResult result = EvaluateInternal(exprStr, jsonContext, false);
    context.lastError = jsonContext.lastError;
    context.errorMessage = jsonContext.errorMessage;
    context.errorPosition = jsonContext.errorPosition;
    if (result.IsUndefined()) {
        return JsonValue();
    }
    switch (result.type) {
        case EvalValueType::STRING: {
            auto adapter = JsonAdapter::CreateString(result.stringValue);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case EvalValueType::NUMBER: {
            if (!std::isfinite(result.numberValue)) {
                return JsonValue();
            }
            auto adapter = JsonAdapter::CreateNumber(result.numberValue);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case EvalValueType::BOOLEAN: {
            auto adapter = JsonAdapter::CreateBool(result.boolValue);
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case EvalValueType::NULL_VALUE: {
            if (!preserveNullResult) {
                return JsonValue();
            }
            auto adapter = JsonAdapter::CreateNull();
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        case EvalValueType::JSON_VALUE: {
            auto adapter = JsonAdapter::Clone(result.AsJson());
            return adapter != nullptr ? adapter->GetRoot() : JsonValue();
        }
        default:
            return JsonValue();
    }
}

ExprParseResult ExpressionEngine::Parse(const std::string& exprStr)
{
    ExprParseResult result;

    if (exprStr.empty()) {
        result.errorMessage = "empty expression";
        return result;
    }

    std::string normalizedExpr = RewriteExpressionPlaceholders(exprStr);
    std::vector<Token> tokens = lexer_.Tokenize(normalizedExpr);
    ParseResult parseResult = parser_.Parse(tokens);
    if (!parseResult.success || parseResult.ast == nullptr) {
        result.errorMessage = parseResult.errorMessage;
        result.errorLine = parseResult.errorLine;
        result.errorColumn = parseResult.errorColumn;
        return result;
    }

    result.ast = std::move(parseResult.ast);
    result.success = true;
    return result;
}

EvaluateAndCollectResult ExpressionEngine::EvaluateAndCollect(const std::string& exprStr, EvaluationContext& context)
{
    EvaluateAndCollectResult result;
    result.result = Evaluate(exprStr, context);
    if (result.result.IsDefined()) {
        // Parse again to get AST for dependency collection (or use cache)
        auto parseResult = Parse(exprStr);
        if (parseResult.success && parseResult.ast != nullptr) {
            result.dependencies = dependencyCollector_.Collect(parseResult.ast);
        }
    }
    return result;
}

void ExpressionEngine::EnableAstCache(bool enabled)
{
    astCacheEnabled_ = enabled;
}

void ExpressionEngine::SetAstCacheCapacity(size_t maxSize)
{
    maxAstCacheSize_ = std::max(maxSize, MIN_AST_CACHE_SIZE);
    maxAstCacheSize_ = std::min(maxAstCacheSize_, MAX_AST_CACHE_SIZE_LIMIT);
    EvictOverflow();
}

std::shared_ptr<AstNode> ExpressionEngine::GetCachedAst(const std::string& key)
{
    if (!astCacheEnabled_) {
        return nullptr;
    }
    auto it = cacheMap_.find(key);
    if (it == cacheMap_.end()) {
        return nullptr;
    }
    cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
    return it->second->ast;
}

void ExpressionEngine::PutCachedAst(const std::string& key, std::shared_ptr<AstNode> ast)
{
    if (!astCacheEnabled_ || ast == nullptr) {
        return;
    }
    auto it = cacheMap_.find(key);
    if (it != cacheMap_.end()) {
        it->second->ast = std::move(ast);
        cacheList_.splice(cacheList_.begin(), cacheList_, it->second);
        return;
    }
    cacheList_.push_front({ key, std::move(ast) });
    cacheMap_[key] = cacheList_.begin();
    EvictOverflow();
}

void ExpressionEngine::ClearAstCache()
{
    cacheMap_.clear();
    cacheList_.clear();
}

size_t ExpressionEngine::GetAstCacheSize() const
{
    return cacheList_.size();
}

void ExpressionEngine::EvictOverflow()
{
    while (cacheList_.size() > maxAstCacheSize_) {
        auto& oldest = cacheList_.back();
        cacheMap_.erase(oldest.key);
        cacheList_.pop_back();
    }
}

} // namespace NativeModule
