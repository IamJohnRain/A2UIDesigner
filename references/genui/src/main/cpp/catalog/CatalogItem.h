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

#ifndef A2UIRENDER_CATALOG_ITEM_H
#define A2UIRENDER_CATALOG_ITEM_H

#include <string>

namespace NativeModule {

enum class CatalogCategory { A2UI_STANDARD = 0, OHOS_EXTENDS = 1 };

enum class CatalogItemType { PROTOCOL_MESSAGE = 0, COMPONENT = 1, LOCAL_FUNCTION = 2 };

class CatalogItem {
public:
    CatalogItem(const std::string& name, CatalogItemType type);
    ~CatalogItem() = default;

    CatalogCategory GetCategory() const;
    void SetCategory(CatalogCategory category);

    CatalogItemType GetType() const;
    void SetType(CatalogItemType type);

    const std::string& GetName() const;
    void SetName(const std::string& name);

    bool IsInnerNative() const;
    void SetInnerNative(bool isInnerNative);

private:
    CatalogCategory category_;
    CatalogItemType type_;
    std::string name_;
    bool isInnerNative_;
};

} // namespace NativeModule

#endif // A2UIRENDER_CATALOG_ITEM_H
