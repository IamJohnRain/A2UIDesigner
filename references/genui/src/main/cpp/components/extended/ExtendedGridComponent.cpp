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
#include <map>
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
        .allowExpression = true };
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
    if (templateValue.empty()) {
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
    const std::map<std::string, JsonValue>& descriptorStore = surfaceSlot.GetAllComponentDescriptorStore();
    auto templateIt = descriptorStore.find(childList.templateComponentId);
    if (templateIt == descriptorStore.end()) {
        LOG_A2UI(LOG_WARN,
            "ExtendedGridComponent::SetupLazyAdapter: template not found, componentId=%{public}s, "
            "templateId=%{public}s",
            GetComponentId().c_str(), childList.templateComponentId.c_str());
        return false;
    }

    ExtendedGridLazyAdapterConfig config;
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
            return false;
        }
        auto arrayOpt = config.dataModel->GetNode(config.templatePath);
        if (!arrayOpt.has_value()) {
            LOG_A2UI(LOG_ERROR, "ExtendedGridComponent::SetupLazyAdapter: data path not found, path=%{public}s",
                config.templatePath.c_str());
            return false;
        }
        itemCount = arrayOpt.value().GetArraySize();
    }

    if (adapterNode_ == nullptr) {
        auto adapterNode = std::make_shared<GridAdapterNode>();
        adapterNode->Initialize(config.templateComponentId, config.templatePath, itemCount,
            childList.resolvedIndexVarName, childList.resolvedItemVarName);
        adapterNode->SetDataModel(config.dataModel);
        adapterNode->SetTemplateDescriptor(config.templateDescriptor);
        adapterNode->SetAllDescriptors(config.allDescriptors);
        adapterNode->SetInheritedLocalVariables(GetLocalVariables());
        adapterNode->SetSurfaceInfo(config.surfaceId, config.renderId);
        adapterNode->SetSurfaceContext(config.surfaceContext);
        adapterNode->SetGridItemHeightWrapContent(ShouldGridItemsWrapContentHeight());
        adapterNode_ = adapterNode;
    } else {
        adapterNode_->Initialize(config.templateComponentId, config.templatePath, itemCount,
            childList.resolvedIndexVarName, childList.resolvedItemVarName);
        adapterNode_->SetDataModel(config.dataModel);
        adapterNode_->SetTemplateDescriptor(config.templateDescriptor);
        adapterNode_->SetAllDescriptors(config.allDescriptors);
        adapterNode_->SetInheritedLocalVariables(GetLocalVariables());
        adapterNode_->SetSurfaceInfo(config.surfaceId, config.renderId);
        adapterNode_->SetSurfaceContext(config.surfaceContext);
        adapterNode_->SetGridItemHeightWrapContent(ShouldGridItemsWrapContentHeight());
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
            if (value.GetStringValue("").empty()) {
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
            if (breakpointValue.GetStringValue("").empty()) {
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
            if (value.GetStringValue("").empty()) {
                ReportDynamicStyleInvalidValue(propertyPath, "is empty");
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
            if (breakpointValue.GetStringValue("").empty()) {
                ReportDynamicStyleInvalidValue(breakpointPath, "is empty");
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
    ApplyColumnsTemplateForContext(context);
    ApplyRowsTemplateForContext(context);
}

PropertyDeclaration ExtendedGridComponent::GetPrivatePropertyDeclaration(const std::string& propertyName)
{
    return ExtendedComponent::GetPrivatePropertyDeclaration(propertyName);
}

void ExtendedGridComponent::ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier)
{
    static_cast<void>(applier);
    if (!styles.IsObject()) {
        return;
    }
    bool isDeltaUpdate = IsApplyingStyleDeltaUpdate();
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
    if (themeManager != nullptr) {
        return themeManager->GetContext();
    }
    return ThemeContext();
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
