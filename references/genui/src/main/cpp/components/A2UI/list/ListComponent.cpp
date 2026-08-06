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

#include "ListComponent.h"

#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "utils/LogA2UI.h"

#include "../../../SurfaceSlot.h"
#include "ListTheme.h"

namespace NativeModule {

ListComponent::ListComponent() : A2UIComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST)) {}

ListComponent::~ListComponent()
{
    for (auto item : listItems_) {
        if (item != nullptr) {
            ArkUINodeApiAdapter::RemoveChild(nativeView_, item);
            ArkUINodeApiAdapter::DisposeNode(item);
        }
    }
    listItems_.clear();
}

std::string ListComponent::GetType() const
{
    return "List";
}

PropertyDeclaration ListComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    static const std::map<std::string, std::function<PropertyDeclaration(ListComponent&)>> declarations = {
        { "direction",
            [](ListComponent& listComponent) {
                return PropertyDeclaration { .name = "direction",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "vertical",
                    .enumAllowed = { "horizontal", "vertical" },
                    .enumFallback = "vertical",
                    .applyValue = [&listComponent](const JsonValue& value) {
                        std::string direction = value.GetStringValue("vertical");
                        if (direction == "horizontal") {
                            listComponent.SetDirection(A2UIAxis::HORIZONTAL);
                            return;
                        }
                        listComponent.SetDirection(A2UIAxis::VERTICAL);
                    } };
            } },
        { "align",
            [](ListComponent& listComponent) {
                return PropertyDeclaration { .name = "align",
                    .type = PropertyValueType::ENUM_STRING,
                    .allowDynamic = false,
                    .fallbackString = "start",
                    .enumAllowed = { "start", "center", "end" },
                    .enumFallback = "start",
                    .applyValue = [&listComponent](const JsonValue& value) {
                        std::string align = value.GetStringValue("start");
                        if (align == "start") {
                            listComponent.SetAlign(A2UIListItemAlignment::START);
                        } else if (align == "center") {
                            listComponent.SetAlign(A2UIListItemAlignment::CENTER);
                        } else if (align == "end") {
                            listComponent.SetAlign(A2UIListItemAlignment::END);
                        } else {
                            listComponent.SetAlign(A2UIListItemAlignment::START);
                        }
                    } };
            } }
    };

    auto it = declarations.find(propertyName);
    if (it != declarations.end()) {
        return it->second(*this);
    }
    return {};
}

void ListComponent::SetDirection(A2UIAxis direction)
{
    ArkUINodeApiAdapter::SetNodeListDirection(nativeView_, direction);
}

void ListComponent::SetAlign(A2UIListItemAlignment align)
{
    ArkUINodeApiAdapter::SetNodeListAlignListItem(nativeView_, align);
}

void ListComponent::SetLazyMode(bool useLazy)
{
    mode_ = useLazy ? Mode::LAZY : Mode::EAGER;
    LOG_A2UI(LOG_INFO, "ListComponent::SetLazyMode: mode=%{public}s", useLazy ? "LAZY" : "EAGER");

    if (useLazy && adapterNode_) {
        ArkUINodeApiAdapter::SetNodeListNodeAdapter(nativeView_, adapterNode_->GetHandle());
        LOG_A2UI(LOG_INFO, "ListComponent::SetLazyMode: NodeAdapter applied");
    }
}

void ListComponent::SetAdapterNode(std::shared_ptr<ListAdapterNode> adapterNode)
{
    adapterNode_ = adapterNode;
    LOG_A2UI(LOG_INFO, "ListComponent::SetAdapterNode: adapterNode=%{public}s", adapterNode ? "set" : "null");

    if (adapterNode && mode_ == Mode::LAZY) {
        ArkUINodeApiAdapter::SetNodeListNodeAdapter(nativeView_, adapterNode->GetHandle());
        LOG_A2UI(LOG_INFO, "ListComponent::SetAdapterNode: NODE_LIST_NODE_ADAPTER attribute set");
    }
}

void ListComponent::SetupLazyAdapter(const LazyAdapterConfig& config)
{
    LOG_A2UI(LOG_INFO, "ListComponent::SetupLazyAdapter: templateComponentId=%{public}s, templatePath=%{public}s",
        config.templateComponentId.c_str(), config.templatePath.c_str());

    std::optional<int32_t> itemCount = ResolveLazyAdapterItemCount(config);
    if (!itemCount.has_value()) {
        return;
    }

    ApplyLazyAdapterConfig(config, itemCount.value());

    LOG_A2UI(LOG_INFO,
        "ListComponent::SetupLazyAdapter: NodeAdapter configured successfully, templateComponentId=%{public}s",
        config.templateComponentId.c_str());
}

std::optional<int32_t> ListComponent::ResolveLazyAdapterItemCount(const LazyAdapterConfig& config) const
{
    if (!config.templatePath.empty() && config.templatePath[0] != '/') {
        LOG_A2UI(LOG_INFO,
            "ListComponent::SetupLazyAdapter: relative path detected, will be resolved at runtime, path=%{public}s",
            config.templatePath.c_str());
        return 0;
    }

    if (config.dataModel == nullptr) {
        LOG_A2UI(LOG_ERROR, "ListComponent::SetupLazyAdapter: data model is null, templateComponentId=%{public}s",
            config.templateComponentId.c_str());
        return std::nullopt;
    }

    auto arrayOpt = config.dataModel->GetNode(config.templatePath);
    if (!arrayOpt.has_value()) {
        DynamicResolveContext context = { .renderId = config.renderId,
            .surfaceId = config.surfaceId,
            .componentId = GetComponentId(),
            .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
        DynamicValueResolver::ReportMissingPath(context, config.templatePath);
        LOG_A2UI(LOG_WARN,
            "ListComponent::SetupLazyAdapter: data path not found, create empty adapter, path=%{public}s",
            config.templatePath.c_str());
        return 0;
    }

    if (!arrayOpt.value().IsArray()) {
        LOG_A2UI(LOG_WARN,
            "ListComponent::SetupLazyAdapter: data path is not array, create empty adapter, path=%{public}s",
            config.templatePath.c_str());
        return 0;
    }

    int32_t itemCount = arrayOpt.value().GetArraySize();
    LOG_A2UI(LOG_INFO, "ListComponent::SetupLazyAdapter: itemCount=%{public}d", itemCount);
    return itemCount;
}

void ListComponent::ApplyLazyAdapterConfig(const LazyAdapterConfig& config, int32_t itemCount)
{
    bool isNewAdapter = !adapterNode_;
    std::shared_ptr<ListAdapterNode> adapterNode = isNewAdapter ? std::make_shared<ListAdapterNode>() : adapterNode_;

    adapterNode->Initialize(config.templateComponentId, config.templatePath, itemCount);
    adapterNode->SetDataModel(config.dataModel);
    adapterNode->SetTemplateDescriptor(config.templateDescriptor);
    adapterNode->SetAllDescriptors(config.allDescriptors);
    adapterNode->SetSurfaceInfo(config.surfaceId, config.renderId);
    adapterNode->SetSurfaceContext(config.surfaceContext);

    if (isNewAdapter) {
        SetLazyMode(true);
        SetAdapterNode(adapterNode);
    } else {
        adapterNode->ReloadAllItems();
    }
}

void ListComponent::CollectChildListDescriptor(const JsonValue& descriptor)
{
    childListDescriptor_ = ChildListParser::ParseChildren(descriptor.GetItem("children"));
}

bool ListComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    childIds.clear();
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();

    auto templateIt = descriptorStore.find(childList.templateComponentId);
    if (templateIt == descriptorStore.end()) {
        LOG_A2UI(LOG_WARN,
            "ListComponent::ExpandTemplateChildren: template not found, componentId=%{public}s, templateId=%{public}s",
            GetComponentId().c_str(), childList.templateComponentId.c_str());
        return false;
    }

    LazyAdapterConfig config;
    config.templateComponentId = childList.templateComponentId;
    config.templatePath = childList.templatePath;
    config.dataModel = surfaceSlot.GetOrCreateDataModel();
    config.templateDescriptor = templateIt->second;
    config.allDescriptors = descriptorStore;
    config.surfaceId = surfaceSlot.GetSurfaceId();
    config.renderId = surfaceSlot.GetRenderId();
    config.surfaceContext = surfaceSlot.GetSurfaceContext();
    SetupLazyAdapter(config);
    return false;
}

void ListComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    static_cast<void>(index); // Suppress unused parameter warning
    if (mode_ == Mode::LAZY) {
        LOG_A2UI(LOG_INFO, "ListComponent::OnAddChild: skipped in LAZY mode (children created by LazyForEach)");
        return;
    }

    auto listItemNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST_ITEM);
    ArkUINodeApiAdapter::AddChild(nativeView_, listItemNode);
    ArkUINodeApiAdapter::AddChild(listItemNode, child->GetNativeView());
    listItems_.push_back(listItemNode);
}

void ListComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    LOG_A2UI(LOG_INFO, "ListComponent::ApplyPrivateAttributes");

    ApplySchemaProperty("direction", descriptor);
    ApplySchemaProperty("align", descriptor);
}

void ListComponent::OnDataUpdate(const std::string& property, const JsonValue& value)
{
    A2UIComponent::OnDataUpdate(property, value);
    LOG_A2UI(LOG_INFO, "ListComponent::OnDataUpdate: property=%{public}s, valueType=%{public}s", property.c_str(),
        value.GetTypeName());
}

void ListComponent::RemoveAllChildren()
{
    if (!listItems_.empty()) {
        LOG_A2UI(LOG_INFO, "ListComponent::RemoveAllChildren: clearing %{public}zu list items", listItems_.size());
    }

    for (auto item : listItems_) {
        if (item != nullptr) {
            ArkUINodeApiAdapter::RemoveChild(nativeView_, item);
            ArkUINodeApiAdapter::DisposeNode(item);
        }
    }
    listItems_.clear();

    Component::RemoveAllChildren();
}

std::shared_ptr<ListTheme> ListComponent::GetTheme()
{
    // Try to get from cache first
    auto theme = cachedTheme_.lock();
    if (theme != nullptr) {
        return theme;
    }

    // Cache miss, get from base class
    std::shared_ptr<ThemeBase> baseTheme = A2UIComponent::GetTheme();
    if (baseTheme == nullptr) {
        return nullptr;
    }

    // Cast to specific type and cache it
    theme = std::dynamic_pointer_cast<ListTheme>(baseTheme);
    if (theme != nullptr) {
        cachedTheme_ = theme;
    }

    return theme;
}

void ListComponent::OnConfigChange(const ThemeContext& context)
{
    auto listTheme = GetTheme();
    if (listTheme == nullptr) {
        return;
    }

    // TODO: Re-apply styles based on new theme context
}

} // namespace NativeModule
