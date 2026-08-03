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

//
// Created on 2026/3/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "NativeComponentFactory.h"

#include "components/A2UI/button/ButtonComponent.h"
#include "components/A2UI/card/CardComponent.h"
#include "components/A2UI/checkbox/CheckboxComponent.h"
#include "components/A2UI/column/ColumnComponent.h"
#include "components/A2UI/image/ImageComponent.h"
#include "components/A2UI/list/ListComponent.h"
#include "components/A2UI/row/RowComponent.h"
#include "components/A2UI/slider/SliderComponent.h"
#include "components/A2UI/text/TextComponent.h"
#include "components/A2UI/textfield/TextFieldComponent.h"

namespace NativeModule {

std::unordered_map<std::string, std::function<std::shared_ptr<Component>()>> NativeComponentFactory::builders_ = {
    { "Button", []() { return std::make_shared<ButtonComponent>(); } },
    { "Card", []() { return std::make_shared<CardComponent>(); } },
    { "CheckBox", []() { return std::make_shared<CheckboxComponent>(); } },
    { "Column", []() { return std::make_shared<ColumnComponent>(); } },
    { "Row", []() { return std::make_shared<RowComponent>(); } },
    { "TextField", []() { return std::make_shared<TextFieldComponent>(); } },
    { "Image", []() { return std::make_shared<ImageComponent>(); } },
    { "List", []() { return std::make_shared<ListComponent>(); } },
    { "Slider", []() { return std::make_shared<SliderComponent>(); } },
    { "Text", []() { return std::make_shared<TextComponent>(); } }
};

std::string NativeComponentFactory::ResolveComponentType(const JsonValue& descriptor)
{
    std::string type = descriptor.GetString("component", "");
    if (!type.empty()) {
        return type;
    }
    return descriptor.GetString("type", "");
}

std::shared_ptr<Component> NativeComponentFactory::CreateComponent(const std::string& type)
{
    auto iter = builders_.find(type);
    if (iter == builders_.end()) {
        return nullptr;
    }
    return iter->second();
}
} // namespace NativeModule
