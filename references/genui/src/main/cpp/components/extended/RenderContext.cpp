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

RenderContext RenderContext::Create(int32_t renderId, const std::string& surfaceId,
    const std::shared_ptr<BindingEngine>& bindingEngine, const std::shared_ptr<Catalog>& catalog, float fontSizeScale,
    int32_t apiVersion, ThemeMode colorMode)
{
    LOG_A2UI(LOG_DEBUG,
        "RenderContext::Create - renderId=%{public}d, surfaceId=%{public}s, hasBindingEngine=%{public}s, "
        "hasCatalog=%{public}s",
        renderId, surfaceId.c_str(), bindingEngine != nullptr ? "true" : "false",
        catalog != nullptr ? "true" : "false");
    RenderContext context;
    context.renderId = renderId;
    context.surfaceId = surfaceId;
    context.bindingEngine = bindingEngine;
    context.catalog = catalog;
    context.fontSizeScale = fontSizeScale > 0.0F ? fontSizeScale : 1.0F;
    context.apiVersion = apiVersion;
    context.colorMode = colorMode;
    if (bindingEngine != nullptr && !surfaceId.empty()) {
        context.dataModel = bindingEngine->GetOrCreateDataModel(surfaceId);
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
