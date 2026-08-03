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

#ifndef A2UI_RENDER_SLOT_H
#define A2UI_RENDER_SLOT_H

#include <cstdint>
#include <memory>

#include "adapter/A2UIArkUITypes.h"

namespace NativeModule {

class SurfaceManager;
class SurfaceSlot;

/**
 * RenderSlot - Manages multiple Surfaces (SurfaceSlots) for a single SurfaceController
 * Each RenderSlot has a unique renderId and contains a SurfaceManager to manage surfaces
 */
class RenderSlot {
public:
    explicit RenderSlot(int32_t renderId);
    ~RenderSlot();

    // Disable copy and move
    RenderSlot(const RenderSlot&) = delete;
    RenderSlot& operator=(const RenderSlot&) = delete;
    RenderSlot(RenderSlot&&) = delete;
    RenderSlot& operator=(RenderSlot&&) = delete;

    /**
     * Get the render ID
     * @return The render ID
     */
    int32_t GetRenderId() const
    {
        return renderId_;
    }

    /**
     * Get the SurfaceManager
     * @return Shared pointer to the SurfaceManager
     */
    std::shared_ptr<SurfaceManager> GetSurfaceManager() const
    {
        return surfaceManager_;
    }

    /**
     * Set the content handle for this RenderSlot
     * Delegates to SurfaceManager::SetContentHandle
     * @param handle The content handle
     */
    void SetContentHandle(A2UINodeContentHandle handle);

    /**
     * Get the content handle
     * Delegates to SurfaceManager::GetContentHandle
     * @return The content handle
     */
    A2UINodeContentHandle GetContentHandle() const;

    /**
     * Configure whether root should be forced to fill host area (100% x 100%).
     */
    void SetRootFillMode(bool forceFill);
    void SetFontSizeScale(float scale);
    float GetFontSizeScale() const;
    void SetApiVersion(int32_t apiVersion);
    int32_t GetApiVersion() const;

    /**
     * Dispose the RenderSlot and clean up resources
     */
    void Dispose();

private:
    int32_t renderId_;
    std::shared_ptr<SurfaceManager> surfaceManager_;
};

} // namespace NativeModule

#endif // A2UI_RENDER_SLOT_H
