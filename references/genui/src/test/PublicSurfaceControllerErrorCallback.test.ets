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
import { CatalogFactory, SurfaceControllerFactory } from '../main/ets/interface/Factories';
import { Catalog } from '../main/ets/interface/Catalog';
import { ClientFunction } from '../main/ets/interface/ClientFunction';
import { SurfaceController, SurfaceControllerOption } from '../main/ets/interface/SurfaceController';
import { SurfaceErrorCode } from '../main/ets/interface/Types';
import {
  NativeActionEventRequest,
  NativeEngineBridge,
  NativeProcessResult,
  NativeRuntimeErrorRequest,
  NativeSchemaWarningRequest,
  SurfaceNativeEngine
} from '../main/ets/core/base/NativeEngineBridge';
import { SurfaceControllerImpl } from '../main/ets/core/base/SurfaceControllerImpl';
import { FunctionBridge } from '../main/ets/core/functions/FunctionBridge';
import { LocalFunctionArgs, LocalFunctionContext } from '../main/ets/core/functions/LocalFunction';
import { SchemaErrorInfoManager } from '../main/ets/core/schema/SchemaErrorInfoManager';

interface NativeEngineBridgePatchTarget {
  getEngine: () => SurfaceNativeEngine | null;
  actionBridgeInstalled: boolean;
  warningBridgeInstalled: boolean;
  runtimeErrorBridgeInstalled: boolean;
  actionDispatcher: ((request: NativeActionEventRequest) => void) | undefined;
  warningDispatcher: ((request: NativeSchemaWarningRequest) => void) | undefined;
  runtimeErrorDispatcher: ((request: NativeRuntimeErrorRequest) => void) | undefined;
}

interface SurfaceControllerImplPatchTarget {
  MIN_SUPPORTED_API_VERSION: number;
  actionBridgeInstalled: boolean;
  schemaWarningBridgeInstalled: boolean;
  runtimeErrorBridgeInstalled: boolean;
  controllerByRenderId: Map<number, SurfaceController>;
  dispatchNativeAction: (request: NativeActionEventRequest) => void;
}

interface SchemaWarningPayloadItem {
  code: string;
  message: string;
  path?: string;
  itemType?: 'message' | 'component' | 'function' | 'schema';
  itemName?: string;
}

interface SchemaWarningPayload {
  type: string;
  warningCount: number;
  warnings: SchemaWarningPayloadItem[];
}

interface ErrorRecord {
  code: SurfaceErrorCode;
  message: string;
}

const DEFAULT_SUCCESS_RESULT: NativeProcessResult = {
  success: true,
  surfaceId: 'main',
  messageType: 0
};

const FORMAT_CURRENCY_SCHEMA_WITH_DYNAMIC_STRING_REF: string = `{
  "$defs": {
    "DynamicString": {
      "oneOf": [
        {
          "type": "string"
        },
        {
          "type": "object",
          "properties": {
            "path": {
              "type": "string"
            }
          },
          "required": [
            "path"
          ],
          "additionalProperties": false
        }
      ]
    }
  },
  "type": "object",
  "properties": {
    "call": {
      "const": "publicFormatCurrencyWarning"
    },
    "args": {
      "type": "object",
      "properties": {
        "value": {
          "type": "number"
        },
        "currency": {
          "$ref": "#/$defs/DynamicString"
        }
      },
      "required": [
        "currency",
        "value"
      ]
    },
    "returnType": {
      "const": "string"
    }
  },
  "required": [
    "call",
    "args",
    "returnType"
  ]
}`;

function createSurfaceController(catalog: Catalog = CatalogFactory.basic()): SurfaceController {
  const option: SurfaceControllerOption = {
    uiContext: {} as UIContext,
    catalog: catalog,
    eventCallback: (_eventType, _controller) => {
    }
  };
  return SurfaceControllerFactory.createSurfaceController(option);
}

export default function PublicSurfaceControllerErrorCallbackUnitTest() {
  describe('PublicSurfaceControllerErrorCallbackTest', () => {
    const originalGetEngine: () => SurfaceNativeEngine | null = NativeEngineBridge.getEngine;
    const nativeBridgePatch = NativeEngineBridge as Object as NativeEngineBridgePatchTarget;
    const controllerPatch = SurfaceControllerImpl as Object as SurfaceControllerImplPatchTarget;
    const originalMinSupportedApiVersion: number = controllerPatch.MIN_SUPPORTED_API_VERSION;

    let processResultOverride: NativeProcessResult | undefined = undefined;
    let runtimeDispatchTemplate: NativeRuntimeErrorRequest | undefined = undefined;
    let warningDispatchTemplates: NativeSchemaWarningRequest[] = [];
    let runtimeErrorCallback: ((request: NativeRuntimeErrorRequest) => void) | undefined = undefined;
    let schemaWarningCallback: ((request: NativeSchemaWarningRequest) => void) | undefined = undefined;

    const mockEngine: SurfaceNativeEngine = {
      initRenderSlot: (_renderId: number): void => {
      },
      destroyRenderSlot: (_renderId: number): void => {
      },
      processMessage: (
        renderId: number,
        _dsl: string,
        _catalog: Object,
        _options: Object
      ): NativeProcessResult => {
        if (runtimeDispatchTemplate !== undefined && runtimeErrorCallback !== undefined) {
          runtimeErrorCallback({
            renderId: renderId,
            surfaceId: runtimeDispatchTemplate.surfaceId,
            componentId: runtimeDispatchTemplate.componentId,
            errorCode: runtimeDispatchTemplate.errorCode,
            errorMessage: runtimeDispatchTemplate.errorMessage,
            source: runtimeDispatchTemplate.source
          });
        }
        if (warningDispatchTemplates.length > 0 && schemaWarningCallback !== undefined) {
          warningDispatchTemplates.forEach((template: NativeSchemaWarningRequest): void => {
            schemaWarningCallback({
              renderId: renderId,
              surfaceId: template.surfaceId,
              componentId: template.componentId,
              code: template.code,
              message: template.message,
              path: template.path,
              itemType: template.itemType,
              itemName: template.itemName
            });
          });
        }
        if (processResultOverride !== undefined) {
          return processResultOverride;
        }
        return DEFAULT_SUCCESS_RESULT;
      },
      bindSurfaceToRender: (_renderId: number, _content: Object): void => {
      },
      unbindSurfaceFromRender: (_renderId: number): void => {
      },
      popSurface: (_renderId: number): Object => {
        return {
          success: false,
          code: SurfaceErrorCode.MULTI_SURFACE_EMPTY_STACK,
          message: 'surface stack is empty'
        } as Object;
      },
      getSurfaceIds: (_renderId: number): string[] => {
        return [];
      },
      getLatestSurfaceId: (_renderId: number): string | undefined => {
        return undefined;
      },
      registerDispatchAction: (_callback: Object): void => {
      },
      setRootFillMode: (_renderId: number, _forceFill: boolean): void => {
      },
      dispatchCustomComponentAction: (
        _renderId: number,
        _surfaceId: string,
        _componentId: string,
        _eventName: string,
        _contextJson: string
      ): void => {
      },
      setFontSizeScale: (_renderId: number, _scale: number): void => {
      },
      setApiVersion: (_renderId: number, _apiVersion: number): void => {
      },
      registerCrossLanguageAttributeCallback: (_callback: Object): void => {
      },
      updateThemeMode: (_renderId: number, _mode: number): void => {
      },
      updateBreakpoint: (_renderId: number, _breakpoint: number): void => {
      },
      registerDispatchSchemaWarning: (callback: Object): void => {
        schemaWarningCallback = callback as (request: NativeSchemaWarningRequest) => void;
      },
      registerDispatchRuntimeError: (callback: Object): void => {
        runtimeErrorCallback = callback as (request: NativeRuntimeErrorRequest) => void;
      }
    };

    const resetBridgeState = (): void => {
      nativeBridgePatch.actionBridgeInstalled = false;
      nativeBridgePatch.warningBridgeInstalled = false;
      nativeBridgePatch.runtimeErrorBridgeInstalled = false;
      nativeBridgePatch.actionDispatcher = undefined;
      nativeBridgePatch.warningDispatcher = undefined;
      nativeBridgePatch.runtimeErrorDispatcher = undefined;
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

    beforeEach(() => {
      processResultOverride = undefined;
      runtimeDispatchTemplate = undefined;
      warningDispatchTemplates = [];
      runtimeErrorCallback = undefined;
      schemaWarningCallback = undefined;
      controllerPatch.MIN_SUPPORTED_API_VERSION = 0;
      resetBridgeState();
      SchemaErrorInfoManager.getInstance().clear();
      nativeBridgePatch.getEngine = (): SurfaceNativeEngine | null => {
        return mockEngine;
      };
    });

    afterEach(() => {
      controllerPatch.MIN_SUPPORTED_API_VERSION = originalMinSupportedApiVersion;
      nativeBridgePatch.getEngine = originalGetEngine;
      resetBridgeState();
      SchemaErrorInfoManager.getInstance().clear();
    });

    it('should_report_error_via_registered_callback_when_action_receiver_is_missing', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      let receivedErrorType: SurfaceErrorCode = SurfaceErrorCode.NO_ERROR;
      let receivedErrorMessage: string = '';

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        receivedErrorType = errorType;
        receivedErrorMessage = errorMessage;
      });

      controllerPatch.dispatchNativeAction({
        renderId: (controller as SurfaceControllerImpl).getRenderId(),
        surfaceId: 'main',
        sourceComponentId: 'button1',
        name: 'tap'
      });

      expect(receivedErrorType).assertEqual(SurfaceErrorCode.ACTION_NOT_REGISTER);
      expect(receivedErrorMessage.indexOf('onAction callback is not registered') >= 0).assertTrue();
      expect(receivedErrorMessage.indexOf('componentId=button1') >= 0).assertTrue();
      controller.destroy();
    });

    it('should_forward_native_runtime_error_to_public_error_callback', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      const errors: ErrorRecord[] = [];
      runtimeDispatchTemplate = {
        renderId: -1,
        surfaceId: 'main',
        componentId: 'title',
        errorCode: SurfaceErrorCode.SCHEMA_WARNING,
        errorMessage: 'native runtime error from parser',
        source: 'parser'
      };

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[0].message.indexOf('native runtime error from parser') >= 0).assertTrue();
      expect(errors[0].message.indexOf('componentId=title') >= 0).assertTrue();
      controller.destroy();
    });

    it('should_forward_local_function_error_to_public_error_callback', 0, () => {
      const functionItem: ClientFunction = {
        name: 'publicLocalCode5000',
        schemaProvider: (_version: string): string => '',
        functionCall: (_params) => {
          return undefined;
        }
      };
      const catalog: Catalog = CatalogFactory.createCatalog('', [], []);
      catalog.addClientFunction(functionItem);
      const controller: SurfaceController = createSurfaceController(catalog);
      const errors: ErrorRecord[] = [];

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      FunctionBridge.register('publicLocalCode5000', (_args: LocalFunctionArgs, context: LocalFunctionContext) => {
        context.onError('public local function failed');
        return undefined;
      });
      FunctionBridge.invokeLocalFunction({
        surfaceId: 'main',
        componentId: 'title',
        functionName: 'publicLocalCode5000',
        args: {},
        returnType: 'string'
      });

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.LOCAL_FUNCTION);
      expect(errors[0].message.indexOf('public local function failed') >= 0).assertTrue();
      expect(errors[0].message.indexOf('componentId=title') >= 0).assertTrue();
      controller.destroy();
    });

    it('should_emit_schema_warning_payload_via_public_error_callback', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      const errors: ErrorRecord[] = [];
      warningDispatchTemplates = [{
        renderId: -1,
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property action is required',
        path: 'btn.action',
        itemType: 'component',
        itemName: 'Button'
      }];

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload: SchemaWarningPayload | undefined = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.type).assertEqual('schemaWarning');
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].code).assertEqual('ERROR_CODE_REQUIRED_MISS');
      expect(payload!.warnings[0].path).assertEqual('btn.action');
      expect(payload!.warnings[0].itemName).assertEqual('Button');
      controller.destroy();
    });

    it('should_report_schema_warning_before_native_process_failure_when_both_exist', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      const errors: ErrorRecord[] = [];
      warningDispatchTemplates = [{
        renderId: -1,
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property action is required',
        path: 'btn.action',
        itemType: 'component',
        itemName: 'Button'
      }];
      processResultOverride = {
        success: false,
        errorCode: 'NATIVE_PROCESS_FAILED',
        errorMessage: 'native process failed'
      };

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      expect(errors.length).assertEqual(2);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      expect(errors[1].code).assertEqual(SurfaceErrorCode.NATIVE_PROCESS_FAILED);
      controller.destroy();
    });

    it('should_forward_local_function_schema_warning_to_public_error_callback', 0, () => {
      const functionItem: ClientFunction = {
        name: 'publicFormatCurrencyWarning',
        schemaProvider: (_version: string): string => FORMAT_CURRENCY_SCHEMA_WITH_DYNAMIC_STRING_REF,
        functionCall: (_params) => {
          return 'CNY 88';
        }
      };
      const catalog: Catalog = CatalogFactory.createCatalog('', [], []);
      catalog.addClientFunction(functionItem);
      const controller: SurfaceController = createSurfaceController(catalog);
      const errors: ErrorRecord[] = [];

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      FunctionBridge.invokeLocalFunction({
        surfaceId: 'main',
        componentId: 'price',
        functionName: 'publicFormatCurrencyWarning',
        args: {
          value: 88
        },
        returnType: 'number'
      });

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload: SchemaWarningPayload | undefined = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].path).assertEqual('function.publicFormatCurrencyWarning.args.currency');
      expect(payload!.warnings[0].itemType).assertEqual('function');
      controller.destroy();
    });

    it('should_forward_schema_warning_for_function_added_after_controller_creation', 0, () => {
      const catalog: Catalog = CatalogFactory.createCatalog('', [], []);
      const controller: SurfaceController = createSurfaceController(catalog);
      const errors: ErrorRecord[] = [];

      catalog.addClientFunction({
        name: 'publicLateFormatCurrencyWarning',
        schemaProvider: (_version: string): string =>
          FORMAT_CURRENCY_SCHEMA_WITH_DYNAMIC_STRING_REF.replace(
            'publicFormatCurrencyWarning',
            'publicLateFormatCurrencyWarning'
          ),
        functionCall: (_params) => {
          return 'CNY 88';
        }
      });

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      FunctionBridge.invokeLocalFunction({
        surfaceId: 'main',
        componentId: 'price',
        functionName: 'publicLateFormatCurrencyWarning',
        args: {
          value: 88
        },
        returnType: 'string'
      });

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload: SchemaWarningPayload | undefined = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.warningCount).assertEqual(1);
      expect(payload!.warnings[0].path).assertEqual('function.publicLateFormatCurrencyWarning.args.currency');
      controller.destroy();
    });

    it('should_drop_invalid_schema_warning_item_type_from_public_payload', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      const errors: ErrorRecord[] = [];
      warningDispatchTemplates = [{
        renderId: -1,
        surfaceId: 'main',
        componentId: 'btn',
        code: 'ERROR_CODE_REQUIRED_MISS',
        message: 'Property action is required',
        path: 'btn.action',
        itemType: 'invalid' as 'message' | 'component' | 'function' | 'schema',
        itemName: 'Button'
      }];

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_WARNING);
      const payload: SchemaWarningPayload | undefined = parseSchemaWarningPayload(errors[0].message);
      expect(payload !== undefined).assertTrue();
      expect(payload!.warnings[0].itemType === undefined).assertTrue();
      controller.destroy();
    });

    it('should_map_catalog_id_missing_failure_to_public_error_code', 0, () => {
      const controller: SurfaceController = createSurfaceController();
      const errors: ErrorRecord[] = [];
      processResultOverride = {
        success: false,
        errorCode: 'CATALOG_ID_MISSING',
        errorMessage: 'createSurface.catalogId is invalid',
        surfaceResultCode: SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING
      };

      controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
        errors.push({
          code: errorType,
          message: errorMessage
        });
      });
      controller.onReceive('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

      expect(errors.length).assertEqual(1);
      expect(errors[0].code).assertEqual(SurfaceErrorCode.SCHEMA_CATALOG_ID_MISSING);
      expect(errors[0].message).assertEqual('createSurface.catalogId is invalid');
      controller.destroy();
    });

    it('should_forward_surface_policy_error_code_to_public_error_callback', 0, () => {
      const scenarios: ErrorRecord[] = [{
        code: SurfaceErrorCode.MULTI_SURFACE_DISABLED,
        message: 'SurfaceController only supports one surface. Active surface: main'
      }, {
        code: SurfaceErrorCode.MULTI_SURFACE_MAX_SURFACE_LIMIT_REACHED,
        message: 'SurfaceController supports at most 15 surfaces.'
      }, {
        code: SurfaceErrorCode.MULTI_SURFACE_ALREADY_EXISTS,
        message: 'Surface already exists: main'
      }];

      scenarios.forEach((scenario: ErrorRecord): void => {
        const controller: SurfaceController = createSurfaceController();
        const errors: ErrorRecord[] = [];
        processResultOverride = {
          success: false,
          errorCode: `SURFACE_POLICY_${scenario.code}`,
          errorMessage: scenario.message,
          surfaceResultCode: scenario.code
        };

        controller.registerErrorCallback((errorType: SurfaceErrorCode, errorMessage: string): void => {
          errors.push({
            code: errorType,
            message: errorMessage
          });
        });
        controller.handleMessage('{"version":"v0.9","createSurface":{"surfaceId":"main"}}');

        expect(errors.length).assertEqual(1);
        expect(errors[0].code).assertEqual(scenario.code);
        expect(errors[0].message).assertEqual(scenario.message);
        controller.destroy();
      });
    });
  });
}
