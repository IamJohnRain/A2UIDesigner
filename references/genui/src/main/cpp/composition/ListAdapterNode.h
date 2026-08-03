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

#ifndef A2UI_LIST_ADAPTER_NODE_H
#define A2UI_LIST_ADAPTER_NODE_H

#include "TemplateAdapterNode.h"

namespace NativeModule {

/**
 * @brief List 组件专用的懒加载适配器。
 *
 * 继承 TemplateAdapterNode 的通用模板实例化能力，增加了 List 特有的逻辑：
 * - 嵌套 List 适配器的递归创建和配置。
 * - 嵌套 List 适配器 itemCount 的运行时更新。
 */
class ListAdapterNode : public TemplateAdapterNode {
public:
    ListAdapterNode() = default;
    ~ListAdapterNode() override = default;

protected:
    void OnNestedAdapterUpdate(const std::shared_ptr<Component>& component, const std::string& parentPath) override;

    void SetupNestedAdapter(const std::shared_ptr<Component>& component, const std::string& componentType,
        const std::string& templateComponentId, const std::string& templatePath,
        const std::map<std::string, JsonValue>& descriptors) override;

    ItemWrapperInfo BuildItemWrapper(const std::shared_ptr<Component>& component) const override;

private:
    static void UpdateNestedItemCounts(const std::shared_ptr<Component>& component, const std::string& parentPath,
        std::shared_ptr<DataModel> dataModel);
};

} // namespace NativeModule

#endif // A2UI_LIST_ADAPTER_NODE_H
