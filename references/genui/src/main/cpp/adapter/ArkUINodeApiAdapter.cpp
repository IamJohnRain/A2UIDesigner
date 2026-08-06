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

#include "ArkUINodeApiAdapter.h"

#include <algorithm>
#include <cctype>
#include <utility>

#include "A2UIArkUITypeConverter.h"
#include "ArkUIConstraintSizeAdapter.h"
#include "ArkUINativeAPI.h"

namespace NativeModule {
namespace {

constexpr int32_t MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX = 21;
constexpr int32_t OBJECT_FIT_NONE_MATRIX_VALUE = 15;

ArkUI_NativeNodeAPI_1* GetNativeNodeAPIInternal()
{
    return reinterpret_cast<ArkUI_NativeNodeAPI_1*>(ArkUINodeApiAdapter::GetNativeNodeAPI());
}

ArkUI_NativeDialogAPI_1* GetNativeDialogAPIInternal()
{
    return reinterpret_cast<ArkUI_NativeDialogAPI_1*>(ArkUINodeApiAdapter::GetNativeDialogAPI());
}

ArkUI_NodeAdapterHandle ToArkUINodeAdapterHandle(A2UINodeAdapterHandle handle)
{
    return reinterpret_cast<ArkUI_NodeAdapterHandle>(handle);
}

ArkUI_NativeDialogHandle ToArkUINativeDialogHandle(A2UINativeDialogHandle handle)
{
    return reinterpret_cast<ArkUI_NativeDialogHandle>(handle);
}

A2UINativeDialogHandle FromArkUINativeDialogHandle(ArkUI_NativeDialogHandle handle)
{
    return reinterpret_cast<A2UINativeDialogHandle>(handle);
}

std::string NormalizeImageObjectFitToken(const std::string& value)
{
    std::string normalized = value;
    normalized.erase(normalized.begin(),
        std::find_if_not(normalized.begin(), normalized.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }));
    normalized.erase(
        std::find_if_not(normalized.rbegin(), normalized.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; })
            .base(),
        normalized.end());
    for (char& ch : normalized) {
        if (ch == '-' || ch == '_') {
            ch = ' ';
            continue;
        }
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    normalized.erase(std::remove(normalized.begin(), normalized.end(), ' '), normalized.end());
    return normalized;
}

int32_t SetAttributeInternal(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->setAttribute == nullptr || node == nullptr) {
        return -1;
    }
    return api->setAttribute(node, attribute, item);
}

int32_t ResetAttributeInternal(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->resetAttribute == nullptr || node == nullptr) {
        return -1;
    }
    return api->resetAttribute(node, attribute);
}

int32_t SetParentLayoutAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute, ArkUI_AttributeItem* item)
{
    ArkUI_NodeHandle content = ArkUIConstraintSizeAdapter::GetContentNode(node);
    ArkUI_NodeHandle mount = ArkUIConstraintSizeAdapter::GetMountNode(content);
    int32_t contentResult = SetAttributeInternal(content, attribute, item);
    if (mount == content) {
        return contentResult;
    }
    int32_t mountResult = SetAttributeInternal(mount, attribute, item);
    return contentResult != A2UI_ERROR_CODE_NO_ERROR ? contentResult : mountResult;
}

int32_t ResetParentLayoutAttribute(ArkUI_NodeHandle node, ArkUI_NodeAttributeType attribute)
{
    ArkUI_NodeHandle content = ArkUIConstraintSizeAdapter::GetContentNode(node);
    ArkUI_NodeHandle mount = ArkUIConstraintSizeAdapter::GetMountNode(content);
    int32_t contentResult = ResetAttributeInternal(content, attribute);
    if (mount == content) {
        return contentResult;
    }
    int32_t mountResult = ResetAttributeInternal(mount, attribute);
    return contentResult != A2UI_ERROR_CODE_NO_ERROR ? contentResult : mountResult;
}

int32_t SetFloatAttribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, float value)
{
    ArkUI_NumberValue values[] = { { .f32 = value } };
    ArkUI_AttributeItem item = { values, 1 };
    return SetAttributeInternal(nodeHandle, attribute, &item);
}

int32_t SetUint32Attribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, uint32_t value)
{
    ArkUI_NumberValue values[] = { { .u32 = value } };
    ArkUI_AttributeItem item = { values, 1 };
    return SetAttributeInternal(nodeHandle, attribute, &item);
}

int32_t SetInt32Attribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, int32_t value)
{
    ArkUI_NumberValue values[] = { { .i32 = value } };
    ArkUI_AttributeItem item = { values, 1 };
    return SetAttributeInternal(nodeHandle, attribute, &item);
}

int32_t SetBoolAttribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, bool value)
{
    return SetInt32Attribute(nodeHandle, attribute, value ? 1 : 0);
}

int32_t SetStringAttribute(ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const std::string& value)
{
    ArkUI_AttributeItem item = { nullptr, 0, value.c_str() };
    return SetAttributeInternal(nodeHandle, attribute, &item);
}

int32_t SetNumberArrayAttribute(
    ArkUI_NodeHandle nodeHandle, ArkUI_NodeAttributeType attribute, const ArkUI_NumberValue* values, int32_t valueCount)
{
    if (values == nullptr || valueCount <= 0) {
        return -1;
    }
    ArkUI_AttributeItem item = { const_cast<ArkUI_NumberValue*>(values), valueCount };
    return SetAttributeInternal(nodeHandle, attribute, &item);
}

int32_t SetListNodeAdapterInternal(ArkUI_NodeHandle nodeHandle, A2UINodeAdapterHandle adapterHandle)
{
    if (adapterHandle == nullptr) {
        return -1;
    }
    ArkUI_AttributeItem item = { nullptr, 0, nullptr, ToArkUINodeAdapterHandle(adapterHandle) };
    return SetAttributeInternal(nodeHandle, NODE_LIST_NODE_ADAPTER, &item);
}

int32_t SetTextInputCancelButtonAttribute(
    ArkUI_NodeHandle nodeHandle, const ArkUI_NumberValue* values, int32_t valueCount, const char* iconSrc)
{
    if (values == nullptr || valueCount <= 0) {
        return -1;
    }
    ArkUI_AttributeItem item = { const_cast<ArkUI_NumberValue*>(values), valueCount, iconSrc };
    return SetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_CANCEL_BUTTON, &item);
}

int32_t SetLinearGradientAttribute(ArkUI_NodeHandle nodeHandle, float angle, int32_t direction, bool repeating,
    const uint32_t* colors, int32_t colorCount, const float* stops, int32_t stopCount)
{
    if (colors == nullptr || stops == nullptr || colorCount <= 0 || colorCount != stopCount) {
        return -1;
    }
    ArkUI_NumberValue values[] = { { .f32 = angle }, { .i32 = direction }, { .i32 = repeating ? 1 : 0 } };
    ArkUI_ColorStop colorStop = { colors, const_cast<float*>(stops), colorCount };
    ArkUI_AttributeItem item = { values, 3, nullptr, &colorStop };
    return SetAttributeInternal(nodeHandle, NODE_LINEAR_GRADIENT, &item);
}

} // namespace

void* ArkUINodeApiAdapter::GetNativeNodeAPI()
{
#ifdef TDD_BUILD
    return ArkUINativeAPI::GetInstance().GetNativeNodeAPI();
#else
    static ArkUI_NativeNodeAPI_1* api = nullptr;
    static bool probed = false;
    if (!probed) {
        void* result = nullptr;
        if (ArkUIOHApiAdapter::GetModuleInterfaceByType(ARKUI_NATIVE_NODE, 1, &result) == 0) {
            api = reinterpret_cast<ArkUI_NativeNodeAPI_1*>(result);
        }
        probed = true;
    }
    return api;
#endif
}

void* ArkUINodeApiAdapter::GetNativeDialogAPI()
{
#ifdef TDD_BUILD
    return ArkUINativeAPI::GetInstance().GetNativeDialogAPI();
#else
    static ArkUI_NativeDialogAPI_1* api = nullptr;
    static bool probed = false;
    if (!probed) {
        void* result = nullptr;
        if (ArkUIOHApiAdapter::GetModuleInterfaceByType(ARKUI_NATIVE_DIALOG, 1, &result) == 0) {
            api = reinterpret_cast<ArkUI_NativeDialogAPI_1*>(result);
        }
        probed = true;
    }
    return api;
#endif
}

bool ArkUINodeApiAdapter::IsAvailable()
{
    return ArkUINativeAPI::GetInstance().IsNativeAPIAvailable();
}

ArkUI_NodeHandle ArkUINodeApiAdapter::CreateNode(A2UINodeType type)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->createNode == nullptr) {
        return nullptr;
    }
    return api->createNode(A2UIArkUITypeConverter::ToArkUINodeType(type));
}

void ArkUINodeApiAdapter::DisposeNode(ArkUI_NodeHandle node)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api != nullptr && api->disposeNode != nullptr && node != nullptr) {
        api->disposeNode(node);
    }
}

int32_t ArkUINodeApiAdapter::AddChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->addChild == nullptr || parent == nullptr || child == nullptr) {
        return -1;
    }
    return api->addChild(parent, child);
}

int32_t ArkUINodeApiAdapter::RemoveChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->removeChild == nullptr || parent == nullptr || child == nullptr) {
        return -1;
    }
    return api->removeChild(parent, child);
}

int32_t ArkUINodeApiAdapter::InsertChildAt(ArkUI_NodeHandle parent, ArkUI_NodeHandle child, int32_t index)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->insertChildAt == nullptr || parent == nullptr || child == nullptr) {
        return -1;
    }
    return api->insertChildAt(parent, child, index);
}

int32_t ArkUINodeApiAdapter::SetUserData(ArkUI_NodeHandle node, void* userData)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->setUserData == nullptr || node == nullptr) {
        return -1;
    }
    return api->setUserData(node, userData);
}

void* ArkUINodeApiAdapter::GetUserData(ArkUI_NodeHandle node)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->getUserData == nullptr || node == nullptr) {
        return nullptr;
    }
    return api->getUserData(node);
}

int32_t ArkUINodeApiAdapter::AddNodeEventReceiver(ArkUI_NodeHandle node, void (*callback)(A2UINodeEvent* event))
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->addNodeEventReceiver == nullptr || node == nullptr || callback == nullptr) {
        return -1;
    }
    return api->addNodeEventReceiver(node, callback);
}

int32_t ArkUINodeApiAdapter::RemoveNodeEventReceiver(ArkUI_NodeHandle node, void (*callback)(A2UINodeEvent* event))
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->removeNodeEventReceiver == nullptr || node == nullptr || callback == nullptr) {
        return -1;
    }
    return api->removeNodeEventReceiver(node, callback);
}

int32_t ArkUINodeApiAdapter::RegisterNodeEvent(
    ArkUI_NodeHandle node, A2UINodeEventType eventType, int32_t eventId, void* userData)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->registerNodeEvent == nullptr || node == nullptr) {
        return -1;
    }
    return api->registerNodeEvent(node, A2UIArkUITypeConverter::ToArkUINodeEventType(eventType), eventId, userData);
}

int32_t ArkUINodeApiAdapter::UnregisterNodeEvent(ArkUI_NodeHandle node, A2UINodeEventType eventType)
{
    ArkUI_NativeNodeAPI_1* api = GetNativeNodeAPIInternal();
    if (api == nullptr || api->unregisterNodeEvent == nullptr || node == nullptr) {
        return -1;
    }
#ifdef TDD_BUILD
    return api->unregisterNodeEvent(node, A2UIArkUITypeConverter::ToArkUINodeEventType(eventType));
#else
    api->unregisterNodeEvent(node, A2UIArkUITypeConverter::ToArkUINodeEventType(eventType));
    return 0;
#endif
}

A2UINativeDialogHandle ArkUINodeApiAdapter::DialogCreate()
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->create == nullptr) {
        return nullptr;
    }
    return FromArkUINativeDialogHandle(api->create());
}

void ArkUINodeApiAdapter::DialogDispose(A2UINativeDialogHandle handle)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api != nullptr && api->dispose != nullptr && handle != nullptr) {
        api->dispose(ToArkUINativeDialogHandle(handle));
    }
}

int32_t ArkUINodeApiAdapter::DialogSetContent(A2UINativeDialogHandle handle, ArkUI_NodeHandle content)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->setContent == nullptr || handle == nullptr || content == nullptr) {
        return -1;
    }
    return api->setContent(ToArkUINativeDialogHandle(handle), content);
}

int32_t ArkUINodeApiAdapter::DialogSetContentAlignment(
    A2UINativeDialogHandle handle, A2UIAlignment alignment, float offsetX, float offsetY)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->setContentAlignment == nullptr || handle == nullptr) {
        return -1;
    }
    return api->setContentAlignment(
        ToArkUINativeDialogHandle(handle), A2UIArkUITypeConverter::ToArkUIDialogAlignment(alignment), offsetX, offsetY);
}

int32_t ArkUINodeApiAdapter::DialogSetModalMode(A2UINativeDialogHandle handle, bool modal)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->setModalMode == nullptr || handle == nullptr) {
        return -1;
    }
    return api->setModalMode(ToArkUINativeDialogHandle(handle), modal);
}

int32_t ArkUINodeApiAdapter::DialogSetAutoCancel(A2UINativeDialogHandle handle, bool autoCancel)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->setAutoCancel == nullptr || handle == nullptr) {
        return -1;
    }
    return api->setAutoCancel(ToArkUINativeDialogHandle(handle), autoCancel);
}

int32_t ArkUINodeApiAdapter::DialogRegisterOnWillDismissWithUserData(
    A2UINativeDialogHandle handle, void* userData, void (*callback)(A2UIDialogDismissEvent* event))
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->registerOnWillDismissWithUserData == nullptr || handle == nullptr ||
        callback == nullptr) {
        return -1;
    }
    return api->registerOnWillDismissWithUserData(
        ToArkUINativeDialogHandle(handle), userData, reinterpret_cast<void (*)(ArkUI_DialogDismissEvent*)>(callback));
}

int32_t ArkUINodeApiAdapter::DialogShow(A2UINativeDialogHandle handle, bool showInSubWindow)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->show == nullptr || handle == nullptr) {
        return -1;
    }
    return api->show(ToArkUINativeDialogHandle(handle), showInSubWindow);
}

int32_t ArkUINodeApiAdapter::DialogClose(A2UINativeDialogHandle handle)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->close == nullptr || handle == nullptr) {
        return -1;
    }
    return api->close(ToArkUINativeDialogHandle(handle));
}

int32_t ArkUINodeApiAdapter::DialogEnableCustomStyle(A2UINativeDialogHandle handle, bool enable)
{
    ArkUI_NativeDialogAPI_1* api = GetNativeDialogAPIInternal();
    if (api == nullptr || api->enableCustomStyle == nullptr || handle == nullptr) {
        return -1;
    }
    return api->enableCustomStyle(ToArkUINativeDialogHandle(handle), enable);
}

ArkUINodeApiAdapter::~ArkUINodeApiAdapter()
{
    ArkUIConstraintSizeAdapter::Dispose(GetRootNode());
}

ArkUINodeApiAdapter::ArkUINodeApiAdapter(RootNodeGetter rootNodeGetter, ComponentIdGetter componentIdGetter,
    EdgeSetter marginSetter, ResetAction resetCommonMargin, ActionRegistrar onClickRegistrar)
    : rootNodeGetter_(std::move(rootNodeGetter)), componentIdGetter_(std::move(componentIdGetter)),
      marginSetter_(std::move(marginSetter)), resetCommonMargin_(std::move(resetCommonMargin)),
      onClickRegistrar_(std::move(onClickRegistrar))
{}

ArkUI_NodeHandle ArkUINodeApiAdapter::GetRootNode() const
{
    return rootNodeGetter_ != nullptr ? rootNodeGetter_() : nullptr;
}

void ArkUINodeApiAdapter::SetWidth(float width)
{
    SetNodeWidth(GetRootNode(), width);
}

void ArkUINodeApiAdapter::SetHeight(float height)
{
    SetNodeHeight(GetRootNode(), height);
}

void ArkUINodeApiAdapter::SetWidthPercent(float percent)
{
    SetNodeWidthPercent(GetRootNode(), percent);
}

void ArkUINodeApiAdapter::SetHeightPercent(float percent)
{
    SetNodeHeightPercent(GetRootNode(), percent);
}

void ArkUINodeApiAdapter::SetBackgroundColor(uint32_t color)
{
    SetNodeBackgroundColor(GetRootNode(), color);
}

void ArkUINodeApiAdapter::SetBorderRadius(float radius)
{
    SetNodeBorderRadius(GetRootNode(), radius);
}

void ArkUINodeApiAdapter::SetBorderRadiusPercent(float topLeft, float topRight, float bottomLeft, float bottomRight)
{
    SetNodeBorderRadiusPercent(GetRootNode(), topLeft, topRight, bottomLeft, bottomRight);
}

void ArkUINodeApiAdapter::SetPadding(float top, float right, float bottom, float left)
{
    SetNodePadding(GetRootNode(), top, right, bottom, left);
}

void ArkUINodeApiAdapter::SetPaddingPercent(float top, float right, float bottom, float left)
{
    SetNodePaddingPercent(GetRootNode(), top, right, bottom, left);
}

void ArkUINodeApiAdapter::SetMargin(float top, float right, float bottom, float left)
{
    if (marginSetter_ != nullptr) {
        marginSetter_(top, right, bottom, left);
        return;
    }
    SetNodeMargin(GetRootNode(), top, right, bottom, left);
}

void ArkUINodeApiAdapter::SetMarginPercent(float top, float right, float bottom, float left)
{
    SetNodeMarginPercent(GetRootNode(), top, right, bottom, left);
}

void ArkUINodeApiAdapter::SetBorderWidthPercent(float width)
{
    SetNodeBorderWidthPercent(GetRootNode(), width);
}

int32_t ArkUINodeApiAdapter::SetNodeAccessibilityDescription(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_ACCESSIBILITY_DESCRIPTION, value);
}

int32_t ArkUINodeApiAdapter::SetNodeAccessibilityGroup(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_ACCESSIBILITY_GROUP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeAccessibilityText(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_ACCESSIBILITY_TEXT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeAspectRatio(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_ASPECT_RATIO, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBackgroundColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_BACKGROUND_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBackgroundImage(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_BACKGROUND_IMAGE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBackgroundImageSizeWithStyle(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_BORDER_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderRadius(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_BORDER_RADIUS, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderWidth(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_BORDER_WIDTH, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderWidthPercent(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_BORDER_WIDTH_PERCENT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeButtonLabel(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_BUTTON_LABEL, value);
}

int32_t ArkUINodeApiAdapter::SetNodeButtonMaxFontScale(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_BUTTON_MAX_FONT_SCALE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeButtonMinFontScale(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_BUTTON_MIN_FONT_SCALE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeButtonType(ArkUI_NodeHandle nodeHandle, A2UIButtonType value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_BUTTON_TYPE, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIButtonType(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroup(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_CHECKBOX_GROUP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupName(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_CHECKBOX_GROUP_NAME, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupSelectAll(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_CHECKBOX_GROUP_SELECT_ALL, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_GROUP_SELECTED_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupShape(ArkUI_NodeHandle nodeHandle, A2UICheckboxShape value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_CHECKBOX_GROUP_SHAPE, A2UIArkUITypeConverter::ToArkUICheckboxShape(value));
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupUnselectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_GROUP_UNSELECTED_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxName(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_CHECKBOX_NAME, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxSelect(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_CHECKBOX_SELECT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxSelectColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_SELECT_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxShape(ArkUI_NodeHandle nodeHandle, A2UICheckboxShape value)
{
    return SetInt32Attribute(nodeHandle, NODE_CHECKBOX_SHAPE, A2UIArkUITypeConverter::ToArkUICheckboxShape(value));
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxUnselectColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_UNSELECT_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeClip(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_CLIP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeColumnAlignItems(ArkUI_NodeHandle nodeHandle, A2UIHorizontalAlignment value)
{
    return SetInt32Attribute(nodeHandle, NODE_COLUMN_ALIGN_ITEMS,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIHorizontalAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeColumnJustifyContent(ArkUI_NodeHandle nodeHandle, A2UIFlexAlignment value)
{
    return SetInt32Attribute(nodeHandle, NODE_COLUMN_JUSTIFY_CONTENT,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeEnabled(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_ENABLED, value);
}

int32_t ArkUINodeApiAdapter::SetNodeFlexShrink(ArkUI_NodeHandle nodeHandle, float value)
{
    ArkUI_NumberValue values[] = { { .f32 = value } };
    ArkUI_AttributeItem item = { values, 1 };
    return SetParentLayoutAttribute(nodeHandle, NODE_FLEX_SHRINK, &item);
}

int32_t ArkUINodeApiAdapter::SetNodeFontColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_FONT_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeFontSize(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_FONT_SIZE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeFontWeight(ArkUI_NodeHandle nodeHandle, A2UIFontWeight value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_FONT_WEIGHT, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFontWeight(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeGridAlignItems(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_GRID_ALIGN_ITEMS, value);
}

int32_t ArkUINodeApiAdapter::SetNodeGridColumnGap(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_GRID_COLUMN_GAP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeGridColumnTemplate(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_GRID_COLUMN_TEMPLATE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeGridRowGap(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_GRID_ROW_GAP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeGridRowTemplate(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_GRID_ROW_TEMPLATE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeHeight(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_HEIGHT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeHeightPercent(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_HEIGHT_PERCENT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeId(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_ID, value);
}

int32_t ArkUINodeApiAdapter::SetNodeImageAlt(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_IMAGE_ALT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeImageFillColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_IMAGE_FILL_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeImageObjectFit(ArkUI_NodeHandle nodeHandle, A2UIObjectFit value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_IMAGE_OBJECT_FIT, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIObjectFit(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeImageSrc(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_IMAGE_SRC, value);
}

A2UIObjectFit ArkUINodeApiAdapter::ParseImageObjectFit(
    const std::string& fit, A2UIObjectFit fallbackObjectFit, int32_t apiVersion)
{
    std::string normalizedFit = NormalizeImageObjectFitToken(fit);
    if (normalizedFit == "contain") {
        return A2UIObjectFit::CONTAIN;
    } else if (normalizedFit == "cover") {
        return A2UIObjectFit::COVER;
    } else if (normalizedFit == "auto") {
        return A2UIObjectFit::AUTO;
    } else if (normalizedFit == "fill") {
        return A2UIObjectFit::FILL;
    } else if (normalizedFit == "scaledown") {
        return A2UIObjectFit::SCALE_DOWN;
    } else if (normalizedFit == "none") {
        return A2UIObjectFit::NONE;
    } else if (normalizedFit == "topstart") {
        return A2UIObjectFit::NONE_AND_ALIGN_TOP_START;
    } else if (normalizedFit == "top") {
        return A2UIObjectFit::NONE_AND_ALIGN_TOP;
    } else if (normalizedFit == "topend") {
        return A2UIObjectFit::NONE_AND_ALIGN_TOP_END;
    } else if (normalizedFit == "start") {
        return A2UIObjectFit::NONE_AND_ALIGN_START;
    } else if (normalizedFit == "center") {
        return A2UIObjectFit::NONE_AND_ALIGN_CENTER;
    } else if (normalizedFit == "end") {
        return A2UIObjectFit::NONE_AND_ALIGN_END;
    } else if (normalizedFit == "bottomstart") {
        return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_START;
    } else if (normalizedFit == "bottom") {
        return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM;
    } else if (normalizedFit == "bottomend") {
        return A2UIObjectFit::NONE_AND_ALIGN_BOTTOM_END;
    } else if (normalizedFit == "matrix") {
        if (apiVersion < MIN_API_VERSION_IMAGE_OBJECT_FIT_MATRIX) {
            return fallbackObjectFit;
        }
        return A2UIObjectFit::NONE_MATRIX;
    }
    return fallbackObjectFit;
}

int32_t ArkUINodeApiAdapter::SetNodeLayoutWeight(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    ArkUI_NumberValue values[] = { { .u32 = value } };
    ArkUI_AttributeItem item = { values, 1 };
    return SetParentLayoutAttribute(nodeHandle, NODE_LAYOUT_WEIGHT, &item);
}

int32_t ArkUINodeApiAdapter::SetNodeLinearGradient(ArkUI_NodeHandle nodeHandle, float angle, int32_t direction,
    bool repeating, const uint32_t* colors, int32_t colorCount, const float* stops, int32_t stopCount)
{
    return SetLinearGradientAttribute(nodeHandle, angle, direction, repeating, colors, colorCount, stops, stopCount);
}

int32_t ArkUINodeApiAdapter::SetNodeListAlignListItem(ArkUI_NodeHandle nodeHandle, A2UIListItemAlignment value)
{
    return SetInt32Attribute(nodeHandle, NODE_LIST_ALIGN_LIST_ITEM,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIListItemAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeListDirection(ArkUI_NodeHandle nodeHandle, A2UIAxis value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_LIST_DIRECTION, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIAxis(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeListLanes(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_LIST_LANES, value);
}

int32_t ArkUINodeApiAdapter::SetNodeListNodeAdapter(ArkUI_NodeHandle nodeHandle, A2UINodeAdapterHandle adapterHandle)
{
    return SetListNodeAdapterInternal(nodeHandle, adapterHandle);
}

int32_t ArkUINodeApiAdapter::SetNodeGridNodeAdapter(ArkUI_NodeHandle nodeHandle, A2UINodeAdapterHandle adapterHandle)
{
    if (adapterHandle == nullptr) {
        return -1;
    }
    ArkUI_AttributeItem item = { nullptr, 0, nullptr, ToArkUINodeAdapterHandle(adapterHandle) };
    return SetAttributeInternal(nodeHandle, NODE_GRID_NODE_ADAPTER, &item);
}

int32_t ArkUINodeApiAdapter::SetNodeListSpace(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_LIST_SPACE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeOpacity(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_OPACITY, value);
}

int32_t ArkUINodeApiAdapter::SetNodeProgressColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_PROGRESS_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeProgressTotal(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_PROGRESS_TOTAL, value);
}

int32_t ArkUINodeApiAdapter::SetNodeProgressType(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_PROGRESS_TYPE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeProgressValue(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_PROGRESS_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeRadioChecked(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_RADIO_CHECKED, value);
}

int32_t ArkUINodeApiAdapter::SetNodeRadioGroup(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_RADIO_GROUP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeRadioValue(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_RADIO_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeRowAlignItems(ArkUI_NodeHandle nodeHandle, A2UIVerticalAlignment value)
{
    return SetInt32Attribute(nodeHandle, NODE_ROW_ALIGN_ITEMS,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIVerticalAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeRowJustifyContent(ArkUI_NodeHandle nodeHandle, A2UIFlexAlignment value)
{
    return SetInt32Attribute(nodeHandle, NODE_ROW_JUSTIFY_CONTENT,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeScrollBarDisplayMode(ArkUI_NodeHandle nodeHandle, A2UIScrollBarDisplayMode value)
{
    return SetInt32Attribute(nodeHandle, NODE_SCROLL_BAR_DISPLAY_MODE,
        static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIScrollBarDisplayMode(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeSliderMaxValue(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_SLIDER_MAX_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeSliderMinValue(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_SLIDER_MIN_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeSliderSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_SLIDER_SELECTED_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeSliderStep(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_SLIDER_STEP, value);
}

int32_t ArkUINodeApiAdapter::SetNodeSliderStyle(ArkUI_NodeHandle nodeHandle, A2UISliderStyle value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_SLIDER_STYLE, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUISliderStyle(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeSliderValue(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_SLIDER_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeStackAlignContent(ArkUI_NodeHandle nodeHandle, A2UIAlignment value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_STACK_ALIGN_CONTENT, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIAlignment(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeTextAlign(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_ALIGN, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextAreaText(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_TEXT_AREA_TEXT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextContent(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_TEXT_CONTENT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputCaretColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TEXT_INPUT_CARET_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputMaxLength(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_INPUT_MAX_LENGTH, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputNumberOfLines(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_INPUT_NUMBER_OF_LINES, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputPlaceholder(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputPlaceholderColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputSelectedBackgroundColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputShowUnderline(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_TEXT_INPUT_SHOW_UNDERLINE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputStyle(ArkUI_NodeHandle nodeHandle, bool inlineStyle)
{
    int32_t style = inlineStyle ? ARKUI_TEXTINPUT_STYLE_INLINE : ARKUI_TEXTINPUT_STYLE_DEFAULT;
    return SetInt32Attribute(nodeHandle, NODE_TEXT_INPUT_STYLE, style);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputText(ArkUI_NodeHandle nodeHandle, const std::string& value)
{
    return SetStringAttribute(nodeHandle, NODE_TEXT_INPUT_TEXT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputType(ArkUI_NodeHandle nodeHandle, A2UITextInputType value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_TEXT_INPUT_TYPE, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUITextInputType(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputWordBreak(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_INPUT_WORD_BREAK, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextMaxFontSize(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_TEXT_MAX_FONT_SIZE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextMaxLines(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_MAX_LINES, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextMinFontSize(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_TEXT_MIN_FONT_SIZE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextOverflow(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_OVERFLOW, value);
}

int32_t ArkUINodeApiAdapter::SetNodeTextWordBreak(ArkUI_NodeHandle nodeHandle, int32_t value)
{
    return SetInt32Attribute(nodeHandle, NODE_TEXT_WORD_BREAK, value);
}

int32_t ArkUINodeApiAdapter::SetNodeToggleSelectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TOGGLE_SELECTED_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeToggleSwitchPointColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TOGGLE_SWITCH_POINT_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeToggleUnselectedColor(ArkUI_NodeHandle nodeHandle, uint32_t value)
{
    return SetUint32Attribute(nodeHandle, NODE_TOGGLE_UNSELECTED_COLOR, value);
}

int32_t ArkUINodeApiAdapter::SetNodeToggleValue(ArkUI_NodeHandle nodeHandle, bool value)
{
    return SetBoolAttribute(nodeHandle, NODE_TOGGLE_VALUE, value);
}

int32_t ArkUINodeApiAdapter::SetNodeVisibility(ArkUI_NodeHandle nodeHandle, A2UIVisibility value)
{
    return SetInt32Attribute(
        nodeHandle, NODE_VISIBILITY, static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIVisibility(value)));
}

int32_t ArkUINodeApiAdapter::SetNodeWidth(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_WIDTH, value);
}

int32_t ArkUINodeApiAdapter::SetNodeWidthPercent(ArkUI_NodeHandle nodeHandle, float value)
{
    return SetFloatAttribute(nodeHandle, NODE_WIDTH_PERCENT, value);
}

int32_t ArkUINodeApiAdapter::SetNodeBackgroundImageSize(ArkUI_NodeHandle nodeHandle, float width, float height)
{
    ArkUI_NumberValue values[] = { { .f32 = width }, { .f32 = height } };
    return SetNumberArrayAttribute(nodeHandle, NODE_BACKGROUND_IMAGE_SIZE, values, 2);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderRadius(
    ArkUI_NodeHandle nodeHandle, float topLeft, float topRight, float bottomLeft, float bottomRight)
{
    ArkUI_NumberValue values[] = { { .f32 = topLeft }, { .f32 = topRight }, { .f32 = bottomLeft },
        { .f32 = bottomRight } };
    return SetNumberArrayAttribute(nodeHandle, NODE_BORDER_RADIUS, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderRadiusPercent(
    ArkUI_NodeHandle nodeHandle, float topLeft, float topRight, float bottomLeft, float bottomRight)
{
    ArkUI_NumberValue values[] = { { .f32 = topLeft }, { .f32 = topRight }, { .f32 = bottomLeft },
        { .f32 = bottomRight } };
    return SetNumberArrayAttribute(nodeHandle, NODE_BORDER_RADIUS_PERCENT, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeBorderWidth(
    ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left)
{
    ArkUI_NumberValue values[] = { { .f32 = top }, { .f32 = right }, { .f32 = bottom }, { .f32 = left } };
    return SetNumberArrayAttribute(nodeHandle, NODE_BORDER_WIDTH, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(
    ArkUI_NodeHandle nodeHandle, uint32_t color, float size, float strokeWidth)
{
    ArkUI_NumberValue values[] = { { .u32 = color }, { .f32 = size }, { .f32 = strokeWidth } };
    return SetNumberArrayAttribute(nodeHandle, NODE_CHECKBOX_GROUP_MARK, values, 3);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxGroupMark(ArkUI_NodeHandle nodeHandle, uint32_t color)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_GROUP_MARK, color);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxMark(
    ArkUI_NodeHandle nodeHandle, uint32_t color, float size, float strokeWidth)
{
    ArkUI_NumberValue values[] = { { .u32 = color }, { .f32 = size }, { .f32 = strokeWidth } };
    return SetNumberArrayAttribute(nodeHandle, NODE_CHECKBOX_MARK, values, 3);
}

int32_t ArkUINodeApiAdapter::SetNodeCheckboxMark(ArkUI_NodeHandle nodeHandle, uint32_t color)
{
    return SetUint32Attribute(nodeHandle, NODE_CHECKBOX_MARK, color);
}

int32_t ArkUINodeApiAdapter::SetNodeConstraintSize(
    ArkUI_NodeHandle nodeHandle, float minWidth, float maxWidth, float minHeight, float maxHeight)
{
    ArkUI_NumberValue values[] = { { .f32 = minWidth }, { .f32 = maxWidth }, { .f32 = minHeight },
        { .f32 = maxHeight } };
    return SetNumberArrayAttribute(nodeHandle, NODE_CONSTRAINT_SIZE, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeCustomShadow(ArkUI_NodeHandle nodeHandle, float radius, bool useColorStrategy,
    float offsetX, float offsetY, int32_t type, uint32_t color, bool fill)
{
    ArkUI_NumberValue values[] = { { .f32 = radius }, { .i32 = useColorStrategy ? 1 : 0 }, { .f32 = offsetX },
        { .f32 = offsetY }, { .i32 = type }, { .u32 = color }, { .u32 = fill ? 1U : 0U } };
    return SetNumberArrayAttribute(nodeHandle, NODE_CUSTOM_SHADOW, values, 7);
}

int32_t ArkUINodeApiAdapter::SetNodeFlexOption(ArkUI_NodeHandle nodeHandle, A2UIFlexDirection direction,
    A2UIFlexWrap wrap, A2UIFlexAlignment justifyContent, A2UIItemAlignment alignItems, A2UIFlexAlignment alignContent)
{
    ArkUI_NumberValue values[] = { { .i32 = static_cast<int32_t>(
                                         A2UIArkUITypeConverter::ToArkUIFlexDirection(direction)) },
        { .i32 = static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexWrap(wrap)) },
        { .i32 = static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(justifyContent)) },
        { .i32 = static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIItemAlignment(alignItems)) },
        { .i32 = static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIFlexAlignment(alignContent)) } };
    return SetNumberArrayAttribute(nodeHandle, NODE_FLEX_OPTION, values, 5);
}

int32_t ArkUINodeApiAdapter::SetNodeFlexSpace(ArkUI_NodeHandle nodeHandle, float main, float cross)
{
    ArkUI_NumberValue values[] = { { .f32 = main }, { .f32 = cross } };
    return SetNumberArrayAttribute(nodeHandle, NODE_FLEX_SPACE, values, 2);
}

int32_t ArkUINodeApiAdapter::SetNodeMargin(
    ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left)
{
    ArkUI_NumberValue values[] = { { .f32 = top }, { .f32 = right }, { .f32 = bottom }, { .f32 = left } };
    ArkUI_AttributeItem item = { values, 4 };
    return SetParentLayoutAttribute(nodeHandle, NODE_MARGIN, &item);
}

int32_t ArkUINodeApiAdapter::SetNodeMarginPercent(
    ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left)
{
    ArkUI_NumberValue values[] = { { .f32 = top }, { .f32 = right }, { .f32 = bottom }, { .f32 = left } };
    ArkUI_AttributeItem item = { values, 4 };
    return SetParentLayoutAttribute(nodeHandle, NODE_MARGIN_PERCENT, &item);
}

int32_t ArkUINodeApiAdapter::SetNodePadding(
    ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left)
{
    ArkUI_NumberValue values[] = { { .f32 = top }, { .f32 = right }, { .f32 = bottom }, { .f32 = left } };
    return SetNumberArrayAttribute(nodeHandle, NODE_PADDING, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodePaddingPercent(
    ArkUI_NodeHandle nodeHandle, float top, float right, float bottom, float left)
{
    ArkUI_NumberValue values[] = { { .f32 = top }, { .f32 = right }, { .f32 = bottom }, { .f32 = left } };
    return SetNumberArrayAttribute(nodeHandle, NODE_PADDING_PERCENT, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodePixelRoundNoForceRound(ArkUI_NodeHandle nodeHandle, int32_t apiVersion)
{
    if (nodeHandle == nullptr || apiVersion < MIN_API_VERSION_PIXEL_ROUND) {
        return -1;
    }
    ArkUI_PixelRoundPolicy* policy = ArkUIOHApiAdapter::CreatePixelRoundPolicyNoForceRound();
    if (policy == nullptr) {
        return -1;
    }
    ArkUI_AttributeItem item = { nullptr, 0, nullptr, policy };
    int32_t result = SetAttributeInternal(nodeHandle, NODE_PIXEL_ROUND, &item);
    ArkUIOHApiAdapter::DisposePixelRoundPolicy(policy);
    return result;
}

int32_t ArkUINodeApiAdapter::SetNodeRadioStyle(ArkUI_NodeHandle nodeHandle, uint32_t checkedBackgroundColor,
    uint32_t uncheckedBackgroundColor, uint32_t indicatorColor)
{
    ArkUI_NumberValue values[] = { { .u32 = checkedBackgroundColor }, { .u32 = uncheckedBackgroundColor },
        { .u32 = indicatorColor } };
    return SetNumberArrayAttribute(nodeHandle, NODE_RADIO_STYLE, values, 3);
}

int32_t ArkUINodeApiAdapter::SetNodeScrollNestedScroll(
    ArkUI_NodeHandle nodeHandle, A2UIScrollNestedMode scrollForward, A2UIScrollNestedMode scrollBackward)
{
    ArkUI_NumberValue values[] = { { .i32 = static_cast<int32_t>(
                                         A2UIArkUITypeConverter::ToArkUIScrollNestedMode(scrollForward)) },
        { .i32 = static_cast<int32_t>(A2UIArkUITypeConverter::ToArkUIScrollNestedMode(scrollBackward)) } };
    return SetNumberArrayAttribute(nodeHandle, NODE_SCROLL_NESTED_SCROLL, values, 2);
}

int32_t ArkUINodeApiAdapter::SetNodeShadow(
    ArkUI_NodeHandle nodeHandle, float radius, uint32_t color, float offsetX, float offsetY)
{
    ArkUI_NumberValue values[] = { { .f32 = radius }, { .u32 = color }, { .f32 = offsetX }, { .f32 = offsetY } };
    return SetNumberArrayAttribute(nodeHandle, NODE_SHADOW, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeShadow(ArkUI_NodeHandle nodeHandle, int32_t shadowStyle)
{
    ArkUI_NumberValue values[] = { { .i32 = shadowStyle } };
    return SetNumberArrayAttribute(nodeHandle, NODE_SHADOW, values, 1);
}

int32_t ArkUINodeApiAdapter::SetNodeTextDecoration(ArkUI_NodeHandle nodeHandle, int32_t type, bool hasColor,
    uint32_t color, bool hasStyle, int32_t style, bool hasThicknessScale, float thicknessScale)
{
    ArkUI_NumberValue values[4] = { { .i32 = type }, { .u32 = 0 }, { .i32 = 0 }, { .f32 = 0.0F } };
    int32_t valueCount = 1;
    if (hasColor || hasStyle || hasThicknessScale) {
        values[1].u32 = hasColor ? color : 0;
        valueCount = 2;
    }
    if (hasStyle || hasThicknessScale) {
        values[2].i32 = style;
        valueCount = 3;
    }
    if (hasThicknessScale) {
        values[3].f32 = thicknessScale;
        valueCount = 4;
    }
    return SetNumberArrayAttribute(nodeHandle, NODE_TEXT_DECORATION, values, valueCount);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputCancelButton(ArkUI_NodeHandle nodeHandle, A2UICancelButtonStyle style,
    bool hasIconSize, float iconSize, bool hasIconColor, uint32_t iconColor, bool hasIconSrc,
    const std::string& iconSrc)
{
    ArkUI_NumberValue values[3] = { { .i32 = static_cast<int32_t>(
                                          A2UIArkUITypeConverter::ToArkUICancelButtonStyle(style)) },
        { .f32 = 0.0F }, { .u32 = 0 } };
    int32_t valueCount = 1;
    if (hasIconSize || hasIconColor) {
        values[1].f32 = hasIconSize ? iconSize : 0.0F;
        valueCount = 2;
    }
    if (hasIconColor) {
        values[2].u32 = iconColor;
        valueCount = 3;
    }
    return SetTextInputCancelButtonAttribute(nodeHandle, values, valueCount, hasIconSrc ? iconSrc.c_str() : nullptr);
}

int32_t ArkUINodeApiAdapter::SetNodeTextInputUnderlineColor(
    ArkUI_NodeHandle nodeHandle, uint32_t typing, uint32_t normal, uint32_t error, uint32_t disabled)
{
    ArkUI_NumberValue values[] = { { .u32 = typing }, { .u32 = normal }, { .u32 = error }, { .u32 = disabled } };
    return SetNumberArrayAttribute(nodeHandle, NODE_TEXT_INPUT_UNDERLINE_COLOR, values, 4);
}

int32_t ArkUINodeApiAdapter::SetNodeWidthLayoutPolicy(ArkUI_NodeHandle nodeHandle, int32_t value, int32_t apiVersion)
{
    if (apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY) {
        return -1;
    }
    return SetInt32Attribute(nodeHandle, NODE_WIDTH_LAYOUTPOLICY, value);
}

int32_t ArkUINodeApiAdapter::SetNodeHeightLayoutPolicy(ArkUI_NodeHandle nodeHandle, int32_t value, int32_t apiVersion)
{
    if (apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY) {
        return -1;
    }
    return SetInt32Attribute(nodeHandle, NODE_HEIGHT_LAYOUTPOLICY, value);
}

int32_t ArkUINodeApiAdapter::ResetNodeWidthLayoutPolicy(ArkUI_NodeHandle nodeHandle, int32_t apiVersion)
{
    if (apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY) {
        return -1;
    }
    return ResetAttributeInternal(nodeHandle, NODE_WIDTH_LAYOUTPOLICY);
}

int32_t ArkUINodeApiAdapter::ResetNodeHeightLayoutPolicy(ArkUI_NodeHandle nodeHandle, int32_t apiVersion)
{
    if (apiVersion > 0 && apiVersion < MIN_API_VERSION_LAYOUT_POLICY) {
        return -1;
    }
    return ResetAttributeInternal(nodeHandle, NODE_HEIGHT_LAYOUTPOLICY);
}

int32_t ArkUINodeApiAdapter::ResetNodeAccessibilityDescription(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ACCESSIBILITY_DESCRIPTION);
}

int32_t ArkUINodeApiAdapter::ResetNodeAccessibilityGroup(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ACCESSIBILITY_GROUP);
}

int32_t ArkUINodeApiAdapter::ResetNodeAccessibilityText(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ACCESSIBILITY_TEXT);
}

int32_t ArkUINodeApiAdapter::ResetNodeAspectRatio(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ASPECT_RATIO);
}

int32_t ArkUINodeApiAdapter::ResetNodeBackgroundColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BACKGROUND_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeBackgroundImage(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BACKGROUND_IMAGE);
}

int32_t ArkUINodeApiAdapter::ResetNodeBackgroundImageSize(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BACKGROUND_IMAGE_SIZE);
}

int32_t ArkUINodeApiAdapter::ResetNodeBackgroundImageSizeWithStyle(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BACKGROUND_IMAGE_SIZE_WITH_STYLE);
}

int32_t ArkUINodeApiAdapter::ResetNodeBorderColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BORDER_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeBorderRadius(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BORDER_RADIUS);
}

int32_t ArkUINodeApiAdapter::ResetNodeBorderRadiusPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BORDER_RADIUS_PERCENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeBorderWidth(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BORDER_WIDTH);
}

int32_t ArkUINodeApiAdapter::ResetNodeBorderWidthPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BORDER_WIDTH_PERCENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeButtonLabel(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BUTTON_LABEL);
}

int32_t ArkUINodeApiAdapter::ResetNodeButtonMaxFontScale(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BUTTON_MAX_FONT_SCALE);
}

int32_t ArkUINodeApiAdapter::ResetNodeButtonMinFontScale(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BUTTON_MIN_FONT_SCALE);
}

int32_t ArkUINodeApiAdapter::ResetNodeButtonType(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_BUTTON_TYPE);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroup(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupMark(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_MARK);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupName(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_NAME);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupSelectAll(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_SELECT_ALL);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupSelectedColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_SELECTED_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupShape(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_SHAPE);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxGroupUnselectedColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_GROUP_UNSELECTED_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxMark(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_MARK);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxName(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_NAME);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxSelect(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_SELECT);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxSelectColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_SELECT_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxShape(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_SHAPE);
}

int32_t ArkUINodeApiAdapter::ResetNodeCheckboxUnselectColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CHECKBOX_UNSELECT_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeClip(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CLIP);
}

int32_t ArkUINodeApiAdapter::ResetNodeColumnAlignItems(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_COLUMN_ALIGN_ITEMS);
}

int32_t ArkUINodeApiAdapter::ResetNodeColumnJustifyContent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_COLUMN_JUSTIFY_CONTENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeConstraintSize(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CONSTRAINT_SIZE);
}

int32_t ArkUINodeApiAdapter::ResetNodeCustomShadow(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_CUSTOM_SHADOW);
}

int32_t ArkUINodeApiAdapter::ResetNodeEnabled(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ENABLED);
}

int32_t ArkUINodeApiAdapter::ResetNodeFlexOption(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_FLEX_OPTION);
}

int32_t ArkUINodeApiAdapter::ResetNodeFlexShrink(ArkUI_NodeHandle nodeHandle)
{
    return ResetParentLayoutAttribute(nodeHandle, NODE_FLEX_SHRINK);
}

int32_t ArkUINodeApiAdapter::ResetNodeFlexSpace(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_FLEX_SPACE);
}

int32_t ArkUINodeApiAdapter::ResetNodeFontColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_FONT_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeFontSize(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_FONT_SIZE);
}

int32_t ArkUINodeApiAdapter::ResetNodeFontWeight(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_FONT_WEIGHT);
}

int32_t ArkUINodeApiAdapter::ResetNodeGridAlignItems(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_GRID_ALIGN_ITEMS);
}

int32_t ArkUINodeApiAdapter::ResetNodeGridColumnGap(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_GRID_COLUMN_GAP);
}

int32_t ArkUINodeApiAdapter::ResetNodeGridColumnTemplate(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_GRID_COLUMN_TEMPLATE);
}

int32_t ArkUINodeApiAdapter::ResetNodeGridRowGap(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_GRID_ROW_GAP);
}

int32_t ArkUINodeApiAdapter::ResetNodeGridRowTemplate(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_GRID_ROW_TEMPLATE);
}

int32_t ArkUINodeApiAdapter::ResetNodeHeight(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_HEIGHT);
}

int32_t ArkUINodeApiAdapter::ResetNodeHeightPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_HEIGHT_PERCENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeId(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ID);
}

int32_t ArkUINodeApiAdapter::ResetNodeImageAlt(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_IMAGE_ALT);
}

int32_t ArkUINodeApiAdapter::ResetNodeImageFillColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_IMAGE_FILL_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeImageObjectFit(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_IMAGE_OBJECT_FIT);
}

int32_t ArkUINodeApiAdapter::ResetNodeImageSrc(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_IMAGE_SRC);
}

int32_t ArkUINodeApiAdapter::ResetNodeLayoutWeight(ArkUI_NodeHandle nodeHandle)
{
    return ResetParentLayoutAttribute(nodeHandle, NODE_LAYOUT_WEIGHT);
}

int32_t ArkUINodeApiAdapter::ResetNodeLinearGradient(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LINEAR_GRADIENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeListAlignListItem(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LIST_ALIGN_LIST_ITEM);
}

int32_t ArkUINodeApiAdapter::ResetNodeListDirection(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LIST_DIRECTION);
}

int32_t ArkUINodeApiAdapter::ResetNodeListLanes(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LIST_LANES);
}

int32_t ArkUINodeApiAdapter::ResetNodeListNodeAdapter(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LIST_NODE_ADAPTER);
}

int32_t ArkUINodeApiAdapter::ResetNodeAttribute(ArkUI_NodeHandle nodeHandle, A2UINodeAttributeType attributeType)
{
    return ResetAttributeInternal(nodeHandle, A2UIArkUITypeConverter::ToArkUINodeAttributeType(attributeType));
}

int32_t ArkUINodeApiAdapter::ResetNodeListSpace(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_LIST_SPACE);
}

int32_t ArkUINodeApiAdapter::ResetNodeMargin(ArkUI_NodeHandle nodeHandle)
{
    if (nodeHandle == GetRootNode() && resetCommonMargin_ != nullptr) {
        resetCommonMargin_();
    }
    return ResetParentLayoutAttribute(nodeHandle, NODE_MARGIN);
}

int32_t ArkUINodeApiAdapter::ResetNodeMarginPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetParentLayoutAttribute(nodeHandle, NODE_MARGIN_PERCENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeOpacity(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_OPACITY);
}

int32_t ArkUINodeApiAdapter::ResetNodePadding(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PADDING);
}

int32_t ArkUINodeApiAdapter::ResetNodePaddingPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PADDING_PERCENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeProgressColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PROGRESS_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeProgressTotal(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PROGRESS_TOTAL);
}

int32_t ArkUINodeApiAdapter::ResetNodeProgressType(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PROGRESS_TYPE);
}

int32_t ArkUINodeApiAdapter::ResetNodeProgressValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_PROGRESS_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeRadioChecked(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_RADIO_CHECKED);
}

int32_t ArkUINodeApiAdapter::ResetNodeRadioGroup(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_RADIO_GROUP);
}

int32_t ArkUINodeApiAdapter::ResetNodeRadioStyle(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_RADIO_STYLE);
}

int32_t ArkUINodeApiAdapter::ResetNodeRadioValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_RADIO_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeRowAlignItems(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ROW_ALIGN_ITEMS);
}

int32_t ArkUINodeApiAdapter::ResetNodeRowJustifyContent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_ROW_JUSTIFY_CONTENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeScrollBarDisplayMode(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SCROLL_BAR_DISPLAY_MODE);
}

int32_t ArkUINodeApiAdapter::ResetNodeScrollNestedScroll(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SCROLL_NESTED_SCROLL);
}

int32_t ArkUINodeApiAdapter::ResetNodeShadow(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SHADOW);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderMaxValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_MAX_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderMinValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_MIN_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderSelectedColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_SELECTED_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderStep(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_STEP);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderStyle(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_STYLE);
}

int32_t ArkUINodeApiAdapter::ResetNodeSliderValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_SLIDER_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeStackAlignContent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_STACK_ALIGN_CONTENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextAlign(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_ALIGN);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextAreaText(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_AREA_TEXT);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextContent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_CONTENT);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextDecoration(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_DECORATION);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputCancelButton(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_CANCEL_BUTTON);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputCaretColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_CARET_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputMaxLength(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_MAX_LENGTH);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputNumberOfLines(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_NUMBER_OF_LINES);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputPlaceholder(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputPlaceholderColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputSelectedBackgroundColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputShowUnderline(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_SHOW_UNDERLINE);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputText(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_TEXT);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputType(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_TYPE);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputUnderlineColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_UNDERLINE_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextInputWordBreak(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_INPUT_WORD_BREAK);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextMaxFontSize(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_MAX_FONT_SIZE);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextMaxLines(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_MAX_LINES);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextMinFontSize(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_MIN_FONT_SIZE);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextOverflow(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_OVERFLOW);
}

int32_t ArkUINodeApiAdapter::ResetNodeTextWordBreak(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TEXT_WORD_BREAK);
}

int32_t ArkUINodeApiAdapter::ResetNodeToggleSelectedColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TOGGLE_SELECTED_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeToggleSwitchPointColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TOGGLE_SWITCH_POINT_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeToggleUnselectedColor(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TOGGLE_UNSELECTED_COLOR);
}

int32_t ArkUINodeApiAdapter::ResetNodeToggleValue(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_TOGGLE_VALUE);
}

int32_t ArkUINodeApiAdapter::ResetNodeVisibility(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_VISIBILITY);
}

int32_t ArkUINodeApiAdapter::ResetNodeWidth(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_WIDTH);
}

int32_t ArkUINodeApiAdapter::ResetNodeWidthPercent(ArkUI_NodeHandle nodeHandle)
{
    return ResetAttributeInternal(nodeHandle, NODE_WIDTH_PERCENT);
}

void ArkUINodeApiAdapter::RegisterOnClick(const std::function<void()>& onClick)
{
    if (onClickRegistrar_ != nullptr) {
        onClickRegistrar_(onClick);
    }
}

std::string ArkUINodeApiAdapter::GetComponentId() const
{
    return componentIdGetter_ != nullptr ? componentIdGetter_() : std::string();
}

} // namespace NativeModule
