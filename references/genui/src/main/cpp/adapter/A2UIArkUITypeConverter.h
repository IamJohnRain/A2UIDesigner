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

#ifndef A2UI_ARKUI_TYPE_CONVERTER_H
#define A2UI_ARKUI_TYPE_CONVERTER_H

#include <arkui/native_dialog.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>

#include "A2UIArkUITypes.h"

namespace NativeModule {

class A2UIArkUITypeConverter final {
public:
    static ArkUI_NodeType ToArkUINodeType(A2UINodeType type);
    static ArkUI_ButtonType ToArkUIButtonType(A2UIButtonType type);
    static ArkUI_FlexAlignment ToArkUIFlexAlignment(A2UIFlexAlignment value);
    static ArkUI_FlexDirection ToArkUIFlexDirection(A2UIFlexDirection value);
    static ArkUI_FlexWrap ToArkUIFlexWrap(A2UIFlexWrap value);
    static ArkUI_HorizontalAlignment ToArkUIHorizontalAlignment(A2UIHorizontalAlignment value);
    static ArkUI_ItemAlignment ToArkUIItemAlignment(A2UIItemAlignment value);
    static ArkUI_VerticalAlignment ToArkUIVerticalAlignment(A2UIVerticalAlignment value);
    static ArkUI_ObjectFit ToArkUIObjectFit(A2UIObjectFit value);
    static A2UIObjectFit FromArkUIObjectFit(ArkUI_ObjectFit value);
    static ArkUI_SliderStyle ToArkUISliderStyle(A2UISliderStyle value);
    static ArkUI_TextInputType ToArkUITextInputType(A2UITextInputType value);
    static ArkUI_Visibility ToArkUIVisibility(A2UIVisibility value);
    static ArkUI_ScrollBarDisplayMode ToArkUIScrollBarDisplayMode(A2UIScrollBarDisplayMode value);
    static ArkUI_ScrollNestedMode ToArkUIScrollNestedMode(A2UIScrollNestedMode value);
    static ArkUI_NodeEventType ToArkUINodeEventType(A2UINodeEventType value);
    static A2UINodeEventType FromArkUINodeEventType(ArkUI_NodeEventType value);
    static ArkUI_CancelButtonStyle ToArkUICancelButtonStyle(A2UICancelButtonStyle value);
    static ArkUI_Axis ToArkUIAxis(A2UIAxis value);
    static ArkUI_ListItemAlignment ToArkUIListItemAlignment(A2UIListItemAlignment value);
    static ArkUI_Alignment ToArkUIAlignment(A2UIAlignment value);
    static ArkUI_FontWeight ToArkUIFontWeight(A2UIFontWeight value);
    static ArkUI_ImageSize ToArkUIImageSize(A2UIImageSize value);
    static int32_t ToArkUILayoutPolicy(A2UILayoutPolicy value);
    static ArkUI_LinearGradientDirection ToArkUILinearGradientDirection(A2UILinearGradientDirection value);
    static ArkUI_ShadowType ToArkUIShadowType(A2UIShadowType value);
    static ArkUI_ShadowStyle ToArkUIShadowStyle(A2UIShadowStyle value);
    static int32_t ToArkUIWordBreak(A2UIWordBreak value);
    static A2UINodeAdapterEventType FromArkUINodeAdapterEventType(uint32_t value);
    static ArkUI_NodeAttributeType ToArkUINodeAttributeType(A2UINodeAttributeType value);
    static int32_t ToArkUIDialogAlignment(A2UIAlignment value);
    static int32_t ToArkUICheckboxShape(A2UICheckboxShape value);
};

} // namespace NativeModule

#endif // A2UI_ARKUI_TYPE_CONVERTER_H
