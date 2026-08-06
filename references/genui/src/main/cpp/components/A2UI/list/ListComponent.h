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

#ifndef A2UI_LIST_COMPONENT_H
#define A2UI_LIST_COMPONENT_H

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "composition/ListAdapterNode.h"

#include "../A2UIComponent.h"
#include "ListTheme.h"
#include "SurfaceContext.h"

namespace NativeModule {

class DataModel;

struct LazyAdapterConfig {
    std::string templateComponentId;
    std::string templatePath;
    std::shared_ptr<DataModel> dataModel;
    JsonValue templateDescriptor;
    std::map<std::string, JsonValue> allDescriptors;
    std::string surfaceId;
    int32_t renderId = -1;
    SurfaceContext surfaceContext;
};

class ListComponent : public A2UIComponent {
public:
    ListComponent();
    ~ListComponent() override;

    std::string GetType() const override;

    // Theme getter with caching
    std::shared_ptr<ListTheme> GetTheme();

    void SetupLazyAdapter(const LazyAdapterConfig& config);
    // Lazy loading support with NodeAdapter
    void SetLazyMode(bool useLazy);
    void SetAdapterNode(std::shared_ptr<ListAdapterNode> adapterNode);
    bool IsLazyMode() const
    {
        return mode_ == Mode::LAZY;
    }
    std::shared_ptr<ListAdapterNode> GetAdapterNode() const
    {
        return adapterNode_;
    }
    std::shared_ptr<TemplateAdapterNode> GetLazyAdapter() const override
    {
        return mode_ == Mode::LAZY ? adapterNode_ : nullptr;
    }

    void SetDirection(A2UIAxis direction);
    void SetAlign(A2UIListItemAlignment align);
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void RemoveAllChildren() override;

protected:
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    void ApplyPrivateAttributes(const JsonValue& descriptor) override;
    void OnAddChild(const std::shared_ptr<Component>& child, size_t index) override;
    PropertyDeclaration GetPrivatePropertyDeclaration(const std::string& propertyName) override;
    void OnConfigChange(const ThemeContext& context) override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

private:
    enum class Mode { EAGER, LAZY };
    Mode mode_ = Mode::EAGER;
    std::shared_ptr<ListAdapterNode> adapterNode_;
    std::vector<ArkUI_NodeHandle> listItems_;

    std::optional<int32_t> ResolveLazyAdapterItemCount(const LazyAdapterConfig& config) const;
    void ApplyLazyAdapterConfig(const LazyAdapterConfig& config, int32_t itemCount);

    // Theme cache
    std::weak_ptr<ListTheme> cachedTheme_;
};

} // namespace NativeModule

#endif // A2UI_LIST_COMPONENT_H
