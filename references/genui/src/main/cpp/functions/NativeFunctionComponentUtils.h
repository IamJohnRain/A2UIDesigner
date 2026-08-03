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

#ifndef A2UI_NATIVE_FUNCTION_COMPONENT_UTILS_H
#define A2UI_NATIVE_FUNCTION_COMPONENT_UTILS_H

#include <memory>
#include <string>

#include "data/DynamicValueResolver.h"

#include "RenderManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {
namespace NativeFunctionComponentUtils {

inline SurfaceSlot* FindSurfaceForContext(const DynamicResolveContext& context)
{
    if (context.surfaceId.empty()) {
        return nullptr;
    }
    RenderSlot* renderSlot = RenderManager::GetInstance().FindRenderSlot(context.renderId);
    if (renderSlot == nullptr) {
        return nullptr;
    }
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return nullptr;
    }
    return surfaceManager->FindSurface(context.surfaceId);
}

inline std::string GetShortType(const std::string& type)
{
    size_t separatorIndex = type.find_last_of('.');
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= type.size()) {
        return type;
    }
    return type.substr(separatorIndex + 1);
}

} // namespace NativeFunctionComponentUtils
} // namespace NativeModule

#endif // A2UI_NATIVE_FUNCTION_COMPONENT_UTILS_H
