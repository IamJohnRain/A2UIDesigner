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

#ifndef A2UIRENDER_CATALOG_H
#define A2UIRENDER_CATALOG_H

#include <memory>
#include <string>
#include <vector>

#include "CatalogItem.h"
#include "SurfaceContext.h"

namespace NativeModule {

class Catalog {
public:
    Catalog(const std::string& id, const std::string& a2UIProtocolVersion = DEFAULT_A2UI_PROTOCOL_VERSION);
    ~Catalog() = default;

    const std::string& GetCatalogId() const;
    const std::string& GetA2UIProtocolVersion() const;
    const SurfaceContext& GetSurfaceContext() const;
    const std::vector<std::shared_ptr<CatalogItem>>& GetComponents() const;
    const std::vector<std::shared_ptr<CatalogItem>>& GetFunctions() const;
    void AddComponent(const std::shared_ptr<CatalogItem>& component);
    void AddFunction(const std::shared_ptr<CatalogItem>& function);
    std::shared_ptr<CatalogItem> GetCatalogItemByName(const std::string& name) const;
    std::shared_ptr<CatalogItem> GetFunctionItemByName(const std::string& name) const;
    bool HasFunction(const std::string& name) const;
    bool Equals(const Catalog& other) const;

    void DebugPrint() const;

private:
    std::string id_;
    SurfaceContext surfaceContext_;
    std::vector<std::shared_ptr<CatalogItem>> components_;
    std::vector<std::shared_ptr<CatalogItem>> functions_;
};

} // namespace NativeModule

#endif // A2UIRENDER_CATALOG_H
