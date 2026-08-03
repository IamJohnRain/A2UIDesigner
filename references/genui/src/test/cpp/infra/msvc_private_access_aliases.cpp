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

#ifdef _MSC_VER

// Some component tests expose private members with `#define private public`.
// MSVC encodes access level in decorated names, so alias the public test names
// to the private/protected names emitted by a2ui_lib.
#pragma comment(linker, \
    "/alternatename:?ApplyTextDecoration@ExtendedStyleResolver@NativeModule@@SAXAEBVJsonValue@2@AEAVArkUINodeApiAdapter@2@@Z=?ApplyTextDecoration@ExtendedStyleResolver@NativeModule@@CAXAEBVJsonValue@2@AEAVArkUINodeApiAdapter@2@@Z")
#pragma comment(linker, \
    "/alternatename:?ParseEdgeStyle@ExtendedStyleResolver@NativeModule@@SA_NAEBVJsonValue@2@PEBD1111AEAUStyleEdge@2@@Z=?ParseEdgeStyle@ExtendedStyleResolver@NativeModule@@CA_NAEBVJsonValue@2@PEBD1111AEAUStyleEdge@2@@Z")
#pragma comment(linker, \
    "/alternatename:?DimensionToFloat@ExtendedStyleResolver@NativeModule@@SA_NAEBUStyleDimension@2@AEAM@Z=?DimensionToFloat@ExtendedStyleResolver@NativeModule@@CA_NAEBUStyleDimension@2@AEAM@Z")

#pragma comment(linker, \
    "/alternatename:?ParseAndRegisterEventHandlers@ExtendedComponent@NativeModule@@QEAAXAEBVJsonValue@2@@Z=?ParseAndRegisterEventHandlers@ExtendedComponent@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
#pragma comment(linker, \
    "/alternatename:?MergeEventContext@ExtendedComponent@NativeModule@@SA?AVJsonValue@2@AEBV32@0@Z=?MergeEventContext@ExtendedComponent@NativeModule@@CA?AVJsonValue@2@AEBV32@0@Z")

#pragma comment(linker, \
    "/alternatename:??0MockArkUINativeProvider@NativeModule@@QEAA@XZ=??0MockArkUINativeProvider@NativeModule@@AEAA@XZ")

#pragma comment(linker, \
    "/alternatename:??0TemplateAdapterNode@NativeModule@@QEAA@XZ=??0TemplateAdapterNode@NativeModule@@IEAA@XZ")
#pragma comment(linker, \
    "/alternatename:?OnStaticAdapterEvent@TemplateAdapterNode@NativeModule@@SAXPEAUArkUI_NodeAdapterEvent@@@Z=?OnStaticAdapterEvent@TemplateAdapterNode@NativeModule@@CAXPEAUArkUI_NodeAdapterEvent@@@Z")
#pragma comment(linker, \
    "/alternatename:?OnAdapterEvent@TemplateAdapterNode@NativeModule@@QEAAXPEAUArkUI_NodeAdapterEvent@@@Z=?OnAdapterEvent@TemplateAdapterNode@NativeModule@@AEAAXPEAUArkUI_NodeAdapterEvent@@@Z")
#pragma comment(linker, \
    "/alternatename:?OnNewItemAttached@TemplateAdapterNode@NativeModule@@QEAAXPEAUArkUI_NodeAdapterEvent@@@Z=?OnNewItemAttached@TemplateAdapterNode@NativeModule@@AEAAXPEAUArkUI_NodeAdapterEvent@@@Z")
#pragma comment(linker, \
    "/alternatename:?OnItemDetached@TemplateAdapterNode@NativeModule@@QEAAXPEAUArkUI_NodeAdapterEvent@@@Z=?OnItemDetached@TemplateAdapterNode@NativeModule@@AEAAXPEAUArkUI_NodeAdapterEvent@@@Z")
#pragma comment(linker, \
    "/alternatename:?CleanUpTargetComponentTreeFromAllComponents@TemplateAdapterNode@NativeModule@@SAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@AEAV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VComponent@NativeModule@@@2@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VComponent@NativeModule@@@2@@std@@@2@@4@@Z=?CleanUpTargetComponentTreeFromAllComponents@TemplateAdapterNode@NativeModule@@CAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@AEAV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VComponent@NativeModule@@@2@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$shared_ptr@VComponent@NativeModule@@@2@@std@@@2@@4@@Z")
#pragma comment(linker, \
    "/alternatename:?BuildTemplateInstanceDescriptorById@TemplateAdapterNode@NativeModule@@SA?AV?$unique_ptr@VJsonAdapter@NativeModule@@U?$default_delete@VJsonAdapter@NativeModule@@@std@@@std@@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@AEBV54@1HPEBV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@NativeModule@@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@NativeModule@@@std@@@2@@4@PEAV64@@Z=?BuildTemplateInstanceDescriptorById@TemplateAdapterNode@NativeModule@@CA?AV?$unique_ptr@VJsonAdapter@NativeModule@@U?$default_delete@VJsonAdapter@NativeModule@@@std@@@std@@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@AEBV54@1HPEBV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@NativeModule@@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@NativeModule@@@std@@@2@@4@PEAV64@@Z")

#pragma comment(linker, \
    "/alternatename:?SetValueText@TextFieldComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z=?SetValueText@TextFieldComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z")
#pragma comment(linker, \
    "/alternatename:?SetValidationRegexp@TextFieldComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z=?SetValidationRegexp@TextFieldComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z")

#define A2UI_MSVC_ALIAS(publicName, targetName) __pragma(comment(linker, "/alternatename:" publicName "=" targetName))

A2UI_MSVC_ALIAS("?AcceptsChild@Component@NativeModule@@UEBA_NAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?AcceptsChild@Component@NativeModule@@MEBA_NAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?AcceptsChild@CustomComponent@NativeModule@@UEBA_NAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?AcceptsChild@CustomComponent@NativeModule@@MEBA_NAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?ApplyCommonAttributes@Component@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ApplyCommonAttributes@Component@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ApplyCommonAttributes@CustomComponent@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ApplyCommonAttributes@CustomComponent@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ApplyComponentSpecificStyles@ExtendedGridComponent@NativeModule@@UEAAXAEBVJsonValue@2@"
                "AEAVArkUINodeApiAdapter@2@@Z",
    "?ApplyComponentSpecificStyles@ExtendedGridComponent@NativeModule@@MEAAXAEBVJsonValue@2@AEAVArkUINodeApiAdapter@2@@"
    "Z")
A2UI_MSVC_ALIAS("?ApplyCustomProperties@CustomComponent@NativeModule@@QEAAXAEBVJsonValue@2@@Z",
    "?ApplyCustomProperties@CustomComponent@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ApplyExtendedComponentDescriptor@SurfaceSlot@NativeModule@@QEBAXAEBVJsonValue@2@AEBV?$shared_ptr@"
                "VComponent@NativeModule@@@std@@_NAEBURenderContext@2@@Z",
    "?ApplyExtendedComponentDescriptor@SurfaceSlot@NativeModule@@AEBAXAEBVJsonValue@2@AEBV?$shared_ptr@VComponent@"
    "NativeModule@@@std@@_NAEBURenderContext@2@@Z")
A2UI_MSVC_ALIAS("?ApplyMarginToChild@Component@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@MM@Z",
    "?ApplyMarginToChild@Component@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@MM@Z")
A2UI_MSVC_ALIAS("?ApplyPrivateAttributes@Component@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ApplyPrivateAttributes@Component@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ApplyPrivateAttributes@CustomComponent@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ApplyPrivateAttributes@CustomComponent@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ApplyRowsTemplateForContext@ExtendedGridComponent@NativeModule@@QEAAXAEBUThemeContext@2@@Z",
    "?ApplyRowsTemplateForContext@ExtendedGridComponent@NativeModule@@AEAAXAEBUThemeContext@2@@Z")
A2UI_MSVC_ALIAS("?ApplySingleResolvedStyle@ExtendedComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@"
                "std@@V?$allocator@D@2@@std@@AEBVJsonValue@2@@Z",
    "?ApplySingleResolvedStyle@ExtendedComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?BuildComponent@SurfaceSlot@NativeModule@@QEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@@AEBV?$"
                "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z",
    "?BuildComponent@SurfaceSlot@NativeModule@@AEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@@AEBV?$basic_string@"
    "DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z")
A2UI_MSVC_ALIAS("?BuildComponentTree@SurfaceSlot@NativeModule@@QEAAXAEBV?$set@V?$shared_ptr@VComponent@NativeModule@@@"
                "std@@UBuildNodeDepthComparator@SurfaceSlot@NativeModule@@V?$allocator@V?$shared_ptr@VComponent@"
                "NativeModule@@@std@@@2@@std@@@Z",
    "?BuildComponentTree@SurfaceSlot@NativeModule@@AEAAXAEBV?$set@V?$shared_ptr@VComponent@NativeModule@@@std@@"
    "UBuildNodeDepthComparator@SurfaceSlot@NativeModule@@V?$allocator@V?$shared_ptr@VComponent@NativeModule@@@std@@@2@@"
    "std@@@Z")
A2UI_MSVC_ALIAS("?BuildCustomProps@CustomComponent@NativeModule@@QEBA?AVJsonValue@2@XZ",
    "?BuildCustomProps@CustomComponent@NativeModule@@AEBA?AVJsonValue@2@XZ")
A2UI_MSVC_ALIAS("?BuildExtendedComponent@SurfaceSlot@NativeModule@@QEBA?AV?$shared_ptr@VComponent@NativeModule@@@std@@"
                "AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z",
    "?BuildExtendedComponent@SurfaceSlot@NativeModule@@AEBA?AV?$shared_ptr@VComponent@NativeModule@@@std@@AEBV?$basic_"
    "string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z")
A2UI_MSVC_ALIAS("?BuildItemWrapper@GridAdapterNode@NativeModule@@UEBA?AUItemWrapperInfo@TemplateAdapterNode@2@AEBV?$"
                "shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?BuildItemWrapper@GridAdapterNode@NativeModule@@MEBA?AUItemWrapperInfo@TemplateAdapterNode@2@AEBV?$shared_ptr@"
    "VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?BuildItemWrapper@TemplateAdapterNode@NativeModule@@UEBA?AUItemWrapperInfo@12@AEBV?$shared_ptr@"
                "VComponent@NativeModule@@@std@@@Z",
    "?BuildItemWrapper@TemplateAdapterNode@NativeModule@@MEBA?AUItemWrapperInfo@12@AEBV?$shared_ptr@VComponent@"
    "NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?BuildRootFromComponents@SurfaceSlot@NativeModule@@QEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@@"
                "AEBVJsonValue@2@AEA_N1@Z",
    "?BuildRootFromComponents@SurfaceSlot@NativeModule@@AEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@@"
    "AEBVJsonValue@2@AEA_N1@Z")
A2UI_MSVC_ALIAS("?CollectChildListDescriptor@Component@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?CollectChildListDescriptor@Component@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?CollectChildListDescriptor@CustomComponent@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?CollectChildListDescriptor@CustomComponent@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?CreateAttributeValue@CustomComponent@NativeModule@@QEBAPEAUnapi_value__@@XZ",
    "?CreateAttributeValue@CustomComponent@NativeModule@@AEBAPEAUnapi_value__@@XZ")
A2UI_MSVC_ALIAS("?DisposeComponentContent@CustomComponent@NativeModule@@QEAAXXZ",
    "?DisposeComponentContent@CustomComponent@NativeModule@@AEAAXXZ")
A2UI_MSVC_ALIAS("?EvaluateCustomExpression@Component@NativeModule@@QEBA?AVJsonValue@2@AEBV?$basic_string@DU?$char_"
                "traits@D@std@@V?$allocator@D@2@@std@@@Z",
    "?EvaluateCustomExpression@Component@NativeModule@@IEBA?AVJsonValue@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@@Z")
A2UI_MSVC_ALIAS("?ExpandTemplateChildren@Component@NativeModule@@UEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@"
                "AEAV?$list@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@"
                "DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@@Z",
    "?ExpandTemplateChildren@Component@NativeModule@@MEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@AEAV?$list@V?$"
    "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@@2@@std@@@Z")
A2UI_MSVC_ALIAS("?ExpandTemplateChildren@CustomComponent@NativeModule@@UEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@"
                "2@AEAV?$list@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_"
                "string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@@Z",
    "?ExpandTemplateChildren@CustomComponent@NativeModule@@MEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@AEAV?$"
    "list@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@"
    "std@@V?$allocator@D@2@@std@@@2@@std@@@Z")
A2UI_MSVC_ALIAS("?ExpandTemplateChildren@ExtendedGridComponent@NativeModule@@UEAA_NAEBUChildListDescriptor@2@"
                "AEAVSurfaceSlot@2@AEAV?$list@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$"
                "allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@@Z",
    "?ExpandTemplateChildren@ExtendedGridComponent@NativeModule@@MEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@"
    "AEAV?$list@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_"
    "traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@@Z")
A2UI_MSVC_ALIAS("?GetComponentDirectRequiredPropertyKeys@Component@NativeModule@@UEBA?AV?$vector@V?$basic_string@DU?$"
                "char_traits@D@std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@@2@@std@@XZ",
    "?GetComponentDirectRequiredPropertyKeys@Component@NativeModule@@MEBA?AV?$vector@V?$basic_string@DU?$char_traits@D@"
    "std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@"
    "XZ")
A2UI_MSVC_ALIAS("?GetOrCreateComponentNode@SurfaceSlot@NativeModule@@QEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@"
                "@AEBVJsonValue@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@1AEA_N@Z",
    "?GetOrCreateComponentNode@SurfaceSlot@NativeModule@@AEAA?AV?$shared_ptr@VComponent@NativeModule@@@std@@"
    "AEBVJsonValue@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@1AEA_N@Z")
A2UI_MSVC_ALIAS("?GetPrivatePropertyDeclaration@Component@NativeModule@@UEAA?AUPropertyDeclaration@2@AEBV?$basic_"
                "string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
    "?GetPrivatePropertyDeclaration@Component@NativeModule@@MEAA?AUPropertyDeclaration@2@AEBV?$basic_string@DU?$char_"
    "traits@D@std@@V?$allocator@D@2@@std@@@Z")
A2UI_MSVC_ALIAS("?GetShortType@CustomComponent@NativeModule@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@"
                "D@2@@std@@AEBV34@@Z",
    "?GetShortType@CustomComponent@NativeModule@@AEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "AEBV34@@Z")
A2UI_MSVC_ALIAS("?HandleSpecialProperty@Component@NativeModule@@UEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@AEBVJsonValue@2@@Z",
    "?HandleSpecialProperty@Component@NativeModule@@MEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?IsExpressionCandidate@Component@NativeModule@@UEBA_NAEBVJsonValue@2@@Z",
    "?IsExpressionCandidate@Component@NativeModule@@MEBA_NAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?IsExpressionSupported@Component@NativeModule@@UEBA_NXZ",
    "?IsExpressionSupported@Component@NativeModule@@MEBA_NXZ")
A2UI_MSVC_ALIAS("?IsExtendedEtsExpressionScope@CustomComponent@NativeModule@@QEBA_NXZ",
    "?IsExtendedEtsExpressionScope@CustomComponent@NativeModule@@AEBA_NXZ")
A2UI_MSVC_ALIAS("?IsKnownAdditionalDescriptorKey@Component@NativeModule@@UEBA_NAEBV?$basic_string@DU?$char_traits@D@"
                "std@@V?$allocator@D@2@@std@@@Z",
    "?IsKnownAdditionalDescriptorKey@Component@NativeModule@@MEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@@Z")
A2UI_MSVC_ALIAS("?IsKnownNestedDescriptorKey@Component@NativeModule@@UEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?"
                "$allocator@D@2@@std@@0@Z",
    "?IsKnownNestedDescriptorKey@Component@NativeModule@@MEBA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@"
    "D@2@@std@@0@Z")
A2UI_MSVC_ALIAS("?mockDialogAPI_@MockArkUINativeProvider@NativeModule@@2UArkUI_NativeDialogAPI_1@@A",
    "?mockDialogAPI_@MockArkUINativeProvider@NativeModule@@0UArkUI_NativeDialogAPI_1@@A")
A2UI_MSVC_ALIAS("?mockNodeAPI_@MockArkUINativeProvider@NativeModule@@2UArkUI_NativeNodeAPI_1@@A",
    "?mockNodeAPI_@MockArkUINativeProvider@NativeModule@@0UArkUI_NativeNodeAPI_1@@A")
A2UI_MSVC_ALIAS("?NormalizeCustomProperty@CustomComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@"
                "V?$allocator@D@2@@std@@AEAVJsonValue@2@@Z",
    "?NormalizeCustomProperty@CustomComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@AEAVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?OnAddChild@Component@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z",
    "?OnAddChild@Component@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z")
A2UI_MSVC_ALIAS("?OnAddChild@CustomComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z",
    "?OnAddChild@CustomComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z")
A2UI_MSVC_ALIAS(
    "?OnAddChild@ExtendedGridComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z",
    "?OnAddChild@ExtendedGridComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K@Z")
A2UI_MSVC_ALIAS(
    "?OnAttachToParent@Component@NativeModule@@UEAAXXZ", "?OnAttachToParent@Component@NativeModule@@MEAAXXZ")
A2UI_MSVC_ALIAS("?OnConfigChange@CustomComponent@NativeModule@@UEAAXAEBUThemeContext@2@@Z",
    "?OnConfigChange@CustomComponent@NativeModule@@MEAAXAEBUThemeContext@2@@Z")
A2UI_MSVC_ALIAS("?OnDataUpdate@CustomComponent@NativeModule@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@AEBVJsonValue@2@@Z",
    "?OnDataUpdate@CustomComponent@NativeModule@@MEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?OnMoveChild@Component@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z",
    "?OnMoveChild@Component@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z")
A2UI_MSVC_ALIAS("?OnMoveChild@CustomComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z",
    "?OnMoveChild@CustomComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z")
A2UI_MSVC_ALIAS(
    "?OnMoveChild@ExtendedGridComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z",
    "?OnMoveChild@ExtendedGridComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_K1@Z")
A2UI_MSVC_ALIAS("?OnNestedAdapterUpdate@GridAdapterNode@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@"
                "std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z",
    "?OnNestedAdapterUpdate@GridAdapterNode@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@AEBV?$"
    "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@@Z")
A2UI_MSVC_ALIAS("?OnPropertyApplied@Component@NativeModule@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@AEBVJsonValue@2@@Z",
    "?OnPropertyApplied@Component@NativeModule@@MEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@"
    "AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?OnPropertyApplied@CustomComponent@NativeModule@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@AEBVJsonValue@2@@Z",
    "?OnPropertyApplied@CustomComponent@NativeModule@@MEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?OnPropertyRemoved@Component@NativeModule@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@@Z",
    "?OnPropertyRemoved@Component@NativeModule@@MEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@"
    "Z")
A2UI_MSVC_ALIAS("?OnPropertyRemoved@CustomComponent@NativeModule@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@@Z",
    "?OnPropertyRemoved@CustomComponent@NativeModule@@MEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@"
    "std@@@Z")
A2UI_MSVC_ALIAS("?OnRemoveChild@Component@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?OnRemoveChild@Component@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?OnRemoveChild@CustomComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?OnRemoveChild@CustomComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS(
    "?OnRemoveChild@ExtendedGridComponent@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z",
    "?OnRemoveChild@ExtendedGridComponent@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@@Z")
A2UI_MSVC_ALIAS("?ParseChecks@CustomComponent@NativeModule@@QEAAXAEBVJsonValue@2@@Z",
    "?ParseChecks@CustomComponent@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ParseTabsMapping@CustomComponent@NativeModule@@QEAAXAEBVJsonValue@2@@Z",
    "?ParseTabsMapping@CustomComponent@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS(
    "?ParseTemplateConfig@ExtendedGridComponent@NativeModule@@SA_NAEBVJsonValue@2@AEAUGridTemplateConfig@12@@Z",
    "?ParseTemplateConfig@ExtendedGridComponent@NativeModule@@CA_NAEBVJsonValue@2@AEAUGridTemplateConfig@12@@Z")
A2UI_MSVC_ALIAS("?PrepareDescriptorById@SurfaceSlot@NativeModule@@QEAAXAEBVJsonValue@2@@Z",
    "?PrepareDescriptorById@SurfaceSlot@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS(
    "?RegisterComponentIfNeeded@SurfaceSlot@NativeModule@@QEBAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_N@Z",
    "?RegisterComponentIfNeeded@SurfaceSlot@NativeModule@@AEBAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@_N@Z")
A2UI_MSVC_ALIAS("?RegisterDataBindings@CustomComponent@NativeModule@@QEAAXAEBVJsonValue@2@@Z",
    "?RegisterDataBindings@CustomComponent@NativeModule@@AEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?RemoveGridItemNode@ExtendedGridComponent@NativeModule@@QEAAXPEAUArkUI_Node@@@Z",
    "?RemoveGridItemNode@ExtendedGridComponent@NativeModule@@AEAAXPEAUArkUI_Node@@@Z")
A2UI_MSVC_ALIAS("?ReportCustomSchemaWarning@CustomComponent@NativeModule@@QEBAXAEBV?$basic_string@DU?$char_traits@D@"
                "std@@V?$allocator@D@2@@std@@00@Z",
    "?ReportCustomSchemaWarning@CustomComponent@NativeModule@@AEBAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@00@Z")
A2UI_MSVC_ALIAS(
    "?ResetReferences@CustomComponent@NativeModule@@QEAAXXZ", "?ResetReferences@CustomComponent@NativeModule@@AEAAXXZ")
A2UI_MSVC_ALIAS("?ResolveExpressionsInValue@CustomComponent@NativeModule@@QEBA?AVJsonValue@2@AEBV32@AEBV?$basic_string@"
                "DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
    "?ResolveExpressionsInValue@CustomComponent@NativeModule@@AEBA?AVJsonValue@2@AEBV32@AEBV?$basic_string@DU?$char_"
    "traits@D@std@@V?$allocator@D@2@@std@@@Z")
A2UI_MSVC_ALIAS("?ResolveResponsiveTemplate@ExtendedGridComponent@NativeModule@@SA?AV?$basic_string@DU?$char_traits@D@"
                "std@@V?$allocator@D@2@@std@@AEBUGridTemplateConfig@12@AEBUThemeContext@2@@Z",
    "?ResolveResponsiveTemplate@ExtendedGridComponent@NativeModule@@CA?AV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@AEBUGridTemplateConfig@12@AEBUThemeContext@2@@Z")
A2UI_MSVC_ALIAS("?ResolveTabsChildIds@CustomComponent@NativeModule@@QEBA?AV?$list@V?$basic_string@DU?$char_traits@D@"
                "std@@V?$allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@"
                "@@2@@std@@AEBVJsonValue@2@@Z",
    "?ResolveTabsChildIds@CustomComponent@NativeModule@@AEBA?AV?$list@V?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@V?$allocator@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@@std@@"
    "AEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ResolveThemeContext@ExtendedGridComponent@NativeModule@@QEBA?AUThemeContext@2@XZ",
    "?ResolveThemeContext@ExtendedGridComponent@NativeModule@@AEBA?AUThemeContext@2@XZ")
A2UI_MSVC_ALIAS("?ResolveVisibleIndex@NavContainerComponent@NativeModule@@QEBAH_K@Z",
    "?ResolveVisibleIndex@NavContainerComponent@NativeModule@@AEBAH_K@Z")
A2UI_MSVC_ALIAS("?SetColumnsGap@ExtendedGridComponent@NativeModule@@QEAAXM@Z",
    "?SetColumnsGap@ExtendedGridComponent@NativeModule@@AEAAXM@Z")
A2UI_MSVC_ALIAS("?SetColumnsTemplate@ExtendedGridComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@"
                "@V?$allocator@D@2@@std@@@Z",
    "?SetColumnsTemplate@ExtendedGridComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@@Z")
A2UI_MSVC_ALIAS("?SetRowsGap@ExtendedGridComponent@NativeModule@@QEAAXM@Z",
    "?SetRowsGap@ExtendedGridComponent@NativeModule@@AEAAXM@Z")
A2UI_MSVC_ALIAS("?SetRowsTemplate@ExtendedGridComponent@NativeModule@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?"
                "$allocator@D@2@@std@@@Z",
    "?SetRowsTemplate@ExtendedGridComponent@NativeModule@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@"
    "D@2@@std@@@Z")
A2UI_MSVC_ALIAS(
    "?SetupLazyAdapter@ExtendedGridComponent@NativeModule@@QEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@@Z",
    "?SetupLazyAdapter@ExtendedGridComponent@NativeModule@@AEAA_NAEBUChildListDescriptor@2@AEAVSurfaceSlot@2@@Z")
A2UI_MSVC_ALIAS("?SetupNestedAdapter@GridAdapterNode@NativeModule@@UEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@"
                "@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@11AEBV?$map@V?$basic_string@DU?$char_"
                "traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@NativeModule@@U?$less@V?$basic_string@DU?$char_traits@"
                "D@std@@V?$allocator@D@2@@std@@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$"
                "allocator@D@2@@std@@VJsonValue@NativeModule@@@std@@@2@@4@@Z",
    "?SetupNestedAdapter@GridAdapterNode@NativeModule@@MEAAXAEBV?$shared_ptr@VComponent@NativeModule@@@std@@AEBV?$"
    "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@4@11AEBV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$"
    "allocator@D@2@@std@@VJsonValue@NativeModule@@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@"
    "@@2@V?$allocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@VJsonValue@"
    "NativeModule@@@std@@@2@@4@@Z")
A2UI_MSVC_ALIAS("?ShouldValidateUnknownDescriptorFields@Component@NativeModule@@UEBA_NXZ",
    "?ShouldValidateUnknownDescriptorFields@Component@NativeModule@@MEBA_NXZ")
A2UI_MSVC_ALIAS("?ShouldValidateUnknownDescriptorFields@CustomComponent@NativeModule@@UEBA_NXZ",
    "?ShouldValidateUnknownDescriptorFields@CustomComponent@NativeModule@@MEBA_NXZ")
A2UI_MSVC_ALIAS("?SyncChildSlots@CustomComponent@NativeModule@@QEAAXPEAUnapi_value__@@@Z",
    "?SyncChildSlots@CustomComponent@NativeModule@@AEAAXPEAUnapi_value__@@@Z")
A2UI_MSVC_ALIAS("?UpdateComponentsArray@SurfaceSlot@NativeModule@@QEAA_NAEBVJsonValue@2@@Z",
    "?UpdateComponentsArray@SurfaceSlot@NativeModule@@AEAA_NAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?UpdateSurfaceProtocolMode@SurfaceSlot@NativeModule@@QEAAXXZ",
    "?UpdateSurfaceProtocolMode@SurfaceSlot@NativeModule@@AEAAXXZ")
A2UI_MSVC_ALIAS("?ValidateComponentDescriptorSchema@Component@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ValidateComponentDescriptorSchema@Component@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ValidateComponentDescriptorSchema@CustomComponent@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ValidateComponentDescriptorSchema@CustomComponent@NativeModule@@MEAAXAEBVJsonValue@2@@Z")
A2UI_MSVC_ALIAS("?ValidateComponentSpecificStylesSchema@ExtendedGridComponent@NativeModule@@UEAAXAEBVJsonValue@2@@Z",
    "?ValidateComponentSpecificStylesSchema@ExtendedGridComponent@NativeModule@@MEAAXAEBVJsonValue@2@@Z")

#undef A2UI_MSVC_ALIAS

#endif
