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

#ifndef A2UI_EXTENDED_COMPONENT_FACTORY_H
#define A2UI_EXTENDED_COMPONENT_FACTORY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "components/Component.h"

namespace NativeModule {

class ExtendedComponent;

class ExtendedComponentFactory final {
public:
    using Builder = std::function<std::shared_ptr<ExtendedComponent>()>;

    static ExtendedComponentFactory& GetInstance();

    std::shared_ptr<ExtendedComponent> CreateComponent(const std::string& type) const;
    bool IsExtendedComponent(const std::string& type) const;
    std::string GetShortName(const std::string& type) const;
    void RegisterComponent(const std::string& type, const Builder& builder);

private:
    ExtendedComponentFactory();
    ~ExtendedComponentFactory() = default;
    ExtendedComponentFactory(const ExtendedComponentFactory&) = delete;
    ExtendedComponentFactory& operator=(const ExtendedComponentFactory&) = delete;

    void RegisterBuiltInComponents();

    std::unordered_map<std::string, Builder> builders_;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_COMPONENT_FACTORY_H
