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

#ifndef A2UI_CHILDREN_RENDER_STRATEGY_H
#define A2UI_CHILDREN_RENDER_STRATEGY_H

#include <cstdint>
#include <list>
#include <string>

#include "ChildListDescriptor.h"

namespace NativeModule {

struct ChildrenRenderContext {
    int32_t renderId = -1;
    std::string surfaceId;
    std::string parentComponentId;
};

// Strategy abstraction for children expansion.
//
// Why strategy:
// - Row/Column can use eager expansion from template data.
// - List should eventually use lazy item creation based on ArkUI list adapter
//   style APIs to avoid creating off-screen nodes.
//
// Keeping this interface now allows List implementation to swap strategy with
// minimal changes in SurfaceSlot tree-building flow.
class ChildrenRenderStrategy {
public:
    virtual ~ChildrenRenderStrategy() = default;

    virtual bool Expand(const ChildListDescriptor& descriptor, const ChildrenRenderContext& context,
        std::list<std::string>& outChildIds) const = 0;
};

} // namespace NativeModule

#endif // A2UI_CHILDREN_RENDER_STRATEGY_H
