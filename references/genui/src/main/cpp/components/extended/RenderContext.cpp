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

#include "RenderContext.h"

#include "catalog/Catalog.h"
#include "data/BindingEngine.h"
#include "data/DataModel.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

RenderContext RenderContext::Create(int32_t renderIdVal, const std::string& surfaceIdVal,
    const std::shared_ptr<BindingEngine>& bindingEngineVal, const std::shared_ptr<Catalog>& catalogVal,
    float fontSizeScaleVal, int32_t apiVersionVal, ThemeMode colorModeVal)
{
    LOG_A2UI(LOG_DEBUG,
        "RenderContext::Create - renderId=%{public}d, surfaceId=%{public}s, hasBindingEngine=%{public}s, "
        "hasCatalog=%{public}s",
        renderIdVal, surfaceIdVal.c_str(), bindingEngineVal != nullptr ? "true" : "false",
        catalogVal != nullptr ? "true" : "false");
    RenderContext context;
    context.renderId = renderIdVal;
    context.surfaceId = surfaceIdVal;
    context.bindingEngine = bindingEngineVal;
    context.catalog = catalogVal;
    context.fontSizeScale = fontSizeScaleVal > 0.0F ? fontSizeScaleVal : 1.0F;
    context.apiVersion = apiVersionVal;
    context.colorMode = colorModeVal;
    if (bindingEngineVal != nullptr && !surfaceIdVal.empty()) {
        context.dataModel = bindingEngineVal->GetOrCreateDataModel(surfaceIdVal);
    }
    LOG_A2UI(LOG_DEBUG, "RenderContext::Create - completed, valid=%{public}s, hasDataModel=%{public}s",
        context.IsValid() ? "true" : "false", context.dataModel != nullptr ? "true" : "false");
    return context;
}

bool RenderContext::IsValid() const
{
    return renderId >= 0 && !surfaceId.empty();
}

} // namespace NativeModule
