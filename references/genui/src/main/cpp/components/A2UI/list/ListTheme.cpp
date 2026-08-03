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

#include "ListTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

ListTheme::ListTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void ListTheme::OnConfigChange(const ThemeContext& context)
{
    InitializeAllProperties();
}

void ListTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_INFO,
        "ListTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    // TODO: Initialize all theme properties based on currentContext_
}

} // namespace NativeModule
