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

import { afterEach, beforeEach, describe, expect, it } from '@ohos/hypium';
import { UIContext } from '@kit.ArkUI';
import { A2UIMessageType } from '../main/ets/core/base/A2UIMessage';
import {
  NativeProcessResult,
  NativeRuntimeErrorRequest,
  NativeSchemaWarningRequest,
  NativeEngineBridge,
  SurfaceNativeEngine
} from '../main/ets/core/base/NativeEngineBridge';
import { BreakpointUtils } from '../main/ets/core/common/BreakpointUtils';
import {
  Breakpoint,
  SchemaWarningInfo,
  SurfaceControllerImpl as SurfaceController,
  SurfaceErrorCode,
  SurfaceEventType,
} from '../main/ets/core/base/SurfaceControllerImpl';
import { CatalogFactory } from '../main/ets/interface/Factories';
import { Catalog } from '../main/ets/interface/Catalog';
import { SurfaceController as PublicSurfaceController } from '../main/ets/interface/SurfaceController';
import { ExpressionEvaluateResult } from '../main/ets/core/expression/ExpressionEngine';
import { SchemaErrorInfoManager } from '../main/ets/core/schema/SchemaErrorInfoManager';

interface NativeEngineBridgePatchTarget {
  getEngine: () => SurfaceNativeEngine | null;
  actionBridgeInstalled: boolean;
  warningBridgeInstalled: boolean;
  runtimeErrorBridgeInstalled: boolean;
  actionDispatcher: ((request: Object) => void) | undefined;
  warningDispatcher: ((request: NativeSchemaWarningRequest) => void) | undefined;
  runtimeErrorDispatcher: ((request: NativeRuntimeErrorRequest) => void) | undefined;
}

interface SurfaceControllerPatchTarget {
  actionBridgeInstalled: boolean;
  schemaWarningBridgeInstalled: boolean;
  runtimeErrorBridgeInstalled: boolean;
  controllerByRenderId: Map<number, SurfaceController>;
  installActionBridgeIfNeeded: () => void;
  installSchemaWarningBridgeIfNeeded: () => void;
  installRuntimeErrorBridgeIfNeeded: () => void;
}

interface SurfaceControllerInstancePatchTarget {
  apiSupported: boolean;
}

interface SurfaceControllerNativeWarningFlushTarget {
  flushPendingNativeSchemaWarnings: () => void;
}

interface SurfaceControllerReportedWarningFlushTarget {
  flushPendingReportedSchemaWarnings: () => void;
}

interface ErrorRecord {
  code: SurfaceErrorCode;
  message: string;
}

interface ControllerErrorCollector {
  controller: SurfaceController;
  errors: ErrorRecord[];
}

interface SchemaWarningPayloadItem {
  code: string;
  message: string;
  path?: string;
  itemType?: string;
  itemName?: string;
}

interface SchemaWarningPayload {
  type: string;
  warningCount: number;
  warnings: SchemaWarningPayloadItem[];
}

interface RuntimeDispatchData {
  surfaceId: string;
  componentId: string;
  errorCode: number;
  errorMessage: string;
}

interface WarningDispatchData {
  surfaceId: string;
  componentId: string;
  code: string;
  message: string;
  path?: string;
  itemType?: string;
  itemName?: string;
}

type DispatchMode = 'none' | 'runtime' | 'warning';

interface MockPopSurfaceResult {
  success: boolean;
  code: number;
  message: string;
}

interface BreakpointUpdateRecord {
  renderId: number;
  breakpoint: number;
}

const EMPTY_PROCESS_RESULT: NativeProcessResult = {
  success: true,
  surfaceId: 'main',
  messageType: A2UIMessageType.UPDATE_COMPONENTS
};

const RUNTIME_ERROR_CODE: number = SurfaceErrorCode.SCHEMA_WARNING;
const EMPTY_UI_CONTEXT: UIContext = {} as UIContext;

function createTestSurfaceController(catalog: Catalog = CatalogFactory.basic(), checkSchema: boolean = true): SurfaceController {
  const controller: SurfaceController = new SurfaceController({
    uiContext: EMPTY_UI_CONTEXT,
    catalog: catalog,
    eventCallback: (_eventType: SurfaceEventType, _controller: PublicSurfaceController): void => {
    }
  });
  controller.enableSchemaWarningReport(checkSchema);
  return controller;
}

export default function SurfaceControllerSchemaErrorBridgeUnitTest() {
  describe('SurfaceControllerSchemaErrorBridgeTest', () => {
    let originalGetEngine: () => SurfaceNativeEngine | null = NativeEngineBridge.getEngine;
    let dispatchMode: DispatchMode = 'none';
    let runtimeDispatchData: RuntimeDispatchData | undefined = undefined;
    let warningDispatchData: WarningDispatchData | undefined = undefined;
    let runtimeErrorCallback: ((request: NativeRuntimeErrorRequest) => void) | undefined = undefined;
    let schemaWarningCallback: ((request: NativeSchemaWarningRequest) => void) | undefined = undefined;
    let breakpointUpdates: BreakpointUpdateRecord[] = [];
    let processResultOverride: NativeProcessResult | undefined = undefined;
    let lastProcessedDsl: string = '';

    const mockEngine: SurfaceNativeEngine = {
      initRenderSlot: (_renderId: number): void => {
      },
      destroyRenderSlot: (_renderId: number): void => {
      },
      processMessage: (
        renderId: number,
        dsl: string,
        _catalog: Object,
        _options: Object
      ): NativeProcessResult => {
        lastProcessedDsl = dsl;
        if (dispatchMode === 'runtime' && runtimeDispatchData !== undefined && runtimeErrorCallback !== undefined) {
          runtimeErrorCallback({
            renderId: renderId,
            surfaceId: runtimeDispatchData.surfaceId,
            componentId: runtimeDispatchData.componentId,
            errorCode: runtimeDispatchData.errorCode,
            errorMessage: runtimeDispatchData.errorMessage
          });
        }
        if (dispatchMode === 'warning' && warningDispatchData !== undefined && schemaWarningCallback !== undefined) {
          const request: NativeSchemaWarningRequest = {
            renderId: renderId,
            surfaceId: warningDispatchData.surfaceId,
            componentId: warningDispatchData.componentId,
            code: warningDispatchData.code,
            message: warningDispatchData.message
          };
          if (warningDispatchData.path !== undefined) {
            request.path = warningDispatchData.path;
          }
          if (warningDispatchData.itemType !== undefined) {
            request.itemType = warningDispatchData.itemType as 'message' | 'component' | 'function' | 'schema';
          }
          if (warningDispatchData.itemName !== undefined) {
            request.itemName = warningDispatchData.itemName;
          }
          schemaWarningCallback(request);
        }
        if (processResultOverride !== undefined) {
          return processResultOverride;
        }
        return EMPTY_PROCESS_RESULT;
      },
      bindSurfaceToRender: (_renderId: number, _content: Object): void => {
      },
      unbindSurfaceFromRender: (_renderId: number): void => {
      },
      popSurface: (_renderId: number): Object => {
        const result: MockPopSurfaceResult = {
          success: false,
          code: 1003,
          message: 'surface stack is empty'
        };
        return result as Object;
      },
      getSurfaceIds: (_renderId: number): string[] => {
        return [];
      },
      getLatestSurfaceId: (_renderId: number): string => {
        return '';
      },
      registerDispatchAction: (_callback: Object): void => {
      },
      dispatchCustomComponentAction: (
        _renderId: number,
        _surfaceId: string,
        _componentId: string,
        _eventName: string,
        _contextJson: string
      ): void => {
      },
      registerDispatchRuntimeError: (callback: Object): void => {
        runtimeErrorCallback = callback as (request: NativeRuntimeErrorRequest) => void;
      },
      setRootFillMode: (_renderId: number, _forceFill: boolean): void => {
      },
      setFontSizeScale: (_renderId: number, _scale: number): void => {
      },
      setApiVersion: (_renderId: number, _apiVersion: number): void => {
      },
      registerCrossLanguageAttributeCallback: (_callback: Object): void => {
      },
      updateThemeMode: (_renderId: number, _mode: number): void => {
      },
      updateBreakpoint: (renderId: number, breakpoint: number): void => {
        breakpointUpdates.push({
          renderId: renderId,
          breakpoint: breakpoint
        });
      },
      registerDispatchSchemaWarning: (callback: Object): void => {
        schemaWarningCallback = callback as (request: NativeSchemaWarningRequest) => void;
      },
      evaluateExpression: (_expression: string): ExpressionEvaluateResult => {
        return {
          success: false,
          type: 'undefined'
        };
      }
    };

    const createControllerWithErrorCollector = (): ControllerErrorCollector => {
      const errors: ErrorRecord[] = [];
      const controller: SurfaceController = createTestSurfaceController();
      const controllerPatch = SurfaceController as Object as SurfaceControllerPatchTarget;
      (controller as Object as SurfaceControllerInstancePatchTarget).apiSupported = true;
      controllerPatch.installActionBridgeIfNeeded();
      controllerPatch.installSchemaWarningBridgeIfNeeded();
      controllerPatch.installRuntimeErrorBridgeIfNeeded();
      controllerPatch.controllerByRenderId.set(controller.getRenderId(), controller);
      controller.registerErrorCallback((code: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: code,
          message: errorMessage
        });
      });
      const collector: ControllerErrorCollector = {
        controller: controller,
        errors: errors
      };
      return collector;
    };

    const createControllerWithCatalogErrorCollector = (
      catalog: Catalog,
      checkSchema: boolean = true
    ): ControllerErrorCollector => {
      const errors: ErrorRecord[] = [];
      const controller: SurfaceController = createTestSurfaceController(catalog, checkSchema);
      const controllerPatch = SurfaceController as Object as SurfaceControllerPatchTarget;
      (controller as Object as SurfaceControllerInstancePatchTarget).apiSupported = true;
      controllerPatch.installActionBridgeIfNeeded();
      controllerPatch.installSchemaWarningBridgeIfNeeded();
      controllerPatch.installRuntimeErrorBridgeIfNeeded();
      controllerPatch.controllerByRenderId.set(controller.getRenderId(), controller);
      controller.registerErrorCallback((code: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: code,
          message: errorMessage
        });
      });
      return {
        controller: controller,
        errors: errors
      };
    };

    const resetStaticBridgeState = (): void => {
      const nativeBridgePatch = NativeEngineBridge as Object as NativeEngineBridgePatchTarget;
      nativeBridgePatch.actionBridgeInstalled = false;
      nativeBridgePatch.warningBridgeInstalled = false;
      nativeBridgePatch.runtimeErrorBridgeInstalled = false;
      nativeBridgePatch.actionDispatcher = undefined;
      nativeBridgePatch.warningDispatcher = undefined;
      nativeBridgePatch.runtimeErrorDispatcher = undefined;

      const controllerPatch = SurfaceController as Object as SurfaceControllerPatchTarget;
      controllerPatch.actionBridgeInstalled = false;
      controllerPatch.schemaWarningBridgeInstalled = false;
      controllerPatch.runtimeErrorBridgeInstalled = false;
      controllerPatch.controllerByRenderId = new Map<number, SurfaceController>();
    };

    const parseSchemaWarningPayload = (message: string): SchemaWarningPayload | undefined => {
      try {
        return JSON.parse(message) as SchemaWarningPayload;
      } catch (_error) {
        return undefined;
      }
    };

    const flushPendingNativeSchemaWarnings = (controller: SurfaceController): void => {
      (controller as Object as SurfaceControllerNativeWarningFlushTarget).flushPendingNativeSchemaWarnings();
    };

    const flushPendingReportedSchemaWarnings = (controller: SurfaceController): void => {
      (controller as Object as SurfaceControllerReportedWarningFlushTarget).flushPendingReportedSchemaWarnings();
    };

    beforeEach(() => {
      dispatchMode = 'none';
      runtimeDispatchData = undefined;
      warningDispatchData = undefined;
      runtimeErrorCallback = undefined;
      schemaWarningCallback = undefined;
      breakpointUpdates = [];
      processResultOverride = undefined;
      lastProcessedDsl = '';
      resetStaticBridgeState();
      SchemaErrorInfoManager.getInstance().clear();
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).getEngine = (): SurfaceNativeEngine | null => {
        return mockEngine;
      };
    });

    afterEach(() => {
      (NativeEngineBridge as Object as NativeEngineBridgePatchTarget).getEngine = originalGetEngine;
      resetStaticBridgeState();
      SchemaErrorInfoManager.getInstance().clear();
    });

    it('should_report_custom_extension_schema_warning_with_schema_warning_info', 0, () => {
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const customSchemaResult: string =
        '{"type":"customSchemaWarning","component":"CustomCard","warnings":[' +
        '{"code":"ERROR_CODE_TYPE_MISMATCH","message":"Property props.title expects string",' +
        '"path":"customCard.props.title"}]}';
      const schemaWarningInfo: SchemaWarningInfo = {
        code: SurfaceErrorCode.SCHEMA_WARNING,
        errorMsg: customSchemaResult
      };

      controller.reportSchemaWarning(schemaWarningInfo);

      expect(errors.length).assertEqual(0);
      flushPendingReportedSchemaWarnings(controller);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message).assertEqual(customSchemaResult);
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
      controller.destroy();
    });

    it('should_not_report_custom_extension_schema_warning_when_schema_check_disabled', 0, () => {
      const collector: ControllerErrorCollector = createControllerWithCatalogErrorCollector(CatalogFactory.basic(), false);
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const customSchemaResult: string = '{"type":"customSchemaWarning","component":"CustomCard"}';
      const schemaWarningInfo: SchemaWarningInfo = {
        code: SurfaceErrorCode.SCHEMA_WARNING,
        errorMsg: customSchemaResult
      };

      controller.reportSchemaWarning(schemaWarningInfo);
      flushPendingReportedSchemaWarnings(controller);

      expect(errors.length).assertEqual(0);
      expect(SchemaErrorInfoManager.getInstance().getWarningCount()).assertEqual(0);
      controller.destroy();
    });

    it('should_report_runtime_error_when_children_is_empty_array', 0, () => {
      dispatchMode = 'runtime';
      runtimeDispatchData = {
        surfaceId: 'main',
        componentId: 'root',
        errorCode: RUNTIME_ERROR_CODE,
        errorMessage: 'Property children cannot be an empty array'
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main",' +
        '"components":[{"id":"root","component":"Column","children":[]}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.includes('children cannot be an empty array')).assertTrue();
      controller.destroy();
    });

    it('should_report_runtime_error_when_children_template_component_id_is_empty', 0, () => {
      dispatchMode = 'runtime';
      runtimeDispatchData = {
        surfaceId: 'main',
        componentId: 'root',
        errorCode: RUNTIME_ERROR_CODE,
        errorMessage: 'Property children.componentId is required for template object'
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"root","component":"Column","children":{"componentId":"","path":"/items"}}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.includes('children.componentId')).assertTrue();
      controller.destroy();
    });

    it('should_report_runtime_error_when_children_path_points_to_non_array_data', 0, () => {
      dispatchMode = 'runtime';
      runtimeDispatchData = {
        surfaceId: 'main',
        componentId: 'root',
        errorCode: RUNTIME_ERROR_CODE,
        errorMessage: "Property children.path expects array data source, got type 'object'"
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"root","component":"Column","children":{"componentId":"rowTemplate","path":"/items"}}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.includes('expects array data source')).assertTrue();
      controller.destroy();
    });

    it('should_report_runtime_error_when_textfield_validation_regexp_is_not_valid_regex', 0, () => {
      dispatchMode = 'runtime';
      runtimeDispatchData = {
        surfaceId: 'main',
        componentId: 'age',
        errorCode: RUNTIME_ERROR_CODE,
        errorMessage: "Property validationRegexp is invalid regex '[': error"
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"age","component":"TextField","validationRegexp":"[","value":"abc"}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.includes('validationRegexp')).assertTrue();
      controller.destroy();
    });

    it('should_report_runtime_error_when_textfield_validation_regexp_contains_syntax_error', 0, () => {
      dispatchMode = 'runtime';
      runtimeDispatchData = {
        surfaceId: 'main',
        componentId: 'age',
        errorCode: RUNTIME_ERROR_CODE,
        errorMessage: "Property validationRegexp is invalid regex '(abc': error"
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"age","component":"TextField","validationRegexp":"(abc","value":"abc"}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.includes('validationRegexp')).assertTrue();
      controller.destroy();
    });

    it('should_keep_schema_warning_payload_when_button_child_is_empty', 0, () => {
      dispatchMode = 'warning';
      warningDispatchData = {
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property child is required',
        path: 'btn.child',
        itemType: 'component',
        itemName: 'Button'
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"btn","component":"Button","child":"","action":{"event":{"name":"tap"}}}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.type).assertEqual('schemaWarning');
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].code).assertEqual('ERROR_CODE_REQUIRED_MISS');
      expect(payload!.warnings[0].path).assertEqual('btn.child');
      controller.destroy();
    });

    it('should_keep_schema_warning_payload_when_button_action_is_missing', 0, () => {
      dispatchMode = 'warning';
      warningDispatchData = {
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property action is required, fallback to default value',
        path: 'btn.action',
        itemType: 'component',
        itemName: 'Button'
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"btn","component":"Button","child":"textId"}]}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.type).assertEqual('schemaWarning');
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].code).assertEqual('ERROR_CODE_REQUIRED_MISS');
      expect(payload!.warnings[0].path).assertEqual('btn.action');
      controller.destroy();
    });

    it('should_emit_schema_warning_payload_when_native_warning_occurs_outside_message_receive', 0, () => {
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      expect(schemaWarningCallback !== undefined).assertTrue();

      schemaWarningCallback!({
        renderId: controller.getRenderId(),
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property call is required',
        path: 'function.missingCallRequired.call',
        itemType: 'function',
        itemName: 'unknown'
      });

      expect(errors.length).assertEqual(0);
      flushPendingNativeSchemaWarnings(controller);
      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.type).assertEqual('schemaWarning');
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].code).assertEqual('ERROR_CODE_REQUIRED_MISS');
      expect(payload!.warnings[0].path).assertEqual('function.missingCallRequired.call');
      expect(payload!.warnings[0].itemType).assertEqual('function');
      expect(payload!.warnings[0].itemName).assertEqual('unknown');
      expect(payload!.warnings[0].message).assertEqual('Property call is required');
      controller.destroy();
    });

    it('should_batch_native_schema_warnings_outside_message_receive', 0, () => {
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      expect(schemaWarningCallback !== undefined).assertTrue();

      schemaWarningCallback!({
        renderId: controller.getRenderId(),
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property call is required',
        path: 'function.missingCallRequired.call',
        itemType: 'function',
        itemName: 'unknown'
      });
      schemaWarningCallback!({
        renderId: controller.getRenderId(),
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property call is required',
        path: 'function.missingCallEmptyArgs.call',
        itemType: 'function',
        itemName: 'unknown'
      });
      schemaWarningCallback!({
        renderId: controller.getRenderId(),
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_TYPE_MISMATCH',
        message: 'Property call expects string value',
        path: 'function.callTypeMismatch.call',
        itemType: 'function',
        itemName: 'unknown'
      });

      expect(errors.length).assertEqual(0);
      flushPendingNativeSchemaWarnings(controller);
      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.type).assertEqual('schemaWarning');
      expect(payload!.warningCount).assertEqual(3);
      expect(payload!.warnings.length).assertEqual(3);
      expect(payload!.warnings[0].path).assertEqual('function.missingCallRequired.call');
      expect(payload!.warnings[1].path).assertEqual('function.missingCallEmptyArgs.call');
      expect(payload!.warnings[2].path).assertEqual('function.callTypeMismatch.call');
      expect(payload!.warnings[2].message).assertEqual('Property call expects string value');
      controller.destroy();
    });

    it('should_resolve_breakpoint_for_each_threshold_range', 0, () => {
      expect(BreakpointUtils.resolveBreakpoint(319.9)).assertEqual(Breakpoint.XS);
      expect(BreakpointUtils.resolveBreakpoint(320)).assertEqual(Breakpoint.SM);
      expect(BreakpointUtils.resolveBreakpoint(600)).assertEqual(Breakpoint.MD);
      expect(BreakpointUtils.resolveBreakpoint(840)).assertEqual(Breakpoint.LG);
      expect(BreakpointUtils.resolveBreakpoint(1440)).assertEqual(Breakpoint.XL);
    });

    it('should_skip_invalid_width_and_update_breakpoint_for_each_valid_range', 0, () => {
      const controller: SurfaceController = createTestSurfaceController();

      controller.updateBreakpointByWidth(NaN);
      controller.updateBreakpointByWidth(0);
      controller.updateBreakpointByWidth(-1);
      expect(breakpointUpdates.length).assertEqual(0);

      controller.updateBreakpointByWidth(200);
      controller.updateBreakpointByWidth(400);
      controller.updateBreakpointByWidth(700);
      controller.updateBreakpointByWidth(1000);
      controller.updateBreakpointByWidth(1600);

      expect(breakpointUpdates.length).assertEqual(5);
      expect(breakpointUpdates[0].breakpoint).assertEqual(Breakpoint.XS);
      expect(breakpointUpdates[1].breakpoint).assertEqual(Breakpoint.SM);
      expect(breakpointUpdates[2].breakpoint).assertEqual(Breakpoint.MD);
      expect(breakpointUpdates[3].breakpoint).assertEqual(Breakpoint.LG);
      expect(breakpointUpdates[4].breakpoint).assertEqual(Breakpoint.XL);
      controller.destroy();
    });

    it('should_map_catalog_id_missing_process_failure_to_dedicated_schema_error', 0, () => {
      processResultOverride = {
        success: false,
        errorCode: 'CATALOG_ID_MISSING',
        errorMessage: 'createSurface.catalogId is invalid',
        surfaceResultCode: SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING
      };
      const collector: ControllerErrorCollector = createControllerWithErrorCollector();
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","createSurface":{"surfaceId":"main"}}';

      controller.handleMessage(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING);
      expect(errors[0].message).assertEqual('createSurface.catalogId is invalid');
      expect(controller.getLastSurfaceErrorCode()).assertEqual(SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING);
      expect(controller.getLastSurfaceMessage()).assertEqual('createSurface.catalogId is invalid');
      controller.destroy();
    });

    it('should_keep_raw_dsl_when_extended_prevalidation_sees_unsupported_version', 0, () => {
      processResultOverride = {
        success: false,
        errorCode: 'UNSUPPORTED_PROTOCOL_VERSION',
        errorMessage: 'unsupported protocol version',
        surfaceResultCode: SurfaceErrorCode.UNSUPPORTED_PROTOCOL_VERSION
      };
      const collector: ControllerErrorCollector = createControllerWithCatalogErrorCollector(CatalogFactory.extended());
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v1.0","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"title","component":"Text","content":{},"styles":{"fontScaleMode":"unsupported"}}]}}';

      controller.handleMessage(dsl);

      expect(lastProcessedDsl).assertEqual(dsl);
      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.UNSUPPORTED_PROTOCOL_VERSION);
      expect(errors[0].message).assertEqual('unsupported protocol version');
      controller.destroy();
    });

    it('should_pass_raw_extended_text_dsl_to_native_and_emit_native_schema_warning_payload', 0, () => {
      dispatchMode = 'warning';
      warningDispatchData = {
        surfaceId: 'main',
        componentId: 'title',
        code: 'ERROR_CODE_INVALID_VALUE',
        message: "Property fontScaleMode got invalid enum value 'unsupported', fallback to default value",
        path: 'title.styles.fontScaleMode',
        itemType: 'component',
        itemName: 'Text'
      };
      const collector: ControllerErrorCollector = createControllerWithCatalogErrorCollector(CatalogFactory.extended());
      const controller: SurfaceController = collector.controller;
      const errors: ErrorRecord[] = collector.errors;
      const dsl = '{"version":"v0.9","updateComponents":{"surfaceId":"main","components":[' +
        '{"id":"title","component":"Text","content":{},"styles":{' +
        '"fontScaleMode":"unsupported","minFontScale":"bad"}}]}}';

      controller.handleMessage(dsl);

      expect(lastProcessedDsl).assertEqual(dsl);

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].path).assertEqual('title.styles.fontScaleMode');
      expect(payload!.warnings[0].code).assertEqual('ERROR_CODE_INVALID_VALUE');
      controller.destroy();
    });
  });
}
