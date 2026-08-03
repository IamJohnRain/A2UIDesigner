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

#ifndef A2UI_THEME_MANAGER_H
#define A2UI_THEME_MANAGER_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "ThemeBase.h"

namespace NativeModule {

class SurfaceSlot;
class ThemeBase;

/**
 * @brief Theme Manager - Manages theme configuration for a specific SurfaceSlot
 *
 * Each SurfaceSlot has its own ThemeManager instance (one-to-one with SurfaceSlot).
 * This class manages:
 * 1. Current theme context (mode, breakpoint, brand color)
 * 2. All component themes (ButtonTheme, TextTheme, etc.) - shared by all components in this surface
 * 3. Notifies all components when theme context changes via callbacks
 */
class ThemeManager {
public:
    /**
     * @brief Constructor
     * @param surfaceId The surface ID this ThemeManager belongs to
     * @param renderId The render ID this ThemeManager belongs to
     * @param context The initial theme context
     */
    ThemeManager(const std::string& surfaceId, int32_t renderId, const ThemeContext& context);
    ~ThemeManager() = default;

    // Disable copy and move
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    ThemeManager(ThemeManager&&) = delete;
    ThemeManager& operator=(ThemeManager&&) = delete;

    /**
     * @brief Update theme mode (light/dark) and notify all components
     * @param mode The new theme mode
     */
    void UpdateThemeMode(ThemeMode mode);

    /**
     * @brief Update breakpoint based on window width and notify all components
     * @param breakpoint The new breakpoint
     */
    void UpdateBreakpoint(Breakpoint breakpoint);

    /**
     * @brief Update brand color and notify all components
     * @param color The brand color value
     */
    void UpdateBrandColor(uint32_t color);

    /**
     * @brief Clear brand color and notify all components
     */
    void ClearBrandColor();

    /**
     * @brief Replace surface-level theme fields in current context and derive brandColor from them.
     * @param themeContext Theme context carrying surface-level theme fields.
     */
    void SetComponentTheme(const ThemeContext& themeContext);

    /**
     * @brief Get the current theme context
     * @return Current theme context
     */
    const ThemeContext& GetContext() const
    {
        return context_;
    }

    /**
     * @brief Get the render ID
     * @return The render ID
     */
    int32_t GetRenderId() const
    {
        return renderId_;
    }

    /**
     * @brief Get the surface ID
     * @return The surface ID
     */
    const std::string& GetSurfaceId() const
    {
        return surfaceId_;
    }

    /**
     * @brief Get a component theme by component type (lazy loading)
     * If theme exists in cache, return it; otherwise create and cache it.
     * @param componentType Component type (e.g., "Button", "Text") from GetType()
     * @return Shared pointer to the theme, or nullptr if type is not supported
     */
    std::shared_ptr<ThemeBase> GetTheme(const std::string& componentType);

    /**
     * @brief Notify theme change to components
     * This should be called by SurfaceSlot when theme context changes
     */
    void NotifyThemeChange(SurfaceSlot* surfaceSlot);

    /**
     * @brief Create a theme object for the given component type
     * @param componentType Component type (e.g., "Button", "Text")
     * @param context The theme context to initialize the theme with
     * @return Shared pointer to the created theme, or nullptr if type is not supported
     */
    std::shared_ptr<ThemeBase> CreateTheme(const std::string& componentType, const ThemeContext& context);

private:
    void RefreshBrandColorFromContext(bool forceClearWhenUnresolved);

    std::string surfaceId_;
    int32_t renderId_;
    ThemeContext context_;
    std::unordered_map<std::string, std::shared_ptr<ThemeBase>> themes_;
};

} // namespace NativeModule

#endif // A2UI_THEME_MANAGER_H
