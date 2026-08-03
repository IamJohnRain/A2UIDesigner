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

import nativeEngine from 'liba2ui_native.so';
import { common } from '@kit.AbilityKit';
import { i18n } from '@kit.LocalizationKit';
import { CatalogImpl } from '../base/CatalogImpl';
import { CatalogItem } from '../base/CatalogItem';
import { A2UIFunctionCall } from '../types/FunctionCall';
import { SurfaceErrorCode } from '../base/SurfaceTypes';
import { SchemaErrorInfoManager } from '../schema/SchemaErrorInfoManager';
import { SchemaResourceLoader } from '../schema/SchemaResourceLoader';
import { SchemaFunctionValidationResult, SchemaValidator } from '../schema/SchemaValidator';
import { A2UIValueType, FunctionContext } from '../types/types';
import { SurfaceControllerImpl } from '../base/SurfaceControllerImpl';
import { InnerFunctionItem } from '../base/FunctionItem';
import { CustomComponentDynamicValueResolver } from '../types/runtime/CustomComponentDynamicValueResolver';
import { applyHostContextToBuiltinFunctions } from './A2UIBasicFunctions';
import {
  LocalFunctionArgs,
  LocalFunctionContext,
  LocalFunctionHandler,
  LocalFunctionRequest,
  LocalFunctionResponse,
  LocalFunctionResult
} from './LocalFunction';

export interface LocalFunctionErrorEvent {
  request: LocalFunctionRequest;
  errorCode: number;
  errorMessage: string;
}

export type LocalFunctionErrorListener = (event: LocalFunctionErrorEvent) => void;

export class FunctionBridge {
  private static installed: boolean = false;
  private static registeredFunctionOverrides: Map<string, InnerFunctionItem> = new Map();
  private static localFunctionErrorListeners: Set<LocalFunctionErrorListener> = new Set();
  private static hostContext: common.UIAbilityContext | undefined = undefined;

  public static install(): void {
    if (FunctionBridge.installed) {
      return;
    }
    let engineObject = nativeEngine as Object | undefined;
    if (engineObject !== undefined && engineObject !== null) {
      let engine = engineObject as Record<string, Object>;
      let registerInvokeLocalFunction = engine['registerInvokeLocalFunction'];
      if (typeof registerInvokeLocalFunction === 'function') {
        nativeEngine.registerInvokeLocalFunction(FunctionBridge.invokeLocalFunction);
      }
      let registerLocale = engine['registerLocale'];
      console.info(`[FunctionBridge] registerLocale typeof=${typeof registerLocale}`);
      if (typeof registerLocale === 'function') {
        try {
          let localeProvider = (): string => {
            try {
              return i18n.System.getSystemLocaleInstance().toString();
            } catch (e) {
              try {
                return i18n.System.getSystemLocale();
              } catch (e2) {
                return 'zh-Hans';
              }
            }
          };
          nativeEngine.registerLocale(localeProvider);
          console.info(`[FunctionBridge] registerLocale provider registered`);
        } catch (e) {
          console.warn(`[FunctionBridge] registerLocale provider failed: ${e}`);
        }
      } else {
        console.warn(`[FunctionBridge] registerLocale not found on native engine`);
      }
    }
    FunctionBridge.installed = true;
  }

  public static registerLocalFunctionErrorListener(listener: LocalFunctionErrorListener): void {
    FunctionBridge.localFunctionErrorListeners.add(listener);
  }

  public static unregisterLocalFunctionErrorListener(listener: LocalFunctionErrorListener): void {
    FunctionBridge.localFunctionErrorListeners.delete(listener);
  }

  public static register(name: string, handler: LocalFunctionHandler): void {
    if (name.length === 0) {
      return;
    }
    FunctionBridge.registeredFunctionOverrides.set(name, {
      name: name,
      isInnerNative: false,
      schemaProvider: (_version: string): string => '',
      functionCall: (args: A2UIValueType, context: FunctionContext): A2UIValueType => {
        const contextRecord: Record<string, Object> = context as Object as Record<string, Object>;
        const surfaceId: string =
          typeof contextRecord['surfaceId'] === 'string' ? contextRecord['surfaceId'] as string : '';
        const componentId: string =
          typeof contextRecord['componentId'] === 'string' ? contextRecord['componentId'] as string : '';
        const renderId: number | undefined =
          typeof contextRecord['renderId'] === 'number' ? contextRecord['renderId'] as number : undefined;
        const localContext: LocalFunctionContext = {
          surfaceId: surfaceId,
          componentId: componentId,
          renderId: renderId,
          returnType: typeof contextRecord['returnType'] === 'string' ? contextRecord['returnType'] as string : 'any',
          onError: context.onError,
          resolver: context.resolver ?? FunctionBridge.createDynamicValueResolver(surfaceId, componentId, renderId)
        };
        return handler(args as LocalFunctionArgs, localContext);
      }
    });
  }


  public static invoke(
    functionCall: A2UIFunctionCall,
    surfaceId: string = '',
    componentId: string = '',
    renderId?: number
  ): A2UIValueType {
    if (functionCall.call.length === 0) {
      return undefined;
    }
    let functionItem: InnerFunctionItem | undefined =
      FunctionBridge.findExecutableCatalogFunctionItem(renderId, functionCall.call);
    if (functionItem === undefined || functionItem.functionCall === undefined) {
      return undefined;
    }

    let args: LocalFunctionArgs = functionCall.args as LocalFunctionArgs;
    let returnType: string = functionCall.returnType ?? 'any';
    let context: LocalFunctionContext = {
      surfaceId: surfaceId,
      componentId: componentId,
      renderId: renderId,
      returnType: returnType,
      onError: (_errorMessage: string): void => {},
      resolver: FunctionBridge.createDynamicValueResolver(surfaceId, componentId, renderId)
    };
    try {
      let result: LocalFunctionResult = functionItem.functionCall(args, context);
      return FunctionBridge.validateReturnType(returnType, result) ? result : undefined;
    } catch (error) {
      console.warn(`[FunctionBridge] invoke failed: ${JSON.stringify(error)}`);
      return undefined;
    }
  }

  public static setHostContext(hostContext: common.UIAbilityContext): void {
    FunctionBridge.hostContext = hostContext;
    applyHostContextToBuiltinFunctions(hostContext);
    SchemaResourceLoader.setHostContext(hostContext);
  }

  public static invokeLocalFunction(request: LocalFunctionRequest): LocalFunctionResponse {
    const functionItem: InnerFunctionItem | undefined =
      FunctionBridge.findCatalogFunctionItem(request.renderId, request.functionName);
    const schemaErrorInfoManager = SchemaErrorInfoManager.getInstance();
    let schemaValidationResult: SchemaFunctionValidationResult;
    if (schemaErrorInfoManager.isCollectingWarningsForMessage()) {
      schemaValidationResult = FunctionBridge.validateAndNormalizeFunctionCall(request, functionItem);
    } else {
      const captureResult = schemaErrorInfoManager.captureWarnings(
        () => FunctionBridge.validateAndNormalizeFunctionCall(request, functionItem)
      );
      schemaValidationResult = captureResult.result;
      schemaErrorInfoManager.dispatchCapturedWarnings(request.surfaceId, captureResult.warnings);
    }
    if (!schemaValidationResult.success) {
      return FunctionBridge.fail(
        request.returnType,
        SurfaceErrorCode.LOCAL_FUNCTION,
        schemaValidationResult.errorMessage ?? `Function ${request.functionName} schema validation failed`
      );
    }

    let args: LocalFunctionArgs = request.args;
    args = schemaValidationResult.normalizedArgs;
    let normalizedReturnType: string = schemaValidationResult.normalizedReturnType;

    if (request.normalizeOnly === true) {
      const response: LocalFunctionResponse = {
        success: true,
        returnType: normalizedReturnType,
        normalizedArgs: args,
        normalizedReturnType: normalizedReturnType
      };
      return response;
    }

    if (functionItem === undefined || functionItem.isInnerNative || functionItem.functionCall === undefined) {
      const errorCode: number = SurfaceErrorCode.LOCAL_FUNCTION;
      const errorMessage = `Local function is not registered: ${request.functionName}`;
      FunctionBridge.notifyLocalFunctionError(request, errorCode, errorMessage);
      return FunctionBridge.fail(request.returnType, errorCode, errorMessage);
    }

    let reportedErrorCode: number = SurfaceErrorCode.NO_ERROR;
    let reportedErrorMessage: string = '';
    let hasReportedError: boolean = false;
    let context: LocalFunctionContext = {
      surfaceId: request.surfaceId,
      componentId: request.componentId,
      renderId: request.renderId,
      returnType: normalizedReturnType,
      onError: (errorMessage: string): void => {
        reportedErrorCode = SurfaceErrorCode.LOCAL_FUNCTION;
        reportedErrorMessage = FunctionBridge.normalizeApplicationErrorMessage(errorMessage);
        hasReportedError = true;
        FunctionBridge.notifyLocalFunctionError(request, reportedErrorCode, reportedErrorMessage);
      },
      resolver: FunctionBridge.createDynamicValueResolver(request.surfaceId, request.componentId, request.renderId)
    };

    try {
      let result: LocalFunctionResult = functionItem.functionCall(args, context);
      if (hasReportedError) {
        return FunctionBridge.fail(normalizedReturnType, reportedErrorCode, reportedErrorMessage);
      }
      if (!FunctionBridge.validateReturnType(normalizedReturnType, result)) {
        const errorCode: number = SurfaceErrorCode.LOCAL_FUNCTION;
        const errorMessage =
          `Function ${request.functionName} returned mismatched value for type ${normalizedReturnType}`;
        FunctionBridge.notifyLocalFunctionError(request, errorCode, errorMessage);
        return FunctionBridge.fail(normalizedReturnType, errorCode, errorMessage);
      }
      return {
        success: true,
        returnType: normalizedReturnType,
        value: result
      };
    } catch (error) {
      if (hasReportedError) {
        console.warn(
          `[FunctionBridge] function threw after context.onError reported, surfaceId=${request.surfaceId}, ` +
          `componentId=${request.componentId}, functionName=${request.functionName}`
        );
        return FunctionBridge.fail(normalizedReturnType, reportedErrorCode, reportedErrorMessage);
      }
      const errorCode: number = SurfaceErrorCode.LOCAL_FUNCTION;
      const errorMessage = `Function ${request.functionName} execution failed: ${JSON.stringify(error)}`;
      FunctionBridge.notifyLocalFunctionError(request, errorCode, errorMessage);
      return FunctionBridge.fail(normalizedReturnType, errorCode, errorMessage);
    }
  }

  private static normalizeApplicationErrorMessage(errorMessage: string): string {
    if (errorMessage.trim().length > 0) {
      return errorMessage.trim();
    }
    return 'local function reported error';
  }

  private static notifyLocalFunctionError(
    request: LocalFunctionRequest,
    errorCode: number,
    errorMessage: string
  ): void {
    console.error(
      `[FunctionBridge] local function error event, surfaceId=${request.surfaceId}, componentId=${request.componentId}, ` +
      `functionName=${request.functionName}, returnType=${request.returnType}, errorCode=${errorCode}, ` +
      `errorMessage=${errorMessage}`
    );
    if (FunctionBridge.localFunctionErrorListeners.size === 0) {
      console.warn(
        `[FunctionBridge] local function error has no listener, functionName=${request.functionName}, ` +
        `surfaceId=${request.surfaceId}, errorCode=${errorCode}`
      );
      return;
    }
    let event: LocalFunctionErrorEvent = {
      request: request,
      errorCode: errorCode,
      errorMessage: errorMessage
    };
    for (const listener of FunctionBridge.localFunctionErrorListeners.values()) {
      try {
        listener(event);
      } catch (error) {
        console.warn(`[FunctionBridge] local function error listener failed: ${JSON.stringify(error)}`);
      }
    }
  }

  private static findCatalogFunctionItem(
    renderId: number | undefined,
    functionName: string
  ): InnerFunctionItem | undefined {
    const registeredOverride: InnerFunctionItem | undefined =
      FunctionBridge.registeredFunctionOverrides.get(functionName);
    if (registeredOverride !== undefined) {
      return registeredOverride;
    }
    const resolvedRenderId: number | undefined = FunctionBridge.normalizeRenderId(renderId);
    if (resolvedRenderId === undefined) {
      return undefined;
    }
    const controller: SurfaceControllerImpl | undefined =
      SurfaceControllerImpl.getControllerByRenderId(resolvedRenderId);
    const catalog = controller?.getCatalog();
    if (catalog === undefined) {
      return undefined;
    }
    return (catalog as CatalogImpl)?.functions.find(
      (catalogItem: InnerFunctionItem) => catalogItem.name === functionName
    );
  }

  private static createDynamicValueResolver(
    surfaceId: string,
    componentId: string,
    renderId?: number
  ): CustomComponentDynamicValueResolver {
    const normalizedRenderId: number | undefined = FunctionBridge.normalizeRenderId(renderId);
    return new CustomComponentDynamicValueResolver({
      surfaceId: surfaceId,
      componentId: componentId,
      renderId: normalizedRenderId,
      catalogId: FunctionBridge.resolveCatalogId(normalizedRenderId),
      invokeFunction: (functionCall: A2UIFunctionCall): A2UIValueType => {
        return FunctionBridge.invoke(functionCall, surfaceId, componentId, normalizedRenderId);
      }
    });
  }

  private static resolveCatalogId(renderId: number | undefined): string {
    if (renderId === undefined) {
      return '';
    }
    const controller: SurfaceControllerImpl | undefined = SurfaceControllerImpl.getControllerByRenderId(renderId);
    return controller?.getCatalog().getCatalogId() ?? '';
  }

  private static findExecutableCatalogFunctionItem(
    renderId: number | undefined,
    functionName: string
  ): InnerFunctionItem | undefined {
    if (functionName.length === 0) {
      return undefined;
    }
    const item: InnerFunctionItem | undefined = FunctionBridge.findCatalogFunctionItem(renderId, functionName);
    if (item === undefined || item.isInnerNative || item.functionCall === undefined) {
      return undefined;
    }
    return item;
  }

  private static normalizeRenderId(renderId: number | undefined): number | undefined {
    if (renderId === undefined || !Number.isFinite(renderId) || renderId < 0) {
      return undefined;
    }
    return renderId;
  }

  private static validateReturnType(returnType: string, value: LocalFunctionResult): boolean {
    switch (returnType) {
      case 'void':
        return value === undefined;
      case 'string':
        return typeof value === 'string';
      case 'number':
        return typeof value === 'number';
      case 'boolean':
        return typeof value === 'boolean';
      case 'null':
        return value === null;
      case 'array':
        return Array.isArray(value);
      case 'object':
        return typeof value === 'object' && value !== null && !Array.isArray(value);
      case 'any':
        return true;
      default:
        return false;
    }
  }

  private static validateAndNormalizeFunctionCall(
    request: LocalFunctionRequest,
    catalogItem: InnerFunctionItem | undefined
  ): SchemaFunctionValidationResult {
    return SchemaErrorInfoManager.getInstance().withSchemaWarningContext(
      undefined,
      request.surfaceId,
      () => SchemaValidator.validateAndNormalizeFunctionCall(
        request.functionName,
        request.args,
        request.returnType,
        catalogItem
      )
    );
  }

  private static fail(returnType: string, errorCode: number, errorMessage: string): LocalFunctionResponse {
    console.error(`[FunctionBridge] ${errorCode}: ${errorMessage}`);
    return {
      success: false,
      returnType: returnType,
      errorCode: errorCode,
      errorMessage: errorMessage
    };
  }
}
