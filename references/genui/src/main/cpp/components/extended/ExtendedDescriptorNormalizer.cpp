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

#include "ExtendedDescriptorNormalizer.h"

#include "utils/LogA2UI.h"

#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {} // namespace

NormalizedExtendedDescriptor ExtendedDescriptorNormalizer::Normalize(const JsonValue& descriptor)
{
    NormalizedExtendedDescriptor normalized;
    LOG_A2UI(LOG_DEBUG, "ExtendedDescriptorNormalizer::Normalize - start, valid=%{public}s, type=%{public}s",
        descriptor.IsValid() ? "true" : "false", descriptor.GetTypeName());
    normalized.adapter = JsonAdapter::Clone(descriptor);
    if (normalized.adapter == nullptr) {
        LOG_A2UI(LOG_ERROR, "ExtendedDescriptorNormalizer::Normalize - clone descriptor failed");
        return normalized;
    }

    normalized.descriptor = normalized.adapter->GetRoot();
    if (!normalized.descriptor.IsObject()) {
        LOG_A2UI(LOG_WARN, "ExtendedDescriptorNormalizer::Normalize - descriptor is not object, type=%{public}s",
            normalized.descriptor.GetTypeName());
        normalized.adapter.reset();
        normalized.descriptor = JsonValue();
        return normalized;
    }

    normalized.styles = normalized.descriptor.GetItem("styles");

    // DFX: validate styles structure
    if (normalized.styles.IsValid() && !normalized.styles.IsObject()) {
        normalized.validationIssues.push_back({ SCHEMA_ERROR_CODE_TYPE_MISMATCH,
            "Property styles expects object value, got type '" + std::string(normalized.styles.GetTypeName()) + "'",
            "styles" });
        LOG_A2UI(LOG_WARN, "ExtendedDescriptorNormalizer::Normalize - styles is not object, type=%{public}s",
            normalized.styles.GetTypeName());
    }

    LOG_A2UI(LOG_DEBUG,
        "ExtendedDescriptorNormalizer::Normalize - completed, componentId=%{public}s, component=%{public}s, "
        "hasStyles=%{public}s, validationIssues=%{public}zu",
        normalized.descriptor.GetString("id", "").c_str(), normalized.descriptor.GetString("component", "").c_str(),
        normalized.styles.IsValid() ? "true" : "false", normalized.validationIssues.size());

    return normalized;
}

} // namespace NativeModule
