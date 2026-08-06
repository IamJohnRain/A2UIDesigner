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

#ifndef A2UI_CUSTOM_COMPONENT_DESCRIPTOR_H
#define A2UI_CUSTOM_COMPONENT_DESCRIPTOR_H

#include <cstdint>
#include <string>

#include "utils/JsonAdapter.h"

namespace NativeModule {

struct CommonStyleProps {
    std::string size;
    std::string padding;
    std::string margin;
    bool hasMargin = false;
    double width = 0.0;
    bool hasWidth = false;
    double height = 0.0;
    bool hasHeight = false;
    double weight = 0.0;
    bool hasWeight = false;
    std::string accessibilityLabel;
    bool hasAccessibilityLabel = false;
    std::string accessibilityDescription;
    bool hasAccessibilityDescription = false;
    double flexShrinkParentDefault = 0.0;
    bool hasFlexShrinkParentDefault = false;
    bool resetFlexShrinkToParentDefault = false;
};

struct CustomComponentDescriptor {
    std::string type;
    std::string id;
    std::string surfaceId;
    JsonValue customProps;
    CommonStyleProps properties;
};

} // namespace NativeModule

#endif // A2UI_CUSTOM_COMPONENT_DESCRIPTOR_H
