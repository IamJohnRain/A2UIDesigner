cmake_minimum_required(VERSION 3.14)
project(HwClawNativeRenderer)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include(cmake/cjson.cmake)

option(ENABLE_EXPRESSION_ENGINE "Enable expression engine" ON)

set(A2UI_COMMIT_ID "$ENV{A2UI_COMMIT_ID}")
if("${A2UI_COMMIT_ID}" STREQUAL "")
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../..
        OUTPUT_VARIABLE A2UI_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if("${A2UI_COMMIT_ID}" STREQUAL "")
    set(A2UI_COMMIT_ID "unknown")
endif()
string(REGEX REPLACE "[^0-9A-Za-z._-]" "_" A2UI_COMMIT_ID "${A2UI_COMMIT_ID}")
set(A2UI_COMMIT_SHORT_ID "$ENV{A2UI_COMMIT_SHORT_ID}")
if("${A2UI_COMMIT_SHORT_ID}" STREQUAL "")
    if(NOT "${A2UI_COMMIT_ID}" STREQUAL "unknown")
        string(SUBSTRING "${A2UI_COMMIT_ID}" 0 12 A2UI_COMMIT_SHORT_ID)
    else()
        set(A2UI_COMMIT_SHORT_ID "unknown")
    endif()
endif()
string(REGEX REPLACE "[^0-9A-Za-z._-]" "_" A2UI_COMMIT_SHORT_ID "${A2UI_COMMIT_SHORT_ID}")
if(NOT "${A2UI_COMMIT_SHORT_ID}" STREQUAL "unknown")
    string(SUBSTRING "${A2UI_COMMIT_SHORT_ID}" 0 12 A2UI_COMMIT_SHORT_ID)
endif()

add_library(a2ui_native SHARED
    adapter/SharedLibrarySymbolLoader.cpp
    adapter/A2UIArkUITypeConverter.cpp
    adapter/ArkUIOHApiAdapter.cpp
    adapter/ArkUINodeApiAdapter.cpp
    adapter/ArkUINativeProvider.cpp
    adapter/NapiProvider.cpp
    adapter/NapiBridge.cpp
    NapiInit.cpp
    NativeEntry.cpp
    NapiResourceManager.cpp
    adapter/ArkUINativeAPI.cpp
    RenderSlot.cpp
    RenderManager.cpp
    SurfaceSlot.cpp
    SurfaceSlotSchemaValidation.cpp
    SurfaceManager.cpp
    theme/ThemeBase.cpp
    theme/ThemeManager.cpp
    theme/ThemeFactory.cpp
    components/extended/ExtendedCommonTheme.cpp
    composition/ChildListParser.cpp
    composition/TemplateInstantiator.cpp
    composition/EagerChildrenStrategy.cpp
    functions/ActionInfo.cpp
    functions/ActionParser.cpp
    functions/ActionDispatchBridge.cpp
    functions/RuntimeErrorDispatchBridge.cpp
    functions/WarningDispatchBridge.cpp
    functions/CrossLanguageAttributeBridge.cpp
    functions/EventContextResolver.cpp
    functions/FunctionBridge.cpp
    functions/FunctionCallInfo.cpp
    functions/FunctionResult.cpp
    functions/NativeFunctionBase.cpp
    functions/NativeFunctionRegistry.cpp
    functions/NativeRequiredFunction.cpp
    functions/NativeRegexFunction.cpp
    functions/NativeLengthFunction.cpp
    functions/NativeNumericFunction.cpp
    functions/NativeEmailFunction.cpp
    functions/NativeFormatStringFunction.cpp
    functions/NativeFormatNumberFunction.cpp
    functions/NativeFormatCurrencyFunction.cpp
    functions/NativeFormatDateFunction.cpp
    functions/NativePluralizeFunction.cpp
    functions/NativeAndFunction.cpp
    functions/NativeOrFunction.cpp
    functions/NativeNotFunction.cpp
    functions/NativeGetToggleValueFunction.cpp
    functions/NativeGetRadioValueFunction.cpp
    functions/NativeGetCheckboxGroupValuesFunction.cpp
    functions/NativeGetSelectValueFunction.cpp
    functions/extended/NativeNavigateFunction.cpp
    components/actions/EventHandlerParser.cpp
    components/actions/EventHandlerChainExecutor.cpp
    components/actions/NativeActionRegistry.cpp
    components/actions/BuiltInActions.cpp
    components/actions/ActionDispatcher.cpp
    components/CustomComponentFactory.cpp
    components/custom/ExtendedTabsPrebuildHelper.cpp
    utils/JsonAdapter.cpp
    utils/NapiUtils.cpp
    utils/ThemeColorUtils.cpp
    utils/DisplayDensityUtils.cpp
    utils/RequiredStringPropertyUtils.cpp
    components/ChildListSchemaValidationUtils.cpp
    components/Component.cpp
    components/A2UI/A2UIComponent.cpp
    components/A2UI/button/ButtonComponent.cpp
    components/A2UI/button/ButtonTheme.cpp
    checks/ChecksEngine.cpp
    components/A2UI/text/TextComponent.cpp
    components/A2UI/text/TextTheme.cpp
    components/A2UI/column/ColumnComponent.cpp
    components/A2UI/column/ColumnTheme.cpp
    components/A2UI/modal/ModalCoordinator.cpp
    components/A2UI/row/RowComponent.cpp
    components/A2UI/row/RowTheme.cpp
    components/A2UI/textfield/TextFieldComponent.cpp
    components/A2UI/textfield/TextFieldTheme.cpp
    components/A2UI/card/CardComponent.cpp
    components/A2UI/card/CardTheme.cpp
    components/A2UI/checkbox/CheckboxComponent.cpp
    components/A2UI/checkbox/CheckboxTheme.cpp
    components/A2UI/checkbox/CheckboxGroupTheme.cpp
    components/A2UI/image/ImageComponent.cpp
    components/A2UI/image/ImageTheme.cpp
    components/A2UI/list/ListComponent.cpp
    components/A2UI/list/ListTheme.cpp
    components/A2UI/slider/SliderComponent.cpp
    components/A2UI/slider/SliderTheme.cpp
    components/custom/CustomComponent.cpp
    components/custom/CustomComponentBindingLifecycle.cpp
    components/custom/CustomComponentExpressionBinding.cpp
    components/custom/CustomComponentTabsValidation.cpp
    components/custom/CustomComponentTabContentValidation.cpp
    components/NativeComponentFactory.cpp
    # Concrete extended components stay under components/extended.
    components/extended/ExtendedButtonComponent.cpp
    components/extended/ExtendedRadioComponent.cpp
    components/extended/ExtendedTextTheme.cpp
    components/extended/ExtendedDividerTheme.cpp
    components/extended/ExtendedProgressTheme.cpp
    components/extended/ExtendedGridTheme.cpp
    components/extended/ExtendedListTheme.cpp
    components/extended/ExtendedTextComponent.cpp
    components/extended/ExtendedTextInputComponent.cpp
    components/extended/ExtendedToggleComponent.cpp
    components/extended/ExtendedColumnComponent.cpp
    components/extended/ExtendedGridComponent.cpp
    components/extended/ExtendedListComponent.cpp
    components/extended/ExtendedRowComponent.cpp
    components/extended/ExtendedDividerComponent.cpp
    components/extended/ExtendedRadioComponent.cpp
    components/extended/ExtendedImageComponent.cpp
     components/extended/ExtendedProgressComponent.cpp
     components/extended/ExtendedStackComponent.cpp
     components/extended/ExtendedCheckboxComponent.cpp
     components/extended/ExtendedCheckboxGroupComponent.cpp
     components/extended/NavContainerComponent.cpp
     components/extended/if/IfComponent.cpp
    # Shared extended component infrastructure lives under components/extended.
     components/extended/ExtendedComponent.cpp
     components/extended/ExtendedComponentStyleValidation.cpp
     components/extended/ExtendedComponentFactory.cpp
     components/extended/ExtendedDescriptorNormalizer.cpp
     components/extended/ExtendedStyleResolver.cpp
     components/extended/RenderContext.cpp
    styles/StyleApplyUtils.cpp
    styles/StyleApplyUtilsLayout.cpp
    styles/StyleApplyUtilsText.cpp
    styles/StyleApplyUtilsEffects.cpp
    styles/StyleParser.cpp
    styles/StyleResolver.cpp
    composition/TemplateAdapterNode.cpp
    composition/ListAdapterNode.cpp
    composition/GridAdapterNode.cpp
    catalog/CatalogItem.cpp
    catalog/Catalog.cpp
    data/DataModel.h
    data/BindingEngine.h
    data/DataModel.cpp
    data/BindingEngine.cpp
    data/PathValidator.cpp
    data/DynamicValueResolver.cpp
    data/ResolvedValue.cpp
    data/DataBinding.h
)

if(ENABLE_EXPRESSION_ENGINE)
    target_sources(a2ui_native PRIVATE
        expression/ExpressionEngine.cpp
        expression/Lexer.cpp
        expression/Parser.cpp
        expression/Evaluator.cpp
        expression/Sandbox.cpp
        expression/EvaluationContext.cpp
        expression/ExpressionFunctions.cpp
        expression/DependencyCollector.cpp
    )
    target_compile_definitions(a2ui_native PRIVATE ENABLE_EXPRESSION_ENGINE)
endif()

find_library(
    hilog-lib
    hilog_ndk.z
)

find_library(
    hitrace-lib
    hitrace_ndk.z
)

find_library(
    native_display_manager_lib
    native_display_manager
)


target_include_directories(a2ui_native PRIVATE
    ${NATIVERENDER_ROOT_PATH}
    ${NATIVERENDER_ROOT_PATH}/adapter
    ${NATIVERENDER_ROOT_PATH}/include
    ${cjson_SOURCE_DIR}
)

target_compile_definitions(
    a2ui_native PRIVATE
        A2UI_COMMIT_SHORT_ID="${A2UI_COMMIT_SHORT_ID}"
)

target_link_libraries(a2ui_native
    PUBLIC
        libace_napi.z.so
        libace_ndk.z.so
        ${hilog-lib}
        ${hitrace-lib}
        ${native_display_manager_lib}
    PRIVATE
        cjson
)

# Set C++17 standard for std::optional support
set_property(TARGET a2ui_native PROPERTY CXX_STANDARD 17)
set_property(TARGET a2ui_native PROPERTY CXX_STANDARD_REQUIRED ON)
