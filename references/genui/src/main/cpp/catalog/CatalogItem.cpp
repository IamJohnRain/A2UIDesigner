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

#include "CatalogItem.h"

namespace NativeModule {

CatalogItem::CatalogItem(const std::string& name, CatalogItemType type)
    : category_(CatalogCategory::OHOS_EXTENDS), type_(type), name_(name), isInnerNative_(false),
      preserveDynamicDescriptors_(false)
{}

CatalogCategory CatalogItem::GetCategory() const
{
    return category_;
}

void CatalogItem::SetCategory(CatalogCategory category)
{
    category_ = category;
}

CatalogItemType CatalogItem::GetType() const
{
    return type_;
}

void CatalogItem::SetType(CatalogItemType type)
{
    type_ = type;
}

const std::string& CatalogItem::GetName() const
{
    return name_;
}

void CatalogItem::SetName(const std::string& name)
{
    name_ = name;
}

bool CatalogItem::IsInnerNative() const
{
    return isInnerNative_;
}

void CatalogItem::SetInnerNative(bool isInnerNative)
{
    isInnerNative_ = isInnerNative;
}

bool CatalogItem::ShouldPreserveDynamicDescriptors() const
{
    return preserveDynamicDescriptors_;
}

void CatalogItem::SetPreserveDynamicDescriptors(bool preserveDynamicDescriptors)
{
    preserveDynamicDescriptors_ = preserveDynamicDescriptors;
}

} // namespace NativeModule
