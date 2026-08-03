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

#include "EagerChildrenStrategy.h"

namespace NativeModule {

bool EagerChildrenStrategy::Expand(const ChildListDescriptor& descriptor, const ChildrenRenderContext& context,
    std::list<std::string>& outChildIds) const
{
    (void)context;

    if (descriptor.type == ChildListType::STATIC_IDS) {
        outChildIds = descriptor.staticChildIds;
        return !outChildIds.empty();
    }

    // Template-path expansion is intentionally left for the next iteration.
    // The call site can keep this strategy for Row/Column and later switch List
    // to a lazy strategy without changing parse/model contracts.
    return false;
}

} // namespace NativeModule
