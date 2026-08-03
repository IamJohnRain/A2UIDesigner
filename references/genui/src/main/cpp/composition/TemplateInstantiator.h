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

#ifndef A2UI_TEMPLATE_INSTANTIATOR_H
#define A2UI_TEMPLATE_INSTANTIATOR_H

#include <cstdint>
#include <string>

namespace NativeModule {

// TemplateInstantiator is introduced as a stable extension point.
//
// Current stage:
// - Only provides deterministic instance id generation helpers.
//
// Future List stage:
// - Build template instances from item data lazily (on-demand).
// - Reuse the same instantiation logic from eager row/column and lazy list.
class TemplateInstantiator {
public:
    static std::string BuildInstanceId(const std::string& templateComponentId, int32_t itemIndex);
};

} // namespace NativeModule

#endif // A2UI_TEMPLATE_INSTANTIATOR_H
