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

#include "ChildListParser.h"

#include "utils/LocalVariableNameUtils.h"
#include "utils/LogA2UI.h"

namespace NativeModule {

namespace {

void ApplyTemplateLoopVariableConfig(const JsonValue& childrenValue, ChildListDescriptor& descriptor)
{
    bool hasIndexVar = childrenValue.Has("indexVar");
    bool hasItemVar = childrenValue.Has("itemVar");
    std::string indexVar = childrenValue.GetString("indexVar", "");
    std::string itemVar = childrenValue.GetString("itemVar", "");

    bool indexValid = !hasIndexVar || IsValidLocalVariableName(indexVar);
    bool itemValid = !hasItemVar || IsValidLocalVariableName(itemVar);

    if (hasIndexVar && !indexValid) {
        LOG_A2UI(
            LOG_WARN, "ChildListParser: invalid indexVar '%{public}s', fallback to default $index", indexVar.c_str());
    }
    if (hasItemVar && !itemValid) {
        LOG_A2UI(LOG_WARN, "ChildListParser: invalid itemVar '%{public}s', fallback to default $item", itemVar.c_str());
    }

    if (hasIndexVar && hasItemVar && indexValid && itemValid && indexVar == itemVar) {
        LOG_A2UI(LOG_WARN, "ChildListParser: conflicting indexVar/itemVar '%{public}s', fallback to defaults",
            indexVar.c_str());
        indexValid = false;
        itemValid = false;
    }

    if (hasIndexVar && indexValid) {
        descriptor.resolvedIndexVarName = indexVar;
        descriptor.useDefaultIndexVar = false;
    }
    if (hasItemVar && itemValid) {
        descriptor.resolvedItemVarName = itemVar;
        descriptor.useDefaultItemVar = false;
    }
}

} // namespace

ChildListDescriptor ChildListParser::ParseChildren(const JsonValue& childrenValue)
{
    ChildListDescriptor descriptor;

    if (childrenValue.IsArray()) {
        descriptor.type = ChildListType::STATIC_IDS;
        int childCount = childrenValue.GetArraySize();
        for (int j = 0; j < childCount; ++j) {
            std::string childId = childrenValue.GetArrayItem(j).GetStringValue("");
            if (!childId.empty()) {
                descriptor.staticChildIds.push_back(childId);
            }
        }
        return descriptor;
    }

    if (childrenValue.IsObject()) {
        std::string templateComponentId = childrenValue.GetString("componentId", "");
        std::string templatePath = childrenValue.GetString("path", "");
        if (!templateComponentId.empty() && !templatePath.empty()) {
            descriptor.type = ChildListType::TEMPLATE_PATH;
            descriptor.templateComponentId = templateComponentId;
            descriptor.templatePath = templatePath;
            ApplyTemplateLoopVariableConfig(childrenValue, descriptor);
            return descriptor;
        }
    }

    descriptor.type = ChildListType::INVALID;
    return descriptor;
}

ChildListDescriptor ChildListParser::ParseChild(const JsonValue& childValue)
{
    ChildListDescriptor descriptor;

    if (childValue.IsString()) {
        std::string childId = childValue.GetStringValue("");
        if (!childId.empty()) {
            descriptor.type = ChildListType::STATIC_IDS;
            descriptor.staticChildIds.push_back(childId);
            return descriptor;
        }
    }

    descriptor.type = ChildListType::INVALID;
    return descriptor;
}

} // namespace NativeModule
