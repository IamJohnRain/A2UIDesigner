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

#include "Catalog.h"

#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

const char* CategoryToString(CatalogCategory category)
{
    switch (category) {
        case CatalogCategory::A2UI_STANDARD:
            return "A2UI_STANDARD";
        case CatalogCategory::OHOS_EXTENDS:
            return "OHOS_EXTENDS";
        default:
            return "UNKNOWN";
    }
}

const char* TypeToString(CatalogItemType type)
{
    switch (type) {
        case CatalogItemType::PROTOCOL_MESSAGE:
            return "PROTOCOL_MESSAGE";
        case CatalogItemType::COMPONENT:
            return "COMPONENT";
        case CatalogItemType::LOCAL_FUNCTION:
            return "LOCAL_FUNCTION";
        default:
            return "UNKNOWN";
    }
}

std::string NormalizeVersionValue(const std::string& value, const std::string& fallback)
{
    return value.empty() ? fallback : value;
}

} // namespace

Catalog::Catalog(const std::string& id, const std::string& a2UIProtocolVersion) : id_(id)
{
    surfaceContext_.a2UIProtocolVersion = NormalizeVersionValue(a2UIProtocolVersion, DEFAULT_A2UI_PROTOCOL_VERSION);
    surfaceContext_.catalogId = id_;
}

const std::string& Catalog::GetCatalogId() const
{
    return id_;
}

const std::string& Catalog::GetA2UIProtocolVersion() const
{
    return surfaceContext_.a2UIProtocolVersion;
}

const SurfaceContext& Catalog::GetSurfaceContext() const
{
    return surfaceContext_;
}

const std::vector<std::shared_ptr<CatalogItem>>& Catalog::GetComponents() const
{
    return components_;
}

const std::vector<std::shared_ptr<CatalogItem>>& Catalog::GetFunctions() const
{
    return functions_;
}

void Catalog::AddComponent(const std::shared_ptr<CatalogItem>& component)
{
    if (component != nullptr) {
        components_.push_back(component);
    }
}

void Catalog::AddFunction(const std::shared_ptr<CatalogItem>& function)
{
    if (function != nullptr) {
        functions_.push_back(function);
    }
}

std::shared_ptr<CatalogItem> Catalog::GetCatalogItemByName(const std::string& name) const
{
    for (const auto& item : components_) {
        if (item != nullptr && item->GetName() == name) {
            return item;
        }
    }
    return nullptr;
}

std::shared_ptr<CatalogItem> Catalog::GetFunctionItemByName(const std::string& name) const
{
    for (const auto& item : functions_) {
        if (item != nullptr && item->GetName() == name) {
            return item;
        }
    }
    return nullptr;
}

bool Catalog::HasFunction(const std::string& name) const
{
    return GetFunctionItemByName(name) != nullptr;
}

bool Catalog::Equals(const Catalog& other) const
{
    if (id_ != other.id_ || components_.size() != other.components_.size() ||
        functions_.size() != other.functions_.size() ||
        surfaceContext_.a2UIProtocolVersion != other.surfaceContext_.a2UIProtocolVersion) {
        return false;
    }

    for (size_t i = 0; i < components_.size(); ++i) {
        const auto& current = components_[i];
        const auto& target = other.components_[i];
        if (current == nullptr || target == nullptr) {
            if (current != target) {
                return false;
            }
            continue;
        }

        if (current->GetName() != target->GetName() || current->GetCategory() != target->GetCategory() ||
            current->GetType() != target->GetType() || current->IsInnerNative() != target->IsInnerNative()) {
            return false;
        }
    }

    for (size_t i = 0; i < functions_.size(); ++i) {
        const auto& current = functions_[i];
        const auto& target = other.functions_[i];
        if (current == nullptr || target == nullptr) {
            if (current != target) {
                return false;
            }
            continue;
        }

        if (current->GetName() != target->GetName() || current->GetCategory() != target->GetCategory() ||
            current->GetType() != target->GetType() || current->IsInnerNative() != target->IsInnerNative()) {
            return false;
        }
    }
    return true;
}

void Catalog::DebugPrint() const
{
    LOG_A2UI(LOG_INFO, "========== Catalog Debug Info ==========");
    LOG_A2UI(LOG_INFO, "Catalog ID: %{public}s", id_.c_str());
    LOG_A2UI(LOG_INFO, "A2UI protocol version: %{public}s", surfaceContext_.a2UIProtocolVersion.c_str());
    LOG_A2UI(LOG_INFO, "Components count: %{public}zu", components_.size());
    LOG_A2UI(LOG_INFO, "Functions count: %{public}zu", functions_.size());
    LOG_A2UI(LOG_INFO, "----------------------------------------");

    for (size_t i = 0; i < components_.size(); ++i) {
        const auto& item = components_[i];
        if (item == nullptr) {
            LOG_A2UI(LOG_INFO, "[%{public}zu] <null>", i);
            continue;
        }

        LOG_A2UI(LOG_INFO,
            "[%{public}zu] name=%{public}s, category=%{public}s, type=%{public}s, isInnerNative=%{public}d", i,
            item->GetName().c_str(), CategoryToString(item->GetCategory()), TypeToString(item->GetType()),
            item->IsInnerNative() ? 1 : 0);
    }

    for (size_t i = 0; i < functions_.size(); ++i) {
        const auto& item = functions_[i];
        if (item == nullptr) {
            LOG_A2UI(LOG_INFO, "[function %{public}zu] <null>", i);
            continue;
        }

        LOG_A2UI(LOG_INFO,
            "[function %{public}zu] name=%{public}s, category=%{public}s, type=%{public}s, isInnerNative=%{public}d", i,
            item->GetName().c_str(), CategoryToString(item->GetCategory()), TypeToString(item->GetType()),
            item->IsInnerNative() ? 1 : 0);
    }

    LOG_A2UI(LOG_INFO, "========================================");
}

} // namespace NativeModule
