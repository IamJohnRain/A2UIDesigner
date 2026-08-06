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

#ifndef A2UI_ARKUI_NODE_API_ADAPTER_H
#define A2UI_ARKUI_NODE_API_ADAPTER_H

#include <cstdint>
#include <functional>
#include <string>

#include "A2UIArkUITypes.h"
#include "ArkUIOHApiAdapter.h"

namespace NativeModule {

constexpr int32_t MIN_API_VERSION_LAYOUT_POLICY = 21;
constexpr int32_t MIN_API_VERSION_PIXEL_ROUND = 21;
constexpr int32_t MIN_API_VERSION_SIZE_CHANGE = 21;
constexpr int32_t MIN_API_VERSION_CUSTOM_MEASURE = 12;

class ArkUINodeApiAdapter {
public:
    using RootNodeGetter = std::function<ArkUI_NodeHandle()>;
    using ComponentIdGetter = std::function<std::string()>;
    using EdgeSetter = std::function<void(float, float, float, float)>;
    using ActionRegistrar = std::function<void(const std::function<void()>&)>;
    using ResetAction = std::function<void()>;

    ArkUINodeApiAdapter() = default;
    ArkUINodeApiAdapter(RootNodeGetter rootNodeGetter, ComponentIdGetter componentIdGetter, EdgeSetter marginSetter,
        ResetAction resetCommonMargin, ActionRegistrar onClickRegistrar);
    ~ArkUINodeApiAdapter();

    static void* GetNativeNodeAPI();
    static void* GetNativeDialogAPI();
    static bool IsAvailable();
    static ArkUI_NodeHandle CreateNode(A2UINodeType type);
    static void DisposeNode(ArkUI_NodeHandle node);
    static int32_t AddChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child);
    static int32_t RemoveChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child);
    static int32_t InsertChildAt(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index);
    static int32_t SetUserData(ArkUI_NodeHandle node, void* userData);
    static void* GetUserData(ArkUI_NodeHandle node);
    static int32_t AddNodeEventReceiver(ArkUI_NodeHandle node, void (*callback)(A2UINodeEvent* event));
    static int32_t RemoveNodeEventReceiver(ArkUI_NodeHandle node, void (*callback)(A2UINodeEvent* event));
    static int32_t RegisterNodeEvent(
        ArkUI_NodeHandle node, A2UINodeEventType eventType, int32_t eventId, void* userData);
    static int32_t UnregisterNodeEvent(ArkUI_NodeHandle node, A2UINodeEventType eventType);
    static A2UINativeDialogHandle DialogCreate();
    static void DialogDispose(A2UINativeDialogHandle handle);
    static int32_t DialogSetContent(A2UINativeDialogHandle handle, ArkUI_NodeHandle content);
    static int32_t DialogSetContentAlignment(
        A2UINativeDialogHandle handle, A2UIAlignment alignment, float offsetX, float offsetY);
    static int32_t DialogSetModalMode(A2UINativeDialogHandle handle, bool modal);
    static int32_t DialogSetAutoCancel(A2UINativeDialogHandle handle, bool autoCancel);
    static int32_t DialogRegisterOnWillDismissWithUserData(
        A2UINativeDialogHandle handle, void* userData, void (*callback)(A2UIDialogDismissEvent* event));
    static int32_t DialogShow(A2UINativeDialogHandle handle, bool showInSubWindow);
    static int32_t DialogClose(A2UINativeDialogHandle handle);
    static int32_t DialogEnableCustomStyle(A2UINativeDialogHandle handle, bool enable);
    static int32_t SetNodeAccessibilityDescription(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeAccessibilityGroup(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeAccessibilityText(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeAspectRatio(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeBackgroundColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeBackgroundImage(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeBackgroundImageSize(ArkUI_NodeHandle nodeHandle, float width, float height);
    static int32_t SetNodeBackgroundImageSizeWithStyle(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeBorderColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeBorderRadius(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeBorderRadius(
        ArkUI_NodeHandle nodeHandle, float topLeft, float topRight, float bottomLeft, float bottomRight);
    static int32_t SetNodeBorderRadiusPercent(
        ArkUI_NodeHandle nodeHandle, float topLeft, float topRight, float bottomLeft, float bottomRight);
    static int32_t SetNodeBorderWidth(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeBorderWidth(ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left);
    static int32_t SetNodeBorderWidthPercent(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeButtonLabel(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeButtonMaxFontScale(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeButtonMinFontScale(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeButtonType(ArkUI_NodeHandle nodeHandle, A2UIButtonType value);
    static int32_t SetNodeCheckboxGroup(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeCheckboxGroupMark(ArkUI_NodeHandle nodeHandle, uint32_t color);
    static int32_t SetNodeCheckboxGroupMark(ArkUI_NodeHandle nodeHandle, uint32_t color, float size, float strokeWidth);
    static int32_t SetNodeCheckboxGroupName(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeCheckboxGroupSelectAll(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeCheckboxGroupSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeCheckboxGroupShape(ArkUI_NodeHandle nodeHandle, A2UICheckboxShape value);
    static int32_t SetNodeCheckboxGroupUnselectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeCheckboxMark(ArkUI_NodeHandle nodeHandle, uint32_t color);
    static int32_t SetNodeCheckboxMark(ArkUI_NodeHandle nodeHandle, uint32_t color, float size, float strokeWidth);
    static int32_t SetNodeCheckboxName(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeCheckboxSelect(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeCheckboxSelectColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeCheckboxShape(ArkUI_NodeHandle nodeHandle, A2UICheckboxShape value);
    static int32_t SetNodeCheckboxUnselectColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeClip(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeColumnAlignItems(ArkUI_NodeHandle nodeHandle, A2UIHorizontalAlignment value);
    static int32_t SetNodeColumnJustifyContent(ArkUI_NodeHandle nodeHandle, A2UIFlexAlignment value);
    static int32_t SetNodeConstraintSize(
        ArkUI_NodeHandle nodeHandle, float minWidth, float maxWidth, float minHeight, float maxHeight);
    static int32_t SetNodeCustomShadow(ArkUI_NodeHandle nodeHandle, float radius, bool useColorStrategy, float offsetX,
        float offsetY, int32_t type, uint32_t color, bool fill);
    static int32_t SetNodeEnabled(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeFlexOption(ArkUI_NodeHandle nodeHandle, A2UIFlexDirection direction, A2UIFlexWrap wrap,
        A2UIFlexAlignment justifyContent, A2UIItemAlignment alignItems, A2UIFlexAlignment alignContent);
    static int32_t SetNodeFlexShrink(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeFlexSpace(ArkUI_NodeHandle nodeHandle, float main, float cross);
    static int32_t SetNodeFontColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeFontSize(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeFontWeight(ArkUI_NodeHandle nodeHandle, A2UIFontWeight value);
    static int32_t SetNodeGridAlignItems(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeGridColumnGap(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeGridColumnTemplate(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeGridRowGap(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeGridRowTemplate(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeHeight(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeHeightPercent(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeId(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeImageAlt(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeImageFillColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeImageObjectFit(ArkUI_NodeHandle nodeHandle, A2UIObjectFit value);
    static int32_t SetNodeImageSrc(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static A2UIObjectFit ParseImageObjectFit(
        const std::string& fit, A2UIObjectFit fallbackObjectFit, int32_t apiVersion = 0);
    static int32_t SetNodeLayoutWeight(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeLinearGradient(ArkUI_NodeHandle nodeHandle, float angle, int32_t direction, bool repeating,
        const uint32_t* colors, int32_t colorCount, const float* stops, int32_t stopCount);
    static int32_t SetNodeListAlignListItem(ArkUI_NodeHandle nodeHandle, A2UIListItemAlignment value);
    static int32_t SetNodeListDirection(ArkUI_NodeHandle nodeHandle, A2UIAxis value);
    static int32_t SetNodeListLanes(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeListNodeAdapter(ArkUI_NodeHandle nodeHandle, A2UINodeAdapterHandle adapterHandle);
    static int32_t SetNodeListSpace(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeMargin(ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left);
    static int32_t SetNodeMarginPercent(ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left);
    static int32_t SetNodeOpacity(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodePadding(ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left);
    static int32_t SetNodePaddingPercent(ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left);
    static int32_t SetNodePixelRoundNoForceRound(
        ArkUI_NodeHandle nodeHandle, int32_t apiVersion = MIN_API_VERSION_PIXEL_ROUND);
    static int32_t SetNodeProgressColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeProgressTotal(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeProgressType(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeProgressValue(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeRadioChecked(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeRadioGroup(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeRadioStyle(ArkUI_NodeHandle nodeHandle, uint32_t checkedBackgroundColor,
        uint32_t uncheckedBackgroundColor, uint32_t indicatorColor);
    static int32_t SetNodeRadioValue(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeRowAlignItems(ArkUI_NodeHandle nodeHandle, A2UIVerticalAlignment value);
    static int32_t SetNodeRowJustifyContent(ArkUI_NodeHandle nodeHandle, A2UIFlexAlignment value);
    static int32_t SetNodeScrollBarDisplayMode(ArkUI_NodeHandle nodeHandle, A2UIScrollBarDisplayMode value);
    static int32_t SetNodeScrollNestedScroll(
        ArkUI_NodeHandle nodeHandle, A2UIScrollNestedMode scrollForward, A2UIScrollNestedMode scrollBackward);
    static int32_t SetNodeShadow(
        ArkUI_NodeHandle nodeHandle, float radius, uint32_t color, float offsetX, float offsetY);
    static int32_t SetNodeShadow(ArkUI_NodeHandle nodeHandle, int32_t shadowStyle);
    static int32_t SetNodeSliderMaxValue(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeSliderMinValue(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeSliderSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeSliderStep(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeSliderStyle(ArkUI_NodeHandle nodeHandle, A2UISliderStyle value);
    static int32_t SetNodeSliderValue(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeStackAlignContent(ArkUI_NodeHandle nodeHandle, A2UIAlignment value);
    static int32_t SetNodeTextAlign(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextAreaText(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeTextContent(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeTextDecoration(ArkUI_NodeHandle nodeHandle, int32_t type, bool hasColor, uint32_t color,
        bool hasStyle, int32_t style, bool hasThicknessScale, float thicknessScale);
    static int32_t SetNodeTextInputCancelButton(ArkUI_NodeHandle nodeHandle, A2UICancelButtonStyle style,
        bool hasIconSize, float iconSize, bool hasIconColor, uint32_t iconColor, bool hasIconSrc,
        const std::string& iconSrc);
    static int32_t SetNodeTextInputCaretColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeTextInputMaxLength(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextInputNumberOfLines(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextInputPlaceholder(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeTextInputPlaceholderColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeTextInputSelectedBackgroundColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeTextInputShowUnderline(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeTextInputStyle(ArkUI_NodeHandle nodeHandle, bool inlineStyle);
    static int32_t SetNodeTextInputText(ArkUI_NodeHandle nodeHandle, const std::string& value);
    static int32_t SetNodeTextInputType(ArkUI_NodeHandle nodeHandle, A2UITextInputType value);
    static int32_t SetNodeTextInputUnderlineColor(
        ArkUI_NodeHandle nodeHandle, uint32_t typing, uint32_t normal, uint32_t error, uint32_t disabled);
    static int32_t SetNodeTextInputWordBreak(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextMaxFontSize(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeTextMaxLines(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextMinFontSize(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeTextOverflow(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeTextWordBreak(ArkUI_NodeHandle nodeHandle, int32_t value);
    static int32_t SetNodeToggleSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeToggleSwitchPointColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeToggleUnselectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value);
    static int32_t SetNodeToggleValue(ArkUI_NodeHandle nodeHandle, bool value);
    static int32_t SetNodeVisibility(ArkUI_NodeHandle nodeHandle, A2UIVisibility value);
    static int32_t SetNodeWidth(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeWidthPercent(ArkUI_NodeHandle nodeHandle, float value);
    static int32_t SetNodeWidthLayoutPolicy(
        ArkUI_NodeHandle nodeHandle, int32_t value, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY);
    static int32_t SetNodeHeightLayoutPolicy(
        ArkUI_NodeHandle nodeHandle, int32_t value, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY);
    static int32_t ResetNodeAccessibilityDescription(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeAccessibilityGroup(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeAccessibilityText(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeAspectRatio(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBackgroundColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBackgroundImage(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBackgroundImageSize(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBackgroundImageSizeWithStyle(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBorderColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBorderRadius(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBorderRadiusPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBorderWidth(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeBorderWidthPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeButtonLabel(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeButtonMaxFontScale(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeButtonMinFontScale(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeButtonType(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroup(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupMark(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupName(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupSelectAll(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupSelectedColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupShape(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxGroupUnselectedColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxMark(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxName(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxSelect(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxSelectColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxShape(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCheckboxUnselectColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeClip(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeColumnAlignItems(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeColumnJustifyContent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeConstraintSize(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeCustomShadow(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeEnabled(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFlexOption(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFlexShrink(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFlexSpace(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFontColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFontSize(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeFontWeight(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeGridAlignItems(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeGridColumnGap(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeGridColumnTemplate(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeGridRowGap(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeGridRowTemplate(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeHeight(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeHeightPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeId(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeImageAlt(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeImageFillColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeImageObjectFit(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeImageSrc(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeLayoutWeight(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeLinearGradient(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeListAlignListItem(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeListDirection(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeListLanes(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeListNodeAdapter(ArkUI_NodeHandle nodeHandle);
    static int32_t SetNodeGridNodeAdapter(ArkUI_NodeHandle nodeHandle, A2UINodeAdapterHandle adapterHandle);
    static int32_t ResetNodeAttribute(ArkUI_NodeHandle nodeHandle, A2UINodeAttributeType attributeType);
    static int32_t ResetNodeListSpace(ArkUI_NodeHandle nodeHandle);
    int32_t ResetNodeMargin(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeMarginPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeOpacity(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodePadding(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodePaddingPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeProgressColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeProgressTotal(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeProgressType(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeProgressValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRadioChecked(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRadioGroup(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRadioStyle(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRadioValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRowAlignItems(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeRowJustifyContent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeScrollBarDisplayMode(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeScrollNestedScroll(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeShadow(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderMaxValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderMinValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderSelectedColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderStep(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderStyle(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeSliderValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeStackAlignContent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextAlign(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextAreaText(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextContent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextDecoration(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputCancelButton(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputCaretColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputMaxLength(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputNumberOfLines(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputPlaceholder(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputPlaceholderColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputSelectedBackgroundColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputShowUnderline(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputText(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputType(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputUnderlineColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextInputWordBreak(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextMaxFontSize(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextMaxLines(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextMinFontSize(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextOverflow(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeTextWordBreak(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeToggleSelectedColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeToggleSwitchPointColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeToggleUnselectedColor(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeToggleValue(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeVisibility(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeWidth(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeWidthPercent(ArkUI_NodeHandle nodeHandle);
    static int32_t ResetNodeWidthLayoutPolicy(
        ArkUI_NodeHandle nodeHandle, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY);
    static int32_t ResetNodeHeightLayoutPolicy(
        ArkUI_NodeHandle nodeHandle, int32_t apiVersion = MIN_API_VERSION_LAYOUT_POLICY);

    ArkUI_NodeHandle GetRootNode() const;
    void SetWidth(float width);
    void SetHeight(float height);
    void SetWidthPercent(float percent);
    void SetHeightPercent(float percent);
    void SetBackgroundColor(uint32_t color);
    void SetBorderRadius(float radius);
    void SetBorderRadiusPercent(float topLeft, float topRight, float bottomLeft, float bottomRight);
    void SetPadding(float top, float right, float bottom, float left);
    void SetPaddingPercent(float top, float right, float bottom, float left);
    void SetMargin(float top, float right, float bottom, float left);
    void SetMarginPercent(float top, float right, float bottom, float left);
    void SetBorderWidthPercent(float width);
    void RegisterOnClick(const std::function<void()>& onClick);

private:
    std::string GetComponentId() const;

    RootNodeGetter rootNodeGetter_;
    ComponentIdGetter componentIdGetter_;
    EdgeSetter marginSetter_;
    ResetAction resetCommonMargin_;
    ActionRegistrar onClickRegistrar_;
};

} // namespace NativeModule

#endif // A2UI_ARKUI_NODE_API_ADAPTER_H
