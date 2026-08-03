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

#ifndef NATIVE_SURFACE_MANAGER_H
#define NATIVE_SURFACE_MANAGER_H

#include <js_native_api.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "theme/ThemeBase.h"

#include "SurfaceSlot.h"

namespace NativeModule {

class SurfaceManager {
public:
    SurfaceManager() = default;
    ~SurfaceManager() = default;

    // Disable copy and move
    SurfaceManager(const SurfaceManager&) = delete;
    SurfaceManager& operator=(const SurfaceManager&) = delete;
    SurfaceManager(SurfaceManager&&) = delete;
    SurfaceManager& operator=(SurfaceManager&&) = delete;

    SurfaceSlot* FindSurface(const std::string& surfaceId);
    SurfaceSlot& CreateSurface(const std::string& surfaceId, A2UINodeContentHandle contentHandle = nullptr);
    bool HasSurface(const std::string& surfaceId) const;
    void RemoveSurface(const std::string& surfaceId);
    void Dispose();

    size_t GetSurfaceCount() const
    {
        return surfaces_.size();
    }
    std::vector<std::string> GetSurfaceIds() const
    {
        return surfaceOrder_;
    }
    /**
     * Get the latest created SurfaceSlot
     * @return Pointer to the latest SurfaceSlot, or nullptr if no surface exists
     */
    SurfaceSlot* GetLatestSurface();

    /**
     * Set the content handle for display
     * @param handle The content handle
     */
    void SetContentHandle(A2UINodeContentHandle handle);

    /**
     * Get the current content handle
     * @return The current content handle
     */
    A2UINodeContentHandle GetContentHandle() const
    {
        return contentHandle_;
    }

    /**
     * Remove the latest surface (go back to previous surface)
     * @return true if successfully removed, false if no surface exists
     */
    bool Back();

    /**
     * Get the current latest surface ID
     * @return The latest surface ID, or empty string if no surface exists
     */
    std::string GetLatestSurfaceId() const
    {
        return latestSurfaceId_;
    }

    /**
     * Configure whether root should be forced to fill host area (100% x 100%).
     */
    void SetRootFillMode(bool forceFill);
    void SetFontSizeScale(float scale);
    float GetFontSizeScale() const
    {
        return fontSizeScale_;
    }
    void SetApiVersion(int32_t apiVersion);
    int32_t GetApiVersion() const
    {
        return apiVersion_;
    }

    /**
     * Set the render ID
     * @param renderId The render ID
     */
    void SetRenderId(int32_t renderId)
    {
        renderId_ = renderId;
    }

    /**
     * Get the render ID
     * @return The render ID
     */
    int32_t GetRenderId() const
    {
        return renderId_;
    }

    /**
     * Update theme mode for all surfaces
     * Traverses surfaces in reverse order (from latest to earliest)
     * @param mode The new theme mode
     */
    void UpdateThemeMode(ThemeMode mode);

    /**
     * Update breakpoint for all surfaces
     * Traverses surfaces in reverse order (from latest to earliest)
     * @param breakpoint The new breakpoint
     */
    void UpdateBreakpoint(Breakpoint breakpoint);

    /**
     * Get the current theme context
     * @return Current theme context
     */
    const ThemeContext& GetThemeContext() const
    {
        return themeContext_;
    }

private:
    std::unordered_map<std::string, SurfaceSlot> surfaces_;
    std::string latestSurfaceId_;                   // Track the latest created surface
    std::vector<std::string> surfaceOrder_;         // Track creation order
    A2UINodeContentHandle contentHandle_ = nullptr; // Current contentHandle for display
    int32_t renderId_ = -1;                         // Render ID for tracking which RenderSlot this belongs to
    bool forceRootFill_ = false;
    float fontSizeScale_ = 1.0F;
    int32_t apiVersion_ = 0;
    ThemeContext themeContext_; // Current theme context
};

} // namespace NativeModule

#endif // NATIVE_SURFACE_MANAGER_H
