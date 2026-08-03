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

#ifndef A2UI_EXPRESSION_DATA_MODEL_PATH_UTILS_H
#define A2UI_EXPRESSION_DATA_MODEL_PATH_UTILS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "Ast.h"

namespace NativeModule {

namespace DataModelPathUtils {

inline bool TryExtractBracketSegment(const std::shared_ptr<AstNode>& bracketKey, std::string& segment)
{
    if (bracketKey == nullptr) {
        return false;
    }

    if (bracketKey->type != AstNodeType::NUMBER_LITERAL) {
        return false;
    }

    double indexValue = static_cast<const NumberLiteral&>(*bracketKey).value;
    if (!std::isfinite(indexValue) || indexValue < 0.0 || std::floor(indexValue) != indexValue) {
        return false;
    }

    constexpr double MAX_INT64_AS_DOUBLE = static_cast<double>(std::numeric_limits<int64_t>::max());
    if (indexValue > MAX_INT64_AS_DOUBLE) {
        return false;
    }

    segment = std::to_string(static_cast<int64_t>(indexValue));
    return true;
}

inline std::string TryExtractDataModelPath(const std::shared_ptr<AstNode>& node, size_t maxDepth = 128)
{
    std::vector<std::string> parts;
    std::shared_ptr<AstNode> current = node;

    while (current != nullptr && current->type == AstNodeType::MEMBER_ACCESS) {
        if (parts.size() >= maxDepth) {
            return "";
        }

        auto member = std::static_pointer_cast<MemberAccess>(current);
        if (member->isBracket) {
            std::string segment;
            if (!TryExtractBracketSegment(member->bracketKey, segment)) {
                return "";
            }
            parts.push_back(std::move(segment));
        } else {
            parts.push_back(member->property);
        }
        current = member->object;
    }

    if (current == nullptr || current->type != AstNodeType::VARIABLE_REFERENCE) {
        return "";
    }

    auto varRef = std::static_pointer_cast<VariableReference>(current);
    if (varRef->name != "__dataModel" || !varRef->isAbsolute) {
        return "";
    }

    std::reverse(parts.begin(), parts.end());
    std::string path = "/";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            path += "/";
        }
        path += parts[i];
    }
    return path;
}

} // namespace DataModelPathUtils

} // namespace NativeModule

#endif // A2UI_EXPRESSION_DATA_MODEL_PATH_UTILS_H
