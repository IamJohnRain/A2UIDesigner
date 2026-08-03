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

#include "SurfaceManager.h"

#include <algorithm>

#include "data/BindingEngine.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"
#include "utils/NapiUtils.h"

namespace NativeModule {

namespace {

constexpr char COLOR_MODE_VARIABLE_NAME[] = "__colorMode";
constexpr char WIDTH_BREAKPOINT_VARIABLE_NAME[] = "__widthBreakpoint";

void NotifyGlobalExpressionVariableChanged(SurfaceSlot& surfaceSlot, const std::string& variableName)
{
    auto bindingEngine = surfaceSlot.GetBindingEngine();
    if (bindingEngine != nullptr) {
        bindingEngine->NotifyGlobalVariableChanged(variableName);
    }
}

} // namespace

SurfaceSlot* SurfaceManager::FindSurface(const std::string& surfaceId)
{
    auto iter = surfaces_.find(surfaceId);
    if (iter == surfaces_.end()) {
        LOG_A2UI(LOG_WARN,
            "SurfaceManager::FindSurface - renderId=%{public}d, surfaceId=%{public}s not found, "
            "surfaceCount=%{public}zu, latestSurfaceId=%{public}s",
            renderId_, surfaceId.c_str(), surfaces_.size(), latestSurfaceId_.c_str());
        return nullptr;
    }
    return &iter->second;
}

SurfaceSlot& SurfaceManager::CreateSurface(const std::string& surfaceId, A2UINodeContentHandle contentHandle)
{
    size_t countBefore = surfaces_.size();
    bool existed = surfaces_.find(surfaceId) != surfaces_.end();
    std::string previousLatestSurfaceId = latestSurfaceId_;
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::CreateSurface - renderId=%{public}d, surfaceId=%{public}s, existed=%{public}s, "
        "countBefore=%{public}zu, previousLatestSurfaceId=%{public}s, contentHandle=%{public}p",
        renderId_, surfaceId.c_str(), existed ? "true" : "false", countBefore, previousLatestSurfaceId.c_str(),
        contentHandle);

    SurfaceSlot& slot = surfaces_[surfaceId];
    if (existed) {
        slot.ClearRuntimeStateStore();
    }
    slot.SetSurfaceId(surfaceId);
    slot.SetRenderId(renderId_); // Set renderId to SurfaceSlot
    slot.SetForceRootFill(forceRootFill_);
    slot.SetFontSizeScale(fontSizeScale_);
    slot.SetApiVersion(apiVersion_);
    slot.InitializeThemeManager(themeContext_);

    // Add to creation order list
    surfaceOrder_.push_back(surfaceId);

    // If there was a previous latest surface, remove all host handles from the old surface.
    if (!latestSurfaceId_.empty()) {
        auto oldIter = surfaces_.find(latestSurfaceId_);
        if (oldIter != surfaces_.end() && oldIter->first != surfaceId) {
            oldIter->second.DismissActiveModal();
            oldIter->second.SetContentHandle(nullptr);
            LOG_A2UI(LOG_INFO, "CreateSurface: Removed content handle from old surface %{public}s",
                latestSurfaceId_.c_str());
        }
    }

    latestSurfaceId_ = surfaceId; // Update latest surfaceId

    // Set contentHandle to the new surface if provided
    if (contentHandle != nullptr) {
        slot.SetContentHandle(contentHandle);
        LOG_A2UI(LOG_INFO, "CreateSurface: Set content handle to new surface %{public}s", surfaceId.c_str());
    }

    LOG_A2UI(LOG_INFO,
        "SurfaceManager::CreateSurface - renderId=%{public}d, surfaceId=%{public}s created, countAfter=%{public}zu, "
        "latestSurfaceId=%{public}s, contentHandle=%{public}p",
        renderId_, surfaceId.c_str(), surfaces_.size(), latestSurfaceId_.c_str(), contentHandle_);

    return slot;
}

void SurfaceManager::SetContentHandle(A2UINodeContentHandle handle)
{
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::SetContentHandle - renderId=%{public}d, oldHandle=%{public}p, newHandle=%{public}p, "
        "surfaceCount=%{public}zu, latestSurfaceId=%{public}s",
        renderId_, contentHandle_, handle, surfaces_.size(), latestSurfaceId_.c_str());
    contentHandle_ = handle;

    // If we have a latest surface, set the contentHandle to it
    if (!latestSurfaceId_.empty()) {
        auto iter = surfaces_.find(latestSurfaceId_);
        if (iter != surfaces_.end()) {
            iter->second.SetContentHandle(handle);
            LOG_A2UI(LOG_INFO, "SurfaceManager::SetContentHandle - Set to latest surface %{public}s",
                latestSurfaceId_.c_str());
        } else {
            LOG_A2UI(LOG_WARN,
                "SurfaceManager::SetContentHandle - renderId=%{public}d, latestSurfaceId=%{public}s missing from map",
                renderId_, latestSurfaceId_.c_str());
        }
    } else {
        LOG_A2UI(LOG_INFO,
            "SurfaceManager::SetContentHandle - renderId=%{public}d, no active surface to receive handle", renderId_);
    }
}

void SurfaceManager::SetRootFillMode(bool forceFill)
{
    if (forceRootFill_ == forceFill) {
        return;
    }
    forceRootFill_ = forceFill;
    for (auto& pair : surfaces_) {
        pair.second.SetForceRootFill(forceFill);
    }
}

void SurfaceManager::SetFontSizeScale(float scale)
{
    fontSizeScale_ = scale > 0.0F ? scale : 1.0F;
    for (auto& pair : surfaces_) {
        pair.second.SetFontSizeScale(fontSizeScale_);
    }
}

void SurfaceManager::SetApiVersion(int32_t apiVersion)
{
    apiVersion_ = apiVersion;
    for (auto& pair : surfaces_) {
        pair.second.SetApiVersion(apiVersion_);
    }
}

bool SurfaceManager::HasSurface(const std::string& surfaceId) const
{
    return surfaces_.find(surfaceId) != surfaces_.end();
}

void SurfaceManager::RemoveSurface(const std::string& surfaceId)
{
    size_t countBefore = surfaces_.size();
    std::string previousLatestSurfaceId = latestSurfaceId_;
    auto iter = surfaces_.find(surfaceId);
    if (iter == surfaces_.end()) {
        LOG_A2UI(LOG_WARN,
            "SurfaceManager::RemoveSurface - renderId=%{public}d, surfaceId=%{public}s not found, "
            "countBefore=%{public}zu, "
            "latestSurfaceId=%{public}s",
            renderId_, surfaceId.c_str(), countBefore, previousLatestSurfaceId.c_str());
        return;
    }

    bool isLatestSurface = (latestSurfaceId_ == surfaceId);
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::RemoveSurface - renderId=%{public}d, surfaceId=%{public}s, isLatestSurface=%{public}s, "
        "countBefore=%{public}zu, latestSurfaceId=%{public}s",
        renderId_, surfaceId.c_str(), isLatestSurface ? "true" : "false", countBefore, previousLatestSurfaceId.c_str());

    // Remove from surfaces map
    iter->second.Dispose();
    surfaces_.erase(iter);

    // Remove from creation order list
    surfaceOrder_.erase(std::remove(surfaceOrder_.begin(), surfaceOrder_.end(), surfaceId), surfaceOrder_.end());

    // If we removed the latest surface and there are other surfaces, transfer contentHandle
    if (isLatestSurface && !surfaceOrder_.empty()) {
        // Get the new latest surfaceId (last element in order list)
        std::string newLatestSurfaceId = surfaceOrder_.back();
        latestSurfaceId_ = newLatestSurfaceId;

        // Transfer contentHandle to the new latest surface
        if (contentHandle_ != nullptr) {
            auto newLatestIter = surfaces_.find(newLatestSurfaceId);
            if (newLatestIter != surfaces_.end()) {
                newLatestIter->second.SetContentHandle(contentHandle_);
                LOG_A2UI(LOG_INFO, "RemoveSurface: Transferred content handle to new latest surface %{public}s",
                    newLatestSurfaceId.c_str());
            }
        }
    } else if (isLatestSurface) {
        // Latest surface removed and no other surfaces left
        latestSurfaceId_.clear();
    }

    LOG_A2UI(LOG_INFO, "RemoveSurface: Removed surface %{public}s, new latest is %{public}s", surfaceId.c_str(),
        latestSurfaceId_.c_str());
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::RemoveSurface - renderId=%{public}d finished, removedSurfaceId=%{public}s, "
        "countAfter=%{public}zu, "
        "newLatestSurfaceId=%{public}s",
        renderId_, surfaceId.c_str(), surfaces_.size(), latestSurfaceId_.c_str());
}

void SurfaceManager::Dispose()
{
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::Dispose - renderId=%{public}d, cleaning up all surfaces, surfaceCount=%{public}zu, "
        "latestSurfaceId=%{public}s, contentHandle=%{public}p",
        renderId_, surfaces_.size(), latestSurfaceId_.c_str(), contentHandle_);

    for (auto& pair : surfaces_) {
        pair.second.Dispose();
    }
    surfaces_.clear();
    surfaceOrder_.clear();
    latestSurfaceId_.clear();
    contentHandle_ = nullptr;
}

SurfaceSlot* SurfaceManager::GetLatestSurface()
{
    if (latestSurfaceId_.empty()) {
        return nullptr;
    }

    auto iter = surfaces_.find(latestSurfaceId_);
    if (iter == surfaces_.end()) {
        return nullptr;
    }

    return &iter->second;
}

bool SurfaceManager::Back()
{
    if (latestSurfaceId_.empty()) {
        LOG_A2UI(LOG_INFO, "SurfaceManager::Back - renderId=%{public}d, no surface to go back to", renderId_);
        return false;
    }

    std::string surfaceIdToRemove = latestSurfaceId_;
    LOG_A2UI(LOG_INFO,
        "SurfaceManager::Back - renderId=%{public}d, going back from surfaceId=%{public}s, surfaceCount=%{public}zu",
        renderId_, surfaceIdToRemove.c_str(), surfaces_.size());

    // RemoveSurface will automatically transfer contentHandle to the new latest surface
    RemoveSurface(surfaceIdToRemove);

    LOG_A2UI(LOG_INFO,
        "SurfaceManager::Back - renderId=%{public}d finished, removedSurfaceId=%{public}s, "
        "currentLatestSurfaceId=%{public}s, "
        "surfaceCount=%{public}zu",
        renderId_, surfaceIdToRemove.c_str(), latestSurfaceId_.c_str(), surfaces_.size());

    return true;
}

void SurfaceManager::UpdateThemeMode(ThemeMode mode)
{
    LOG_A2UI(LOG_INFO, "SurfaceManager::UpdateThemeMode - renderId=%{public}d, mode=%{public}d", renderId_,
        static_cast<int32_t>(mode));

    // Update theme context
    themeContext_.colorMode = mode;

    // Traverse surfaces in reverse order (from latest to earliest)
    for (auto it = surfaceOrder_.rbegin(); it != surfaceOrder_.rend(); ++it) {
        auto iter = surfaces_.find(*it);
        if (iter == surfaces_.end()) {
            continue;
        }

        auto themeManager = iter->second.GetThemeManager();
        if (themeManager == nullptr) {
            continue;
        }

        themeManager->UpdateThemeMode(mode);
        themeManager->NotifyThemeChange(&iter->second);
        NotifyGlobalExpressionVariableChanged(iter->second, COLOR_MODE_VARIABLE_NAME);
    }
}

void SurfaceManager::UpdateBreakpoint(Breakpoint breakpoint)
{
    LOG_A2UI(LOG_INFO, "SurfaceManager::UpdateBreakpoint - renderId=%{public}d, breakpoint=%{public}d", renderId_,
        static_cast<int32_t>(breakpoint));

    themeContext_.breakpoint = breakpoint;

    for (auto it = surfaceOrder_.rbegin(); it != surfaceOrder_.rend(); ++it) {
        auto iter = surfaces_.find(*it);
        if (iter == surfaces_.end()) {
            continue;
        }

        auto themeManager = iter->second.GetThemeManager();
        if (themeManager == nullptr) {
            continue;
        }

        themeManager->UpdateBreakpoint(breakpoint);
        themeManager->NotifyThemeChange(&iter->second);
        NotifyGlobalExpressionVariableChanged(iter->second, WIDTH_BREAKPOINT_VARIABLE_NAME);
    }
}

} // namespace NativeModule
