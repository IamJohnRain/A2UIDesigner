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
import { A2UIMessageType } from './A2UIMessage';
import { SchemaItemType } from '../schema/SchemaErrorInfoManager';
import { A2UIValueType, Breakpoint, ThemeMode } from '../types/types';
import { ExpressionEvaluateResult } from '../expression/ExpressionEngine';

export { Breakpoint, ThemeMode } from '../types/types';

export interface NativeProcessOptions {
  supportsMultipleSurfaces: boolean;
  maxSurfaceCount: number;
  isExtend: boolean;
}

export interface NativeSchemaWarningRequest {
  renderId: number;
  surfaceId: string;
  componentId: string;
  code: string;
  message: string;
  path?: string;
  itemType?: SchemaItemType;
  itemName?: string;
}

export interface NativeActionEventRequest {
  renderId: number;
  surfaceId: string;
  sourceComponentId: string;
  name: string;
  context?: A2UIValueType;
  timestamp?: string;
}

export interface NativeRuntimeErrorRequest {
  renderId: number;
  surfaceId: string;
  componentId: string;
  errorCode: number;
  errorMessage: string;
  source?: string;
}

export interface NativeProcessResult {
  success: boolean;
  errorCode?: string;
  errorMessage?: string;
  surfaceId?: string;
  messageType?: A2UIMessageType;
  surfaceResultCode?: number;
}

export interface ClientActionPayload {
  name: string;
  surfaceId: string;
  sourceComponentId: string;
  timestamp: string;
  context: A2UIValueType;
}

export interface ClientToServerActionMessage {
  version: string;
  action: ClientActionPayload;
}

export interface CrossLanguageAttributeRequest {
  renderId: number;
  componentId: string;
  nodeUniqueId: number;
  componentType: string;
  attributeName: string;
  floatValue: number;
  stringValue: string;
  payloadJson: string;
  reset: boolean;
}

export interface SurfaceNativeEngine {
  initRenderSlot: (renderId: number) => void;
  destroyRenderSlot: (renderId: number) => void;
  processMessage: (renderId: number, dsl: string, catalog: Object, options: Object) => NativeProcessResult;
  bindSurfaceToRender: (renderId: number, content: Object) => void;
  unbindSurfaceFromRender: (renderId: number) => void;
  popSurface: (renderId: number) => Object;
  getSurfaceIds: (renderId: number) => string[];
  getLatestSurfaceId: (renderId: number) => string | undefined;
  registerDispatchAction: (callback: Object) => void;
  setRootFillMode: (renderId: number, forceFill: boolean) => void;
  dispatchCustomComponentAction: (
    renderId: number, surfaceId: string, componentId: string, eventName: string, contextJson: string
  ) => void;
  setFontSizeScale: (renderId: number, scale: number) => void;
  setApiVersion: (renderId: number, apiVersion: number) => void;
  registerCrossLanguageAttributeCallback: (callback: Object) => void;
  updateThemeMode: (renderId: number, mode: number) => void;
  updateBreakpoint: (renderId: number, breakpoint: number) => void;
  registerDispatchSchemaWarning: (callback: Object) => void;
  registerDispatchRuntimeError: (callback: Object) => void;
  evaluateExpression: (expression: string, renderId?: number, surfaceId?: string) => ExpressionEvaluateResult;
}

export class NativeEngineBridge {
  private static actionBridgeInstalled: boolean = false;
  private static actionDispatcher: ((request: NativeActionEventRequest) => void) | undefined;
  private static warningBridgeInstalled: boolean = false;
  private static warningDispatcher: ((request: NativeSchemaWarningRequest) => void) | undefined;
  private static crossLanguageAttributeCallbackInstalled: boolean = false;
  private static crossLanguageAttributeHandler:
    ((request: CrossLanguageAttributeRequest) => void) | undefined;
  private static runtimeErrorBridgeInstalled: boolean = false;
  private static runtimeErrorDispatcher: ((request: NativeRuntimeErrorRequest) => void) | undefined;

  private static logEngineUnavailable(methodName: string, renderId?: number): void {
    let message = `[NativeEngineBridge] ${methodName} ignored: native engine unavailable`;
    if (renderId !== undefined) {
      message += `, renderId=${renderId}`;
    }
    console.warn(message);
  }

  static getEngine(): SurfaceNativeEngine | null {
    let engineObject = nativeEngine as Object | undefined;
    if (engineObject === undefined || engineObject === null) {
      return null;
    }
    return engineObject as SurfaceNativeEngine;
  }

  static initRenderSlot(renderId: number): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('initRenderSlot', renderId);
      return;
    }
    engine.initRenderSlot(renderId);
  }

  static destroyRenderSlot(renderId: number): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('destroyRenderSlot', renderId);
      return;
    }
    engine.destroyRenderSlot(renderId);
  }

  static processMessage(
    renderId: number,
    dsl: string,
    catalog: Object,
    options: NativeProcessOptions
  ): NativeProcessResult | undefined {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('processMessage', renderId);
      return undefined;
    }
    console.info(`[NativeEngineBridge] processMessage: renderId=${renderId}, dslLength=${dsl.length}`);
    return engine.processMessage(renderId, dsl, catalog, options as Object);
  }

  static bindSurfaceToRender(renderId: number, content: Object): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('bindSurfaceToRender', renderId);
      return;
    }
    engine.bindSurfaceToRender(renderId, content);
  }

  static unbindSurfaceFromRender(renderId: number): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('unbindSurfaceFromRender', renderId);
      return;
    }
    engine.unbindSurfaceFromRender(renderId);
  }

  static popSurface(renderId: number): Object | undefined {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('popSurface', renderId);
      return undefined;
    }
    return engine.popSurface(renderId);
  }

  static getSurfaceIds(renderId: number): string[] | undefined {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      return undefined;
    }
    let result = engine.getSurfaceIds(renderId);
    return Array.isArray(result) ? result : undefined;
  }

  static getLatestSurfaceId(renderId: number): string | undefined {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      return undefined;
    }
    return engine.getLatestSurfaceId(renderId);
  }

  static installActionBridge(dispatcher: (request: NativeActionEventRequest) => void): void {
    if (NativeEngineBridge.actionBridgeInstalled) {
      console.info('[NativeEngineBridge] installActionBridge ignored: already installed');
      return;
    }
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('installActionBridge');
      return;
    }
    NativeEngineBridge.actionDispatcher = dispatcher;
    engine.registerDispatchAction(NativeEngineBridge.handleNativeActionEvent);
    NativeEngineBridge.actionBridgeInstalled = true;
    console.info('[NativeEngineBridge] installActionBridge success');
  }

  static installSchemaWarningBridge(dispatcher: (request: NativeSchemaWarningRequest) => void): void {
    if (NativeEngineBridge.warningBridgeInstalled) {
      console.info('[NativeEngineBridge] installSchemaWarningBridge ignored: already installed');
      return;
    }
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('installSchemaWarningBridge');
      return;
    }
    NativeEngineBridge.warningDispatcher = dispatcher;
    engine.registerDispatchSchemaWarning(NativeEngineBridge.handleNativeSchemaWarningEvent);
    NativeEngineBridge.warningBridgeInstalled = true;
    console.info('[NativeEngineBridge] installSchemaWarningBridge success');
  }

  static installRuntimeErrorBridge(dispatcher: (request: NativeRuntimeErrorRequest) => void): void {
    if (NativeEngineBridge.runtimeErrorBridgeInstalled) {
      console.info('[NativeEngineBridge] installRuntimeErrorBridge ignored: already installed');
      return;
    }
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('installRuntimeErrorBridge');
      return;
    }
    NativeEngineBridge.runtimeErrorDispatcher = dispatcher;
    engine.registerDispatchRuntimeError(NativeEngineBridge.handleNativeRuntimeErrorEvent);
    NativeEngineBridge.runtimeErrorBridgeInstalled = true;
    console.info('[NativeEngineBridge] installRuntimeErrorBridge success');
  }

  private static handleNativeActionEvent(request: NativeActionEventRequest): void {
    if (request === undefined || request === null || typeof request.renderId !== 'number') {
      console.warn('[NativeEngineBridge] handleNativeActionEvent ignored: invalid request');
      return;
    }
    if (NativeEngineBridge.actionDispatcher !== undefined) {
      console.info(
        `[NativeEngineBridge] dispatch native action, renderId=${request.renderId}, surfaceId=${request.surfaceId}, ` +
        `componentId=${request.sourceComponentId}, action=${request.name}`
      );
      NativeEngineBridge.actionDispatcher(request);
    }
  }

  private static handleNativeSchemaWarningEvent(request: NativeSchemaWarningRequest): void {
    if (request === undefined || request === null || typeof request.renderId !== 'number' ||
      typeof request.code !== 'string' || typeof request.message !== 'string') {
      console.warn('[NativeEngineBridge] handleNativeSchemaWarningEvent ignored: invalid request');
      return;
    }
    if (NativeEngineBridge.warningDispatcher !== undefined) {
      console.info(
        `[NativeEngineBridge] dispatch native schema warning, renderId=${request.renderId}, ` +
        `componentId=${request.componentId}, code=${request.code}`
      );
      NativeEngineBridge.warningDispatcher(request);
    }
  }

  private static handleNativeRuntimeErrorEvent(request: NativeRuntimeErrorRequest): void {
    if (request === undefined || request === null ||
      typeof request.renderId !== 'number' ||
      typeof request.errorCode !== 'number' ||
      typeof request.errorMessage !== 'string') {
      console.warn('[NativeEngineBridge] handleNativeRuntimeErrorEvent ignored: invalid request');
      return;
    }
    if (NativeEngineBridge.runtimeErrorDispatcher !== undefined) {
      console.warn(
        `[NativeEngineBridge] dispatch native runtime error, renderId=${request.renderId}, ` +
        `surfaceId=${request.surfaceId}, componentId=${request.componentId}, errorCode=${request.errorCode}, ` +
        `source=${request.source ?? 'unknown'}`
      );
      NativeEngineBridge.runtimeErrorDispatcher(request);
    }
  }
  static setRootFillMode(renderId: number, forceFill: boolean): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('setRootFillMode', renderId);
      return;
    }
    engine.setRootFillMode(renderId, forceFill);
  }

  static dispatchCustomComponentAction(
    renderId: number, surfaceId: string, componentId: string, eventName: string, contextJson: string
  ): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      return;
    }
    engine.dispatchCustomComponentAction(renderId, surfaceId, componentId, eventName, contextJson);
  }

  static setFontSizeScale(renderId: number, scale: number): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      return;
    }
    engine.setFontSizeScale(renderId, scale);
  }

  static setApiVersion(renderId: number, apiVersion: number): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      return;
    }
    engine.setApiVersion(renderId, apiVersion);
  }

  static installCrossLanguageAttributeCallback(
    handler: (request: CrossLanguageAttributeRequest) => void
  ): boolean {
    if (NativeEngineBridge.crossLanguageAttributeCallbackInstalled) {
      console.info('[NativeEngineBridge] installCrossLanguageAttributeCallback ignored: already installed');
      return true;
    }
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('installCrossLanguageAttributeCallback');
      return false;
    }
    NativeEngineBridge.crossLanguageAttributeHandler = handler;
    engine.registerCrossLanguageAttributeCallback(NativeEngineBridge.handleCrossLanguageAttributeRequest);
    NativeEngineBridge.crossLanguageAttributeCallbackInstalled = true;
    console.info('[NativeEngineBridge] installCrossLanguageAttributeCallback success');
    return true;
  }

  private static handleCrossLanguageAttributeRequest(request: CrossLanguageAttributeRequest): void {
    if (request === undefined || request === null ||
      typeof request.renderId !== 'number' || typeof request.componentId !== 'string') {
      console.warn('[NativeEngineBridge] handleCrossLanguageAttributeRequest ignored: invalid request');
      return;
    }
    if (NativeEngineBridge.crossLanguageAttributeHandler !== undefined) {
      NativeEngineBridge.crossLanguageAttributeHandler(request);
    } else {
      console.warn('[NativeEngineBridge] handleCrossLanguageAttributeRequest ignored: handler not set');
    }
  }

  static updateThemeMode(renderId: number, mode: ThemeMode): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('updateThemeMode', renderId);
      return;
    }
    engine.updateThemeMode(renderId, mode);
  }

  static updateBreakpoint(renderId: number, breakpoint: Breakpoint): void {
    let engine = NativeEngineBridge.getEngine();
    if (engine === null) {
      NativeEngineBridge.logEngineUnavailable('updateBreakpoint', renderId);
      return;
    }
    engine.updateBreakpoint(renderId, breakpoint);
  }
}
