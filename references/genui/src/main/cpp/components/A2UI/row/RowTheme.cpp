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

#include "RowTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {
namespace {
constexpr float ROW_DEFAULT_SPACE = 16.0F;
}

RowTheme::RowTheme(const ThemeContext& context) : ThemeBase(context)
{
    InitializeAllProperties();
}

void RowTheme::OnConfigChange(const ThemeContext& context)
{
    InitializeAllProperties();
}

void RowTheme::InitializeAllProperties()
{
    // DFX: Print theme context information
    LOG_A2UI(LOG_INFO,
        "RowTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    // TODO: Initialize all theme properties based on currentContext_
}

A2UIVerticalAlignment RowTheme::ResolveAlignItems(const std::string& align)
{
    if (align == "start") {
        return A2UIVerticalAlignment::TOP;
    }
    if (align == "center") {
        return A2UIVerticalAlignment::CENTER;
    }
    if (align == "end") {
        return A2UIVerticalAlignment::BOTTOM;
    }
    // Invalid token fallback.
    return A2UIVerticalAlignment::TOP;
}

A2UIFlexAlignment RowTheme::ResolveJustifyContent(const std::string& justify)
{
    if (justify == "start") {
        return A2UIFlexAlignment::START;
    }
    if (justify == "center") {
        return A2UIFlexAlignment::CENTER;
    }
    if (justify == "end") {
        return A2UIFlexAlignment::END;
    }
    if (justify == "spaceAround") {
        return A2UIFlexAlignment::SPACE_AROUND;
    }
    if (justify == "spaceBetween") {
        return A2UIFlexAlignment::SPACE_BETWEEN;
    }
    if (justify == "spaceEvenly") {
        return A2UIFlexAlignment::SPACE_EVENLY;
    }
    // Invalid token fallback.
    return A2UIFlexAlignment::START;
}

float RowTheme::GetDefaultSpace()
{
    return ROW_DEFAULT_SPACE;
}

} // namespace NativeModule
