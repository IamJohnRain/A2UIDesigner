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

#ifndef A2UI_STYLE_APPLY_UTILS_INTERNAL_H
#define A2UI_STYLE_APPLY_UTILS_INTERNAL_H

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "StyleApplyUtils.h"

namespace NativeModule {

namespace StyleApplyUtilsInternal {

inline std::string NormalizeHexColor(const std::string& rawValue)
{
    if (rawValue.empty() || rawValue[0] != '#') {
        return "";
    }
    std::string hex = rawValue.substr(1);
    return (hex.size() == 6) ? ("FF" + hex) : ((hex.size() == 8) ? hex : "");
}

inline bool ParseFloatToken(const std::string& token, float& value)
{
    if (token.empty()) {
        return false;
    }

    char* end = nullptr;
    float parsed = std::strtof(token.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return false;
    }
    value = parsed;
    return true;
}

inline bool ParseNonNegativeFloatToken(const std::string& token, float& value)
{
    float parsedValue = 0.0F;
    if (!ParseFloatToken(token, parsedValue)) {
        return false;
    }
    if (!std::isfinite(parsedValue) || parsedValue < 0.0F) {
        return false;
    }
    value = parsedValue;
    return true;
}

inline std::string ToLowerToken(const std::string& value)
{
    std::string token = StyleApplyUtils::TrimToken(value);
    std::transform(token.begin(), token.end(), token.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return token;
}

template<typename TValue>
inline bool TryFindMappedValue(
    const std::unordered_map<std::string, TValue>& mappings, const std::string& key, TValue& value)
{
    auto iter = mappings.find(key);
    if (iter == mappings.end()) {
        return false;
    }
    value = iter->second;
    return true;
}

inline StyleDimension BuildVpDimension(float value)
{
    StyleDimension dimension;
    dimension.unit = StyleDimensionUnit::VP;
    dimension.value = value;
    return dimension;
}

inline StyleDimension BuildDimension(StyleDimensionUnit unit, float value)
{
    StyleDimension dimension;
    dimension.unit = unit;
    dimension.value = value;
    return dimension;
}

inline std::string NormalizeNameToken(const std::string& value)
{
    std::string normalized = StyleApplyUtils::TrimToken(value);
    for (char& ch : normalized) {
        if (ch == '-' || ch == '_') {
            ch = ' ';
            continue;
        }
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    normalized.erase(std::remove(normalized.begin(), normalized.end(), ' '), normalized.end());
    return normalized;
}

template<typename TApply>
inline void TryApplyParsedDimension(const JsonValue& value, const char* key, TApply&& apply, bool& hasValue)
{
    StyleDimension parsedDimension;
    if (!StyleApplyUtils::ParseDimension(value.GetItem(key), parsedDimension)) {
        return;
    }
    apply(parsedDimension);
    hasValue = true;
}

inline void ApplyEdgeValue(StyleEdge& edge, const StyleDimension& value)
{
    edge.top = value;
    edge.right = value;
    edge.bottom = value;
    edge.left = value;
}

inline void ApplyRadiusValue(StyleRadius& radius, const StyleDimension& value)
{
    radius.topLeft = value;
    radius.topRight = value;
    radius.bottomRight = value;
    radius.bottomLeft = value;
}

} // namespace StyleApplyUtilsInternal

} // namespace NativeModule

#endif // A2UI_STYLE_APPLY_UTILS_INTERNAL_H
