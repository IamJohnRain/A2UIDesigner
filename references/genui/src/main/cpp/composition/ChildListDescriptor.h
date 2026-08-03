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

#ifndef A2UI_CHILD_LIST_DESCRIPTOR_H
#define A2UI_CHILD_LIST_DESCRIPTOR_H

#include <list>
#include <string>

namespace NativeModule {

enum class ChildListType {
    INVALID = 0,
    STATIC_IDS = 1,
    TEMPLATE_PATH = 2,
};

// Parsed representation of the `children` field.
//
// Protocol semantics:
// 1) STATIC_IDS: children is string[]
// 2) TEMPLATE_PATH: children is { componentId, path }
//
// The descriptor is intentionally transport-only. It does not decide render
// strategy (eager/lazy). This separation makes it possible to keep Row/Column
// on eager rendering while later plugging List into lazy item creation.
struct ChildListDescriptor {
    ChildListType type = ChildListType::INVALID;
    std::list<std::string> staticChildIds;
    std::string templateComponentId;
    std::string templatePath;
    std::string resolvedIndexVarName = "index";
    std::string resolvedItemVarName = "item";
    bool useDefaultIndexVar = true;
    bool useDefaultItemVar = true;

    bool IsValid() const
    {
        if (type == ChildListType::STATIC_IDS) {
            return !staticChildIds.empty();
        }
        if (type == ChildListType::TEMPLATE_PATH) {
            return !templateComponentId.empty() && !templatePath.empty();
        }
        return false;
    }
};

} // namespace NativeModule

#endif // A2UI_CHILD_LIST_DESCRIPTOR_H
