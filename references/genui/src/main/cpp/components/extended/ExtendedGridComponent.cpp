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

#include "ExtendedGridComponent.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <sstream>
#include <string>

#include "components/ChildListSchemaValidationUtils.h"
#include "data/DataModel.h"
#include "data/DynamicValueResolver.h"
#include "styles/StyleApplyUtils.h"
#include "theme/ThemeManager.h"
#include "utils/LogA2UI.h"

#include "../../SurfaceSlot.h"
#include "A2UIArkUITypeConverter.h"
#include "ExtendedGridTheme.h"

namespace NativeModule {

namespace {

constexpr char DEFAULT_GRID_TEMPLATE[] = "1fr";
constexpr int32_t GRID_ITEM_ALIGNMENT_DEFAULT_VALUE = 0;
constexpr std::array<const char*, 5> GRID_TEMPLATE_BREAKPOINT_KEYS = { "xs", "sm", "md", "lg", "xl" };

struct ExtendedGridLazyAdapterConfig {
    std::string templateComponentId;
    std::string templatePath;
    std::shared_ptr<DataModel> dataModel;
    JsonValue templateDescriptor;
    std::map<std::string, JsonValue> allDescriptors;
    std::string surfaceId;
    int32_t renderId = -1;
    SurfaceContext surfaceContext;
};

void InitializeLazyAdapterTemplate(GridAdapterNode& adapterNode, const ExtendedGridLazyAdapterConfig& config,
    const ChildListDescriptor& childList, int itemCount)
{
    adapterNode.Initialize(config.templateComponentId, config.templatePath, itemCount, childList.resolvedIndexVarName,
        childList.resolvedItemVarName);
}

void ConfigureLazyAdapterData(GridAdapterNode& adapterNode, const ExtendedGridLazyAdapterConfig& config,
    const std::map<std::string, JsonValue>& inheritedLocalVariables)
{
    adapterNode.SetDataModel(config.dataModel);
    adapterNode.SetTemplateDescriptor(config.templateDescriptor);
    adapterNode.SetAllDescriptors(config.allDescriptors);
    adapterNode.SetInheritedLocalVariables(inheritedLocalVariables);
}

void ConfigureLazyAdapterSurface(
    GridAdapterNode& adapterNode, const ExtendedGridLazyAdapterConfig& config, bool wrapContentHeight)
{
    adapterNode.SetSurfaceInfo(config.surfaceId, config.renderId);
    adapterNode.SetSurfaceContext(config.surfaceContext);
    adapterNode.SetGridItemHeightWrapContent(wrapContentHeight);
}

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

float NormalizeGridGap(float gap)
{
    return gap < 0.0F ? 0.0F : gap;
}

bool IsSupportedGridFractionToken(const std::string& token)
{
    if (token.size() <= 2 || token.compare(token.size() - 2, 2, "fr") != 0) {
        return false;
    }

    bool hasDigit = false;
    bool hasDecimalPoint = false;
    for (size_t index = 0; index < token.size() - 2; ++index) {
        const char current = token[index];
        if (current >= '0' && current <= '9') {
            hasDigit = true;
            continue;
        }
        if (current == '.' && !hasDecimalPoint) {
            hasDecimalPoint = true;
            continue;
        }
        return false;
    }
    return hasDigit;
}

bool IsSupportedGridTemplate(const std::string& templateValue)
{
    std::istringstream stream(templateValue);
    std::string token;
    bool hasToken = false;
    while (stream >> token) {
        hasToken = true;
        if (!IsSupportedGridFractionToken(token)) {
            return false;
        }
    }
    return hasToken;
}

void ApplyGridItemHeightPolicy(ArkUI_NodeHandle gridItemNode, bool wrapContent)
{
    if (gridItemNode == nullptr) {
        return;
    }
    if (!wrapContent) {
        ArkUINodeApiAdapter::ResetNodeHeightLayoutPolicy(gridItemNode);
        return;
    }
    ArkUINodeApiAdapter::SetNodeHeightLayoutPolicy(
        gridItemNode, A2UIArkUITypeConverter::ToArkUILayoutPolicy(A2UILayoutPolicy::WRAP_CONTENT));
}

std::string ResolveGridTemplateOrDefault(const JsonValue& value)
{
    if (!value.IsString()) {
        return DEFAULT_GRID_TEMPLATE;
    }
    std::string templateValue = value.GetStringValue("");
    if (!IsSupportedGridTemplate(templateValue)) {
        return DEFAULT_GRID_TEMPLATE;
    }
    return templateValue;
}

size_t BreakpointToIndex(Breakpoint breakpoint)
{
    switch (breakpoint) {
        case Breakpoint::XS:
            return 0;
        case Breakpoint::SM:
            return 1;
        case Breakpoint::MD:
            return 2;
        case Breakpoint::LG:
            return 3;
        case Breakpoint::XL:
            return 4;
        default:
            return 1;
    }
}

std::optional<int> PrepareGridAdapterConfig(ExtendedGridLazyAdapterConfig& config, const ChildListDescriptor& childList,
    SurfaceSlot& surfaceSlot, const std::string& componentId)
{
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();
    auto templateIt = descriptorStore.find(childList.templateComponentId);
    if (templateIt == descriptorStore.end()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedGridComponent::SetupLazyAdapter: template not found, componentId=%{public}s, "
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
                "ExtendedGridComponent::SetupLazyAdapter: data model is null, templateComponentId=%{public}s",
                config.templateComponentId.c_str());
            return std::nullopt;
        }
        auto arrayOpt = config.dataModel->GetNode(config.templatePath);
        if (!arrayOpt.has_value()) {
            DynamicResolveContext context = { .renderId = config.renderId,
                .surfaceId = config.surfaceId,
                .componentId = componentId,
                .missingPathPolicy = MissingPathPolicy::DEFER_UNTIL_DATA_UPDATE };
            DynamicValueResolver::ReportMissingPath(context, config.templatePath);
            LOG_A2UI(LOG_ERROR, "ExtendedGridComponent::SetupLazyAdapter: data path not found, path=%{public}s",
                config.templatePath.c_str());
            return itemCount;
        }
        if (arrayOpt.value().IsArray()) {
            itemCount = arrayOpt.value().GetArraySize();
        }
    }
    return itemCount;
}

} // namespace

ExtendedGridComponent::ExtendedGridComponent() : ExtendedComponent(ArkUINodeApiAdapter::CreateNode(A2UINodeType::GRID))
{
    columnsTemplateConfig_.mode = TemplateMode::THEME_DEFAULT;
}

ExtendedGridComponent::~ExtendedGridComponent()
{
    RemoveAllChildren();
}

std::string ExtendedGridComponent::GetType() const
{
    return "Grid";
}

void ExtendedGridComponent::SetLazyMode(bool useLazy)
{
    mode_ = useLazy ? Mode::LAZY : Mode::EAGER;
    if (!useLazy || adapterNode_ == nullptr || nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridNodeAdapter(nativeView_, adapterNode_->GetHandle());
}

void ExtendedGridComponent::SetAdapterNode(const std::shared_ptr<GridAdapterNode>& adapterNode)
{
    adapterNode_ = adapterNode;
    if (adapterNode_ != nullptr) {
        adapterNode_->SetGridItemHeightWrapContent(ShouldGridItemsWrapContentHeight());
    }
    if (adapterNode_ == nullptr || mode_ != Mode::LAZY || nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridNodeAdapter(nativeView_, adapterNode_->GetHandle());
}

bool ExtendedGridComponent::SetupLazyAdapter(const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot)
{
    if (nativeView_ == nullptr) {
        return false;
    }
    ExtendedGridLazyAdapterConfig config;
    auto itemCountOpt = PrepareGridAdapterConfig(config, childList, surfaceSlot, GetComponentId());
    if (!itemCountOpt.has_value()) {
        return false;
    }
    int itemCount = *itemCountOpt;

    bool wrapContentHeight = ShouldGridItemsWrapContentHeight();
    if (adapterNode_ == nullptr) {
        auto adapterNode = std::make_shared<GridAdapterNode>();
        InitializeLazyAdapterTemplate(*adapterNode, config, childList, itemCount);
        ConfigureLazyAdapterData(*adapterNode, config, GetLocalVariables());
        ConfigureLazyAdapterSurface(*adapterNode, config, wrapContentHeight);
        adapterNode_ = adapterNode;
    } else {
        InitializeLazyAdapterTemplate(*adapterNode_, config, childList, itemCount);
        ConfigureLazyAdapterData(*adapterNode_, config, GetLocalVariables());
        ConfigureLazyAdapterSurface(*adapterNode_, config, wrapContentHeight);
        adapterNode_->IncrementTemplateVersion();
        adapterNode_->UpdateItemCount(itemCount);
        adapterNode_->ReloadAllItems();
    }

    int32_t result = ArkUINodeApiAdapter::SetNodeGridNodeAdapter(nativeView_, adapterNode_->GetHandle());
    if (result != 0) {
        LOG_A2UI(
            LOG_WARN, "ExtendedGridComponent::SetupLazyAdapter: set node adapter failed, result=%{public}d", result);
        mode_ = Mode::EAGER;
        return false;
    }
    mode_ = Mode::LAZY;
    adapterNode_->ReloadAllItems();
    return true;
}

void ExtendedGridComponent::ApplyPrivateAttributes(const JsonValue& descriptor)
{
    static_cast<void>(descriptor);
    ArkUINodeApiAdapter::SetNodePixelRoundNoForceRound(nativeView_, GetRenderContext().apiVersion);
    ArkUINodeApiAdapter::SetNodeGridAlignItems(nativeView_, GRID_ITEM_ALIGNMENT_DEFAULT_VALUE);
    ApplyColumnsTemplateForContext(ResolveThemeContext());
    SetColumnsGap(0.0F);
    SetRowsGap(0.0F);
}

void ExtendedGridComponent::ValidateComponentDescriptorSchema(const JsonValue& descriptor)
{
    for (const auto& issue : ValidateChildListSchema(descriptor, "children", ChildListEmptyArrayPolicy::ALLOW)) {
        ReportExtendedSchemaWarning(issue.code, issue.message, issue.propertyPath);
    }
}

void ExtendedGridComponent::ValidateComponentSpecificStylesSchema(const JsonValue& styles)
{
    auto validateTemplateStyle = [this, &styles](const char* styleName) {
        if (!styles.IsObject() || styleName == nullptr || !styles.Has(styleName)) {
            return;
        }

        JsonValue value = styles.GetItem(styleName);
        if (IsDynamicOrExpressionStyleMember(value)) {
            return;
        }

        const std::string propertyPath = "styles." + std::string(styleName);
        if (value.IsString()) {
            if (!IsSupportedGridTemplate(value.GetStringValue(""))) {
                ReportStyleInvalidValue(propertyPath);
            }
            return;
        }

        if (!value.IsObject()) {
            ReportStyleTypeMismatch(propertyPath, "string or object");
            return;
        }

        bool hasAnyResponsiveValue = false;
        for (const char* breakpointKey : GRID_TEMPLATE_BREAKPOINT_KEYS) {
            if (breakpointKey == nullptr || !value.Has(breakpointKey)) {
                continue;
            }

            JsonValue breakpointValue = value.GetItem(breakpointKey);
            const std::string breakpointPath = propertyPath + "." + breakpointKey;
            if (IsDynamicOrExpressionStyleMember(breakpointValue)) {
                hasAnyResponsiveValue = true;
                continue;
            }
            if (!breakpointValue.IsString()) {
                ReportStyleTypeMismatch(breakpointPath, "string");
                return;
            }
            if (!IsSupportedGridTemplate(breakpointValue.GetStringValue(""))) {
                ReportStyleInvalidValue(breakpointPath);
                return;
            }
            hasAnyResponsiveValue = true;
        }

        if (!hasAnyResponsiveValue) {
            ReportStyleInvalidValue(propertyPath);
        }
    };

    validateTemplateStyle("columnsTemplate");
    validateTemplateStyle("rowsTemplate");
    ValidateStyleNumberProperty(styles, "columnsGap");
    ValidateStyleNumberProperty(styles, "rowsGap");
}

void ExtendedGridComponent::ValidateComponentSpecificDynamicStylesDfx(
    const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys)
{
    auto validateTemplateStyle = [this, &styles, &dynamicStyleKeys](const char* styleName) {
        if (styleName == nullptr || !HasDynamicStyleValue(styles, dynamicStyleKeys, styleName)) {
            return;
        }

        JsonValue value = styles.GetItem(styleName);
        const std::string propertyPath = "styles." + std::string(styleName);
        if (value.IsString()) {
            if (!IsSupportedGridTemplate(value.GetStringValue(""))) {
                ReportDynamicStyleInvalidValue(propertyPath, "is not a supported fr template");
            }
            return;
        }

        if (!value.IsObject()) {
            ReportDynamicStyleTypeMismatch(propertyPath, "string or object");
            return;
        }

        bool hasAnyResponsiveValue = false;
        for (const char* breakpointKey : GRID_TEMPLATE_BREAKPOINT_KEYS) {
            if (breakpointKey == nullptr || !value.Has(breakpointKey)) {
                continue;
            }

            JsonValue breakpointValue = value.GetItem(breakpointKey);
            const std::string breakpointPath = propertyPath + "." + breakpointKey;
            if (!breakpointValue.IsString()) {
                ReportDynamicStyleTypeMismatch(breakpointPath, "string");
                return;
            }
            if (!IsSupportedGridTemplate(breakpointValue.GetStringValue(""))) {
                ReportDynamicStyleInvalidValue(breakpointPath, "is not a supported fr template");
                return;
            }
            hasAnyResponsiveValue = true;
        }

        if (!hasAnyResponsiveValue) {
            ReportDynamicStyleInvalidValue(propertyPath, "is empty");
        }
    };

    validateTemplateStyle("columnsTemplate");
    validateTemplateStyle("rowsTemplate");
    ValidateDynamicStyleNumberProperty(styles, dynamicStyleKeys, "columnsGap");
    ValidateDynamicStyleNumberProperty(styles, dynamicStyleKeys, "rowsGap");
}

void ExtendedGridComponent::OnConfigChange(const ThemeContext& context)
{
    ThemeContext componentContext = context;
    if (componentBreakpoint_.has_value()) {
        componentContext.breakpoint = componentBreakpoint_.value();
    }
    ApplyColumnsTemplateForContext(componentContext);
    ApplyRowsTemplateForContext(componentContext);
}

PropertyDeclaration ExtendedGridComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

void ExtendedGridComponent::ApplyColumnsTemplateStyle(const JsonValue& styles, bool isDeltaUpdate)
{
    if (styles.Has("columnsTemplate")) {
        JsonValue columnsTemplateValue = styles.GetItem("columnsTemplate");
        std::unique_ptr<JsonAdapter> resolvedColumnsTemplate =
            ResolveDynamicStyleObjectMembers(columnsTemplateValue, GetRenderContext(), GetComponentId());
        if (resolvedColumnsTemplate != nullptr) {
            columnsTemplateValue = resolvedColumnsTemplate->GetRoot();
        }
        if (!ParseTemplateConfig(columnsTemplateValue, columnsTemplateConfig_)) {
            columnsTemplateConfig_.mode = TemplateMode::FIXED;
            columnsTemplateConfig_.fixedValue = DEFAULT_GRID_TEMPLATE;
            columnsTemplateConfig_.responsiveValues.fill("");
        }
        ApplyColumnsTemplateForContext(ResolveThemeContext());
    } else if (!isDeltaUpdate) {
        columnsTemplateConfig_.mode = TemplateMode::THEME_DEFAULT;
        columnsTemplateConfig_.fixedValue.clear();
        columnsTemplateConfig_.responsiveValues.fill("");
        ApplyColumnsTemplateForContext(ResolveThemeContext());
    }
}

void ExtendedGridComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
    ApplyColumnsTemplateStyle(styles, isDeltaUpdate);
    if (styles.Has("rowsTemplate")) {
        JsonValue rowsTemplateValue = styles.GetItem("rowsTemplate");
        std::unique_ptr<JsonAdapter> resolvedRowsTemplate =
            ResolveDynamicStyleObjectMembers(rowsTemplateValue, GetRenderContext(), GetComponentId());
        if (resolvedRowsTemplate != nullptr) {
            rowsTemplateValue = resolvedRowsTemplate->GetRoot();
        }
        if (!ParseTemplateConfig(rowsTemplateValue, rowsTemplateConfig_)) {
            rowsTemplateConfig_.mode = TemplateMode::FIXED;
            rowsTemplateConfig_.fixedValue = DEFAULT_GRID_TEMPLATE;
            rowsTemplateConfig_.responsiveValues.fill("");
        }
        ApplyRowsTemplateForContext(ResolveThemeContext());
        RefreshGridItemHeightPolicies();
    } else if (!isDeltaUpdate) {
        rowsTemplateConfig_.mode = TemplateMode::RESET;
        rowsTemplateConfig_.fixedValue.clear();
        rowsTemplateConfig_.responsiveValues.fill("");
        RefreshGridItemHeightPolicies();
    }
    if (styles.Has("columnsGap")) {
        SetColumnsGap(static_cast<float>(styles.GetItem("columnsGap").GetNumberValue(0.0)));
    } else if (!isDeltaUpdate) {
        SetColumnsGap(0.0F);
    }
    if (styles.Has("rowsGap")) {
        SetRowsGap(static_cast<float>(styles.GetItem("rowsGap").GetNumberValue(0.0)));
    } else if (!isDeltaUpdate) {
        SetRowsGap(0.0F);
    }
}

void ExtendedGridComponent::RegisterComponentSpecificListeners()
{
    bool useAreaChange = GetRenderContext().apiVersion < MIN_API_VERSION_SIZE_CHANGE;
    RegisterNodeEventHandlerWithEvent(
        useAreaChange ? A2UINodeEventType::ON_AREA_CHANGE : A2UINodeEventType::ON_SIZE_CHANGE,
        [this, useAreaChange](A2UINodeEvent* event) { HandleSizeChange(event, useAreaChange); });
}

void ExtendedGridComponent::HandleSizeChange(A2UINodeEvent* event, bool isAreaChange)
{
    if (event == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedGridComponent::HandleSizeChange: event is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }
    A2UINodeComponentEvent* componentEvent = ArkUIOHApiAdapter::NodeEventGetNodeComponentEvent(event);
    if (componentEvent == nullptr) {
        LOG_A2UI(LOG_WARN, "ExtendedGridComponent::HandleSizeChange: component event is null, componentId=%{public}s",
            GetComponentId().c_str());
        return;
    }

    float oldWidth = componentEvent->data[0].f32;
    float newWidth = componentEvent->data[isAreaChange ? 6 : 2].f32;
    if (!std::isfinite(newWidth)) {
        LOG_A2UI(LOG_WARN, "ExtendedGridComponent::HandleSizeChange: width is not finite, componentId=%{public}s",
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
        "ExtendedGridComponent::HandleSizeChange: breakpoint updated, componentId=%{public}s, "
        "newWidth=%{public}f, breakpoint=%{public}d",
        GetComponentId().c_str(), newWidth, static_cast<int32_t>(breakpoint));
    OnConfigChange(ResolveThemeContext());
}

void ExtendedGridComponent::OnAddChild(const std::shared_ptr<Component>& child, size_t index)
{
    if (mode_ == Mode::LAZY) {
        LOG_A2UI(LOG_INFO, "ExtendedGridComponent::OnAddChild: skipped in LAZY mode");
        return;
    }
    if (nativeView_ == nullptr || child == nullptr || child->GetNativeView() == nullptr) {
        return;
    }

    ArkUI_NodeHandle itemNode = ArkUINodeApiAdapter::CreateNode(A2UINodeType::GRID_ITEM);
    if (itemNode == nullptr) {
        return;
    }
    ApplyGridItemHeightPolicy(itemNode, ShouldGridItemsWrapContentHeight());
    ArkUINodeApiAdapter::AddChild(itemNode, child->GetNativeView());

    size_t insertIndex = std::min(index, gridItems_.size());
    ArkUINodeApiAdapter::InsertChildAt(nativeView_, itemNode, static_cast<int32_t>(insertIndex));

    GridItemSlot slot { .child = child, .itemNode = itemNode };
    gridItems_.insert(gridItems_.begin() + static_cast<std::ptrdiff_t>(insertIndex), slot);
}

void ExtendedGridComponent::OnMoveChild(
    const std::shared_ptr<Component>& child, size_t currentIndex, size_t targetIndex)
{
    if (child == nullptr || currentIndex >= gridItems_.size()) {
        return;
    }

    targetIndex = std::min(targetIndex, gridItems_.size() - 1);
    GridItemSlot slot = gridItems_[currentIndex];
    gridItems_.erase(gridItems_.begin() + static_cast<std::ptrdiff_t>(currentIndex));
    gridItems_.insert(gridItems_.begin() + static_cast<std::ptrdiff_t>(targetIndex), slot);

    if (nativeView_ != nullptr) {
        ArkUINodeApiAdapter::InsertChildAt(nativeView_, slot.itemNode, static_cast<int32_t>(targetIndex));
    }
}

void ExtendedGridComponent::OnRemoveChild(const std::shared_ptr<Component>& child)
{
    if (child == nullptr) {
        return;
    }

    auto it = std::find_if(gridItems_.begin(), gridItems_.end(),
        [&child](const GridItemSlot& slot) { return slot.child.lock() == child; });
    if (it == gridItems_.end()) {
        return;
    }

    DetachGridItemNode(it->itemNode);
    gridItems_.erase(it);
}

bool ExtendedGridComponent::ExpandTemplateChildren(
    const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds)
{
    childIds.clear();
    if (SetupLazyAdapter(childList, surfaceSlot)) {
        surfaceSlot.OnTemplateExpansionResolved(GetComponentId());
        return false;
    }
    mode_ = Mode::EAGER;
    bool result = ExpandTemplateChildrenEager(childList, surfaceSlot, childIds);
    if (result) {
        surfaceSlot.OnTemplateExpansionResolved(GetComponentId());
    } else {
        surfaceSlot.OnTemplateExpansionDeferred(GetComponentId());
    }
    return result;
}

void ExtendedGridComponent::RemoveAllChildren()
{
    for (const auto& slot : gridItems_) {
        DetachGridItemNode(slot.itemNode);
    }
    gridItems_.clear();
    Component::RemoveAllChildren();
}

void ExtendedGridComponent::ApplyColumnsTemplateForContext(const ThemeContext& context)
{
    switch (columnsTemplateConfig_.mode) {
        case TemplateMode::THEME_DEFAULT: {
            ExtendedGridTheme theme(context);
            SetColumnsTemplate(theme.GetColumnsTemplate());
            return;
        }
        case TemplateMode::RESPONSIVE:
            SetColumnsTemplate(ResolveResponsiveTemplate(columnsTemplateConfig_, context));
            return;
        case TemplateMode::FIXED:
            SetColumnsTemplate(columnsTemplateConfig_.fixedValue);
            return;
        case TemplateMode::RESET:
        default: {
            // Columns reset to the theme-driven default rather than issuing a native reset.
            ExtendedGridTheme theme(context);
            SetColumnsTemplate(theme.GetColumnsTemplate());
            return;
        }
    }
}

void ExtendedGridComponent::ApplyRowsTemplateForContext(const ThemeContext& context)
{
    if (nativeView_ == nullptr) {
        return;
    }

    switch (rowsTemplateConfig_.mode) {
        case TemplateMode::RESPONSIVE:
            SetRowsTemplate(ResolveResponsiveTemplate(rowsTemplateConfig_, context));
            return;
        case TemplateMode::FIXED:
            SetRowsTemplate(rowsTemplateConfig_.fixedValue);
            return;
        case TemplateMode::RESET:
        case TemplateMode::THEME_DEFAULT:
        default:
            // NODE_GRID_ROW_TEMPLATE reset maps to native "1fr", which switches Grid into
            // static layout when columnsTemplate is also set. Leave rows untouched so a
            // columns-only Grid stays in scrollable auto-row mode.
            return;
    }
}

ThemeContext ExtendedGridComponent::ResolveThemeContext() const
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

bool ExtendedGridComponent::ParseTemplateConfig(const JsonValue& value, GridTemplateConfig& config)
{
    GridTemplateConfig parsedConfig;
    if (value.IsString()) {
        parsedConfig.mode = TemplateMode::FIXED;
        parsedConfig.fixedValue = ResolveGridTemplateOrDefault(value);
        config = parsedConfig;
        return true;
    }
    if (!value.IsObject()) {
        return false;
    }

    parsedConfig.mode = TemplateMode::RESPONSIVE;
    bool hasAnyResponsiveValue = false;
    for (size_t index = 0; index < GRID_TEMPLATE_BREAKPOINT_KEYS.size(); ++index) {
        JsonValue breakpointValue = value.GetItem(GRID_TEMPLATE_BREAKPOINT_KEYS[index]);
        if (!breakpointValue.IsValid()) {
            continue;
        }
        if (!breakpointValue.IsString()) {
            return false;
        }
        std::string templateValue = breakpointValue.GetStringValue("");
        if (templateValue.empty()) {
            continue;
        }
        if (!IsSupportedGridTemplate(templateValue)) {
            return false;
        }
        parsedConfig.responsiveValues[index] = templateValue;
        hasAnyResponsiveValue = true;
    }
    if (!hasAnyResponsiveValue) {
        return false;
    }

    config = parsedConfig;
    return true;
}

std::string ExtendedGridComponent::ResolveResponsiveTemplate(
    const GridTemplateConfig& config, const ThemeContext& context)
{
    int32_t currentIndex = static_cast<int32_t>(BreakpointToIndex(context.breakpoint));
    for (int32_t index = currentIndex; index >= 0; --index) {
        const std::string& candidate = config.responsiveValues[static_cast<size_t>(index)];
        if (!candidate.empty()) {
            return candidate;
        }
    }
    for (size_t index = static_cast<size_t>(currentIndex + 1); index < config.responsiveValues.size(); ++index) {
        const std::string& candidate = config.responsiveValues[index];
        if (!candidate.empty()) {
            return candidate;
        }
    }
    return DEFAULT_GRID_TEMPLATE;
}

void ExtendedGridComponent::SetColumnsTemplate(const std::string& columnsTemplate)
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridColumnTemplate(nativeView_, columnsTemplate);
}

void ExtendedGridComponent::SetRowsTemplate(const std::string& rowsTemplate)
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridRowTemplate(nativeView_, rowsTemplate);
}

void ExtendedGridComponent::SetColumnsGap(float columnsGap)
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridColumnGap(nativeView_, NormalizeGridGap(columnsGap));
}

void ExtendedGridComponent::SetRowsGap(float rowsGap)
{
    if (nativeView_ == nullptr) {
        return;
    }
    ArkUINodeApiAdapter::SetNodeGridRowGap(nativeView_, NormalizeGridGap(rowsGap));
}

void ExtendedGridComponent::RefreshGridItemHeightPolicies()
{
    bool wrapContent = ShouldGridItemsWrapContentHeight();
    for (const auto& slot : gridItems_) {
        ApplyGridItemHeightPolicy(slot.itemNode, wrapContent);
    }
    if (adapterNode_ != nullptr) {
        adapterNode_->SetGridItemHeightWrapContent(wrapContent);
    }
}

bool ExtendedGridComponent::ShouldGridItemsWrapContentHeight() const
{
    return rowsTemplateConfig_.mode == TemplateMode::RESET || rowsTemplateConfig_.mode == TemplateMode::THEME_DEFAULT;
}

void ExtendedGridComponent::DetachGridItemNode(ArkUI_NodeHandle itemNode)
{
    if (itemNode == nullptr) {
        return;
    }
    if (nativeView_ != nullptr) {
        ArkUINodeApiAdapter::RemoveChild(nativeView_, itemNode);
    }
    ArkUINodeApiAdapter::DisposeNode(itemNode);
}

} // namespace NativeModule
