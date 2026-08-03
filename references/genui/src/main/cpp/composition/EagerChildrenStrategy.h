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

#ifndef A2UI_EAGER_CHILDREN_STRATEGY_H
#define A2UI_EAGER_CHILDREN_STRATEGY_H

#include "ChildrenRenderStrategy.h"

namespace NativeModule {

// Eager strategy for static children and non-list containers.
//
// Notes for future List implementation:
// - List should not use this strategy for large data collections.
// - Keep List on a dedicated lazy strategy to avoid off-screen node creation.
class EagerChildrenStrategy : public ChildrenRenderStrategy {
public:
    bool Expand(const ChildListDescriptor& descriptor, const ChildrenRenderContext& context,
        std::list<std::string>& outChildIds) const override;
};

} // namespace NativeModule

#endif // A2UI_EAGER_CHILDREN_STRATEGY_H
