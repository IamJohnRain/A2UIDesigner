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

#ifndef A2UI_DYNAMIC_VALUE_RESOLVER_H
#define A2UI_DYNAMIC_VALUE_RESOLVER_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../functions/FunctionCallInfo.h"
#include "../utils/JsonAdapter.h"
#include "ResolvedValue.h"

namespace NativeModule {

class DataModel;

enum class MissingPathPolicy { REPORT_ALWAYS = 0, DEFER_UNTIL_DATA_UPDATE };

struct DynamicResolveContext {
    int32_t renderId = 0;
    std::string surfaceId;
    std::string componentId;
    std::shared_ptr<DataModel> dataModel;
    bool allowExpression = false;
    MissingPathPolicy missingPathPolicy = MissingPathPolicy::REPORT_ALWAYS;
    std::map<std::string, JsonValue> localVariables;
};

struct DynamicValueDependencies {
    std::vector<std::string> dataPaths;
    std::vector<std::string> globalVariables;
};

class DynamicValueResolver {
public:
    static void ReportMissingPath(const DynamicResolveContext& context, const std::string& path);
    static ResolvedValue Resolve(const JsonValue& value, const DynamicResolveContext& context);
    static ResolvedValue ResolveRecursively(const JsonValue& value, const DynamicResolveContext& context);
    static ResolvedValue ResolveRecursivelyAllowPartial(const JsonValue& value, const DynamicResolveContext& context);
    static std::shared_ptr<FunctionCallInfo> ResolveFunctionCallDescriptor(
        const JsonValue& value, const DynamicResolveContext& context);
    static DynamicValueDependencies ExtractDependencies(const JsonValue& value);
    static std::vector<std::string> ExtractDataPaths(const JsonValue& value);
};

} // namespace NativeModule

#endif // A2UI_DYNAMIC_VALUE_RESOLVER_H
