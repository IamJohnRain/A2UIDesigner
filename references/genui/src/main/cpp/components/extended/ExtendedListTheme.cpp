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

#include "ExtendedListTheme.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr int32_t SINGLE_COLUMN_LANES = 1;
constexpr int32_t DOUBLE_COLUMN_LANES = 2;
constexpr int32_t TRIPLE_COLUMN_LANES = 3;

} // namespace

ExtendedListTheme::ExtendedListTheme(const ThemeContext& context) : ExtendedCommonTheme(context)
{
    InitializeAllProperties();
}

void ExtendedListTheme::OnConfigChange(const ThemeContext& context)
{
    currentContext_ = context;
    InitializeAllProperties();
}

int32_t ExtendedListTheme::GetLanes() const
{
    return lanes_;
}

void ExtendedListTheme::InitializeAllProperties()
{
    LOG_A2UI(LOG_DEBUG,
        "ExtendedListTheme::InitializeAllProperties - hasBrandColor=%{public}s, brandColor=0x%{public}x, "
        "colorMode=%{public}d, breakpoint=%{public}d",
        currentContext_.hasBrandColor ? "true" : "false", currentContext_.brandColor,
        static_cast<int32_t>(currentContext_.colorMode), static_cast<int32_t>(currentContext_.breakpoint));

    switch (currentContext_.breakpoint) {
        case Breakpoint::XS:
        case Breakpoint::SM:
            lanes_ = SINGLE_COLUMN_LANES;
            return;
        case Breakpoint::MD:
            lanes_ = DOUBLE_COLUMN_LANES;
            return;
        case Breakpoint::LG:
        case Breakpoint::XL:
            lanes_ = TRIPLE_COLUMN_LANES;
            return;
        default:
            lanes_ = SINGLE_COLUMN_LANES;
            return;
    }
}

} // namespace NativeModule
