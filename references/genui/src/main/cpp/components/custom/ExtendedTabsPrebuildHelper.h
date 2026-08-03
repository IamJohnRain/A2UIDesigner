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

#ifndef A2UI_EXTENDED_TABS_PREBUILD_HELPER_H
#define A2UI_EXTENDED_TABS_PREBUILD_HELPER_H

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "composition/ChildListDescriptor.h"
#include "utils/JsonAdapter.h"

namespace NativeModule {

class SurfaceSlot;

inline constexpr char EXTENDED_TABS_COMPONENT_TYPE[] = "Extended.Tabs";

inline bool IsExtendedTabsComponentType(const std::string& componentType)
{
    return componentType == EXTENDED_TABS_COMPONENT_TYPE;
}

bool IsExtendedTabsChildComponentType(const std::string& componentType);
ChildListDescriptor ParseExtendedTabsChildList(const JsonValue& descriptor);
std::list<std::string> ResolveExtendedTabsChildIds(const JsonValue& descriptor,
    const std::function<std::list<std::string>(const std::string&, const std::string&)>& resolveTemplateChildIds);
void MergeExtendedTabsChildIds(const std::vector<std::string>& tabChildIds, JsonValue& childrenArray);

void PrebuildExtendedTabsChildren(SurfaceSlot& surfaceSlot, const JsonValue& nodeValue);

} // namespace NativeModule

#endif // A2UI_EXTENDED_TABS_PREBUILD_HELPER_H
