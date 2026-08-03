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

#include "DisplayDensityUtils.h"

#include <cmath>

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

bool IsPositiveFinite(float value)
{
    return std::isfinite(value) && value > 0.0F;
}

} // namespace

DisplayDensityUtils& DisplayDensityUtils::GetInstance()
{
    static DisplayDensityUtils instance;
    return instance;
}

void DisplayDensityUtils::SetDisplayDensity(int32_t renderId, float densityPixels, float fpToVpScale)
{
    if (renderId < 0) {
        LOG_A2UI(LOG_WARN, "DisplayDensityUtils::SetDisplayDensity: invalid renderId=%{public}d", renderId);
        return;
    }
    if (!IsPositiveFinite(densityPixels)) {
        LOG_A2UI(LOG_WARN, "DisplayDensityUtils::SetDisplayDensity: invalid densityPixels=%{public}f", densityPixels);
        return;
    }

    DisplayDensityInfo info { densityPixels, 0.0F };
    if (IsPositiveFinite(fpToVpScale)) {
        info.fpToVpScale = fpToVpScale;
    } else if (fpToVpScale != 0.0F) {
        LOG_A2UI(LOG_WARN, "DisplayDensityUtils::SetDisplayDensity: invalid fpToVpScale=%{public}f", fpToVpScale);
    }
    densityByRenderId_[renderId] = info;
    LOG_A2UI(LOG_INFO,
        "DisplayDensityUtils::SetDisplayDensity: renderId=%{public}d, density=%{public}f, fpToVpScale=%{public}f",
        renderId, densityPixels, info.fpToVpScale);
}

void DisplayDensityUtils::ClearDisplayDensity(int32_t renderId)
{
    densityByRenderId_.erase(renderId);
}

const DisplayDensityInfo* DisplayDensityUtils::FindDensity(int32_t renderId) const
{
    auto it = densityByRenderId_.find(renderId);
    if (it != densityByRenderId_.end()) {
        return &(it->second);
    }
    return nullptr;
}

float DisplayDensityUtils::ConvertPxToVp(int32_t renderId, float px) const
{
    const DisplayDensityInfo* info = FindDensity(renderId);
    if (info == nullptr || !IsPositiveFinite(info->densityPixels)) {
        LOG_A2UI(LOG_WARN, "ConvertPxToVp: density not found, renderId=%{public}d, px=%{public}f", renderId, px);
        return px;
    }
    float resolvedValue = px / info->densityPixels;
    if (!std::isfinite(resolvedValue) || resolvedValue < 0.0F) {
        return px;
    }
    return resolvedValue;
}

float DisplayDensityUtils::ConvertFpToVp(int32_t renderId, float fp) const
{
    const DisplayDensityInfo* info = FindDensity(renderId);
    if (info == nullptr || !IsPositiveFinite(info->fpToVpScale)) {
        LOG_A2UI(LOG_WARN, "ConvertFpToVp: fp scale not found, renderId=%{public}d, fp=%{public}f", renderId, fp);
        return fp;
    }
    float resolvedValue = fp * info->fpToVpScale;
    if (!std::isfinite(resolvedValue) || resolvedValue < 0.0F) {
        return fp;
    }
    return resolvedValue;
}

} // namespace NativeModule
