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

#include "ExtendedListComponent.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>

#include "components/ChildListSchemaValidationUtils.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "../../SurfaceSlot.h"
#include "ExtendedListTheme.h"
#include "SchemaErrorCodes.h"

namespace NativeModule {

namespace {

constexpr A2UIScrollNestedMode DEFAULT_NESTED_SCROLL_MODE = A2UIScrollNestedMode::SELF_FIRST;
constexpr const char* DEFAULT_NESTED_SCROLL_MODE_NAME = "selfFirst";

bool IsDynamicStyleMember(const JsonValue& value)
{
    return value.IsObject() && (value.Has("path") || value.Has("call"));
}

bool IsDynamicOrExpressionStyleMember(const JsonValue& value)
{
    return IsDynamicStyleMember(value) ||
           (value.IsString() && StyleApplyUtils::IsExpressionString(value.GetStringValue("")));
}

std::unique_ptr<JsonAdapter> ResolveDynamicStyleObjectMembers(
    const JsonValue& styleObjectValue, const RenderContext& renderContext, const std::string& componentId)
{
    if (!styleObjectValue.IsObject() || IsDynamicStyleMember(styleObjectValue)) {
        return nullptr;
    }

    std::unique_ptr<JsonAdapter> resolvedAdapter = JsonAdapter::CreateObject();
    if (resolvedAdapter == nullptr) {
        return nullptr;
    }

    JsonValue resolvedRoot = resolvedAdapter->GetRoot();
    bool hasDynamicMember = false;
    DynamicResolveContext context = { .renderId = renderContext.renderId,
        .surfaceId = renderContext.surfaceId,
        .componentId = componentId,
        .allowExpression = true,
        .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
    for (JsonValue child = styleObjectValue.GetChild(); child.IsValid(); child = child.GetNext()) {
        std::string key = child.GetKey();
        if (key.empty()) {
            continue;
        }
        if (IsDynamicOrExpressionStyleMember(child)) {
            hasDynamicMember = true;
            ResolvedValue resolvedValue = DynamicValueResolver::Resolve(child, context);
            if (resolvedValue.success && resolvedValue.value.IsValid()) {
                resolvedRoot.Put(key.c_str(), resolvedValue.value);
                continue;
            }
        }
        resolvedRoot.Put(key.c_str(), child);
    }

    if (!hasDynamicMember) {
        return nullptr;
    }
    return resolvedAdapter;
}

A2UIAxis ResolveListDirection(const std::string& direction)
{
    if (direction == "horizontal") {
        return A2UIAxis::HORIZONTAL;
    }
    return A2UIAxis::VERTICAL;
}

float NormalizeListSpace(float space)
{
    return std::isfinite(space) && space >= 0.0F ? space : 0.0F;
}

A2UIScrollBarDisplayMode ResolveScrollBar(const std::string& scrollBar)
{
    if (scrollBar == "off") {
        return A2UIScrollBarDisplayMode::OFF;
    }
    if (scrollBar == "on") {
        return A2UIScrollBarDisplayMode::ON;
    }
    return A2UIScrollBarDisplayMode::AUTO;
}

A2UIScrollNestedMode ResolveNestedScrollMode(const std::string& mode)
{
    if (mode == "selfOnly") {
        return A2UIScrollNestedMode::SELF_ONLY;
    }
    if (mode == "selfFirst") {
        return A2UIScrollNestedMode::SELF_FIRST;
    }
    if (mode == "parentFirst") {
        return A2UIScrollNestedMode::PARENT_FIRST;
    }
    if (mode == "paraller") {
        return A2UIScrollNestedMode::PARALLEL;
    }
    return DEFAULT_NESTED_SCROLL_MODE;
}

struct ExtendedLazyAdapterConfig {
    std::string templateComponentId;
    std::string templatePath;
    std::shared_ptr<DataModel> dataModel;
    JsonValue templateDescriptor;
    std::map<std::string, JsonValue> allDescriptors;
    std::string surfaceId;
    int32_t renderId = -1;
    SurfaceContext surfaceContext;
};

std::optional<int> PrepareAdapterConfig(ExtendedLazyAdapterConfig& config, const ChildListDescriptor& childList,
    SurfaceSlot& surfaceSlot, const std::string& componentId)
{
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();
    auto templateIt = descriptorStore.find(childList.templateComponentId);
    if (templateIt == descriptorStore.end()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedListComponent::SetupLazyAdapter: template not found, componentId=%{public}s, "
            "templateId=%{public}s",
            componentId.c_str(), childList.templateComponentId.c_str());
        return std::nullopt;
    }
    config.templateComponentId = childList.templateComponentId;
    config.templatePath = childList.templatePath;
    config.dataModel = surfaceSlot.GetOrCreateDataModel();
    config.templateDescriptor = templateIt->second;
    config.allDescriptors = descriptorStore;
    config.surfaceId = surfaceSlot.GetSurfaceId();
    config.renderId = surfaceSlot.GetRenderId();
    config.surfaceContext = surfaceSlot.GetSurfaceContext();
    int itemCount = 0;
    bool isRelativePath = !config.templatePath.empty() && config.templatePath[0] != '/';
    if (!isRelativePath) {
        if (config.dataModel == nullptr) {
            LOG_A2UI(LOG_ERROR,
                "ExtendedListComponent::SetupLazyAdapter: data model is null, templateComponentId=%{public}s",
                config.templateComponentId.c_str());
            return 0;
        }
        auto arrayOpt = config.dataModel->GetNode(config.templatePath);
        if (!arrayOpt.has_value()) {
            DynamicResolveContext context = { .renderId = config.renderId,
                .surfaceId = config.surfaceId,
                .componentId = componentId,
                .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
            DynamicValueResolver::ReportMissingPath(context, config.templatePath);
            LOG_A2UI(LOG_ERROR, "ExtendedListComponent::SetupLazyAdapter: data path not found, path=%{public}s",
                config.templatePath.c_str());
            return itemCount;
        }
        if (arrayOpt.value().IsArray()) {
            itemCount = arrayOpt.value().GetArraySize();
        }
    }
    return itemCount;
}

void InitializeLazyAdapter(ListAdapterNode& adapterNode, const ExtendedLazyAdapterConfig& config,
    const ChildListDescriptor& childList, const std::map<std::string, JsonValue>& inheritedLocalVariables,
    int itemCount)
{
    adapterNode.Initialize(config.templateComponentId, config.templatePath, itemCount, childList.resolvedIndexVarName,
        childList.resolvedItemVarName);
    adapterNode.SetDataModel(config.dataModel);
    adapterNode.SetTemplateDescriptor(config.templateDescriptor);
    adapterNode.SetAllDescriptors(config.allDescriptors);
    adapterNode.SetInheritedLocalVariables(inheritedLocalVariables);
    adapterNode.SetSurfaceInfo(config.surfaceId, config.renderId);
    adapterNode.SetSurfaceContext(config.surfaceContext);
}

} // namespace

ExtendedListComponent::ExtendedListComponent() : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST))
{}

ExtendedListComponent::~ExtendedListComponent()
{
    RemoveAllChildren();
}

std::string ExtendedListComponent::GetType() const
{
    return "List";
}

void ExtendedListComponent::SetLazyMode(bool useLazy)
{
    mode_ = useLazy ? Mode::LAZY : Mode::EAGER;
    LOG_A2UI(LOG_INFO, "ExtendedListComponent::SetLazyMode: mode=%{public}s", useLazy ? "LAZY" : "EAGER");

    if (useLazy && adapterNode_ != nullptr && nativeView_ != nullptr) {
        ArkUINodeApiAdapter::SetNodeListNodeAdapter(nativeView_, adapterNode_->GetHandle());
        LOG_A2UI(LOG_INFO, "ExtendedListComponent::SetLazyMode: NodeAdapter applied");
    }
}

void ExtendedListComponent::SetAdapterNode(const std::shared_ptr<ListAdapterNode>& adapterNode)
{
    adapterNode_ = adapterNode;
    LOG_A2UI(LOG_INFO, "ExtendedListComponent::SetAdapterNode: adapterNode=%{public}s",
        adapterNode_ != nullptr ? "set" : "null");

    if (adapterNode_ != nullptr && mode_ == Mode::LAZY && nativeView_ != nullptr) {
        ArkUINodeApiAdapter::SetNodeListNodeAdapter(nativeView_, adapterNode_->GetHandle());
        LOG_A2UI(LOG_INFO, "ExtendedListComponent::SetAdapterNode: NODE_LIST_NODE_ADAPTER attribute set");
    }
}

void ExtendedListComponent::SetupLazyAdapter(const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot)
{
    ExtendedLazyAdapterConfig config;
    std::optional<int> itemCount = PrepareAdapterConfig(config, childList, surfaceSlot, GetComponentId());
    if (!itemCount.has_value()) {
        return;
    }

    if (adapterNode_ == nullptr) {
        auto adapterNode = std::make_shared<ListAdapterNode>();
        InitializeLazyAdapter(*adapterNode, config, childList, GetLocalVariables(), itemCount.value());
        SetLazyMode(true);
        SetAdapterNode(adapterNode);
        return;
    }

    InitializeLazyAdapter(*adapterNode_, config, childList, GetLocalVariables(), itemCount.value());
    adapterNode_->IncrementTemplateVersion();
    adapterNode_->UpdateItemCount(itemCount.value());
    adapterNode_->ReloadAllItems();
}

bool ExtendedListComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    childIds.clear();
    SetupLazyAdapter(childList, surfaceSlot);
    if (adapterNode_ == nullptr) {
        surfaceSlot.OnTemplateExpansionDeferred(GetComponentId());
    } else {
        surfaceSlot.OnTemplateExpansionResolved(GetComponentId());
    }
    return false;
}

void ExtendedListComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    ArkUINodeApiAdapter::SetNodePixelRoundNoForceRound(nativeView_, GetRenderContext().apiVersion);
    ApplyDeclaredPropertyOrFallback(descriptor, "space");
    ApplyDefaultLanes(ResolveThemeContext());
    SetListDirection(A2UIAxis::VERTICAL);
    SetScrollBar(A2UIScrollBarDisplayMode::AUTO);
    SetNestedScroll(DEFAULT_NESTED_SCROLL_MODE, DEFAULT_NESTED_SCROLL_MODE);
}

void ExtendedListComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

void ExtendedListComponent::OnConfigChange(const ThemeContext& context)
{
    ThemeContext componentContext = context;
    if (componentBreakpoint_.has_value()) {
        componentContext.breakpoint = componentBreakpoint_.value();
    }
    ApplyDefaultLanes(componentContext);
}

PropertyDeclaration ExtendedListComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    if (propertyName == "space") {
        return PropertyDeclaration { .name = "space",
            .type = PropertyValueType::NUMBER,
            .allowDynamic = true,
            .allowExpression = true,
            .fallbackNumber = 0.0,
            .applyValue = [this](const JsonValue& value) { SetSpace(static_cast<float>(value.GetNumberValue(0.0))); } };
    }
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

void ExtendedListComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    ValidateStyleEnumProperty(styles, "listDirection", { "vertical", "horizontal" });
    ValidateStyleEnumProperty(styles, "scrollBar", { "off", "auto", "on" });

    if (!styles.IsObject() || !styles.Has("nestedScroll")) {
        return;
    }

    JsonValue nestedScrollValue = styles.GetItem("nestedScroll");
    if (IsDynamicOrExpressionStyleMember(nestedScrollValue)) {
        return;
    }

    if (nestedScrollValue.IsString()) {
        ValidateStyleEnumProperty(styles, "nestedScroll", { "selfOnly", "selfFirst", "parentFirst", "paraller" });
        return;
    }

    if (!nestedScrollValue.IsObject()) {
        ReportStyleTypeMismatch("styles.nestedScroll", "string or object");
        return;
    }

    auto validateNestedMode = [this, &nestedScrollValue](const char* key) {
        if (key == nullptr || !nestedScrollValue.Has(key)) {
            return;
        }
        JsonValue modeValue = nestedScrollValue.GetItem(key);
        const std::string propertyPath = "styles.nestedScroll." + std::string(key);
        if (IsDynamicOrExpressionStyleMember(modeValue)) {
            return;
        }
        if (!modeValue.IsString()) {
            ReportStyleTypeMismatch(propertyPath, "string");
            return;
        }
        const std::string rawValue = modeValue.GetStringValue("");
        if (rawValue == "selfOnly" || rawValue == "selfFirst" || rawValue == "parentFirst" || rawValue == "paraller") {
            return;
        }
        ReportStyleInvalidValue(propertyPath);
    };
    validateNestedMode("scrollForward");
    validateNestedMode("scrollBackward");
}

void ExtendedListComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "listDirection", { "vertical", "horizontal" });
    ValidateDynamicStyleEnumProperty(styles, dynamicStyleKeys, "scrollBar", { "off", "auto", "on" });

    if (!HasDynamicStyleValue(styles, dynamicStyleKeys, "nestedScroll")) {
        return;
    }

    JsonValue nestedScrollValue = styles.GetItem("nestedScroll");
    const std::string propertyPath = "styles.nestedScroll";
    auto validateNestedMode = [this](const JsonValue& modeValue, const std::string& modePath) {
        if (!modeValue.IsString()) {
            ReportDynamicStyleTypeMismatch(modePath, "string");
            return;
        }
        const std::string mode = modeValue.GetStringValue("");
        if (mode == "selfOnly" || mode == "selfFirst" || mode == "parentFirst" || mode == "paraller") {
            return;
        }
        ReportDynamicStyleInvalidValue(modePath, "is out of enum range");
    };

    if (nestedScrollValue.IsString()) {
        validateNestedMode(nestedScrollValue, propertyPath);
        return;
    }

    if (!nestedScrollValue.IsObject()) {
        ReportDynamicStyleTypeMismatch(propertyPath, "string or object");
        return;
    }

    if (nestedScrollValue.Has("scrollForward")) {
        validateNestedMode(nestedScrollValue.GetItem("scrollForward"), propertyPath + ".scrollForward");
    }
    if (nestedScrollValue.Has("scrollBackward")) {
        validateNestedMode(nestedScrollValue.GetItem("scrollBackward"), propertyPath + ".scrollBackward");
    }
}

void ExtendedListComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    if (styles.Has("listDirection")) {
        SetListDirection(ResolveListDirection(styles.GetItem("listDirection").GetStringValue("vertical")));
    } else if (!isDeltaUpdate) {
        SetListDirection(A2UIAxis::VERTICAL);
    }
    if (styles.Has("scrollBar")) {
        SetScrollBar(ResolveScrollBar(styles.GetItem("scrollBar").GetStringValue("auto")));
    } else if (!isDeltaUpdate) {
        SetScrollBar(A2UIScrollBarDisplayMode::AUTO);
    }
    if (styles.Has("nestedScroll")) {
        ApplyNestedScrollValueOrDefault(styles.GetItem("nestedScroll"));
    } else if (!isDeltaUpdate) {
        SetNestedScroll(DEFAULT_NESTED_SCROLL_MODE, DEFAULT_NESTED_SCROLL_MODE);
    }
}

void ExtendedListComponent::ApplyNestedScrollValueOrDefault(const JsonValue& value)
{
    JsonValue resolvedValue = value;
    std::unique_ptr<JsonAdapter> resolvedAdapter =
        ResolveDynamicStyleObjectMembers(value, GetRenderContext(), GetComponentId());
    if (resolvedAdapter != nullptr) {
        resolvedValue = resolvedAdapter->GetRoot();
    }

    if (resolvedValue.IsObject()) {
        A2UIScrollNestedMode scrollForward =
            ResolveNestedScrollMode(resolvedValue.GetString("scrollForward", DEFAULT_NESTED_SCROLL_MODE_NAME));
        A2UIScrollNestedMode scrollBackward =
            ResolveNestedScrollMode(resolvedValue.GetString("scrollBackward", DEFAULT_NESTED_SCROLL_MODE_NAME));
        SetNestedScroll(scrollForward, scrollBackward);
        return;
    }
    if (resolvedValue.IsString()) {
        A2UIScrollNestedMode mode =
            ResolveNestedScrollMode(resolvedValue.GetStringValue(DEFAULT_NESTED_SCROLL_MODE_NAME));
        SetNestedScroll(mode, mode);
        return;
    }
    SetNestedScroll(DEFAULT_NESTED_SCROLL_MODE, DEFAULT_NESTED_SCROLL_MODE);
}

void ExtendedListComponent::RegisterComponentSpecificListeners()
{
    std::function<void()> onReachStart;
    if (HasEventHandler("onReachStart")) {
        onReachStart = [this]() { DispatchEvent("onReachStart"); };
    }
    RegisterNodeEventHandler(A2UINodeEventType::SCROLL_ON_REACH_START, onReachStart);

    std::function<void()> onReachEnd;
    if (HasEventHandler("onReachEnd")) {
        onReachEnd = [this]() { DispatchEvent("onReachEnd"); };
    }
    RegisterNodeEventHandler(A2UINodeEventType::SCROLL_ON_REACH_END, onReachEnd);
    bool useAreaChange = GetRenderContext().apiVersion < MIN_API_VERSION_SIZE_CHANGE;
    RegisterNodeEventHandlerWithEvent(
        useAreaChange ? A2UINodeEventType::ON_AREA_CHANGE : A2UINodeEventType::ON_SIZE_CHANGE,
        [this, useAreaChange](A2UINodeEvent* event) { HandleSizeChange(event, useAreaChange); });
}

void ExtendedListComponent::HandleSizeChange(A2UINodeEvent* event, bool isAreaChange)
{
    if (event == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedListComponent::HandleSizeChange: event is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }
    A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
    if (componentEvent == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedListComponent::HandleSizeChange: component event is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    float oldWidth = componentEvent->data[0].f32;
    float newWidth = componentEvent->data[isAreaChange ? 6 : 2].f32;
    if (!std::isfinite(newWidth)) {
        LOG_A2UI(LOG_WARN, "ExtendedListComponent::HandleSizeChange: width is not finite, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }
    if (newWidth <= 0.0F || oldWidth == newWidth) {
        return;
    }

    Breakpoint breakpoint = ResolveBreakpointFromWidth(newWidth);
    if (componentBreakpoint_.has_value() && componentBreakpoint_.value() == breakpoint) {
        return;
    }
    componentBreakpoint_ = breakpoint;
    LOG_A2UI(LOG_INFO,
        "ExtendedListComponent::HandleSizeChange: breakpoint updated, componentId=%{public}s, "
        "newWidth=%{public}f, breakpoint=%{public}d",
        GetComponentId().c_str(), newWidth, static_cast<int32_t>(breakpoint));
    OnConfigChange(ResolveThemeContext());
}

void ExtendedListComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    if (mode_ == Mode::LAZY) {
        LOG_A2UI(LOG_INFO, "ExtendedListComponent::OnAddChild: skipped in LAZY mode");
        return;
    }
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }

    ArkUI_NodeHandle listItemNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::LIST_ITEM);
    if (listItemNode == nullptr) {
        return;
    }

    ArkUINodeApiAdapter::AddChild(listItemNode, child->GetNativeView());
    size_t insertIndex = std::min(index, listItems_.size());
    ArkUINodeApiAdapter::InsertChildAt(nativeView_, listItemNode, static_cast<int32_t>(insertIndex));

    ListItemSlot slot { .child = child, .listItemNode = listItemNode };
    listItems_.insert(listItems_.begin() + static_cast<std::ptrdiff_t>(insertIndex), slot);
}

void ExtendedListComponent::OnMoveChild(
    const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    if (child == nullptr || currentIndex >= listItems_.size()) {
        return;
    }

    targetIndex = std::min(targetIndex, listItems_.size() - 1);
    ListItemSlot slot = listItems_[currentIndex];
    listItems_.erase(listItems_.begin() + static_cast<std::ptrdiff_t>(currentIndex));
    listItems_.insert(listItems_.begin() + static_cast<std::ptrdiff_t>(targetIndex), slot);

    if (nativeView_ != nullptr) {
        ArkUINodeApiAdapter::InsertChildAt(nativeView_, slot.listItemNode, static_cast<int32_t>(targetIndex));
    }
}

void ExtendedListComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    if (child == nullptr) {
        return;
    }

    auto it = std::find_if(listItems_.begin(), listItems_.end(),
        [&child](const ListItemSlot& slot) { return slot.child.lock() == child; });
    if (it == listItems_.end()) {
        return;
    }

    RemoveListItemNode(it->listItemNode);
    listItems_.erase(it);
}

void ExtendedListComponent::RemoveAllChildren()
{
    for (const auto& slot : listItems_) {
        RemoveListItemNode(slot.listItemNode);
    }
    listItems_.clear();
    Component::RemoveAllChildren();
}

void ExtendedListComponent::RemoveListItemNode(ArkUI_NodeHandle listItemNode)
{
    if (listItemNode == nullptr) {
        return;
    }
    if (nativeView_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, listItemNode);
    }
    ArkUINodeApiAdapter::DisposeNode(listItemNode);
}

void ExtendedListComponent::ApplyDefaultLanes(const ThemeContext& context)
{
    ExtendedListTheme theme(context);
    SetLanes(theme.GetLanes());
}

ThemeContext ExtendedListComponent::ResolveThemeContext() const
{
    std::shared_ptr<ThemeManager> themeManager = GetThemeManager();
    ThemeContext context;
    if (themeManager != nullptr) {
        context = themeManager->GetContext();
    }
    if (componentBreakpoint_.has_value()) {
        context.breakpoint = componentBreakpoint_.value();
    }
    return context;
}

void ExtendedListComponent::SetSpace(float space)
{
    if (!std::isfinite(space) || space < 0.0F) {
        ReportExtendedSchemaWarning(SCHEMA_ERROR_CODE_INVALID_VALUE,
            "Property space must be greater than or equal to 0 and has been reset to default", "space");
    }
    ArkUINodeApiAdapter::SetNodeListSpace(nativeView_, NormalizeListSpace(space));
}

void ExtendedListComponent::SetLanes(int32_t lanes)
{
    ArkUINodeApiAdapter::SetNodeListLanes(nativeView_, lanes);
}

void ExtendedListComponent::SetListDirection(A2UIAxis direction)
{
    ArkUINodeApiAdapter::SetNodeListDirection(nativeView_, direction);
}

void ExtendedListComponent::SetScrollBar(A2UIScrollBarDisplayMode displayMode)
{
    ArkUINodeApiAdapter::SetNodeScrollBarDisplayMode(nativeView_, displayMode);
}

void ExtendedListComponent::SetNestedScroll(A2UIScrollNestedMode scrollForward, A2UIScrollNestedMode scrollBackward)
{
    ArkUINodeApiAdapter::SetNodeScrollNestedScroll(nativeView_, scrollForward, scrollBackward);
}

} // namespace NativeModule
