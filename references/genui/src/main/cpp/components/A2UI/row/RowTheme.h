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

#ifndef A2UI_ROW_THEME_H
#define A2UI_ROW_THEME_H

#include <string>

#include "theme/ThemeBase.h"

#include "ArkUINodeApiAdapter.h"

namespace NativeModule {

/**
 * @brief Row component theme configuration
 */
class RowTheme : public ThemeBase {
public:
    explicit RowTheme(const ThemeContext& context);
    ~RowTheme() override = default;

    void OnConfigChange(const ThemeContext& context) override;

    static A2UIVerticalAlignment ResolveAlignItems(const std::string& align);
    static A2UIFlexAlignment ResolveJustifyContent(const std::string& justify);
    static float GetDefaultSpace();

private:
    /**
     * @brief Initialize all theme properties based on context
     */
    void InitializeAllProperties();
};

} // namespace NativeModule

#endif // A2UI_ROW_THEME_H
