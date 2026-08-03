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

#ifndef A2UI_THEME_BASE_H
#define A2UI_THEME_BASE_H

#include <cstdint>
#include <string>

namespace NativeModule {

/**
 * @brief Color theme mode enumeration
 */
enum class ThemeMode {
    LIGHT, // Light mode
    DARK,  // Dark mode
};

/**
 * @brief HarmonyOS breakpoint system
 * Based on window width in vp
 */
enum class Breakpoint {
    XS, // (0, 320) vp
    SM, // [320, 600) vp
    MD, // [600, 840) vp
    LG, // [840, 1440) vp
    XL, // [1440, +inf) vp
};

/**
 * @brief Theme context containing all factors affecting theme values
 *
 * This structure combines:
 * - Color-related: theme mode (light/dark), brand color
 * - Value-related: breakpoint
 * - Surface theme fields from createSurface.theme
 */
struct ThemeContext {
    // === Color related ===
    ThemeMode colorMode = ThemeMode::LIGHT; // Light or dark mode
    bool hasBrandColor = false;             // Whether brand color is set
    uint32_t brandColor = 0;                // Brand color value

    // Surface-level theme fields derived from createSurface.theme
    bool hasPrimaryColor = false;
    uint32_t primaryColorArgb = 0;
    bool hasDarkPrimaryColor = false;
    uint32_t darkPrimaryColorArgb = 0;
    std::string iconUrl;
    std::string agentDisplayName;

    // === Value related ===
    Breakpoint breakpoint = Breakpoint::SM; // HarmonyOS breakpoint

    bool HasComponentThemeValue() const
    {
        return hasPrimaryColor || hasDarkPrimaryColor || !iconUrl.empty() || !agentDisplayName.empty();
    }
};

/**
 * @brief Base class for all component themes
 *
 * This class defines the interface for theme configuration changes.
 * All component-specific themes should inherit from this class and
 * implement the OnConfigChange method to handle theme context switching.
 *
 * Theme storage design:
 * - Color values: stored as arrays indexed by ThemeMode (brand color has highest priority)
 * - Numeric values: stored as arrays indexed by Breakpoint
 */
class ThemeBase {
public:
    /**
     * @brief Default constructor with default context
     */
    ThemeBase() = default;

    /**
     * @brief Constructor with initial context
     * @param context The initial theme context
     */
    explicit ThemeBase(const ThemeContext& context) : currentContext_(context) {}

    virtual ~ThemeBase() = default;

    /**
     * @brief Update theme mode (light/dark) and trigger OnConfigChange
     * @param mode The new theme mode
     */
    void UpdateThemeMode(ThemeMode mode);

    /**
     * @brief Update breakpoint and trigger OnConfigChange
     * @param breakpoint The new breakpoint
     */
    void UpdateBreakpoint(Breakpoint breakpoint);

    /**
     * @brief Update brand color and trigger OnConfigChange
     * @param color The brand color value
     */
    void UpdateBrandColor(uint32_t color);

    /**
     * @brief Clear brand color and trigger OnConfigChange
     */
    void ClearBrandColor();

    /**
     * @brief Update all context fields at once and trigger OnConfigChange
     * @param context The new theme context
     */
    void UpdateContext(const ThemeContext& context);

    /**
     * @brief Called when theme configuration changes
     * Subclasses should override this to apply the new context to their specific theme values.
     * @param context The new theme context containing mode, breakpoint, brand color, etc.
     */
    virtual void OnConfigChange(const ThemeContext& context) {}

    /**
     * @brief Get the current theme context
     * @return Current theme context
     */
    const ThemeContext& GetContext() const
    {
        return currentContext_;
    }

protected:
    ThemeContext currentContext_;
};

} // namespace NativeModule

#endif // A2UI_THEME_BASE_H
