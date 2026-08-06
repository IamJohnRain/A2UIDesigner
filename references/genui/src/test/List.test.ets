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

import NativeFunctionsUnitTest from './NativeFunctions.test';
import CatalogUnitTest from './Catalog.test';
import ComponentThemeUnitTest from './ComponentTheme.test';
import CustomComponentFactoryCatalogRoutingUnitTest from './CustomComponentFactoryCatalogRouting.test';
import UIRendererComponentThemeModeUnitTest from './UIRendererComponentThemeMode.test';
import UIRendererComponentBreakpointUnitTest from './UIRendererComponentWindowBreakpoint.test';
import SurfaceControllerSchemaErrorBridgeUnitTest from './SurfaceControllerSchemaErrorBridge.test';
import SchemaWarningComponentValidationUnitTest from './SchemaWarningComponentValidation.test';
import ExpressionEngineUnitTest from './ExpressionEngine.test';
import ExpressionEngineWithContextUnitTest from './ExpressionEngineWithContext.test';
import CustomComponentDynamicValueResolverUnitTest from './CustomComponentDynamicValueResolver.test';

export default function testsuite() {
  NativeFunctionsUnitTest();
  CatalogUnitTest();
  ComponentThemeUnitTest();
  CustomComponentFactoryCatalogRoutingUnitTest();
  UIRendererComponentThemeModeUnitTest();
  UIRendererComponentBreakpointUnitTest();
  SurfaceControllerSchemaErrorBridgeUnitTest();
  SchemaWarningComponentValidationUnitTest();
  ExpressionEngineUnitTest();
  ExpressionEngineWithContextUnitTest();
  CustomComponentDynamicValueResolverUnitTest();
}
