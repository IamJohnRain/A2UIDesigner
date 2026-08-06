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

#ifndef A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H
#define A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H

#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

class CustomComponent;

bool IsExpressionStringValue(const JsonValue& value);
std::string BuildCustomExpressionBindingKey(const std::string& propertyName);
bool IsCustomExpressionBindingProperty(const std::string& propertyName);
std::string ResolveCustomExpressionSourceProperty(const std::string& propertyName);
void RefreshCustomExpressionBindings(
    CustomComponent& component, const std::string& propertyName, const JsonValue& value);

} // namespace NativeModule

#endif // A2UI_CUSTOM_COMPONENT_EXPRESSION_BINDING_H
