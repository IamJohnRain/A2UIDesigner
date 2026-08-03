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

#include <array>
#include <sstream>
#include <utility>
#include <vector>

#include "StyleApplyUtilsInternal.h"

namespace NativeModule {

namespace {

StyleDimension BuildZeroVpDimension()
{
    return StyleApplyUtilsInternal::BuildVpDimension(0.0F);
}

bool ParseKeywordDimension(const std::string& token, StyleDimension& dimension)
{
    std::string normalized = StyleApplyUtilsInternal::NormalizeNameToken(token);
    if (normalized == "matchparent" || normalized == "fill") {
        dimension = StyleApplyUtilsInternal::BuildDimension(StyleDimensionUnit::MATCH_PARENT, 100.0F);
        return true;
    }
    if (normalized == "wrapcontent") {
        dimension = StyleApplyUtilsInternal::BuildDimension(StyleDimensionUnit::WRAP_CONTENT, 0.0F);
        return true;
    }
    if (normalized == "fixatidealsize") {
        dimension = StyleApplyUtilsInternal::BuildDimension(StyleDimensionUnit::FIX_AT_IDEAL_SIZE, 0.0F);
        return true;
    }
    return false;
}

bool ParseDimensionUnitSuffix(const std::string& suffix, StyleDimensionUnit& unit)
{
    std::string normalized = StyleApplyUtilsInternal::ToLowerToken(suffix);
    if (normalized.empty() || normalized == "vp") {
        unit = StyleDimensionUnit::VP;
        return true;
    }
    if (normalized == "fp") {
        unit = StyleDimensionUnit::FP;
        return true;
    }
    if (normalized == "%") {
        unit = StyleDimensionUnit::PERCENT;
        return true;
    }
    return false;
}

bool ParseStrokeWidthTokenInternal(const std::string& token, float& strokeWidth, std::string& unit)
{
    std::string trimmedToken = StyleApplyUtils::TrimToken(token);
    if (trimmedToken.empty()) {
        return false;
    }

    std::string normalizedToken = StyleApplyUtilsInternal::ToLowerToken(trimmedToken);
    std::string valueToken = trimmedToken;
    unit = "vp";

    if (normalizedToken.back() == '%') {
        unit = "%";
        valueToken = trimmedToken.substr(0, trimmedToken.size() - 1);
    } else if (normalizedToken.size() >= 2 && normalizedToken.compare(normalizedToken.size() - 2, 2, "px") == 0) {
        unit = "px";
        valueToken = trimmedToken.substr(0, trimmedToken.size() - 2);
    } else if (normalizedToken.size() >= 2 && normalizedToken.compare(normalizedToken.size() - 2, 2, "vp") == 0) {
        unit = "vp";
        valueToken = trimmedToken.substr(0, trimmedToken.size() - 2);
    } else if (normalizedToken.size() >= 2 && normalizedToken.compare(normalizedToken.size() - 2, 2, "fp") == 0) {
        unit = "fp";
        valueToken = trimmedToken.substr(0, trimmedToken.size() - 2);
    }

    return StyleApplyUtilsInternal::ParseNonNegativeFloatToken(StyleApplyUtils::TrimToken(valueToken), strokeWidth);
}

} // namespace

bool StyleApplyUtils::ParseDividerStrokeWidth(const JsonValue& value, float& strokeWidth, std::string& unit)
{
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsNumber()) {
        float parsedValue = static_cast<float>(value.GetNumberValue(0.0));
        if (!std::isfinite(parsedValue) || parsedValue < 0.0F) {
            return false;
        }
        strokeWidth = parsedValue;
        unit = "vp";
        return true;
    }
    if (!value.IsString()) {
        return false;
    }
    return ParseStrokeWidthTokenInternal(value.GetStringValue(""), strokeWidth, unit);
}

bool StyleApplyUtils::ParseDimension(const JsonValue& value, StyleDimension& dimension)
{
    if (!value.IsValid()) {
        return false;
    }
    if (value.IsNumber()) {
        float parsed = static_cast<float>(value.GetNumberValue(0.0));
        if (!std::isfinite(parsed) || parsed < 0.0F) {
            return false;
        }
        dimension = StyleApplyUtilsInternal::BuildVpDimension(parsed);
        return true;
    }
    if (!value.IsString()) {
        return false;
    }

    std::string token = TrimToken(value.GetStringValue(""));
    if (ParseKeywordDimension(token, dimension)) {
        return true;
    }

    if (token.empty()) {
        return false;
    }

    char* end = nullptr;
    float parsed = std::strtof(token.c_str(), &end);
    if (end == nullptr || end == token.c_str()) {
        return false;
    }
    if (!std::isfinite(parsed) || parsed < 0.0F) {
        return false;
    }

    std::string suffix = TrimToken(std::string(end));
    StyleDimensionUnit unit = StyleDimensionUnit::INVALID;
    if (!ParseDimensionUnitSuffix(suffix, unit)) {
        return false;
    }

    dimension = StyleApplyUtilsInternal::BuildDimension(unit, parsed);
    return true;
}

bool StyleApplyUtils::ParseEdge(const JsonValue& value, StyleEdge& edge)
{
    if (!value.IsValid()) {
        return false;
    }

    StyleDimension dimension;
    if (ParseDimension(value, dimension)) {
        StyleApplyUtilsInternal::ApplyEdgeValue(edge, dimension);
        return true;
    }
    if (value.IsString()) {
        return ParseEdgeShorthand(TrimToken(value.GetStringValue("")), edge);
    }
    return ParseEdgeObject(value, edge);
}

bool StyleApplyUtils::ParseRadius(const JsonValue& value, StyleRadius& radius)
{
    if (!value.IsValid()) {
        return false;
    }

    StyleDimension dimension;
    if (ParseDimension(value, dimension)) {
        StyleApplyUtilsInternal::ApplyRadiusValue(radius, dimension);
        return true;
    }
    if (value.IsString()) {
        return ParseRadiusShorthand(TrimToken(value.GetStringValue("")), radius);
    }
    return ParseRadiusObject(value, radius);
}

bool StyleApplyUtils::ParseEdgeObject(const JsonValue& value, StyleEdge& edge)
{
    if (!value.IsObject() || value.Has("path") || value.Has("call")) {
        return false;
    }

    StyleDimension zeroDimension = BuildZeroVpDimension();
    edge.top = zeroDimension;
    edge.right = zeroDimension;
    edge.bottom = zeroDimension;
    edge.left = zeroDimension;
    if (!value.GetChild().IsValid()) {
        return true;
    }

    bool hasValue = false;
    StyleApplyUtilsInternal::TryApplyParsedDimension(
        value, "all",
        [&](const StyleDimension& dimension) { StyleApplyUtilsInternal::ApplyEdgeValue(edge, dimension); }, hasValue);
    StyleApplyUtilsInternal::TryApplyParsedDimension(
        value, "vertical",
        [&](const StyleDimension& dimension) {
            edge.top = dimension;
            edge.bottom = dimension;
        },
        hasValue);
    StyleApplyUtilsInternal::TryApplyParsedDimension(
        value, "horizontal",
        [&](const StyleDimension& dimension) {
            edge.right = dimension;
            edge.left = dimension;
        },
        hasValue);

    static const std::array<std::pair<const char*, StyleDimension StyleEdge::*>, 4> edgeTargets = {
        std::make_pair("top", &StyleEdge::top), std::make_pair("right", &StyleEdge::right),
        std::make_pair("bottom", &StyleEdge::bottom), std::make_pair("left", &StyleEdge::left)
    };
    for (const auto& edgeTarget : edgeTargets) {
        const char* key = edgeTarget.first;
        StyleDimension StyleEdge::*member = edgeTarget.second;
        StyleApplyUtilsInternal::TryApplyParsedDimension(
            value, key, [&](const StyleDimension& dimension) { edge.*member = dimension; }, hasValue);
    }
    return hasValue;
}

bool StyleApplyUtils::ParseEdgeShorthand(const std::string& value, StyleEdge& edge)
{
    std::istringstream stream(value);
    std::vector<StyleDimension> dimensions;
    std::string token;
    while (stream >> token) {
        std::unique_ptr<JsonAdapter> adapter = JsonAdapter::CreateString(token);
        if (adapter == nullptr) {
            return false;
        }
        StyleDimension dimension;
        if (!ParseDimension(adapter->GetRoot(), dimension)) {
            return false;
        }
        dimensions.push_back(dimension);
    }

    switch (dimensions.size()) {
        case 1:
            StyleApplyUtilsInternal::ApplyEdgeValue(edge, dimensions[0]);
            return true;
        case 2:
            edge.top = dimensions[0];
            edge.bottom = dimensions[0];
            edge.right = dimensions[1];
            edge.left = dimensions[1];
            return true;
        case 3:
            edge.top = dimensions[0];
            edge.right = dimensions[1];
            edge.left = dimensions[1];
            edge.bottom = dimensions[2];
            return true;
        case 4:
            edge.top = dimensions[0];
            edge.right = dimensions[1];
            edge.bottom = dimensions[2];
            edge.left = dimensions[3];
            return true;
        default:
            return false;
    }
}

bool StyleApplyUtils::ParseRadiusObject(const JsonValue& value, StyleRadius& radius)
{
    if (!value.IsObject() || value.Has("path") || value.Has("call")) {
        return false;
    }

    StyleDimension zeroDimension = BuildZeroVpDimension();
    radius.topLeft = zeroDimension;
    radius.topRight = zeroDimension;
    radius.bottomRight = zeroDimension;
    radius.bottomLeft = zeroDimension;
    if (!value.GetChild().IsValid()) {
        return true;
    }

    bool hasValue = false;
    StyleApplyUtilsInternal::TryApplyParsedDimension(
        value, "all",
        [&](const StyleDimension& dimension) { StyleApplyUtilsInternal::ApplyRadiusValue(radius, dimension); },
        hasValue);

    static const std::array<std::pair<const char*, StyleDimension StyleRadius::*>, 4> radiusTargets = {
        std::make_pair("topLeft", &StyleRadius::topLeft), std::make_pair("topRight", &StyleRadius::topRight),
        std::make_pair("bottomRight", &StyleRadius::bottomRight), std::make_pair("bottomLeft", &StyleRadius::bottomLeft)
    };
    for (const auto& radiusTarget : radiusTargets) {
        const char* key = radiusTarget.first;
        StyleDimension StyleRadius::*member = radiusTarget.second;
        StyleApplyUtilsInternal::TryApplyParsedDimension(
            value, key, [&](const StyleDimension& dimension) { radius.*member = dimension; }, hasValue);
    }
    return hasValue;
}

bool StyleApplyUtils::ParseRadiusShorthand(const std::string& value, StyleRadius& radius)
{
    StyleEdge edge;
    if (!ParseEdgeShorthand(value, edge)) {
        return false;
    }
    radius.topLeft = edge.top;
    radius.topRight = edge.right;
    radius.bottomRight = edge.bottom;
    radius.bottomLeft = edge.left;
    return true;
}

} // namespace NativeModule
