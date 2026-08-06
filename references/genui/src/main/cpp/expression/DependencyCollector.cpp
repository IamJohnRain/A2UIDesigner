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

#include "DependencyCollector.h"

#include "DataModelPathUtils.h"
#include "ExpressionEngine.h"
#include "ExpressionIntrinsics.h"

namespace NativeModule {

namespace {

bool TryCollectJsonPointerIntrinsicDependency(const std::shared_ptr<FunctionCall>& node, Dependency& dependency)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::JSON_POINTER || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr || literal->value.empty() || literal->value[0] != '/') {
        return false;
    }

    dependency = { "__dataModel", literal->value };
    return true;
}

bool TryGetFragmentIntrinsicExpression(const std::shared_ptr<FunctionCall>& node, std::string& expression)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::FRAGMENT || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr) {
        return false;
    }

    expression = literal->value;
    return true;
}

bool TryGetSizeFragmentIntrinsicExpression(const std::shared_ptr<FunctionCall>& node, std::string& expression)
{
    if (node == nullptr || node->name != ExpressionIntrinsics::SIZE_FRAGMENT || node->arguments.size() != 1u) {
        return false;
    }

    auto literal = std::dynamic_pointer_cast<StringLiteral>(node->arguments[0]);
    if (literal == nullptr) {
        return false;
    }

    expression = literal->value;
    return true;
}

std::string TrimWhitespace(const std::string& input)
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

size_t FindMatchingParenthesis(const std::string& expression, size_t openIndex)
{
    if (openIndex >= expression.size() || expression[openIndex] != '(') {
        return std::string::npos;
    }

    size_t depth = 1u;
    char quote = '\0';
    bool escaping = false;
    for (size_t index = openIndex + 1u; index < expression.size(); ++index) {
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

void CollectRecursive(const std::shared_ptr<AstNode>& node, std::vector<Dependency>& deps);
void CollectFunctionCallDeps(const std::shared_ptr<FunctionCall>& func, std::vector<Dependency>& deps);

void CollectVariableReferenceDeps(const std::shared_ptr<AstNode>& node, std::vector<Dependency>& deps)
{
    auto varRef = std::static_pointer_cast<VariableReference>(node);
    if (varRef->isAbsolute) {
        deps.push_back({ varRef->name, "" });
    }
}

void CollectBinaryDeps(const std::shared_ptr<AstNode>& node, std::vector<Dependency>& deps)
{
    auto binary = std::static_pointer_cast<BinaryExpression>(node);
    CollectRecursive(binary->left, deps);
    CollectRecursive(binary->right, deps);
}

void CollectConditionalDeps(const std::shared_ptr<AstNode>& node, std::vector<Dependency>& deps)
{
    auto cond = std::static_pointer_cast<ConditionalExpression>(node);
    CollectRecursive(cond->condition, deps);
    CollectRecursive(cond->consequent, deps);
    CollectRecursive(cond->alternate, deps);
}

void CollectRecursive(const std::shared_ptr<AstNode>& node, std::vector<Dependency>& deps)
{
    if (node == nullptr) {
        return;
    }

    switch (node->type) {
        case AstNodeType::VARIABLE_REFERENCE: {
            CollectVariableReferenceDeps(node, deps);
            break;
        }
        case AstNodeType::MEMBER_ACCESS: {
            auto member = std::static_pointer_cast<MemberAccess>(node);
            std::string dataPath = DataModelPathUtils::TryExtractDataModelPath(node);
            if (!dataPath.empty()) {
                deps.push_back({ "__dataModel", dataPath });
                // Full DataModel path already captured — don't recurse into object
                // which would produce duplicate deps at each intermediate level
            } else {
                CollectRecursive(member->object, deps);
            }
            if (member->bracketKey) {
                CollectRecursive(member->bracketKey, deps);
            }
            break;
        }
        case AstNodeType::BINARY_EXPRESSION: {
            CollectBinaryDeps(node, deps);
            break;
        }
        case AstNodeType::UNARY_EXPRESSION: {
            auto unary = std::static_pointer_cast<UnaryExpression>(node);
            CollectRecursive(unary->operand, deps);
            break;
        }
        case AstNodeType::CONDITIONAL_EXPRESSION: {
            CollectConditionalDeps(node, deps);
            break;
        }
        case AstNodeType::FUNCTION_CALL: {
            auto func = std::static_pointer_cast<FunctionCall>(node);
            CollectFunctionCallDeps(func, deps);
            break;
        }
        case AstNodeType::GROUPED_EXPRESSION: {
            auto grouped = std::static_pointer_cast<GroupedExpression>(node);
            CollectRecursive(grouped->expression, deps);
            break;
        }
        default:
            break;
    }
}

void CollectFunctionCallDeps(const std::shared_ptr<FunctionCall>& func, std::vector<Dependency>& deps)
{
    Dependency dependency;
    if (TryCollectJsonPointerIntrinsicDependency(func, dependency)) {
        deps.push_back(std::move(dependency));
        return;
    }
    std::string fragmentExpression;
    if (TryGetFragmentIntrinsicExpression(func, fragmentExpression)) {
        auto parseResult = ExpressionEngine::GetInstance().Parse(fragmentExpression);
        if (parseResult.success) {
            CollectRecursive(parseResult.ast, deps);
        }
        return;
    }
    std::string sizeArgumentExpression;
    if (TryGetSizeFragmentIntrinsicExpression(func, sizeArgumentExpression)) {
        auto parseResult = ExpressionEngine::GetInstance().Parse(sizeArgumentExpression);
        if (parseResult.success) {
            CollectRecursive(parseResult.ast, deps);
        }
        return;
    }
    for (const auto& arg : func->arguments) {
        CollectRecursive(arg, deps);
    }
}

} // namespace

std::vector<Dependency> DependencyCollector::Collect(const std::shared_ptr<AstNode>& node)
{
    std::vector<Dependency> deps;
    CollectRecursive(node, deps);
    return deps;
}

} // namespace NativeModule
