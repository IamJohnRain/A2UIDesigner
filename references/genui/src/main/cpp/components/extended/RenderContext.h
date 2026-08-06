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

#ifndef A2UI_EXTENDED_RENDER_CONTEXT_H
#define A2UI_EXTENDED_RENDER_CONTEXT_H

#include <cstdint>
#include <memory>
#include <string>

#include "theme/ThemeBase.h"

namespace NativeModule {

constexpr int32_t MIN_API_VERSION_FONT_SCALE = 18;

class BindingEngine;
class DataModel;
class Catalog;

struct RenderContext {
    int32_t renderId = -1;
    std::string surfaceId;
    std::shared_ptr<BindingEngine> bindingEngine;
    std::shared_ptr<DataModel> dataModel;
    std::shared_ptr<Catalog> catalog;
    float fontSizeScale = 1.0F;
    int32_t apiVersion = 0;
    ThemeMode colorMode = ThemeMode::LIGHT;

    static RenderContext Create(int32_t renderIdVal, const std::string& surfaceIdVal,
        const std::shared_ptr<BindingEngine>& bindingEngineVal, const std::shared_ptr<Catalog>& catalogVal,
        float fontSizeScaleVal = 1.0F, int32_t apiVersionVal = 0, ThemeMode colorModeVal = ThemeMode::LIGHT);
    bool IsValid() const;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_RENDER_CONTEXT_H
