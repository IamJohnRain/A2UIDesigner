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

#include "components/custom/CustomComponent.h"
#include "data/BindingEngine.h"
#include "utils/JsonAdapter.h"

#include "RenderManager.h"
#include "SurfaceSlot.h"

namespace NativeModule {

namespace {

bool CloneJsonValue(const JsonValue& input, JsonValue& output)
{
    if (!input.IsValid()) {
        return false;
    }
    std::unique_ptr<JsonAdapter> adapter = JsonAdapter::Clone(input);
    if (adapter == nullptr) {
        return false;
    }
    output = adapter->GetRoot();
    return output.IsValid();
}

} // namespace

JsonValue CustomComponent::GetCustomProperty(const std::string& propertyName) const
{
    auto iter = properties_.find(propertyName);
    if (iter == properties_.end()) {
        return JsonValue();
    }
    JsonValue clonedValue;
    if (!CloneJsonValue(iter->second, clonedValue)) {
        return JsonValue();
    }
    return clonedValue;
}

bool CustomComponent::SetRuntimeCustomProperty(const std::string& propertyName, const JsonValue& value)
{
    if (propertyName.empty() || !value.IsValid()) {
        return false;
    }

    JsonValue clonedValue;
    if (!CloneJsonValue(value, clonedValue)) {
        return false;
    }

    properties_[propertyName] = clonedValue;
    customPropertyNames_.insert(propertyName);
    descriptor_.customProps = BuildCustomProps();
    return true;
}

void CustomComponent::SyncCheckedToBoundDataModel(const std::string& bindingPath, bool value)
{
    if (bindingPath.empty()) {
        return;
    }
    std::string surfaceId = GetSurfaceId();
    if (surfaceId.empty()) {
        surfaceId = "default";
    }
    SurfaceSlot* surfaceSlot = RenderManager::GetInstance().FindSurface(surfaceId);
    if (surfaceSlot == nullptr) {
        return;
    }
    std::shared_ptr<BindingEngine> bindingEngine = surfaceSlot->GetBindingEngine();
    if (bindingEngine == nullptr) {
        return;
    }
    std::unique_ptr<JsonAdapter> valueAdapter = JsonAdapter::CreateBool(value);
    if (valueAdapter == nullptr) {
        return;
    }
    bindingEngine->UpdateDataModelByPath(surfaceId, bindingPath, valueAdapter->GetRoot());
}

} // namespace NativeModule
