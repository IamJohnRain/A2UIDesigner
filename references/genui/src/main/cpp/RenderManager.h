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

#ifndef A2UI_RENDER_MANAGER_H
#define A2UI_RENDER_MANAGER_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace NativeModule {

class RenderSlot;
class SurfaceSlot;
class NapiResourceManager;

/**
 * RenderManager - Manages all RenderSlot instances in the process
 * This is a singleton class that manages RenderSlots by their renderId
 */
class RenderManager {
public:
    static RenderManager& GetInstance();

    // Disable copy and move
    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;
    RenderManager(RenderManager&&) = delete;
    RenderManager& operator=(RenderManager&&) = delete;

    /**
     * Create a new RenderSlot with the given renderId
     * @param renderId The unique render ID
     * @return Reference to the created RenderSlot
     */
    RenderSlot& CreateRenderSlot(int32_t renderId);

    /**
     * Find a RenderSlot by renderId
     * @param renderId The render ID to find
     * @return Pointer to the RenderSlot, or nullptr if not found
     */
    RenderSlot* FindRenderSlot(int32_t renderId);

    /**
     * Find a SurfaceSlot by surfaceId across all RenderSlots
     * @param surfaceId The surface ID to find
     * @return Pointer to the SurfaceSlot, or nullptr if not found
     */
    SurfaceSlot* FindSurface(const std::string& surfaceId);

    /**
     * Find a SurfaceSlot by renderId and surfaceId
     * @param renderId The render ID to find
     * @param surfaceId The surface ID to find
     * @return Pointer to the SurfaceSlot, or nullptr if not found
     */
    SurfaceSlot* FindSurface(int32_t renderId, const std::string& surfaceId);

    /**
     * Remove a RenderSlot by renderId
     * @param renderId The render ID to remove
     */
    void RemoveRenderSlot(int32_t renderId);

    /**
     * Check if a RenderSlot exists
     * @param renderId The render ID to check
     * @return true if exists, false otherwise
     */
    bool HasRenderSlot(int32_t renderId) const;

    /**
     * Get the number of RenderSlots
     * @return The count of RenderSlots
     */
    size_t GetRenderSlotCount() const
    {
        return renderSlots_.size();
    }

    /**
     * Get the NAPI resource manager
     * @return Pointer to the NAPI resource manager
     */
    NapiResourceManager* GetNapiResourceManager();

private:
    RenderManager() = default;
    ~RenderManager() = default;

    std::unordered_map<int32_t, std::unique_ptr<RenderSlot>> renderSlots_;
    std::unique_ptr<NapiResourceManager> napiResourceManager_;
};

} // namespace NativeModule

#endif // A2UI_RENDER_MANAGER_H
