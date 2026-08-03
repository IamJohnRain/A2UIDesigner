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

#ifndef A2UI_EXTENDED_COMPONENT_H
#define A2UI_EXTENDED_COMPONENT_H

#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "components/A2UI/A2UIComponent.h"
#include "components/actions/EventHandlerParser.h"
#include "functions/ActionInfo.h"

#include "ArkUINodeApiAdapter.h"
#include "ExtendedCommonTheme.h"
#include "RenderContext.h"

namespace NativeModule {

class DataModel;
class SurfaceSlot;

class ExtendedComponent : public A2UIComponent {
public:
    explicit ExtendedComponent(ArkUI_NodeHandle nativeView, bool ownsNativeView = true, bool isCompositeType = false);
    ~ExtendedComponent() override = default;

    bool InitFromDescriptor(const JsonValue& descriptor, const RenderContext& context);
    bool UpdateFromDescriptor(const JsonValue& descriptor, const RenderContext& context);

    std::shared_ptr<ExtendedCommonTheme> GetCommonTheme();

protected:
    virtual bool CreateArkUINode();
    virtual void ApplyComponentSpecificAttributes(const JsonValue& normalizedDescriptor);
    virtual void ApplyComponentSpecificStyles(const JsonValue& styles, ArkUINodeApiAdapter& applier);
    virtual void ValidateComponentSpecificStylesSchema(const JsonValue& styles);
    virtual void ValidateComponentSpecificDynamicStylesDfx(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys);
    virtual void RegisterClickHandler();
    virtual void RegisterComponentSpecificListeners();
    void RegisterExtendedListeners();
    bool IsExpressionSupported() const override;
    bool IsExpressionCandidate(const JsonValue& value) const override;
    std::shared_ptr<DataModel> GetDynamicResolveDataModel() const override;
    bool IsKnownAdditionalDescriptorKey(const std::string& propertyName) const override;
    virtual void OnFontSizeScaleChanged(float newScale);
    void OnDataUpdate(const std::string& property, const JsonValue& value) override;
    void OnConfigChange(const ThemeContext& context) override;
    void CollectChildListDescriptor(const JsonValue& descriptor) override;
    bool ExpandTemplateChildren(
        const ChildListDescriptor& childList, SurfaceSlot& surfaceSlot, std::list<std::string>& childIds) override;

    void ApplyDeclaredPropertyOrFallback(const JsonValue& descriptor, const std::string& propertyName);
    bool HasEventHandler(const std::string& eventName) const;
    void DispatchEvent(const std::string& eventName, const JsonValue& extraContext = JsonValue()) const;
    void DispatchActionInfo(const std::string& actionName, const std::shared_ptr<ActionInfo>& actionInfo,
        const JsonValue& extraContext = JsonValue()) const;
    const EventHandlerMap& GetEventHandlers() const;
    const RenderContext& GetRenderContext() const;
    ArkUINodeApiAdapter* GetNodeApplier() const;
    bool IsApplyingStyleDeltaUpdate() const;
    void ReportExtendedSchemaWarning(
        const std::string& code, const std::string& message, const std::string& propertyPath) const;
    void ReportStyleTypeMismatch(const std::string& propertyPath, const std::string& expectedType) const;
    void ReportStyleInvalidValue(const std::string& propertyPath) const;
    void ValidateStyleEnumProperty(
        const JsonValue& styles, const std::string& styleName, std::initializer_list<const char*> allowedValues) const;
    void ValidateStyleStringProperty(const JsonValue& styles, const std::string& styleName) const;
    void ValidateStyleNumberProperty(
        const JsonValue& styles, const std::string& styleName, double minimumValue = 0.0) const;
    bool HasDynamicStyleValue(
        const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys, const std::string& styleName) const;
    void ReportDynamicStyleTypeMismatch(const std::string& propertyPath, const std::string& expectedType) const;
    void ReportDynamicStyleInvalidValue(const std::string& propertyPath, const std::string& reason) const;
    void ValidateDynamicStyleEnumProperty(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys,
        const std::string& styleName, std::initializer_list<const char*> allowedValues) const;
    void ValidateDynamicStyleNumberProperty(const JsonValue& styles, const std::set<std::string>& dynamicStyleKeys,
        const std::string& styleName, bool requirePositive = false) const;
    static bool IsDynamicValueDescriptor(const JsonValue& value);

#ifdef TDD_BUILD
    void SetApplyingStyleDeltaUpdateForTest(bool isApplying)
    {
        isApplyingStyleDeltaUpdate_ = isApplying;
    }
#endif

private:
    friend class SurfaceSlot;

    bool ApplyExtendedDescriptor(const JsonValue& descriptor, const RenderContext& context);
    void ApplyResolvedStyles(const JsonValue& styles);
    void ApplySingleResolvedStyle(const std::string& styleName, const JsonValue& styleValue);
    void ParseAndRegisterEventHandlers(const JsonValue& descriptor);
    void RegisterAppearHandler();
    static JsonValue MergeEventContext(const JsonValue& baseContext, const JsonValue& extraContext);

    EventHandlerMap eventHandlers_;
    RenderContext renderContext_;
    std::shared_ptr<ArkUINodeApiAdapter> nodeApplier_;
    std::set<std::string> appliedStyleKeys_;
    bool isApplyingStyleDeltaUpdate_ = false;

    // Cached shadow JSON for theme re-application
    JsonValue cachedShadowValue_;
    bool hasCachedShadow_ = false;
    // Theme cache
    std::weak_ptr<ExtendedCommonTheme> cachedCommonTheme_;
};

} // namespace NativeModule

#endif // A2UI_EXTENDED_COMPONENT_H
