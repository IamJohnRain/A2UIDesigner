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

#include "theme/ThemeFactory.h"

#include <functional>
#include <unordered_map>

#include "components/A2UI/button/ButtonTheme.h"
#include "components/A2UI/card/CardTheme.h"
#include "components/A2UI/checkbox/CheckboxGroupTheme.h"
#include "components/A2UI/checkbox/CheckboxTheme.h"
#include "components/A2UI/column/ColumnTheme.h"
#include "components/A2UI/image/ImageTheme.h"
#include "components/A2UI/list/ListTheme.h"
#include "components/A2UI/row/RowTheme.h"
#include "components/A2UI/slider/SliderTheme.h"
#include "components/A2UI/text/TextTheme.h"
#include "components/A2UI/textfield/TextFieldTheme.h"
#include "components/extended/ExtendedGridTheme.h"

namespace NativeModule {

namespace {
// Theme builder map - constant map defined in cpp
const std::unordered_map<std::string, std::function<std::shared_ptr<ThemeBase>(const ThemeContext&)>> themeBuilders = {
    { "Button", [](const ThemeContext& ctx) { return std::make_shared<ButtonTheme>(ctx); } },
    { "Text", [](const ThemeContext& ctx) { return std::make_shared<TextTheme>(ctx); } },
    { "Card", [](const ThemeContext& ctx) { return std::make_shared<CardTheme>(ctx); } },
    { "CheckBox", [](const ThemeContext& ctx) { return std::make_shared<CheckboxTheme>(ctx); } },
    { "Checkbox", [](const ThemeContext& ctx) { return std::make_shared<CheckboxTheme>(ctx); } },
    { "CheckboxGroup", [](const ThemeContext& ctx) { return std::make_shared<CheckboxGroupTheme>(ctx); } },
    { "Column", [](const ThemeContext& ctx) { return std::make_shared<ColumnTheme>(ctx); } },
    { "Row", [](const ThemeContext& ctx) { return std::make_shared<RowTheme>(ctx); } },
    { "Image", [](const ThemeContext& ctx) { return std::make_shared<ImageTheme>(ctx); } },
    { "List", [](const ThemeContext& ctx) { return std::make_shared<ListTheme>(ctx); } },
    { "Grid", [](const ThemeContext& ctx) { return std::make_shared<ExtendedGridTheme>(ctx); } },
    { "Slider", [](const ThemeContext& ctx) { return std::make_shared<SliderTheme>(ctx); } },
    { "TextField", [](const ThemeContext& ctx) { return std::make_shared<TextFieldTheme>(ctx); } },
};
} // namespace

std::shared_ptr<ThemeBase> ThemeFactory::CreateTheme(const std::string& componentType, const ThemeContext& context)
{
    auto it = themeBuilders.find(componentType);
    if (it == themeBuilders.end()) {
        return nullptr;
    }
    return it->second(context);
}

} // namespace NativeModule
