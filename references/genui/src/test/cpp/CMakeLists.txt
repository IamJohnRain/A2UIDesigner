cmake_minimum_required(VERSION 3.14)
project(A2UI_Cpp_TDD)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_COVERAGE "Enable gcov/lcov coverage instrumentation" OFF)
option(ENABLE_EXPRESSION_ENGINE "Enable expression engine" ON)

if(MSVC)
    add_compile_definitions(_ALLOW_KEYWORD_MACROS)
endif()

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../main/cpp)
set(MOCK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/infra/mock)
set(STUBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stubs)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/suites)
set(INFRA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/infra)
set(CPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(TEST_INCLUDE_DIR ${INFRA_DIR}/include)

include(${NATIVERENDER_ROOT_PATH}/cmake/cjson.cmake)

if (ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        message(STATUS "Coverage instrumentation enabled")
    else ()
        message(FATAL_ERROR "ENABLE_COVERAGE is only supported with GCC or Clang")
    endif ()
endif ()



if(MSVC)
    add_compile_definitions(_ALLOW_KEYWORD_MACROS)
endif()

add_library(a2ui_lib STATIC
    ${MOCK_DIR}/mock_arkui_native_provider.cpp
    ${MOCK_DIR}/mock_napi_provider.cpp
    ${MOCK_DIR}/mock_arkui_oh_api_adapter.cpp
    ${MOCK_DIR}/mock_arkui_node_api_adapter.cpp
    ${STUBS_DIR}/trace.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/A2UIArkUITypeConverter.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/ArkUIConstraintSizeAdapter.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/ArkUINativeAPI.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/NapiBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/NativeEntry.cpp
    ${NATIVERENDER_ROOT_PATH}/NapiResourceManager.cpp
    ${NATIVERENDER_ROOT_PATH}/RenderSlot.cpp
    ${NATIVERENDER_ROOT_PATH}/RenderManager.cpp
    ${NATIVERENDER_ROOT_PATH}/SurfaceSlot.cpp
    ${NATIVERENDER_ROOT_PATH}/SurfaceSlotSchemaValidation.cpp
    ${NATIVERENDER_ROOT_PATH}/SurfaceManager.cpp
    ${NATIVERENDER_ROOT_PATH}/theme/ThemeBase.cpp
    ${NATIVERENDER_ROOT_PATH}/theme/ThemeManager.cpp
    ${NATIVERENDER_ROOT_PATH}/theme/ThemeFactory.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedCommonTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/JsonAdapter.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/NapiUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/ThemeColorUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/DisplayDensityUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/SystemProperties.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/RequiredStringPropertyUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/LocalVariableNameUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/components/ChildListSchemaValidationUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/data/DataModel.cpp
    ${NATIVERENDER_ROOT_PATH}/data/BindingEngine.cpp
    ${NATIVERENDER_ROOT_PATH}/data/PathValidator.cpp
    ${NATIVERENDER_ROOT_PATH}/data/ResolvedValue.cpp
    ${NATIVERENDER_ROOT_PATH}/data/DynamicValueResolver.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/FunctionResult.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/FunctionCallInfo.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeFunctionBase.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeFunctionRegistry.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeRequiredFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeRegexFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeLengthFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeNumericFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeEmailFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeFormatStringFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeFormatNumberFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeFormatCurrencyFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeFormatDateFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativePluralizeFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeAndFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeOrFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/impl/NativeNotFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeGetToggleValueFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeGetRadioValueFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeGetCheckboxGroupValuesFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/NativeGetSelectValueFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/extended/NativeNavigateFunction.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/ActionInfo.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/ActionParser.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/ActionDispatchBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/CrossLanguageAttributeBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/RuntimeErrorDispatchBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/EventContextResolver.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/WarningDispatchBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/functions/FunctionBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/ChildListParser.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/TemplateInstantiator.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/EagerChildrenStrategy.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/TemplateAdapterNode.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/ListAdapterNode.cpp
    ${NATIVERENDER_ROOT_PATH}/composition/GridAdapterNode.cpp
    ${NATIVERENDER_ROOT_PATH}/components/Component.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/A2UIComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/button/ButtonComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/button/ButtonTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/text/TextComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/text/TextTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/column/ColumnComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/column/ColumnTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/row/RowComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/row/RowTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/textfield/TextFieldComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/textfield/TextFieldTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/card/CardComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/card/CardTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/checkbox/CheckboxComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/checkbox/CheckboxTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/checkbox/CheckboxGroupTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/image/ImageComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/image/ImageTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/list/ListComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/list/ListTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/slider/SliderComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/slider/SliderTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/CustomComponentFactory.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/ExtendedTabsPrebuildHelper.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentAttributeValue.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentBindingLifecycle.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentCommonStyleValidation.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentExpressionBinding.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentPropsBuilder.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentRuntimeProperties.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentTabsValidation.cpp
    ${NATIVERENDER_ROOT_PATH}/components/custom/CustomComponentTabContentValidation.cpp
    ${NATIVERENDER_ROOT_PATH}/components/NativeComponentFactory.cpp
    # Concrete extended components stay under components/extended.
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedButtonComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedRadioComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedTextTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedDividerTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedProgressTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedGridTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedListTheme.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedTextComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedTextInputComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedToggleComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedColumnComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedGridComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedListComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedRowComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedDividerComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedImageComponent.cpp
     ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedProgressComponent.cpp
     ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedStackComponent.cpp
     ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedCheckboxComponent.cpp
     ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedCheckboxGroupComponent.cpp
     ${NATIVERENDER_ROOT_PATH}/components/extended/NavContainerComponent.cpp
    # Shared extended component infrastructure lives under components/extended.
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/if/IfComponent.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedComponentStyleValidation.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedComponentFactory.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedDescriptorNormalizer.cpp
    ${NATIVERENDER_ROOT_PATH}/components/actions/EventHandlerParser.cpp
    ${NATIVERENDER_ROOT_PATH}/components/actions/EventHandlerChainExecutor.cpp
    ${NATIVERENDER_ROOT_PATH}/components/actions/NativeActionRegistry.cpp
    ${NATIVERENDER_ROOT_PATH}/components/actions/BuiltInActions.cpp
    ${NATIVERENDER_ROOT_PATH}/components/actions/ActionDispatcher.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/ExtendedStyleResolver.cpp
    ${NATIVERENDER_ROOT_PATH}/components/extended/RenderContext.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleApplyUtils.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleApplyUtilsLayout.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleApplyUtilsText.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleApplyUtilsEffects.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleParser.cpp
    ${NATIVERENDER_ROOT_PATH}/styles/StyleResolver.cpp
    ${NATIVERENDER_ROOT_PATH}/catalog/CatalogItem.cpp
    ${NATIVERENDER_ROOT_PATH}/catalog/Catalog.cpp
    ${NATIVERENDER_ROOT_PATH}/checks/ChecksEngine.cpp
    ${NATIVERENDER_ROOT_PATH}/components/A2UI/modal/ModalCoordinator.cpp
)

if(ENABLE_EXPRESSION_ENGINE)
    target_sources(a2ui_lib PUBLIC
        ${NATIVERENDER_ROOT_PATH}/expression/ExpressionEngine.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/Lexer.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/Parser.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/Evaluator.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/Sandbox.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/EvaluationContext.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/ExpressionFunctions.cpp
        ${NATIVERENDER_ROOT_PATH}/expression/DependencyCollector.cpp
    )
    target_compile_definitions(a2ui_lib PUBLIC ENABLE_EXPRESSION_ENGINE)
endif()

target_include_directories(a2ui_lib PUBLIC
    ${STUBS_DIR}
    ${NATIVERENDER_ROOT_PATH}
    ${NATIVERENDER_ROOT_PATH}/include
    ${NATIVERENDER_ROOT_PATH}/adapter
    ${NATIVERENDER_ROOT_PATH}/functions
    ${MOCK_DIR}
    ${INFRA_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CPP_INCLUDE_DIR}
    ${TEST_INCLUDE_DIR}
    ${cjson_SOURCE_DIR}
)

target_compile_definitions(a2ui_lib PUBLIC TDD_BUILD)
if(MSVC)
    target_compile_options(a2ui_lib PUBLIC /EHsc)
endif()
target_link_libraries(a2ui_lib PUBLIC cjson)

find_package(Threads REQUIRED)

include(${NATIVERENDER_ROOT_PATH}/cmake/googletest.cmake)

function(FilterBrokenUcrtIncludePath targetName propertyName)
    if (NOT TARGET ${targetName})
        return()
    endif ()

    set(targetForPatch ${targetName})
    get_target_property(aliasedTarget ${targetName} ALIASED_TARGET)
    if (aliasedTarget AND NOT aliasedTarget STREQUAL "aliasedTarget-NOTFOUND")
        set(targetForPatch ${aliasedTarget})
    endif ()

    get_target_property(includeDirs ${targetForPatch} ${propertyName})
    if (NOT includeDirs OR includeDirs STREQUAL "includeDirs-NOTFOUND")
        return()
    endif ()

    set(filteredIncludeDirs ${includeDirs})
    list(FILTER filteredIncludeDirs EXCLUDE REGEX "^/ucrt64/include$")
    list(FILTER filteredIncludeDirs EXCLUDE REGEX "^[A-Za-z]:/msys64/ucrt64/include$")
    list(FILTER filteredIncludeDirs EXCLUDE REGEX "^[A-Za-z]:\\\\msys64\\\\ucrt64\\\\include$")

    set_target_properties(${targetForPatch} PROPERTIES
        ${propertyName} "${filteredIncludeDirs}"
    )
endfunction()

function(SanitizeGTestIncludePath targetName)
    FilterBrokenUcrtIncludePath(${targetName} INTERFACE_INCLUDE_DIRECTORIES)
    FilterBrokenUcrtIncludePath(${targetName} INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
endfunction()

SanitizeGTestIncludePath(GTest::gtest)
SanitizeGTestIncludePath(GTest::gtest_main)
include(GoogleTest)

enable_testing()

set(A2UI_TEST_LIBS
    a2ui_lib
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# ========== suites tests ==========
add_executable(UtilsTest
    ${TEST_DIR}/utils/A2UIArkUITypeConverterTest.cpp
    ${TEST_DIR}/utils/JsonAdapterTest.cpp
    ${TEST_DIR}/utils/StyleApplyUtilsExtendedTest.cpp
    ${TEST_DIR}/utils/StyleParserPropertyNameTddTest.cpp
    ${TEST_DIR}/utils/ChildListParserBranchTest.cpp
    ${TEST_DIR}/utils/LocalVariableSchemaValidationTddTest.cpp
    ${TEST_DIR}/utils/ArkUINativeAPITest.cpp
    ${TEST_DIR}/utils/MockNapiProviderTest.cpp
    ${TEST_DIR}/utils/ThemeColorUtilsTest.cpp
    ${TEST_DIR}/utils/DisplayDensityUtilsTest.cpp
    ${TEST_DIR}/utils/LayoutPolicyFallbackTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(UtilsTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    UtilsTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(UtilsTest)

add_executable(FrameWorkTest
    ${TEST_DIR}/framework/ChecksEngineTest.cpp
    ${TEST_DIR}/framework/DataModelTest.cpp
    ${TEST_DIR}/framework/DisplayDensityNativeEntryTest.cpp
    ${TEST_DIR}/framework/InitRenderSlotTest.cpp
    ${TEST_DIR}/framework/NativeEntryCoverageTest.cpp
    ${TEST_DIR}/framework/ProcessMessageTest.cpp
    ${TEST_DIR}/framework/RenderSlotCoverageTest.cpp
    ${TEST_DIR}/framework/SurfaceManagerCoverageTest.cpp
    ${TEST_DIR}/framework/BindingEngineDepthTddTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(FrameWorkTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    FrameWorkTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(FrameWorkTest)

add_executable(FunctionsTest
    ${TEST_DIR}/functions/NativeFunctionsTest.cpp
    ${TEST_DIR}/functions/NativeFunctionsCoverageTest.cpp
    ${TEST_DIR}/functions/FunctionBridgeActionCoverageTest.cpp
    ${TEST_DIR}/functions/NativeGetCheckboxGroupValuesFunctionTest.cpp
    ${TEST_DIR}/functions/NativeGetSelectValueFunctionTest.cpp
    ${TEST_DIR}/functions/NativeNavigateFunctionTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(FunctionsTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    FunctionsTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(FunctionsTest)

add_executable(ComponentsTest
    ${TEST_DIR}/components/ButtonTest.cpp
    ${TEST_DIR}/components/ComponentCoreTest.cpp
    ${TEST_DIR}/components/FunctionCallBindingTest.cpp
    ${TEST_DIR}/components/CustomComponentTest.cpp
    ${TEST_DIR}/components/CustomComponentFactoryTest.cpp
    ${TEST_DIR}/components/CustomComponentExpressionTddTest.cpp
    ${TEST_DIR}/components/ComponentAttachOrderTest.cpp
    ${TEST_DIR}/components/ExtendedDividerComponentCoverageTest.cpp
    ${TEST_DIR}/components/ExtendedGridComponentTddTest.cpp
    ${TEST_DIR}/components/ExtendedImageComponentTest.cpp
    ${TEST_DIR}/components/ExtendedProgressComponentTest.cpp
    ${TEST_DIR}/components/ExtendedStackComponentTddTest.cpp
    ${TEST_DIR}/components/ExtendedTextComponentCoverageTest.cpp
    ${TEST_DIR}/components/ExtendedThemeDefaultsTest.cpp
    ${TEST_DIR}/components/ExtendedCheckboxComponentTest.cpp
    ${TEST_DIR}/components/ExtendedCheckboxRuntimeStateCoverageTest.cpp
    ${TEST_DIR}/components/ExtendedCheckboxGroupComponentTest.cpp
    ${TEST_DIR}/components/EventHandlerComponentBranchTest.cpp
    ${TEST_DIR}/components/FontScaleModeTest.cpp
    ${TEST_DIR}/components/GridAdapterNodeBranchTest.cpp
    ${TEST_DIR}/components/LinearLayoutSpacingTest.cpp
    ${TEST_DIR}/components/ListAdapterNodeBranchTest.cpp
    ${TEST_DIR}/components/CheckboxComponentTddTest.cpp
    ${TEST_DIR}/components/ImageComponentTddTest.cpp
    ${TEST_DIR}/components/ListComponentTddTest.cpp
    ${TEST_DIR}/components/ModalCoordinatorTest.cpp
    ${TEST_DIR}/components/ModalCoordinatorTddTest.cpp
    ${TEST_DIR}/components/NativeComponentFactoryTest.cpp
    ${TEST_DIR}/components/NavContainerComponentTddTest.cpp
    ${TEST_DIR}/components/RowComponentTddTest.cpp
    ${TEST_DIR}/components/SchemaValidationHelpersTddTest.cpp
    ${TEST_DIR}/components/SliderComponentTddTest.cpp
    ${TEST_DIR}/components/SurfaceSlotComponentIntegrationTddTest.cpp
    ${TEST_DIR}/components/SurfaceSlotCustomComponentCoverageTest.cpp
    ${TEST_DIR}/components/SurfaceSlotCustomComponentInternalCoverageTest.cpp
    ${TEST_DIR}/components/SurfaceSlotResourceLimitTddTest.cpp
    ${TEST_DIR}/components/ThemeBrandColorTest.cpp
    ${TEST_DIR}/components/TextTest.cpp
    ${TEST_DIR}/components/DispatchActionInfoTest.cpp
    ${TEST_DIR}/components/ExtendedDescriptorNormalizerTddTest.cpp
    ${TEST_DIR}/components/ExtendedComponentDynamicStyleValidationTddTest.cpp
    ${TEST_DIR}/components/ExtendedStyleResolverTddTest.cpp
    ${TEST_DIR}/components/ExtendedStyleResolverDfxTest.cpp
    ${TEST_DIR}/components/ExtendedShadowThemeTddTest.cpp
    ${TEST_DIR}/components/ExtendedComponentCoreTddTest.cpp
    ${TEST_DIR}/components/ExtendedBranchCoverageTddTest.cpp
    ${TEST_DIR}/components/TemplateAdapterNodeBranchTest.cpp
    ${TEST_DIR}/components/TextFieldComponentTddTest.cpp
    ${TEST_DIR}/components/TextFieldComponentTest.cpp
    ${TEST_DIR}/components/TypeValidationTddTest.cpp
    ${INFRA_DIR}/msvc_private_access_aliases.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(ComponentsTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    ComponentsTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(ComponentsTest)

add_executable(ConstraintSizePercentTddTest
    ${TEST_DIR}/components/ConstraintSizePercentTddTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(ConstraintSizePercentTddTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(ConstraintSizePercentTddTest PRIVATE
    ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(ConstraintSizePercentTddTest)

add_executable(CustomComponentDescriptorTest
    ${TEST_DIR}/components/CustomComponentDescriptorPreservationTest.cpp
    ${INFRA_DIR}/msvc_private_access_aliases.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(CustomComponentDescriptorTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(CustomComponentDescriptorTest PRIVATE
    ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(CustomComponentDescriptorTest)

add_executable(ExtendedTextAliasTest
    ${TEST_DIR}/components/ExtendedTextAliasTest.cpp
    ${INFRA_DIR}/msvc_private_access_aliases.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(ExtendedTextAliasTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(ExtendedTextAliasTest PRIVATE
    ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(ExtendedTextAliasTest)

add_executable(ActionsTest
    ${TEST_DIR}/actions/EventHandlerParserTest.cpp
    ${TEST_DIR}/actions/EventHandlerChainExecutorTest.cpp
    ${TEST_DIR}/actions/NativeActionRegistryTest.cpp
    ${TEST_DIR}/actions/ActionsTest.cpp
    ${TEST_DIR}/actions/EventHandlerBranchCoverageTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(ActionsTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    ActionsTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
if(MSVC)
    target_compile_options(ActionsTest PRIVATE /EHsc)
endif()
gtest_discover_tests(ActionsTest)

# ========== Adapter branch-coverage test ==========
# Compiles the REAL adapter/ArkUINodeApiAdapter.cpp (not the mock) into its own
# executable that does NOT link a2ui_lib, avoiding duplicate-symbol collision
# with infra/mock/mock_arkui_node_api_adapter.cpp. TDD_BUILD routes
# GetNativeNodeAPI()/GetNativeDialogAPI() to the injectable ArkUINativeAPI provider.
add_executable(NodeApiAdapterBranchCoverageTest
    ${TEST_DIR}/adapters/ArkUINodeApiAdapterBranchCoverageTest.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/ArkUINodeApiAdapter.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/ArkUIConstraintSizeAdapter.cpp
    ${MOCK_DIR}/mock_arkui_oh_api_adapter.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/ArkUINativeAPI.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/A2UIArkUITypeConverter.cpp
    ${NATIVERENDER_ROOT_PATH}/adapter/NapiBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/utils/SystemProperties.cpp
    ${MOCK_DIR}/mock_arkui_native_provider.cpp
    ${MOCK_DIR}/mock_arkui_oh_api_adapter.cpp
    ${MOCK_DIR}/mock_napi_provider.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
    ${MOCK_DIR}/stub_impl.cpp
    ${INFRA_DIR}/msvc_private_access_aliases.cpp
)
target_include_directories(NodeApiAdapterBranchCoverageTest PRIVATE
    ${STUBS_DIR}
    ${NATIVERENDER_ROOT_PATH}
    ${NATIVERENDER_ROOT_PATH}/include
    ${NATIVERENDER_ROOT_PATH}/adapter
    ${MOCK_DIR}
    ${INFRA_DIR}
    ${CPP_INCLUDE_DIR}
    ${TEST_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${cjson_SOURCE_DIR})
target_compile_definitions(NodeApiAdapterBranchCoverageTest PRIVATE TDD_BUILD)
target_link_libraries(NodeApiAdapterBranchCoverageTest PRIVATE GTest::gtest GTest::gtest_main Threads::Threads cjson)
gtest_discover_tests(NodeApiAdapterBranchCoverageTest)

if(ENABLE_EXPRESSION_ENGINE)
add_executable(ExpressionTest
    ${TEST_DIR}/expression/ExpressionEvaluatorTest.cpp
    ${TEST_DIR}/expression/VariableResolutionTest.cpp
    ${TEST_DIR}/expression/IfComponentTest.cpp
    ${TEST_DIR}/expression/IfComponentIntegrationTest.cpp
    ${INFRA_DIR}/mock/stub_impl.cpp
)
target_link_libraries(ExpressionTest PRIVATE ${A2UI_TEST_LIBS})
target_include_directories(
    ExpressionTest PRIVATE ${TEST_INCLUDE_DIR} ${INFRA_DIR} ${MOCK_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CPP_INCLUDE_DIR})
gtest_discover_tests(ExpressionTest)
else()
add_test(NAME ExpressionTest_DISABLED COMMAND ${CMAKE_COMMAND} -E echo "ExpressionTest skipped: ENABLE_EXPRESSION_ENGINE=OFF")
set_tests_properties(ExpressionTest_DISABLED PROPERTIES DISABLED TRUE)
endif()

# Backward-compatible aliases for existing build commands.
add_custom_target(JsonAdapterTest DEPENDS UtilsTest)
add_custom_target(ArkUINativeAPITest DEPENDS UtilsTest)
add_custom_target(MockNapiProviderTest DEPENDS UtilsTest)
add_custom_target(DataModelTest DEPENDS FrameWorkTest)
add_custom_target(InitRenderSlotTest DEPENDS FrameWorkTest)
add_custom_target(NativeFunctionsTest DEPENDS FunctionsTest)
add_custom_target(ComponentTest DEPENDS ComponentsTest)
if(ENABLE_EXPRESSION_ENGINE)
add_custom_target(ExpressionEvaluatorTest DEPENDS ExpressionTest)
endif()

if (ENABLE_COVERAGE)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)
    find_package(Python3 COMPONENTS Interpreter QUIET)

    if (NOT LCOV_EXECUTABLE OR NOT GENHTML_EXECUTABLE)
        message(WARNING "lcov/genhtml not found, coverage target will not be generated")
    else ()
        set(COVERAGE_INFO_FILE ${CMAKE_BINARY_DIR}/coverage.info)
        set(COVERAGE_FILTERED_FILE ${CMAKE_BINARY_DIR}/coverage.filtered.info)
        set(COVERAGE_HTML_DIR ${CMAKE_BINARY_DIR}/coverage-html)
        set(COVERAGE_BRANCH_HTML_FILE ${COVERAGE_HTML_DIR}/branch-coverage.html)
        set(COVERAGE_BRANCH_JSON_FILE ${COVERAGE_HTML_DIR}/branch-coverage.json)
        set(BRANCH_COVERAGE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/tools/branch_coverage_report.py)
        # geninfo_no_exception_branch=1 drops compiler-generated exception/unwind arcs
        # (bad_alloc paths from std::string/std::set/std::unique_ptr) that no test can drive,
        # so branch coverage reflects reachable logic instead of unreachable unwind edges.
        set(COVERAGE_LCOV_RC_ARGS --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1)
        set(COVERAGE_GENHTML_ARGS --branch-coverage)

        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} ${COVERAGE_LCOV_RC_ARGS}
                    --directory ${CMAKE_BINARY_DIR} --zerocounters
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -j 4
            COMMAND ${LCOV_EXECUTABLE} ${COVERAGE_LCOV_RC_ARGS}
                    --capture --directory ${CMAKE_BINARY_DIR}
                    --output-file ${COVERAGE_INFO_FILE}
            COMMAND ${LCOV_EXECUTABLE} ${COVERAGE_LCOV_RC_ARGS}
                    --remove ${COVERAGE_INFO_FILE}
                    "/usr/*"
                    "/usr/include/*"
                    "*/x86_64-linux-gnu/*"
                    "/usr/lib/*"
                    "*/include/c++/*"
                    "*/gtest/*"
                    "*/third_party/*"
                    "*/src/test/*"
                    --output-file ${COVERAGE_FILTERED_FILE}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_HTML_DIR}
            COMMAND ${GENHTML_EXECUTABLE} ${COVERAGE_GENHTML_ARGS}
                    ${COVERAGE_FILTERED_FILE}
                    --output-directory ${COVERAGE_HTML_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ComponentsTest FrameWorkTest FunctionsTest UtilsTest
            ActionsTest NodeApiAdapterBranchCoverageTest
            $<IF:$<BOOL:${ENABLE_EXPRESSION_ENGINE}>,ExpressionTest,>
            USES_TERMINAL
            COMMENT "Build tests, run them, and generate an lcov HTML coverage report"
        )

        if (Python3_Interpreter_FOUND)
            add_custom_target(coverage_branch_web
                COMMAND ${Python3_EXECUTABLE} ${BRANCH_COVERAGE_SCRIPT}
                        --lcov ${COVERAGE_FILTERED_FILE}
                        --output ${COVERAGE_BRANCH_HTML_FILE}
                        --json-output ${COVERAGE_BRANCH_JSON_FILE}
                        --root ${NATIVERENDER_ROOT_PATH}
                        --title "A2UI C++ Branch Coverage Report"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS coverage
                USES_TERMINAL
                COMMENT "Generate branch coverage web report from filtered lcov data"
            )
        else ()
            message(WARNING "Python3 interpreter not found; coverage_branch_web target will not be generated")
        endif ()

        add_custom_target(coverage_summary
            COMMAND ${LCOV_EXECUTABLE} ${COVERAGE_LCOV_RC_ARGS}
                    --summary ${COVERAGE_FILTERED_FILE}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS coverage
            USES_TERMINAL
            COMMENT "Print the filtered lcov coverage summary"
        )
    endif ()
endif ()
