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

#ifndef A2UI_DEPENDENCY_COLLECTOR_H
#define A2UI_DEPENDENCY_COLLECTOR_H

#include <memory>
#include <string>
#include <vector>

#include "Ast.h"

namespace NativeModule {

struct Dependency {
    std::string variableName;
    std::string path;
};

class DependencyCollector {
public:
    std::vector<Dependency> Collect(const std::shared_ptr<AstNode>& node);
};

} // namespace NativeModule

#endif // A2UI_DEPENDENCY_COLLECTOR_H
