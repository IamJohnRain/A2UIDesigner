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

#ifndef A2UI_STYLE_RESOLVER_H
#define A2UI_STYLE_RESOLVER_H

#include <map>
#include <set>
#include <string>

#include "components/extended/RenderContext.h"
#include "data/DynamicValueResolver.h"

#include "StyleTypes.h"

namespace NativeModule {

class StyleResolver final {
public:
    static constexpr const char* STYLE_BINDING_PREFIX = "styles.";

    static StyleResolveResult Resolve(const StyleParseResult& parseResult, const RenderContext& renderContext,
        const std::string& componentId, const std::set<std::string>& previousStyleKeys,
        const std::map<std::string, JsonValue>& localVariables = {});
    static bool IsStyleBindingProperty(const std::string& property);
    static std::string ExtractStyleNameFromBindingProperty(const std::string& property);
    static std::string BuildStyleBindingProperty(const std::string& styleName);

private:
    static void BuildClearBindingPlan(StyleResolveResult& result, const std::set<std::string>& previousStyleKeys);
    static void BuildResetPlan(StyleResolveResult& result, const std::set<std::string>& previousStyleKeys);
    static bool ResolveProperty(
        const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result);
    static bool ResolveDynamicProperty(
        const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result);
    static bool ResolveDynamicDescriptorProperty(
        const StyleProperty& property, const DynamicResolveContext& dynamicContext, StyleResolveResult& result);
    static void RegisterDescriptorBindings(
        const StyleProperty& property, const DynamicValueDependencies& dependencies, StyleResolveResult& result);
    static bool PutResolvedValue(StyleResolveResult& result, const StyleProperty& property, const JsonValue& value);
};

} // namespace NativeModule

#endif // A2UI_STYLE_RESOLVER_H
