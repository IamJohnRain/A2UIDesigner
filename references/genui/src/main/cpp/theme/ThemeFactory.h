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

#ifndef A2UI_THEME_FACTORY_H
#define A2UI_THEME_FACTORY_H

#include <memory>
#include <string>

#include "ThemeBase.h"

namespace NativeModule {

/**
 * @brief Theme Factory - Creates theme objects based on component type
 *
 * This factory follows the same pattern as NativeComponentFactory,
 * using a static map of builder functions to create themes.
 */
class ThemeFactory final {
public:
    /**
     * @brief Create a theme for the given component type
     * @param componentType Component type (e.g., "Button", "Text")
     * @param context The theme context to initialize the theme with
     * @return Shared pointer to the created theme, or nullptr if type is not supported
     */
    static std::shared_ptr<ThemeBase> CreateTheme(const std::string& componentType, const ThemeContext& context);

private:
    ThemeFactory() = default;
    ~ThemeFactory() = default;
    ThemeFactory(const ThemeFactory&) = delete;
    ThemeFactory& operator=(const ThemeFactory&) = delete;
};

} // namespace NativeModule

#endif // A2UI_THEME_FACTORY_H
