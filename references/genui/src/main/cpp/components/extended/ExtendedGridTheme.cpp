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

#include "ExtendedGridTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr char TWO_COLUMN_TEMPLATE[] = "1fr 1fr";
constexpr char THREE_COLUMN_TEMPLATE[] = "1fr 1fr 1fr";
constexpr char FIVE_COLUMN_TEMPLATE[] = "1fr 1fr 1fr 1fr 1fr";

} // namespace

ExtendedGridTheme::ExtendedGridTheme(const ThemeContext& context) : ExtendedCommonTheme(context)
{
    InitializeAllProperties();
}

void ExtendedGridTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

const std::string& ExtendedGridTheme::GetColumnsTemplate() const
{
    return columnsTemplate_;
}

void ExtendedGridTheme::InitializeAllProperties()
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedGridTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    switch (currentContext_.breakpoint) {
        case Breakpoint::XS:
        case Breakpoint::SM:
            columnsTemplate_ = TWO_COLUMN_TEMPLATE;
            return;
        case Breakpoint::MD:
            columnsTemplate_ = THREE_COLUMN_TEMPLATE;
            return;
        case Breakpoint::LG:
        case Breakpoint::XL:
            columnsTemplate_ = FIVE_COLUMN_TEMPLATE;
            return;
        default:
            columnsTemplate_ = TWO_COLUMN_TEMPLATE;
            return;
    }
}

} // namespace NativeModule
