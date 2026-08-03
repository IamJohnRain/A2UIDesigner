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

#include "ExtendedComponentFactory.h"

#include "components/extended/ExtendedButtonComponent.h"
#include "components/extended/ExtendedCheckboxComponent.h"
#include "components/extended/ExtendedCheckboxGroupComponent.h"
#include "components/extended/ExtendedColumnComponent.h"
#include "components/extended/ExtendedDividerComponent.h"
#include "components/extended/ExtendedGridComponent.h"
#include "components/extended/ExtendedImageComponent.h"
#include "components/extended/ExtendedListComponent.h"
#include "components/extended/ExtendedProgressComponent.h"
#include "components/extended/ExtendedRadioComponent.h"
#include "components/extended/ExtendedRowComponent.h"
#include "components/extended/ExtendedStackComponent.h"
#include "components/extended/ExtendedTextComponent.h"
#include "components/extended/ExtendedTextInputComponent.h"
#include "components/extended/ExtendedToggleComponent.h"
#include "components/extended/NavContainerComponent.h"
#include "components/extended/if/IfComponent.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

constexpr char EXTENDED_COMPONENT_TYPE_SEPARATOR = '.';

} // namespace

ExtendedComponentFactory& ExtendedComponentFactory::GetInstance()
{
    static ExtendedComponentFactory instance;
    return instance;
}

ExtendedComponentFactory::ExtendedComponentFactory()
{
    RegisterBuiltInComponents();
}

std::shared_ptr<ExtendedComponent> ExtendedComponentFactory::CreateComponent(const std::string& type) const
{
    std::string shortName = GetShortName(type);
    LOG_A2UI(LOG_DEBUG, "ExtendedComponentFactory::CreateComponent - type=%{public}s, shortName=%{public}s",
        type.c_str(), shortName.c_str());
    auto iter = builders_.find(shortName);
    if (iter == builders_.end()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedComponentFactory::CreateComponent - builder not found, type=%{public}s, shortName=%{public}s",
            type.c_str(), shortName.c_str());
        return nullptr;
    }
    return iter->second();
}

bool ExtendedComponentFactory::IsExtendedComponent(const std::string& type) const
{
    std::string shortName = GetShortName(type);
    return builders_.find(shortName) != builders_.end();
}

std::string ExtendedComponentFactory::GetShortName(const std::string& type) const
{
    if (type.empty()) {
        return "";
    }

    size_t separatorIndex = type.find_last_of(EXTENDED_COMPONENT_TYPE_SEPARATOR);
    if (separatorIndex == std::string::npos || separatorIndex + 1 >= type.size()) {
        return type;
    }
    return type.substr(separatorIndex + 1);
}

void ExtendedComponentFactory::RegisterComponent(const std::string& type, const Builder& builder)
{
    if (type.empty() || builder == nullptr) {
        LOG_A2UI(LOG_WARN,
            "ExtendedComponentFactory::RegisterComponent - invalid input, type=%{public}s, builderNull=%{public}s",
            type.c_str(), builder == nullptr ? "true" : "false");
        return;
    }
    builders_[type] = builder;
    LOG_A2UI(LOG_INFO, "ExtendedComponentFactory::RegisterComponent - registered type=%{public}s, total=%{public}zu",
        type.c_str(), builders_.size());
}

void ExtendedComponentFactory::RegisterBuiltInComponents()
{
    LOG_A2UI(LOG_INFO, "ExtendedComponentFactory::RegisterBuiltInComponents - start");
    RegisterComponent("Button", []() { return std::make_shared<ExtendedButtonComponent>(); });
    RegisterComponent("Text", []() { return std::make_shared<ExtendedTextComponent>(); });
    RegisterComponent("Column", []() { return std::make_shared<ExtendedColumnComponent>(); });
    RegisterComponent("Row", []() { return std::make_shared<ExtendedRowComponent>(); });
    RegisterComponent("Stack", []() { return std::make_shared<ExtendedStackComponent>(); });
    RegisterComponent("Grid", []() { return std::make_shared<ExtendedGridComponent>(); });
    RegisterComponent("List", []() { return std::make_shared<ExtendedListComponent>(); });
    RegisterComponent("Toggle", []() { return std::make_shared<ExtendedToggleComponent>(); });
    RegisterComponent("Radio", []() { return std::make_shared<ExtendedRadioComponent>(); });
    RegisterComponent("TextInput", []() { return std::make_shared<ExtendedTextInputComponent>(); });
    RegisterComponent("Divider", []() { return std::make_shared<ExtendedDividerComponent>(); });
    RegisterComponent("Image", []() { return std::make_shared<ExtendedImageComponent>(); });
    RegisterComponent("NavContainer", []() { return std::make_shared<NavContainerComponent>(); });
    RegisterComponent("Progress", []() { return std::make_shared<ExtendedProgressComponent>(); });
    RegisterComponent("Checkbox", []() { return std::make_shared<ExtendedCheckboxComponent>(); });
    RegisterComponent("CheckboxGroup", []() { return std::make_shared<ExtendedCheckboxGroupComponent>(); });
    RegisterComponent("If", []() { return std::make_shared<IfComponent>(); });
    LOG_A2UI(LOG_INFO, "ExtendedComponentFactory::RegisterBuiltInComponents - completed, total=%{public}zu",
        builders_.size());
}

} // namespace NativeModule
