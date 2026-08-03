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

#include "RenderManager.h"

#include "utils/LogA2UI.h"

#include "NapiResourceManager.h"
#include "RenderSlot.h"
#include "SurfaceManager.h"

namespace NativeModule {
RenderManager& RenderManager::GetInstance()
{
    static RenderManager instance;
    return instance;
}

RenderSlot& RenderManager::CreateRenderSlot(int32_t renderId)
{
    LOG_A2UI(LOG_INFO, "RenderManager::CreateRenderSlot - renderId=%{public}d", renderId);

    auto it = renderSlots_.find(renderId);
    if (it != renderSlots_.end()) {
        LOG_A2UI(
            LOG_WARN, "RenderManager::CreateRenderSlot - RenderSlot already exists for renderId=%{public}d", renderId);
        return *(it->second);
    }

    auto renderSlot = std::make_unique<RenderSlot>(renderId);
    RenderSlot& ref = *renderSlot;
    renderSlots_[renderId] = std::move(renderSlot);

    LOG_A2UI(LOG_INFO, "RenderManager::CreateRenderSlot - Created RenderSlot for renderId=%{public}d", renderId);
    return ref;
}

RenderSlot* RenderManager::FindRenderSlot(int32_t renderId)
{
    auto iter = renderSlots_.find(renderId);
    if (iter == renderSlots_.end()) {
        return nullptr;
    }
    return iter->second.get();
}

SurfaceSlot* RenderManager::FindSurface(const std::string& surfaceId)
{
    for (auto& entry : renderSlots_) {
        if (entry.second == nullptr) {
            continue;
        }
        std::shared_ptr<SurfaceManager> surfaceManager = entry.second->GetSurfaceManager();
        if (surfaceManager == nullptr) {
            continue;
        }

        SurfaceSlot* slot = surfaceManager->FindSurface(surfaceId);
        if (slot != nullptr) {
            return slot;
        }
    }
    return nullptr;
}

SurfaceSlot* RenderManager::FindSurface(int32_t renderId, const std::string& surfaceId)
{
    RenderSlot* renderSlot = FindRenderSlot(renderId);
    if (renderSlot == nullptr) {
        return nullptr;
    }
    std::shared_ptr<SurfaceManager> surfaceManager = renderSlot->GetSurfaceManager();
    if (surfaceManager == nullptr) {
        return nullptr;
    }
    return surfaceManager->FindSurface(surfaceId);
}

void RenderManager::RemoveRenderSlot(int32_t renderId)
{
    LOG_A2UI(LOG_INFO, "RenderManager::RemoveRenderSlot - renderId=%{public}d", renderId);

    auto iter = renderSlots_.find(renderId);
    if (iter == renderSlots_.end()) {
        LOG_A2UI(LOG_WARN, "RenderManager::RemoveRenderSlot - RenderSlot not found for renderId=%{public}d", renderId);
        return;
    }

    iter->second->Dispose();
    renderSlots_.erase(iter);

    LOG_A2UI(LOG_INFO, "RenderManager::RemoveRenderSlot - Removed RenderSlot for renderId=%{public}d", renderId);
}

bool RenderManager::HasRenderSlot(int32_t renderId) const
{
    return renderSlots_.find(renderId) != renderSlots_.end();
}

NapiResourceManager* RenderManager::GetNapiResourceManager()
{
    if (napiResourceManager_ == nullptr) {
        napiResourceManager_ = std::make_unique<NapiResourceManager>();
        LOG_A2UI(LOG_INFO, "RenderManager::GetNapiResourceManager - Created NapiResourceManager");
    }
    return napiResourceManager_.get();
}

} // namespace NativeModule
