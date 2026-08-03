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

#ifndef A2UI_DISPLAY_DENSITY_UTILS_H
#define A2UI_DISPLAY_DENSITY_UTILS_H

#include <cstdint>
#include <map>

namespace NativeModule {

struct DisplayDensityInfo {
    float densityPixels = 0.0F;
    float fpToVpScale = 0.0F;
};

class DisplayDensityUtils final {
public:
    static DisplayDensityUtils& GetInstance();

    void SetDisplayDensity(int32_t renderId, float densityPixels, float fpToVpScale = 0.0F);
    void ClearDisplayDensity(int32_t renderId);

    float ConvertPxToVp(int32_t renderId, float px) const;
    float ConvertFpToVp(int32_t renderId, float fp) const;

private:
    DisplayDensityUtils() = default;
    ~DisplayDensityUtils() = default;
    DisplayDensityUtils(const DisplayDensityUtils&) = delete;
    DisplayDensityUtils& operator=(const DisplayDensityUtils&) = delete;

    const DisplayDensityInfo* FindDensity(int32_t renderId) const;

    std::map<int32_t, DisplayDensityInfo> densityByRenderId_;
};

} // namespace NativeModule

#endif // A2UI_DISPLAY_DENSITY_UTILS_H
