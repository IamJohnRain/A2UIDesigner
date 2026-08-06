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

#include "RenderSlot.h"

#include "utils/LogA2UI.h"
#include "utils/SystemProperties.h"

#include "SurfaceManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

RenderSlot::RenderSlot(int32_t renderId) : renderId_(renderId), surfaceManager_(std::make_shared<SurfaceManager>())
{
    LOG_A2UI(LOG_INFO, "RenderSlot: Constructor - renderId=%{public}d", renderId_);
    // Set renderId to SurfaceManager
    surfaceManager_->SetRenderId(renderId_);
}

RenderSlot::~RenderSlot()
{
    LOG_A2UI(LOG_INFO, "RenderSlot: Destructor - renderId=%{public}d", renderId_);
}

void RenderSlot::SetContentHandle(A2UINodeContentHandle handle)
{
    LOG_A2UI(LOG_INFO, "RenderSlot::SetContentHandle - renderId=%{public}d, delegating to SurfaceManager", renderId_);
    if (surfaceManager_ != nullptr) {
        surfaceManager_->SetContentHandle(handle);
    }
}

A2UINodeContentHandle RenderSlot::GetContentHandle() const
{
    if (surfaceManager_ != nullptr) {
        return surfaceManager_->GetContentHandle();
    }
    return nullptr;
}

void RenderSlot::SetRootFillMode(bool forceFill)
{
    if (surfaceManager_ != nullptr) {
        surfaceManager_->SetRootFillMode(forceFill);
    }
}

void RenderSlot::SetFontSizeScale(float scale)
{
    if (surfaceManager_ != nullptr) {
        surfaceManager_->SetFontSizeScale(scale);
    }
}

float RenderSlot::GetFontSizeScale() const
{
    if (surfaceManager_ != nullptr) {
        return surfaceManager_->GetFontSizeScale();
    }
    return 1.0F;
}

void RenderSlot::SetApiVersion(int32_t apiVersion)
{
    SystemProperties::GetInstance().SetApiVersion(apiVersion);
}

int32_t RenderSlot::GetApiVersion() const
{
    return SystemProperties::GetInstance().GetApiVersion();
}

void RenderSlot::Dispose()
{
    LOG_A2UI(LOG_INFO, "RenderSlot::Dispose - renderId=%{public}d", renderId_);

    if (surfaceManager_ != nullptr) {
        surfaceManager_->Dispose();
        surfaceManager_.reset();
    }
}

} // namespace NativeModule
